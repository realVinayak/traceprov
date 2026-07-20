// The source for running duckdb (smokedduck) queries.
// This is referenced as smokedduck in bunch of places because
// it was originally written as a smokedduck driver. The code ended up being
// generic enough to be used as a general duckdb driver.
// Some options don't make sense for a generic driver (like capture lineage)
// TODO: Remove those options.

#include <iostream>
#include <string>
#include <cstring>

#include <sstream>
#include <fstream>
#include <chrono>
#include <vector>

#include "traceprov_duckdb_infer.hpp"
#include "traceprov_extra_funcs.hpp"
#include "traceprov_partition_info.hpp"

#undef sprintf

#include "duckdb.hpp"
#include "traceprov.hpp"
#include "utils.hpp"

#include "traceprov_settings.hpp"
#include <traceprov_node.hpp>

#include <derivation_utils.hpp>
#include "traceprov_derive.hpp"

extern "C" {
    #include <mem_alloc.h>
    #include "utils.h"
}

#define TP_ENABLE_PROFILING "PRAGMA enable_profiling=json"
#define TP_ENABLE_PROFILING_QUERY_TREE "PRAGMA enable_profiling=query_tree"
#define TP_SET_PROFILE_OUTPUT "PRAGMA profile_output='%s'"
#define TP_DISABLE_PROFILING "PRAGMA disable_profiling;"

#define TP_ENABLE_DETAILED_PROFILING "PRAGMA profiling_mode = 'detailed';"

// For OLD SmokedDuck.
#define TP_ENABLE_LINEAGE "PRAGMA enable_lineage;"
#define TP_DISABLE_LINEAGE "PRAGMA disable_lineage;"
#define TP_CLEAR_LINEAGE "PRAGMA clear_lineage;"

// For new SD.
#define TP_ENABLE_LINEAGE_NEW "PRAGMA set_lineage(True)"
#define TP_DISABLE_LINEAGE_NEW "PRAGMA set_lineage(False)"
#define TP_CLEAR_LINEAGE_NEW TP_CLEAR_LINEAGE

#define TP_SET_STATS_OUTPUT "COPY (select * from duckdb_queries_list()) TO '%s'"
#define TP_DUMP_SETTINGS "copy (select json_group_object(name, value) as settings from duckdb_settings()) TO '%s';"

#define TP_SET_STATS_OUTPUT_NEW "copy (select * from lineage_meta()) to '%s'"

#define TP_LAYER_STATS_OUTPUT(QUERY, OUT) ("copy (select * from (" + QUERY + ")) to '" + OUT + "'")

#define TP_SD_DISABLE_CHUNK_CACHE "PRAGMA disable_cache;"
#define TP_SD_ENABLE_CHUNK_CACHE "PRAGMA enable_cache;"

#define TP_SD_DISABLE_PERFECT_HASH "SET perfect_ht_threshold=0;"
#define TP_SD_ENABLE_PERFECT_HASH "SET perfect_ht_threshold=12;"

#define DUCKDB_DEFAULT_SELECTIVITY 0.001
#define DUCKDB_DEFAULT_SCAN_MAX_COUNT 2048

void wrapped_enable_lineage();
void wrapped_disable_lineage();
void wrapped_clear_lineage();

// Add dummy definitions.
#if TRACEPROV_SD_MODE==0
void wrapped_enable_lineage(){}
void wrapped_disable_lineage(){};
void wrapped_clear_lineage(){};
#else
//  Ugh.
#define LINEAGE
#include "duckdb/execution/lineage/lineage_manager.hpp"
void wrapped_enable_lineage(){
    duckdb::gEnableLineage();
}
void wrapped_disable_lineage(){
    duckdb::gDisableLineage();
};
void wrapped_clear_lineage(){
    duckdb::gClearLineage();
};
#endif

typedef struct TraceProvLogExtra {
    // what is the log offset?
    int64_t log_offset;
    // what layer is it for?
    TraceProvLayerNumber top_level_log_layer_number;
    // what layer does it point to?
    // TODO: Generialize this.
    TraceProvLayerNumber child_layer_number;
} TraceProvLogExtra;

typedef struct ExtraQueryGroup {
    std::unordered_map<TraceProvLayerNumber, std::vector<std::string *> *> *query_map;
} ExtraQueryGroup;

std::string read_file(std::string file);
void register_func(duckdb::ScalarFunction *func, duckdb_connection con);

// Some misc values that get maintained.
// TODO: Migrate some other values from Options..
typedef struct MiscKeyValue {
    TraceProvLogSize total_log_size;
    uint64_t sql_compilation_time;
    std::string layer_stats;
    uint64_t index_time;
    std::string layers_with_index;
} MiscKeyValue;

const static MiscKeyValue g_init_misc_key_value = {
    .total_log_size = TraceProvLogSize {
        .page_requested_size = 0,
        .page_used_size = 0,
        .bytes_used_size = 0
    },
    .sql_compilation_time = 0,
    .layer_stats = "",
    .index_time = 0,
    .layers_with_index = ""
};

enum CustomGraphType {
    INVALID = 0,
    LOG_CHAIN,
    AGG_LOG
};

// TODO: Migrate to a better option handling system than this in-house mess.
struct Options {
    // via --lineage
    bool capture_lineage;
    // via --db
    std::string db_path;
    // via --profile
    // If empty, no profiling is done.
    std::string profile_out_path;
    // via --pending
    // whwther to utilie pending system (for testing.)
    bool use_pending;
    // via --threads
    uint32 num_threads;
    // via --stats
    std::string stats_path;
    // via --i
    std::string input_path;
    // via --repeat
    int repeat;
    // via --settings
    std::string settings_out_path;
    // via --time
    std::string time_out_path;
    // via --idx_scan_percent
    std::string index_scan_percentage;
    // via --dry_run
    bool dry_run;
    // via --no_reinit
    bool no_reinit_state;
    // via --min_layer_number
    uint32_t min_layer_number;
    // via --extra
    // vector because there can be multiple.
    std::vector<std::string> *extra_query_paths;
    // via --disable_col_opt
    bool disable_column_optimizer;
    // via --main_once_extra_all
    // In some cases, for the sake of timing,
    // need to run the extra queries together.
    bool main_once_extra_all;
    // via --extra_file
    std::string extra_file;
    // via --is_new_sd
    bool is_new_sd;
    // via --sd_extension_path
    std::string sd_extension_path;
    // via --query_tree
    bool query_tree;
    // via --load_micro_benchmarks
    bool load_micro_benchmarks;
    // via --use_part_agg
    // this is a bitset to speed things up.
    uint64_t use_partition_agg;
    // something extra, not directly setable via an option.
    void *_extra;
    std::string _extra_output;
    // via --layer_stats_out
    std::string layer_stats_out;
    bool traceprov_perform_derivation;
    bool traceprov_materialize_derivation;
    // Just print derivation, don't perform it. Useful for debugging.
    bool traceprov_dry_run_derivation;
    uint32_t initial_page_count;
    // Because we implictly make queries for all the layers, some of them are intermediate.
    // If non-empty, only these ones will be derived.
    std::vector<uint32_t> *traceprov_layers_to_derive;
    ExtraQueryGroup *extra_query_groups;
    bool use_extra_threads;
    std::vector<uint64_t> *log_offsets;
    std::vector<std::string> *pre_main_sql;
    uint32_t extra_multiple_count;
    bool get_log_size;
    MiscKeyValue misc_store;
    /** TraceProv Settings */
    // Note that the values are not repeated here (the update is inlined for these.)
    // via --traceprov_use_partition_in_agg
    // via --traceprov_use_partition_in_log
    // via --traceprov_use_row_in_agg_partition
    // via --traceprov_skip_page_cache
    // via --traceprov_use_implicit_union
    // via --traceprov_use_merge_chunks
    // via --traceprov_force_seq_scan
    bool dump_base_table;
    uint64_t warm_up_time;
    CustomGraphType custom_graph_type;
    uint64_t custom_graph_type_log_chain_table_count;
    bool sd_join_mode;
    bool disable_chunk_cache;
    uint64_t output_column_idx;
    bool disable_perfect_hash;
    bool _dump_worker_layer_time;
};

#define IS_OPTION(X) (strcmp(argv[i], X) == 0)
#define IS_SET(X) (X.size() != 0)

typedef struct Funcs {
    duckdb_table_function tp_read_func;
    duckdb_table_function tp_read_offset_func;
} Funcs;

std::unordered_map<TraceProvLayerNumber, std::string> *get_layer_string_map(
    Options *options,
    TraceProvDerivationSpec **derivation_spec,
    std::vector<std::string> &ddls,
    std::vector<std::pair<uint64_t, uint64_t>> &added_ddls,
    const bool derive_sql_mapping = true
);
MiscKeyValue setup_traceprov_indexes(duckdb_connection con, Options *options, Funcs table_funcs);
static std::string serialize_worker_layer_time(TraceProvLayerTime** worker_layer_time);
std::string *get_disabled_optimizations(const Options *options){
    std::string disabled = "";
    std::vector<std::string> disabled_names;
    if (options->sd_join_mode){
        disabled_names.push_back("filter_pushdown");
        disabled_names.push_back("statistics_propagation");
        // disabled_names.push_back("join_filter_pushdown");
    }
    if (options->disable_column_optimizer){
        disabled_names.push_back("unused_columns");
    }
    for (int idx = 0; idx < disabled_names.size(); idx++){
       if (idx > 0) disabled += ",";
       disabled += disabled_names[idx];
    }
    return new std::string("SET disabled_optimizers='" + disabled + "';");
}

struct Options get_base_option(){
    struct Options options {
        .capture_lineage = false,
        .db_path = "",
        .profile_out_path = "",
        .use_pending = false,
        .num_threads = 1,
        .stats_path = "",
        .repeat = 1,
        .settings_out_path = "",
        .time_out_path = "",
        .index_scan_percentage = "",
        .dry_run = false,
        .no_reinit_state = false,
        .min_layer_number = 0,
        .extra_query_paths = new std::vector<std::string>,
        .disable_column_optimizer = false,
        .main_once_extra_all = false,
        .extra_file = "",
        .is_new_sd = false,
        .sd_extension_path = "",
        .query_tree = false,
        .load_micro_benchmarks = false,
        .use_partition_agg = 0,
        ._extra = NULL,
        ._extra_output = "",
        .layer_stats_out = "",
        // This needs to be another option, unfortunately.
        .traceprov_perform_derivation = false,
        .traceprov_materialize_derivation = false,
        .traceprov_dry_run_derivation = false,
        .initial_page_count = 0,
        .traceprov_layers_to_derive = new std::vector<uint32_t>,
        .extra_query_groups = tp_alloc0_object(ExtraQueryGroup),
        .use_extra_threads = false,
        .log_offsets = new std::vector<uint64_t>,
        .pre_main_sql = new std::vector<std::string>,
        .extra_multiple_count = 1,
        .get_log_size = false,
        .misc_store = g_init_misc_key_value,
        .dump_base_table = false,
        .warm_up_time = 0,
        .custom_graph_type = CustomGraphType::INVALID,
        .custom_graph_type_log_chain_table_count = 0,
        .sd_join_mode = false,
        .disable_chunk_cache = false,
        .output_column_idx = 0,
        .disable_perfect_hash = false,
        ._dump_worker_layer_time = false
    };
    return options;
}

struct Options parse_args(int argc, char **argv){
    auto options = get_base_option();
    for (int i = 1; i < argc; i++){
        if (IS_OPTION("--lineage")){
            options.capture_lineage = true;
            continue;
        } else if (IS_OPTION("--db")){
            options.db_path = std::string(argv[++i]);
            continue;
        } else if (IS_OPTION("--profile")){
            options.profile_out_path = std::string(argv[++i]);
            continue;
        } else if (IS_OPTION("--pending")){
            options.use_pending = true;
            continue;
        } else if (IS_OPTION("--threads")){
            options.num_threads = std::atoi(argv[++i]);
            continue;
        } else if (IS_OPTION("--stats")){
            options.stats_path = std::string(argv[++i]);
            continue;
        } else if (IS_OPTION("--i")){
            options.input_path = std::string(argv[++i]);
            continue;
        } else if (IS_OPTION("--repeat")){
            options.repeat = std::atoi(argv[++i]);
            continue;
        } else if (IS_OPTION("--settings")){
            options.settings_out_path = std::string(argv[++i]);
            continue;
        } else if (IS_OPTION("--time")){
            options.time_out_path = std::string(argv[++i]);
            continue;
        } else if (IS_OPTION("--idx_scan_percent")){
            options.index_scan_percentage = std::string(argv[++i]);
            continue;
        } else if (IS_OPTION("--dry_run")){
            options.dry_run = true;
            continue;
        } else if (IS_OPTION("--no_reinit")){
            options.no_reinit_state = true;
            continue;
        } else if (IS_OPTION("--min_layer_number")){
            options.min_layer_number = std::atoi(argv[++i]);
            continue;
        } else if (IS_OPTION("--extra")){
            auto extra_path = std::string(argv[++i]);
            options.extra_query_paths->push_back(extra_path);
            continue;
        } else if (IS_OPTION("--disable_col_opt")){
            options.disable_column_optimizer = true;
            continue;
        } else if (IS_OPTION("--main_once_extra_all")){
            options.main_once_extra_all = true;
            continue;
        } else if (IS_OPTION("--extra_file")){
            auto extra_path = std::string(argv[++i]);
            std::ifstream extra_file_stream(extra_path.c_str());
            uint64_t added = 0;
            TraceProvLayerNumber layer = 0;
            sscanf(extra_path.c_str(), "/tmp/infer_%d_template", &layer);
            if (layer == 0){
                elog(INFO, "Error parsing: %s", extra_path.c_str());
            }
            for (std::string extra_file_path; std::getline(extra_file_stream, extra_file_path);){
                if (extra_file_path.size() > 0){
                    if (options.extra_query_groups->query_map == NULL){
                        options.extra_query_groups->query_map = new std::unordered_map<TraceProvLayerNumber, std::vector<std::string *> *>;
                    }
                    if (options.extra_query_groups->query_map->find(layer) == options.extra_query_groups->query_map->end()){
                        options.extra_query_groups->query_map->insert({layer, new std::vector<std::string *>});
                    }
                    auto dest_ptr = options.extra_query_groups->query_map->at(layer);
                    dest_ptr->push_back(new std::string(read_file(extra_file_path)));
                    added++;
                }
            }
            if (added == 0){
                elog(ERROR, "Expected some files to be added!");
            }
            continue;
        } else if (IS_OPTION("--is_new_sd")){
            options.is_new_sd = true;
            continue;
        } else if (IS_OPTION("--sd_extension_path")){
            options.sd_extension_path = std::string(argv[++i]);
            continue;
        } else if (IS_OPTION("--query_tree")){
            options.query_tree = true;
            continue;
        } else if (IS_OPTION("--load_micro_benchmarks")){
            options.load_micro_benchmarks = true;
            continue;
        } else if (IS_OPTION("--traceprov_use_partition_in_agg")){
            traceprov_use_partition_in_agg = true;
            continue;
        } else if (IS_OPTION("--traceprov_use_partition_in_log")){
            traceprov_use_partition_in_log = true;
            continue;
        } else if (IS_OPTION("--traceprov_use_row_in_agg_partition")){
            traceprov_use_row_in_agg_partition = true;
            continue;
        } else if (IS_OPTION("--use_part_agg")){
            options.use_partition_agg |= (1 << std::atoi(argv[++i]));
            continue;
        } else if (IS_OPTION("--layer_stats_out")){
            options.layer_stats_out = std::string(argv[++i]);
            continue;
        } else if (IS_OPTION("--traceprov_perform_derivation")){
            options.traceprov_perform_derivation = true;
            continue;
        } else if (IS_OPTION("--traceprov_materialize_derivation")){
            options.traceprov_materialize_derivation = true;
            continue;
        } else if (IS_OPTION("--traceprov_skip_page_cache")){
            traceprov_skip_page_cache = true;
            continue;
        } else if (IS_OPTION("--traceprov_dry_run_derivation")){
            options.traceprov_dry_run_derivation = true;
            continue;
        } else if (IS_OPTION("--traceprov_use_implicit_union")){
            traceprov_use_implicit_union = true;
            continue;
        } else if (IS_OPTION("--initial_page_count")){
            options.initial_page_count = std::atoi(argv[++i]);
            continue;
        } else if (IS_OPTION("--traceprov_layers_to_derive")){
            options.traceprov_layers_to_derive->push_back(std::atoi(argv[++i]));
            continue;
        } else if (IS_OPTION("--traceprov_use_merge_chunks")){
            traceprov_use_merge_chunks = true;
            continue;
        } else if (IS_OPTION("--traceprov_combine_in_memory")){
            traceprov_combine_in_memory = true;
            continue;
        } else if (IS_OPTION("--traceprov_split_combine")){
            traceprov_split_combine = true;
            continue;
        } else if (IS_OPTION("--use_extra_threads")){
            options.use_extra_threads = true;
            continue;
        } else if (IS_OPTION("--traceprov_use_compact")){
            traceprov_use_compact = true;
            continue;
        }  else if (IS_OPTION("--traceprov_assume_null")){
            traceprov_assume_null = true;
            continue;
        } else if (IS_OPTION("--log_offset")){
            options.log_offsets->push_back(std::atol(argv[++i]));
            continue;
        } else if (IS_OPTION("--log_offset_file")){
            auto log_offset_file = std::string(argv[++i]);
            std::ifstream pre_main_sql_path_stream(log_offset_file.c_str());
            for (std::string log_offset; std::getline(pre_main_sql_path_stream, log_offset);){
                if (log_offset.size() > 0){
                    options.log_offsets->push_back(std::atol(log_offset.c_str()));
                }
            }
            continue;
        } else if (IS_OPTION("--pre_main_sql")){
            auto pre_main_sql_path = std::string(argv[++i]);
            std::ifstream pre_main_sql_path_stream(pre_main_sql_path.c_str());
            for (std::string pre_main_sql; std::getline(pre_main_sql_path_stream, pre_main_sql);){
                if (pre_main_sql.size() > 0){
                    options.pre_main_sql->push_back(pre_main_sql);
                }
            }
            continue;
        } else if (IS_OPTION("--extra_multiple_count")){
            options.extra_multiple_count = std::atoi(argv[++i]);
            continue;
        } else if (IS_OPTION("--traceprov_force_seq_scan")){
            traceprov_force_seq_scan = true;
            continue;
        } else if (IS_OPTION("--get_log_size")){
            options.get_log_size = true;
            continue;
        } else if (IS_OPTION("--traceprov_skip_sql_cache")){
            traceprov_skip_sql_cache = true;
            continue;
        } else if (IS_OPTION("--dump_base_table")){
            options.dump_base_table = true;
            continue;
        } else if (IS_OPTION("--traceprov_use_table_stats")){
            traceprov_use_table_stats = true;
            continue;
        } else if (IS_OPTION("--warm_up_time")){
            // Switch from seconds to microseconds.
            options.warm_up_time = std::atol(argv[++i]) * (1000*1000);
            continue;
        } else if (IS_OPTION("--traceprov_ignore_direct_join")){
            traceprov_ignore_direct_join = true;
            continue;
        } else if (IS_OPTION("--custom_graph_type")){
            options.custom_graph_type = (CustomGraphType)std::atol(argv[++i]);
            continue;
        } else if (IS_OPTION("--log_chain_table_count")){
            options.custom_graph_type_log_chain_table_count = std::atol(argv[++i]);
            continue;
        } else if (IS_OPTION("--sd_join_mode")){
            options.sd_join_mode = true;
            continue;
        } else if (IS_OPTION("--disable_chunk_cache")){
            options.disable_chunk_cache = true;
            continue;
        } else if (IS_OPTION("--traceprov_use_index")){
            traceprov_use_index = true;
            continue;
        }  else if (IS_OPTION("--output_col_idx")){
            options.output_column_idx = std::atoi(argv[++i]);
            continue;
        }  else if (IS_OPTION("--traceprov_use_join_filter_rewrite")){
            traceprov_use_join_filter_rewrite = true;
            continue;
        }  else if (IS_OPTION("--traceprov_use_filter_pushdown")){
            traceprov_use_filter_pushdown = true;
            continue;
        }  else if (IS_OPTION("--traceprov_use_hash_index")){
            traceprov_use_hash_index = true;
            continue;
        } else if (IS_OPTION("--disable_perfect_hash")){
            options.disable_perfect_hash = true;
            continue;
        }

        std::cout << "Got unexpected option: " << argv[i] << std::endl;
        std::exit(1);
    }
    std::cout << "OPTIONS: [" << std::endl;
    std::cout << "\tcapture_lineage: " << options.capture_lineage << std::endl;
    std::cout << "\tdb_path: " << options.db_path << std::endl;
    std::cout << "\tprofile_out_path: " << options.profile_out_path << std::endl;
    std::cout << "\tuse_pending: " << options.use_pending << std::endl;
    std::cout << "\tnum_threads: " << options.num_threads << std::endl;
    std::cout << "\tstats_path: " << options.stats_path << std::endl;
    std::cout << "\tinput_path: " << options.input_path << std::endl;
    std::cout << "\trepeat: " << options.repeat<< std::endl;
    std::cout << "\tsettings_out_path: " << options.settings_out_path << std::endl;
    std::cout << "\ttime_out_path: " << options.time_out_path << std::endl;
    std::cout << "]" << std::endl;
    traceprov_thread_count = options.num_threads;
    if (options.custom_graph_type == CustomGraphType::LOG_CHAIN || options.custom_graph_type == CustomGraphType::AGG_LOG){
        traceprov_create_join_chain_dependency(options.custom_graph_type_log_chain_table_count, options.custom_graph_type == CustomGraphType::AGG_LOG);
    }
    return options;
};

typedef struct TraceProvLightData {
    uint64_t column_count;
    uint64_t row_count;
    std::vector<uint64_t>* output_log;
    std::vector<duckdb_data_chunk> *chunk_cache;
} TraceProvLightData;

// Simply populates the data, given a chunk.
static void populate_traceprov_data(
    TraceProvLightData *traceprov_data, 
    duckdb_data_chunk *chunk,
    const uint64_t output_log_id=0
){
    const uint64_t column_count = duckdb_data_chunk_get_column_count(*chunk);
    const uint64_t row_count = duckdb_data_chunk_get_size(*chunk);
    // Unlikely because it'll happen just once, for the first chunk.
    if ((column_count != traceprov_data->column_count)){
        if (traceprov_data->row_count != 0){
            std::cout << "Attempting to set different number of column entries" << std::endl;
            exit(1);
        }
        traceprov_data->column_count = column_count;
        // for (uint64_t col_idx = 0; col_idx < column_count; col_idx++){
        //     auto column_data = new TraceProvColumnData;
        //     column_data->data = new std::vector<uint64_t>();
        //     traceprov_data->push_back(column_data);
        // }
    }
    traceprov_data->row_count += row_count;
    if (output_log_id){
        duckdb_vector col = duckdb_data_chunk_get_vector(*chunk, output_log_id-1);
        uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col);
        if (traceprov_data->output_log == nullptr)
            traceprov_data->output_log = new std::vector<uint64_t>;
        traceprov_data->output_log->reserve(traceprov_data->output_log->size() + row_count);
        for (uint64_t row_idx = 0; row_idx < row_count; row_idx++){
            traceprov_data->output_log->push_back(col_data[row_idx]);
        }
    }
    if (traceprov_data->chunk_cache){
        traceprov_data->chunk_cache->push_back(*chunk);
    }
    // for (uint64_t col_idx = 0; col_idx < column_count; col_idx++){
    //     duckdb_vector col = duckdb_data_chunk_get_vector(*chunk, col_idx);
    //     uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col);
    //     uint64_t *col_validity = (uint64_t *)duckdb_vector_get_validity(col);
    //     traceprov_data->row_count += row_count;
    //     // auto current_column_data = traceprov_data->at(col_idx)->data;
    //     // // for (uint64_t row_idx = 0; row_idx < row_count; row_idx++){
    //     // //     if((!duckdb_validity_row_is_valid(col_validity, row_idx))){
    //     // //         std::cout << "Expected all non-null row, for now" << std::endl;
    //     // //         exit(1);
    //     // //     }
    //     // //     current_column_data->push_back(col_data[row_idx]);
    //     // // }
    // }
}

static void populate_log_offset(
    const uint64_t log_offset,
    TraceProvPointerContext *pc,
    TraceProvPartitionLayers *partition_layers,
    TraceProvPartitionInfo *partition_info
);

static std::string serialize_option(Options *option, const TraceProvNullMap *null_map);

typedef struct PerformQueryResult {
    int64_t computed_time;
    TraceProvLightData *data;
    Options option;
    TraceProvLayerTime** worker_layer_time_dump;
} PerformQueryResult;

PerformQueryResult *make_result(int64_t computed_time, TraceProvLightData *data, const Options *options, TraceProvLayerTime** worker_layer_time_dump){
    auto result = new PerformQueryResult;
    result->computed_time = computed_time;
    result->data = data;
    result->option = *options;
    result->worker_layer_time_dump = worker_layer_time_dump;
    return result;
}

PerformQueryResult *perform_query(
    const struct Options *options, 
    duckdb_connection &con, 
    std::string &in_sql,
    std::vector<PerformQueryResult *> *agg_result,
    const char *final_profile_out,
    const char *final_stats_query,
    std::string layer_stats_out,
    duckdb_prepared_statement *later_stmt = NULL,
    const bool cache_chunks = false
){
    if (!options->no_reinit_state){
        DUCKDB_RUN_SHORT_QUERY(con, "select reinit_state();", "reinit-state");
        reset_global_context();
        // traceprov_write_max_used_layer(options->min_layer_number);
    }
    DUCKDB_RUN_SHORT_QUERY(con, get_disabled_optimizations(options)->c_str(), get_disabled_optimizations(options)->c_str());
    #if TRACEPROV_SD_MODE==1
    if (options->disable_chunk_cache){
        DUCKDB_RUN_SHORT_QUERY(con, TP_SD_DISABLE_CHUNK_CACHE, TP_SD_DISABLE_CHUNK_CACHE);
    }else{
        DUCKDB_RUN_SHORT_QUERY(con, TP_SD_ENABLE_CHUNK_CACHE, TP_SD_ENABLE_CHUNK_CACHE);
    }
    if (options->disable_perfect_hash){
        DUCKDB_RUN_SHORT_QUERY(con, TP_SD_DISABLE_PERFECT_HASH, TP_SD_DISABLE_PERFECT_HASH);
    }else{
        DUCKDB_RUN_SHORT_QUERY(con, TP_SD_ENABLE_PERFECT_HASH, TP_SD_ENABLE_PERFECT_HASH);
    }
    #endif
    // if (options->disable_column_optimizer){
    //     // DUCKDB_RUN_SHORT_QUERY(con, "SET disabled_optimizers = 'join_order,materialized_cte,common_subplan';", "run disable optimizer..;");
    //     DUCKDB_RUN_SHORT_QUERY(con, "SET disabled_optimizers = 'unused_columns';", "run disable optimizer..;");
    // }else{
    //     DUCKDB_RUN_SHORT_QUERY(con, "SET disabled_optimizers = '';", "run disable optimizer..;");
    // }
    if (options->min_layer_number){
        traceprov_current.maximum_local_layer_used = options->min_layer_number;
    }


    if (IS_SET(options->profile_out_path) && final_profile_out != NULL){
        DUCKDB_RUN_SHORT_QUERY(con, (options->query_tree ? TP_ENABLE_PROFILING_QUERY_TREE : TP_ENABLE_PROFILING), "enable profiling");
        #if TRACEPROV_DEBUG_PERF==1
        DUCKDB_RUN_SHORT_QUERY(con, TP_ENABLE_DETAILED_PROFILING, "enable detailed profiling");
        #endif
        DUCKDB_RUN_SHORT_QUERY(con, final_profile_out, "set json out");
    }

    if (options->capture_lineage){
        wrapped_clear_lineage();
        wrapped_enable_lineage();
        // DUCKDB_RUN_SHORT_QUERY(con, (options->is_new_sd ? TP_CLEAR_LINEAGE_NEW : TP_CLEAR_LINEAGE), "clear lineage");
        // DUCKDB_RUN_SHORT_QUERY(con, (options->is_new_sd ? TP_ENABLE_LINEAGE_NEW : TP_ENABLE_LINEAGE), "enable lineage");
    }

    // Need to use both, the pending and the streaming API.
    duckdb_prepared_statement stmt = NULL;
    duckdb_result final_result;

    auto start_time = std::chrono::steady_clock::now();

    // if (later_stmt != NULL){
    //     stmt = *later_stmt;
    // }

    if (stmt == NULL)
        DUCKDB_EXIT_ON_ERROR_MSG(duckdb_prepare(con, in_sql.c_str(), &stmt), duckdb_prepare_error(stmt));

    if (options->use_pending){
        duckdb_pending_result result;
        DUCKDB_EXIT_ON_ERROR(duckdb_pending_prepared_streaming(stmt, &result));
        DUCKDB_EXIT_ON_ERROR(duckdb_execute_pending(
            result,
            &final_result
        ));
    }else{
        DUCKDB_EXIT_ON_ERROR_MSG(duckdb_execute_prepared(stmt, &final_result), duckdb_result_error(&final_result));
    }
    // if (later_stmt)
    //     *later_stmt = stmt;

    std::cout << "Is streaming: " << duckdb_result_is_streaming(final_result) << std::endl;

    uint64_t chunk_count = 0;
    auto traceprov_data = new TraceProvLightData;
    traceprov_data->column_count = 0;
    traceprov_data->row_count = 0;
    traceprov_data->output_log = nullptr;
    traceprov_data->chunk_cache = cache_chunks ? new std::vector<duckdb_data_chunk> : nullptr;
    if (options->use_pending){
        while (true) {
            duckdb_data_chunk data_chunk = duckdb_stream_fetch_chunk(final_result);
            if (!data_chunk) break;
            chunk_count++;
            populate_traceprov_data(traceprov_data, &data_chunk, options->output_column_idx);
            if (!cache_chunks) duckdb_destroy_data_chunk(&data_chunk);
        }
    }else{
        uint64_t total_chunk_count = duckdb_result_chunk_count(final_result);
        if (traceprov_data->chunk_cache) traceprov_data->chunk_cache->reserve(total_chunk_count);
        for (idx_t chunk_idx = 0; chunk_idx < total_chunk_count; chunk_idx++){
            duckdb_data_chunk data_chunk = duckdb_result_get_chunk(final_result, chunk_idx);
            populate_traceprov_data(traceprov_data, &data_chunk, options->output_column_idx);
            chunk_count++;
            if (!cache_chunks) duckdb_destroy_data_chunk(&data_chunk);
        }
    }

    if (!cache_chunks){
        duckdb_destroy_result(&final_result);
        duckdb_destroy_prepare(&stmt);
    }


    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    if (duration.count() == 0){
        std::cout << "Got 0 as the measured time, use a finer granularity..." << std::endl;
        exit(1);
    }

    PerformQueryResult *result = make_result(duration.count(), traceprov_data, options, dump_worker_layer_time());

    if (agg_result){
        if (options->get_log_size){
            result->option.misc_store.total_log_size = traceprov_get_total_layer_size();
            result->option.misc_store.layer_stats = traceprov_get_layer_stats();
        }
        agg_result->push_back(result);
    }

    if (options->capture_lineage){
        // DUCKDB_RUN_SHORT_QUERY(con, (options->is_new_sd ? TP_DISABLE_LINEAGE_NEW : TP_DISABLE_LINEAGE), "disable lineage");
        wrapped_disable_lineage();
    }

    // This needs to run before anything else bc of overwrites.
    DUCKDB_RUN_SHORT_QUERY(con, TP_DISABLE_PROFILING, "disable profiling");

    std::cout << "Chunks: " << chunk_count;

    if (IS_SET(options->stats_path) && final_stats_query != NULL){

        if (options->is_new_sd){
            DUCKDB_RUN_SHORT_QUERY(con, final_stats_query, "new sd result dump");
        }else{
            // DUCKDB_RUN_SHORT_QUERY(con, final_stats_query, "old sd result dump");
            if((duckdb_prepare(con, final_stats_query, &stmt)) == DuckDBError){
                std::cout << duckdb_prepare_error(stmt) << std::endl;
            }
            (duckdb_bind_varchar(stmt, 1, in_sql.c_str()));
            DUCKDB_EXIT_ON_ERROR(duckdb_execute_prepared(stmt, &final_result));
            duckdb_destroy_result(&final_result);
            duckdb_destroy_prepare(&stmt);
        }
    }

    if (IS_SET(layer_stats_out)){
        std::string _layer_stats_query = traceprov_get_layer_info_query();
        _layer_stats_query = (TP_LAYER_STATS_OUTPUT(_layer_stats_query, layer_stats_out));
        DUCKDB_RUN_SHORT_QUERY(con, _layer_stats_query.c_str(), "layer stats out");
    }

    #if TRACEPROV_SD_MODE==1
    DUCKDB_RUN_SHORT_QUERY(con, TP_SD_ENABLE_CHUNK_CACHE, TP_SD_ENABLE_CHUNK_CACHE);
    DUCKDB_RUN_SHORT_QUERY(con, TP_SD_ENABLE_PERFECT_HASH, TP_SD_ENABLE_PERFECT_HASH);
    #endif

    return result;
}

typedef struct ExtraQuery {
    std::string sql;
    std::string extra;
} ExtraQuery;

TraceProvDerivationSpec* augment_extra_sql(
    std::vector<ExtraQuery> &extra_sqls,
    Options *options,
    std::vector<TraceProvTableExtra *> *table_func_extra,
    std::vector<uint64_t> *log_offsets,
    TraceProvPartitionLayers *partition_layers,
    std::vector<uint64_t> *output_log_offset
);

TraceProvTableExtra *make_table_extra(){
    TraceProvTableExtra *table_extra = (TraceProvTableExtra *)malloc(sizeof(TraceProvTableExtra));
    memset(table_extra, 0, sizeof(TraceProvTableExtra));
    return table_extra;
}

std::string read_file(std::string file){
    std::string contents = "";
    std::ifstream file_stream(file.c_str());
    std::stringstream buffer;
    buffer << file_stream.rdbuf();
    contents = buffer.str();
    return contents;
}

#if TRACEPROV_SD_MODE==0
struct TraceProvCreateScalarFunctionInfo : public CreateFunctionInfo {
	DUCKDB_API explicit TraceProvCreateScalarFunctionInfo(ScalarFunction function);
	DUCKDB_API explicit TraceProvCreateScalarFunctionInfo(ScalarFunctionSet set);

	ScalarFunctionSet functions;

public:
	DUCKDB_API unique_ptr<CreateInfo> Copy() const override;
};


TraceProvCreateScalarFunctionInfo::TraceProvCreateScalarFunctionInfo(ScalarFunction function)
    : CreateFunctionInfo(CatalogType::SCALAR_FUNCTION_ENTRY), functions(function.name) {
    name = function.name;
    functions.AddFunction(std::move(function));
    internal = true;
}
TraceProvCreateScalarFunctionInfo::TraceProvCreateScalarFunctionInfo(ScalarFunctionSet set)
    : CreateFunctionInfo(CatalogType::SCALAR_FUNCTION_ENTRY), functions(std::move(set)) {
    name = functions.name;
    for (auto &func : functions.functions) {
        func.name = functions.name;
    }
    internal = true;
}

unique_ptr<CreateInfo> TraceProvCreateScalarFunctionInfo::Copy() const {
    ScalarFunctionSet set(name);
    set.functions = functions.functions;
    auto result = make_uniq<TraceProvCreateScalarFunctionInfo>(std::move(set));
    CopyProperties(*result);
    return std::move(result);
}

void register_func(duckdb::ScalarFunction *func, duckdb_connection con){
    auto duck_con = reinterpret_cast<duckdb::Connection *>(con);
    duck_con->BeginTransaction();
    TraceProvCreateScalarFunctionInfo info(*func);
    info.schema = DEFAULT_SCHEMA;
    duck_con->context->RegisterFunction(info);
    duck_con->Commit();
}

#else
void register_func(duckdb::ScalarFunction *func, duckdb_connection con){
    auto duck_con = reinterpret_cast<duckdb::Connection *>(con);
    duck_con->BeginTransaction();
    CreateScalarFunctionInfo info(*func);
    info.schema = DEFAULT_SCHEMA;
    duck_con->context->RegisterFunction(info);
    duck_con->Commit();
}
#endif

Funcs traceprov_add_funcs(duckdb_connection con){
    auto reinit_func = traceprov_create_reinit_state();
    register_func(reinit_func, con);

    duckdb_table_function tp_read_func = traceprov_create_table_func();
    DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, tp_read_func));

    duckdb_table_function tp_read_offset_func = traceprov_create_table_offset_func();
    DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, tp_read_offset_func));

    duckdb_table_function tp_read_offset_partition_func = traceprov_create_table_offset_partition_func();
    DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, tp_read_offset_partition_func));
    
    #if TRACEPROV_SD_MODE==0
    duckdb_scalar_function tp_table_window_func = traceprov_create_table_window_func(2, 0, NULL);
    DUCKDB_EXIT_ON_ERROR(duckdb_register_scalar_function(con, tp_table_window_func));

    duckdb_scalar_function tp_read_vector_func = traceprov_create_read_vector_func();
    DUCKDB_EXIT_ON_ERROR(duckdb_register_scalar_function(con, tp_read_vector_func));
    #endif

    if (traceprov_use_hash_index){
        duckdb_table_function tp_read_hash_table_chunk = traceprov_create_hash_table_func();
        DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, tp_read_hash_table_chunk));
    }

    return Funcs {
        .tp_read_func = tp_read_func,
        .tp_read_offset_func = tp_read_offset_func
    };
}

std::vector<duckdb_connection> *make_duckdb_connections(const uint32_t num_threads, const char *path = NULL){
    auto conns = new std::vector<duckdb_connection>;
    for (uint32_t idx = 0; idx < num_threads; idx++){
        duckdb_database db;
        duckdb_connection con;
        char *error_msg;
        duckdb_config db_config;
        DUCKDB_EXIT_ON_ERROR(duckdb_create_config(&db_config));
        DUCKDB_EXIT_ON_ERROR_MSG(duckdb_open_ext(path, &db, nullptr, &error_msg), error_msg);
        DUCKDB_EXIT_ON_ERROR(duckdb_connect(db, &con));
        DUCKDB_RUN_SHORT_QUERY(con, "SET threads=1;", "doing threads!");
        // DUCKDB_RUN_SHORT_QUERY(con, "SET streaming_buffer_size='16KiB';", "setting streaming_buffer_size!");
        DUCKDB_RUN_SHORT_QUERY(con, "SET preserve_insertion_order=false;", "unsetting preserve_insertion_order!");
        #if TRACEPROV_SD_MODE==0
        DUCKDB_RUN_SHORT_QUERY(con, "set pin_threads=\"on\";", "set pin threads");
        #endif
        traceprov_add_funcs(con);
        conns->push_back(con);
    }
    return conns;
}

int main(int argc, char **argv){

    traceprov_set_mem_config(TraceProvMemoryAllocator {.allocator = malloc, .free = free});

    struct Options options = parse_args(argc, argv);

    std::ifstream in_sql_stream(options.input_path.c_str());
    std::stringstream buffer;
    buffer << in_sql_stream.rdbuf();
    std::string in_sql(buffer.str());
    std::cout << "From file: " << in_sql << std::endl;
    std::vector<PerformQueryResult *> agg_result;

    std::vector<ExtraQuery> extra_sqls;

    for (auto extra_sql_path: *options.extra_query_paths){
        std::string extra_sql = "";
        std::ifstream extra_sql_stream(extra_sql_path.c_str());
        std::stringstream extra_buffer;
        extra_buffer << extra_sql_stream.rdbuf();
        extra_sql = extra_buffer.str();
        // std::cout << "From file (extra): " << extra_sql << std::endl;
        extra_sqls.push_back(ExtraQuery {.sql = extra_sql, .extra= ""});
    }

    if (!options.use_extra_threads && options.extra_query_groups->query_map != NULL){
        for (auto extra_sql_group: *options.extra_query_groups->query_map){
            for (auto extra_sql: *extra_sql_group.second){
                extra_sqls.push_back(ExtraQuery{.sql = extra_sql->c_str(), .extra = ""});
            }
        }
        options.extra_query_groups = tp_alloc0_object(ExtraQueryGroup);
    }

    duckdb_database db;
    duckdb_connection con;
    char *error_msg;
    duckdb_config db_config;
    DUCKDB_EXIT_ON_ERROR(duckdb_create_config(&db_config));
    if (options.is_new_sd){
        DUCKDB_EXIT_ON_ERROR(
            duckdb_set_config(db_config, "allow_unsigned_extensions", "true")
        );
    }
    DUCKDB_EXIT_ON_ERROR_MSG(duckdb_open_ext(options.db_path.c_str(), &db, nullptr, &error_msg), error_msg);
    DUCKDB_EXIT_ON_ERROR(duckdb_connect(db, &con));

    DUCKDB_RUN_SHORT_QUERY(con, "load JSON;", "load json");
    #if TRACEPROV_SD_MODE==0
    DUCKDB_RUN_SHORT_QUERY(con, "set pin_threads=\"on\";", "set pin threads");
    #endif
    DUCKDB_RUN_SHORT_QUERY(con, "SET preserve_insertion_order=false;", "set insertion order preserve");



    if (options.is_new_sd){
        if (!IS_SET(options.sd_extension_path))
            elog(ERROR, "Expected extenstion path to be set!");
        std::string load_str = "LOAD '" + options.sd_extension_path + "';";
        DUCKDB_RUN_SHORT_QUERY(con, load_str.c_str(), "load new sd extension");
    }

    TraceProvNullMap *null_map = traceprov_infer_nulls();
    TraceProvPartitionLayers *partition_layers = traceprov_layers_to_partition();
    TraceProvStatsCollectorMap *stats_collector_map = traceprov_get_stat_columns();

    const uint32_t num_args = 12;
    // duckdb_aggregate_function *funcs = traceprov_create_funcs(num_args, false, false, options.use_partition_agg);
    // duckdb_aggregate_function *ignore_gn_funcs = traceprov_create_funcs(num_args, false, true, options.use_partition_agg);
    // duckdb_aggregate_function *window_funcs = traceprov_create_window_funcs(num_args);
    auto log_funcs = traceprov_create_log_function(num_args, false, null_map, false);
    auto volatile_log_funcs = traceprov_create_log_function(num_args, true, null_map, false);
    auto boolean_log_funcs = traceprov_create_log_function(num_args, false, null_map, true);
    traceprov_create_and_register_agg(num_args, con, null_map, partition_layers, stats_collector_map);
    for (uint32_t farg_idx = 0; farg_idx < num_args; farg_idx++){
        // DUCKDB_EXIT_ON_ERROR(duckdb_register_aggregate_function(con, funcs[farg_idx]));
        // std::cout << "ran aggregate register successfully!" << std::endl;
    
        // DUCKDB_EXIT_ON_ERROR(duckdb_register_aggregate_function(con, ignore_gn_funcs[farg_idx]));
        // std::cout << "ran aggregate register ignore group nums successfully!" << std::endl;

        // DUCKDB_EXIT_ON_ERROR(duckdb_register_aggregate_function(con, window_funcs[farg_idx]));
        // std::cout << "ran aggregate register window successfully!" << std::endl;

        register_func(log_funcs[farg_idx], con);
        std::cout << "ran top-level log register successfully!" << std::endl;

        register_func(volatile_log_funcs[farg_idx], con);
        std::cout << "ran top-level volatile log register successfully!" << std::endl;

        register_func(boolean_log_funcs[farg_idx], con);
        std::cout << "ran top-level boolean log register successfully!" << std::endl;
    }

    Funcs table_funcs = traceprov_add_funcs(con);

    if (options.load_micro_benchmarks){
        traceprov_create_vary_chunk_funcs(con);
        traceprov_create_debug_table_funcs(con);
        traceprov_create_chunk_table_func(con);
        traceprov_create_chunk_table_adapted_func(con);
    }
        
    //if (options.disable_column_optimizer){
    //    DUCKDB_RUN_SHORT_QUERY(con, "SET disabled_optimizers = 'unused_columns';", "run disable optimizer..;");
        //DUCKDB_RUN_SHORT_QUERY(con, "SET disabled_optimizers = 'COLUMN_LIFETIME,unused_columns';", "run disable optimizer..;");
    //}

    DUCKDB_RUN_SHORT_QUERY(con, "ANALYZE;", "run analyze;");
    traceprov_setup_page_cache(options.num_threads, options.initial_page_count);

    if (!options.no_reinit_state){
        DUCKDB_RUN_SHORT_QUERY(con, "select reinit_state();", "reinit-state");
        reset_global_context();
    }

    if (options.min_layer_number){
        traceprov_current.maximum_local_layer_used_copy = options.min_layer_number;
    }

    if (options.dry_run)
        return 0;

    char thread_set_query[256] = {0};
    sprintf(thread_set_query, "SET threads=%d;", options.num_threads);
    DUCKDB_RUN_SHORT_QUERY(con, thread_set_query, "setting threads");

    // DUCKDB_RUN_SHORT_QUERY(con, "set memory_limit='58GiB';", "setting memory");

    if (IS_SET(options.index_scan_percentage)){
        std::string indx_set_query = "SET index_scan_percentage=" + options.index_scan_percentage + ";";
        DUCKDB_RUN_SHORT_QUERY(con, indx_set_query.c_str(), "setting index scan percent");
    }

    if (!options.main_once_extra_all){

        // If there is a warm up time, need to rerun the query those many times before we actually run it.
        if (options.warm_up_time){
            Options warm_up_options = options;
            warm_up_options.stats_path = "";
            uint64_t elapsed = 0;
            if (options.pre_main_sql->size()){
                for (auto pre_main_sql: *options.pre_main_sql){
                    DUCKDB_RUN_SHORT_QUERY(con, pre_main_sql.c_str(), "pre-main-sql");
                }
            }
            elog(INFO, "Warming up for %ld\n", warm_up_options.warm_up_time);
            while (elapsed < options.warm_up_time){
                const auto start_time = std::chrono::steady_clock::now();
                perform_query(&warm_up_options, con, in_sql, NULL, NULL, NULL, "");
                const auto end_time = std::chrono::steady_clock::now();
                elapsed += (std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time)).count();
            }
        }
        duckdb_prepared_statement cached_stmt = NULL;
        for (int i = 0; i < options.repeat; i++){
            char final_profile_out[256] = {0};
            char final_stats_query[256] = {0};
            char profile_out[256] = {0};
            std::string layer_stats_out_str = "";
            if (IS_SET(options.profile_out_path)){
                sprintf(profile_out, options.profile_out_path.c_str(), i);
                sprintf(final_profile_out, TP_SET_PROFILE_OUTPUT, profile_out);
            }

            if (IS_SET(options.stats_path)){
                char stats_query[256] = {0};
                sprintf(stats_query,  options.stats_path.c_str(), i);
                sprintf(final_stats_query, (options.is_new_sd ? TP_SET_STATS_OUTPUT_NEW : TP_SET_STATS_OUTPUT), stats_query);
                std::cout << "STATS QUERY: " << final_stats_query << std::endl;
            }
            
            if (IS_SET(options.layer_stats_out)){
                char layer_stats_out[256] = {0};
                sprintf(layer_stats_out, options.layer_stats_out.c_str(), i);
                layer_stats_out_str = std::string(layer_stats_out);
            }

            if (options.pre_main_sql->size()){
                for (auto pre_main_sql: *options.pre_main_sql){
                    DUCKDB_RUN_SHORT_QUERY(con, pre_main_sql.c_str(), "pre-main-sql");
                }
            }

            auto curr_result = perform_query(&options, con, in_sql, &agg_result, final_profile_out, final_stats_query, layer_stats_out_str, &cached_stmt);

            auto extra_sqls_clone = (extra_sqls);
            std::vector<TraceProvTableExtra *> table_func_extra;

            const auto spec_result = augment_extra_sql(
                extra_sqls_clone,
                &options,
                &table_func_extra,
                options.log_offsets->size() == 0 ? new std::vector<uint64_t>(1, -1) : options.log_offsets,
                partition_layers,
                nullptr
            );
            if (spec_result){
                curr_result->option.misc_store.sql_compilation_time = spec_result->sql_compilation_time;
            }

            uint32_t extra_idx = 0;
            // DUCKDB_RUN_SHORT_QUERY(con, "SET disabled_optimizers = 'join_order,build_side_probe_side';", "run disable join order");


            for (auto extra_sql: extra_sqls_clone){
                
                TraceProvTableExtra *extra = NULL;
                if (table_func_extra.size() != 0){
                    extra = table_func_extra.at(extra_idx);
                }

                duckdb_table_function_set_extra_info(table_funcs.tp_read_func, extra, free); // whatever
                duckdb_table_function_set_extra_info(table_funcs.tp_read_offset_func, extra, free);

                // traceprov_attempt_prefaults();
                extra_idx++;
                Options new_options = options;
                new_options.no_reinit_state = true;
                new_options.capture_lineage = false;
                new_options.stats_path = "";
                new_options.get_log_size = false;
                new_options.disable_column_optimizer = false;
                std::string *extra_profile_str = new std::string((std::string(profile_out) + "_" + std::to_string(extra_idx) + "_extra.json"));
                memset(final_profile_out, 0, sizeof(char)*256);
                sprintf(final_profile_out, TP_SET_PROFILE_OUTPUT, extra_profile_str->c_str());
                new_options._extra_output = extra_sql.extra;
                for (uint32_t discard_idx = 0; discard_idx < options.extra_multiple_count; discard_idx++){
                    perform_query(
                        &new_options,
                        con,
                        extra_sql.sql,
                        // Discard all the ones that are before the last run.
                        discard_idx == (options.extra_multiple_count - 1) ? &agg_result : NULL, 
                        discard_idx == (options.extra_multiple_count - 1) ? final_profile_out : NULL,
                        NULL,
                        ""
                    );
                }
                // eh, so that the state is still consistent later.
                duckdb_table_function_set_extra_info(table_funcs.tp_read_offset_func, NULL, nullptr);
                duckdb_table_function_set_extra_info(table_funcs.tp_read_func, NULL, nullptr);
            }

            // DUCKDB_RUN_SHORT_QUERY(con, "SET disabled_optimizers = '';", "run enable all optimaztions");

        }
        duckdb_destroy_prepare(&cached_stmt);

    }else{
        // run main once.
        PerformQueryResult *main_result = NULL;
        {
            Options new_options = options;
            char final_stats_query[256] = {0};
            char final_profile_out[256] = {0};
            if (IS_SET(new_options.stats_path)){
                char stats_query[256] = {0};
                sprintf(stats_query,  new_options.stats_path.c_str(), 0);
                sprintf(final_stats_query, (options.is_new_sd ? TP_SET_STATS_OUTPUT_NEW : TP_SET_STATS_OUTPUT), stats_query);
                std::cout << "STATS QUERY: " << final_stats_query << std::endl;
            }
            for (int i  = 0; i < new_options.repeat; i++){
                auto run_option = new_options;
                if (i != new_options.repeat - 1){
                    run_option.stats_path = "";
                }
                if (IS_SET(run_option.profile_out_path)){
                    char profile_out[256] = {0};
                    sprintf(profile_out, options.profile_out_path.c_str(), 0, i);
                    sprintf(final_profile_out, TP_SET_PROFILE_OUTPUT, profile_out);
                }
                main_result = perform_query(&run_option, con, in_sql, &agg_result, final_profile_out, final_stats_query, "");
            }   
        }
        if (traceprov_use_index){
            const auto index_misc_key_value = setup_traceprov_indexes(con, &options, table_funcs);
            if (main_result){
                main_result->option.misc_store.index_time = index_misc_key_value.index_time;
                main_result->option.misc_store.layers_with_index = index_misc_key_value.layers_with_index;
            }
        }
        int extra_sql_idx = 0;
        auto extra_sqls_clone = (extra_sqls);
        std::vector<TraceProvTableExtra *> table_func_extra;
        const auto spec_result = augment_extra_sql(
            extra_sqls_clone,
            &options,
            &table_func_extra,
            options.log_offsets->size() == 0 ? new std::vector<uint64_t>(1, -1) : options.log_offsets,
            partition_layers,
            main_result != NULL ? main_result->data->output_log : nullptr
        );
        if (main_result != NULL){
            if (spec_result != NULL)
                main_result->option.misc_store.sql_compilation_time = spec_result->sql_compilation_time;
            if (main_result->data->output_log)
                delete main_result->data->output_log;
        }
        // Run extra all ;)
        for (auto extra_sql: extra_sqls_clone){

            TraceProvTableExtra *extra = NULL;
            if (table_func_extra.size() != 0){
                extra = table_func_extra.at(extra_sql_idx);
            }

            duckdb_table_function_set_extra_info(table_funcs.tp_read_func, (void *)extra, free);
            duckdb_table_function_set_extra_info(table_funcs.tp_read_offset_func, (void *)extra, free);

            extra_sql_idx++;
            Options extra_options = options;
            extra_options.no_reinit_state = true;
            extra_options.capture_lineage = false;
            extra_options.stats_path = "";
            extra_options.get_log_size = false;
            extra_options.output_column_idx = 0;
            char final_profile_out[256] = {0};
            duckdb_prepared_statement stmt = NULL;
            for (int i = 0; i < extra_options.repeat; i++){
                if (IS_SET(extra_options.profile_out_path)){
                    char profile_out[256] = {0};
                    sprintf(profile_out, options.profile_out_path.c_str(), extra_sql_idx, i);
                    sprintf(final_profile_out, TP_SET_PROFILE_OUTPUT, profile_out);
                }
                extra_options._extra_output = extra_sql.extra;
                perform_query(&extra_options, con, extra_sql.sql, &agg_result, final_profile_out, NULL, "", &stmt);
            }
            duckdb_destroy_prepare(&stmt);
            // eh, so that the state is still consistent later.
            duckdb_table_function_set_extra_info(table_funcs.tp_read_offset_func, NULL, nullptr);
            duckdb_table_function_set_extra_info(table_funcs.tp_read_func, NULL, nullptr);
        }
    }

    if (IS_SET(options.settings_out_path)){
        char settings_out_query[256] = {0};
        sprintf(settings_out_query, TP_DUMP_SETTINGS, options.settings_out_path.c_str());
        DUCKDB_RUN_SHORT_QUERY(con, settings_out_query, "dumping settings");
    }

    if (IS_SET(options.time_out_path)){
        std::string time_out_json = "[";
        for (uint64_t computed_time_idx = 0; computed_time_idx < agg_result.size(); computed_time_idx++){
            auto current = agg_result.at(computed_time_idx);
            if (computed_time_idx > 0) time_out_json += ",";
            const uint64_t width = (current->data->column_count);
            uint64_t row_count = 0;
            if (width > 0){
                // row_count = (current->data->at(0)->data->size());
                row_count = current->data->row_count;
            }
            time_out_json += "{";
            time_out_json += "\"time\":" + std::to_string(current->computed_time);
            time_out_json += ",";
            time_out_json += "\"width\":" + std::to_string(width);
            time_out_json += ",";
            time_out_json += "\"row_count\": " + std::to_string(row_count);
            time_out_json += ",";
            time_out_json += "\"option\": " + serialize_option(&current->option, null_map);
            if (current->worker_layer_time_dump){
                time_out_json += ",";
                time_out_json += "\"worker_layer_time\": " + serialize_worker_layer_time(current->worker_layer_time_dump);
                reset_layer_time(current->worker_layer_time_dump);
            }
            time_out_json += "}";
        }
        time_out_json += "]";
        std::ofstream out(options.time_out_path);
        out << time_out_json;
        out.close();
    }

    duckdb_disconnect(&con);
    duckdb_close(&db);   
}

static void populate_log_offset(
    uint64_t log_offset,
    TraceProvPointerContext *pc,
    TraceProvPartitionLayers *partition_layers,
    TraceProvPartitionInfo *partition_info
){
    partition_info->partition_time = 0;
    const auto start_time = std::chrono::steady_clock::now();
    initialize_global_context();

    // Need to, now, check which partitions to consider, for which rows.
    // The partition will be different for each worker (if there are some)
    // Ugh.

    // To do so, we first determine which entries in the top level log are pointers to aggregates.
    // If they were combined, things get a bit more complicated (need to join to the combine layer, and then read the partitions)
    TraceProvParseContext *parsed_back_context = NULL;
    List *graphs = deserializeTraceProvDependency(&parsed_back_context, NULL, TRACEPROV_GRAPH_FILE, false);
    if (list_length(graphs) != 1){
        elog(ERROR, "Expected only 1 graph!");
    }
    const TraceProvDependency *graph = (TraceProvDependency*)lfirst(list_head(graphs));

    auto worker_id = TRACEPROV_GET_WORKER_ID(log_offset);
    if (worker_id == 0){
        auto worker_layers = find_layers_across_workers(
            graph->headNumber,
            g_tp_duckdb_state.worker_local_contexts,
            0
        );
        if (worker_layers->size() != 1){
            elog(ERROR, "Expected worker id to be set!!");
        }
        auto worker_layer = worker_layers->at(0);
        worker_id = worker_layer.first;
        log_offset = TRACEPROV_SET_WORKER_ID(log_offset, worker_id);
    }

    void *row = traceprov_get_row(
        worker_id,
        graph->headNumber,
        TRACEPROV_STRIP_WORKER_ID(log_offset),
        pc->size_map->at(graph->headNumber)
    );

    if (partition_info->cached_data == NULL){
        partition_info->cached_data = new std::unordered_map<TraceProvLayerNumber, void *>;
    }
    partition_info->cached_data->insert({graph->headNumber, row});
    if (partition_info->layer_log_map == NULL){
        partition_info->layer_log_map = new std::unordered_map<TraceProvLayerNumber, uint64_t>;
    }
    partition_info->layer_log_map->insert({graph->headNumber, log_offset});

    if (!traceprov_use_partition_in_agg){
        const auto early_end_time = std::chrono::steady_clock::now();
        partition_info->partition_time = (std::chrono::duration_cast<std::chrono::microseconds>(early_end_time - start_time)).count();
        return;
    }

    for (auto pl_item: *partition_layers){
        const uint64_t logged_entry = ((uint64_t *)row)[pl_item.entry_idx];
        if (!TRACEPROV_GET_IS_COMBINED(logged_entry)){
            // In this case, the partition is embedded in the log entry.
            auto log_worker_id = TRACEPROV_GET_WORKER_ID(logged_entry);
            if (log_worker_id == 0){
                log_worker_id = 1;
            }
            auto log_bucket_id = TRACEPROV_GET_BUCKET(logged_entry);
            elog(INFO, "Using bucket: %d", log_bucket_id);
            if (partition_info->partition_data == NULL){
                partition_info->partition_data = new std::unordered_map<TraceProvLayerNumber, TraceProvWorkerPartition*>;
            }
            if (partition_info->partition_data->find(pl_item.layer) == partition_info->partition_data->end()){
                partition_info->partition_data->insert({pl_item.layer, new TraceProvWorkerPartition});
            }
            auto worker_partition_map = partition_info->partition_data->at(pl_item.layer);
            if (worker_partition_map->find(log_worker_id) != worker_partition_map->end()){
                elog(ERROR, "Expected to not find the worker in the map yet!");
            }
            worker_partition_map->insert({log_worker_id, log_bucket_id});
        }
    }

    const auto end_time = std::chrono::steady_clock::now();
    partition_info->partition_time = (std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time)).count();
}

bool is_initial_misc_key_value(const MiscKeyValue *key_value){
    return memcmp(&key_value->total_log_size, &g_init_misc_key_value, sizeof(MiscKeyValue));
}

static std::string serialize_map(TraceProvLayerTime* layer_time){
    std::string serialized = "{";
    bool needs_sep = false;
    for (auto entry: *layer_time){
        if (needs_sep){
            serialized += ",";
        }
        serialized += "\"" + std::to_string(entry.first) + "\": ";
        serialized += std::to_string(entry.second);
        needs_sep = true;
    }
    serialized += "}";
    return serialized;
}

static std::string serialize_worker_layer_time(TraceProvLayerTime** worker_layer_time){
    std::string serialized = "";
    uint32_t not_empty_count = 0;
    // elog(INFO, "traceprov thread count: %d", traceprov_thread_count);
    for (uint32_t worker_idx = 0; worker_idx < traceprov_thread_count; worker_idx++){
        const auto worker_layer_data = worker_layer_time[worker_idx];
        if (serialized != "") serialized += ",";
        if (!worker_layer_data->empty()){
            not_empty_count++;
            serialized += serialize_map(worker_layer_data);
        }else{
            serialized += "null";
        }
    }
    if (not_empty_count==0) return "null";
    serialized = "[" + serialized + "]";
    return serialized;
}
// This doesn't do all of option (that'll be too much)
static std::string serialize_option(Options *option, const TraceProvNullMap *null_map){
    std::string option_serialized;
    option_serialized += "{";
    option_serialized += "\"partition\": ";
    if (option->_extra){
        option_serialized += *traceprov_serialize_partition((TraceProvLayerPartition *)option->_extra);
    }else{
        option_serialized += "null";
    }
    option_serialized += ",";
    option_serialized += "\"extra\": [";
    if (IS_SET(option->_extra_output)){
        option_serialized += "\"";
        option_serialized += option->_extra_output;
        option_serialized += "\"";
    }
    option_serialized += "]";
    option_serialized += ",";
    option_serialized += "\"traceprov_assume_null\": ";
    option_serialized += traceprov_assume_null ? "true" : "false";
    if (null_map != NULL){
        option_serialized += ",";
        option_serialized += "\"null_map\": {";
        bool needs_sep = false;
        for (auto null_map_entry: *null_map){
            if (needs_sep){
                option_serialized += ",";
            }
            needs_sep = true;
            option_serialized += "\"";
            option_serialized += std::to_string(null_map_entry.first);
            option_serialized += "\": ";
            option_serialized += "[";
            bool needs_inner_sep = false;
            for (auto col_idx: *null_map_entry.second){
                if (needs_inner_sep){
                    option_serialized += ",";
                }
                needs_inner_sep = true;
                option_serialized += std::to_string(col_idx);
            }
            option_serialized += "]";
        }
        option_serialized += "}";
    }
    option_serialized += ",";
    option_serialized += "\"misc_key_value_total_log_size\": ";
    option_serialized += "[";
    option_serialized += "{";
    option_serialized += "\"page_requested_size\": ";
    option_serialized += std::to_string(option->misc_store.total_log_size.page_requested_size);
    option_serialized += ",";
    option_serialized += "\"page_used_size\": ";
    option_serialized += std::to_string(option->misc_store.total_log_size.page_used_size);
    option_serialized += ",";
    option_serialized += "\"bytes_used_size\": ";
    option_serialized += std::to_string(option->misc_store.total_log_size.bytes_used_size);
    option_serialized += "}";
    option_serialized += "]";
    option_serialized += ",";
    option_serialized += "\"misc_key_value_sql_compilation_time\": " + std::to_string(option->misc_store.sql_compilation_time);
    option_serialized += ",";
    option_serialized += "\"misc_key_value_layer_stats\": " +(option->misc_store.layer_stats == "" ? "null" : option->misc_store.layer_stats);
    option_serialized += ",";
    option_serialized += "\"misc_key_value_index_time\": " + std::to_string(option->misc_store.index_time);
    option_serialized += ",";
    option_serialized += "\"misc_key_value_layers_with_index\": " + (option->misc_store.layers_with_index == "" ? "null" : option->misc_store.layers_with_index);
    option_serialized += "}";
    return option_serialized;
}

std::string get_traceprov_table_view(const TraceProvLayerNumber layer_number){
    return "traceprov_lineage_view_" + std::to_string(layer_number);
}

TraceProvIndexContext *make_tp_index_context(const TraceProvLayerNumber layer_number){
    auto index_context = tp_alloc0_object(TraceProvIndexContext);
    index_context->index_context_mutex = new std::mutex;
    index_context->root_layer_number = layer_number;
    if (traceprov_use_hash_index)
        index_context->hash_index_map = new HashIndexItemMap;
    return index_context;
}


bool should_make_index(duckdb_connection con, std::string table_name){
    duckdb_result count_result;
    std::string query = "select count(*)::bigint from " + table_name;
    DUCKDB_EXIT_ON_ERROR_MSG(duckdb_query(con, query.c_str(), &count_result), duckdb_result_error(&count_result));
    duckdb_data_chunk data_chunk = duckdb_result_get_chunk(count_result, 0);
    duckdb_vector count_column_0 = duckdb_data_chunk_get_vector(data_chunk, 0);
    const uint64_t table_count = ((uint64_t *) duckdb_vector_get_data(count_column_0))[0];
    duckdb_destroy_data_chunk(&data_chunk);
    duckdb_destroy_result(&count_result);
    const uint64_t base_result_count = g_tp_duckdb_state.index_context->vector_size;
    const uint64_t average_per_out = table_count / base_result_count;
    return average_per_out <= MAX(DUCKDB_DEFAULT_SCAN_MAX_COUNT, DUCKDB_DEFAULT_SELECTIVITY*((double)(average_per_out)));
}

// In MOST cases, counts are less than 32.
// In few as low as 256.
// So generally a good idea to use a compact representation here, especially since misjudgint the sizes
// can blow it out the water (cache, hehe)
template <typename T>  HashIndexDir populate_counts(std::vector<duckdb_data_chunk> *chunks, const uint64_t prov_row_size, const uint64_t output_row_size){
    auto counts = new std::vector<T>(output_row_size+1, 0);
    auto counts_copy = new std::vector<T>(output_row_size+1, 0);
    // counts->
    for (auto chunk: *chunks){
        duckdb_vector row_id_vec = duckdb_data_chunk_get_vector(chunk, 0);
        auto row_id_data =  (uint64_t*)duckdb_vector_get_data(row_id_vec);
        const uint64_t chunk_size = duckdb_data_chunk_get_size(chunk);
        for (uint64_t row_id_index = 0; row_id_index < chunk_size; row_id_index++){
            counts->at(row_id_data[row_id_index])++;
        }
    }
    T last_sum = 0;
    T last_value = 0;
    for (uint64_t count_id = 0; count_id < output_row_size+1; count_id++){
        T current_sum = last_sum + last_value;
        last_value = counts->at(count_id);
        counts->at(count_id) = current_sum;
        counts_copy->at(count_id) = current_sum;
        last_sum = current_sum;
    }
    std::vector<uint64_t> *chunk_map = new std::vector<uint64_t>(prov_row_size);
    for (uint64_t chunk_idx = 0; chunk_idx < chunks->size(); chunk_idx++){
        auto chunk = chunks->at(chunk_idx);
        duckdb_vector row_id_vec = duckdb_data_chunk_get_vector(chunk, 0);
        auto row_id_data =  (uint64_t*)duckdb_vector_get_data(row_id_vec);
        const uint64_t chunk_size = duckdb_data_chunk_get_size(chunk);
        for (uint64_t row_id_index = 0; row_id_index < chunk_size; row_id_index++){
            chunk_map->at(counts_copy->at(row_id_data[row_id_index])++) = ((uint64_t)(chunk_idx) << 32) | (row_id_index);
        }
    }
    // Now go over the chunks again and inser them in the correct place.
    return HashIndexDir {
        .count_list = counts,
        .count_map = chunk_map
    };
}


MiscKeyValue setup_traceprov_indexes(duckdb_connection con, Options *options, Funcs table_funcs){
    const auto start_time = std::chrono::steady_clock::now();
    TraceProvDerivationSpec *result_spec = NULL;
    std::vector<std::string> ddls;
    std::vector<std::pair<uint64_t, uint64_t>> added_ddls;
    auto layer_string_map = get_layer_string_map(options, &result_spec, ddls, added_ddls);
    auto table_extra = make_table_extra();
    table_extra->pointer_spec = result_spec->p_context;
    table_extra->partition_spec = NULL;

    duckdb_table_function_set_extra_info(table_funcs.tp_read_func, table_extra, free); // whatever
    duckdb_table_function_set_extra_info(table_funcs.tp_read_offset_func, table_extra, free);

    Options new_options = get_base_option();
    new_options.no_reinit_state = true;
    g_tp_duckdb_state.index_context = make_tp_index_context(result_spec->root_layer_number);
    // std::vector<PerformQueryResult *> agg_result;
    std::string layers_with_index = "";
    layers_with_index += "[";
    bool needs_sep = false;
    for (auto layer_string_pair : *layer_string_map){
        if (traceprov_use_hash_index){
            // Cache the chunks instead of creating a table.
            // We then scan over it to determine cache
            // std::string query = "select * from (" + layer_string_pair.second + ") order by column_0";
            const auto query_result = perform_query(&new_options, con, layer_string_pair.second, NULL, NULL, NULL, "", NULL, true);
            auto hash_index_item = tp_alloc0_object(HashIndexItem);
            auto light_data = query_result->data;
            auto chunk_cache = light_data->chunk_cache;
            auto row_count = light_data->row_count;
            hash_index_item->chunk_cache = chunk_cache;
            if (list_member_int(result_spec->directly_derivable, layer_string_pair.first)){
                // setup a vector to hold counts.
                if (!g_tp_duckdb_state.index_context->vector_size){
                    elog(ERROR, "Expected vector size to be set!");
                }
                uint8_t col_size = 0;
                HashIndexDir dir;
                if (row_count < (((uint64_t)1)<<8)){
                    dir = populate_counts<uint8_t>(chunk_cache, row_count, g_tp_duckdb_state.index_context->vector_size);
                    col_size = sizeof(uint8_t);
                }else if (row_count < (((uint64_t)1)<<16)){
                    dir = populate_counts<uint16_t>(chunk_cache, row_count, g_tp_duckdb_state.index_context->vector_size);
                    col_size = sizeof(uint16_t);
                } else if (row_count < (((uint64_t)1)<<32)){
                    dir = populate_counts<uint32_t>(chunk_cache, row_count, g_tp_duckdb_state.index_context->vector_size);
                    col_size = sizeof(uint32_t);
                }else {
                    dir = populate_counts<uint64_t>(chunk_cache, row_count, g_tp_duckdb_state.index_context->vector_size);
                    col_size = sizeof(uint64_t);
                }
                hash_index_item->count_list_item_size = col_size;
                hash_index_item->dir = dir;
            }
            hash_index_item->running_count = row_count;
            g_tp_duckdb_state.index_context->hash_index_map->insert({layer_string_pair.first,hash_index_item});
        }else{
            // Do the original indexing
            std::string layer_sql = get_traceprov_table_view(layer_string_pair.first);
            auto table_name = get_traceprov_table_view(layer_string_pair.first);
            std::string wrapped = "create or replace temp table " + table_name + " as (" + layer_string_pair.second + ");";
            elog(INFO, "Index query: %s", wrapped.c_str())
            const bool old_traceprov_force_seq_scan = traceprov_force_seq_scan;
            traceprov_force_seq_scan = true;
            perform_query(&new_options, con, wrapped, NULL, NULL, NULL, "", NULL);
            traceprov_force_seq_scan = old_traceprov_force_seq_scan;
            if (list_member_int(result_spec->directly_derivable, layer_string_pair.first) && should_make_index(con, table_name)){
                // Need to create index.
                std::string index_name = table_name + "_column_0_idx";
                DUCKDB_RUN_SHORT_QUERY(con, ("drop index if exists "+ index_name).c_str(), ("drop index "+index_name));
                char *index_command = tp_psprintf("create index %s on %s (column_0)", index_name.c_str(), table_name.c_str());
                DUCKDB_RUN_SHORT_QUERY(con, index_command, index_command);
                if (needs_sep){
                    layers_with_index += ",";
                }
                layers_with_index += std::to_string(layer_string_pair.first);
                needs_sep = true;
            }
        }
    }
    layers_with_index += "]";
    g_tp_duckdb_state.index_context->root_layer_number = 0;
    g_tp_duckdb_state.index_context->directly_derivable = result_spec->directly_derivable;

    duckdb_table_function_set_extra_info(table_funcs.tp_read_func, nullptr, nullptr);
    duckdb_table_function_set_extra_info(table_funcs.tp_read_offset_func, nullptr, nullptr);
    const auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    MiscKeyValue key_value;
    key_value.index_time = duration.count();
    key_value.layers_with_index = layers_with_index;
    return key_value;
}

std::unordered_map<TraceProvLayerNumber, std::string> *get_sql_mapping(
    TraceProvDerivationSpec *result_spec,
    Options *options,
    std::vector<std::string> &ddls,
    std::vector<std::pair<uint64_t, uint64_t>> &added_ddls,
    void *row_content=NULL
){

    auto layer_string_map = new std::unordered_map<TraceProvLayerNumber, std::string>;
    std::unordered_map<uint64_t, std::string> sql_cache;
    for (auto result_map_pair: *result_spec->result_map){
        if (result_map_pair.second->tag == T_TP_RELATION && traceprov_use_implicit_union){
            // In this case, it is a simple scan.
            // Apparently, for some reason, DuckDB does not parallelise this????
            // Anyways, right now, that breaks things.
            // So, remember that it was a simple scan.
            TraceProvRelation *relation = (TraceProvRelation *)result_map_pair.second;
            relation->rel_args->table_flags |= TRACEPROV_TABLE_SEQ_SCAN;
        }
        if ((options->traceprov_layers_to_derive->size() != 0) &&
            (std::find(
                options->traceprov_layers_to_derive->begin(), 
                options->traceprov_layers_to_derive->end(), result_map_pair.first
            )) == options->traceprov_layers_to_derive->end())
            {
                continue;
            }
        auto node_sql = traceprov_node_to_sql(
            result_map_pair.second, 
            TraceProvToSQLContext{
                .context = result_spec->parse_context,
                .use_table_def = true,
                .ddls = &ddls,
                .added_ddls = &added_ddls,
                .pointer_context = NULL,
                .cache = traceprov_skip_sql_cache ? NULL : &sql_cache,
                .row_content = (uint64_t*)row_content
            }
        );
        layer_string_map->insert({result_map_pair.first, node_sql});
    }
    return layer_string_map;
}

std::unordered_map<TraceProvLayerNumber, std::string> *get_layer_string_map(
    Options *options,
    TraceProvDerivationSpec **derivation_spec,
    std::vector<std::string> &ddls,
    std::vector<std::pair<uint64_t, uint64_t>> &added_ddls,
    const bool derive_sql_mapping
){
    TraceProvParseContext *parsed_back_context;
    char *parsed_sql = NULL;
    const auto start_time = std::chrono::steady_clock::now();
    auto result_spec = get_generic_derivation_spec(&parsed_back_context, NULL, &parsed_sql);
    if (traceprov_use_join_filter_rewrite){
        for (auto entry: *result_spec->result_map){
            result_spec->result_map->at(entry.first) = traceprov_perform_join_to_condition(entry.second, result_spec->root_layer_number);
        }
    }
    result_spec->parse_context = parsed_back_context;
    *derivation_spec = result_spec;
    if (!derive_sql_mapping) {
        const auto end_time = std::chrono::steady_clock::now();
        result_spec->sql_compilation_time = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        return nullptr;
    }
    auto layer_string_map = get_sql_mapping(
        result_spec,
        options,
        ddls,
        added_ddls
    );
    const auto later_end_time = std::chrono::steady_clock::now();
    if (result_spec->sql_compilation_time != 0){
        elog(ERROR, "Expected SQL compilation time to be empty!");
    }
    result_spec->sql_compilation_time = std::chrono::duration_cast<std::chrono::microseconds>(later_end_time - start_time).count();
    return layer_string_map;
}

std::string get_index_query(const TraceProvLayerNumber layer, const uint64_t offset){
    if (traceprov_use_hash_index){
        return std::string(tp_psprintf("select * from traceprov_read_chunk_cache(%ld::bigint, %ld::bigint)", layer, offset));
    }
    std::string query = std::string("select * from ") + get_traceprov_table_view(layer);
    if (g_tp_duckdb_state.index_context){
        if (list_member_int(g_tp_duckdb_state.index_context->directly_derivable, layer)){
            // Need to check the offset vector.
            // const uint64_t column_0 = g_tp_duckdb_state.index_context->vector_data[offset];
            query += std::string(" where column_0=") + std::to_string(offset);
        }
    }
    return query;
}

static int global_counter = 0;

TraceProvDerivationSpec* augment_extra_sql(
    std::vector<ExtraQuery> &extra_sqls,
    Options *options,
    std::vector<TraceProvTableExtra *> *table_func_extra,
    std::vector<uint64_t> *log_offsets,
    TraceProvPartitionLayers *partition_layers,
    std::vector<uint64_t> *output_log_offset
){
    if (!options->traceprov_perform_derivation) return NULL;
    TraceProvDerivationSpec *result_spec = NULL;
    std::vector<std::string> ddls;
    std::vector<std::pair<uint64_t, uint64_t>> added_ddls;
    auto layer_string_map = get_layer_string_map(options, &result_spec, ddls, added_ddls, !traceprov_use_join_filter_rewrite);
    for (auto log_offset: *log_offsets){
        TraceProvPartitionInfo *info = NULL;
        if (log_offset != -1 && !traceprov_use_index){
            info = new TraceProvPartitionInfo;
            info->cached_data = NULL;
            info->partition_data = NULL;
            info->layer_log_map = NULL;
            const uint64_t mapped_true_offset = output_log_offset == nullptr ? log_offset : output_log_offset->at(log_offset);
            populate_log_offset(mapped_true_offset, result_spec->p_context, partition_layers, info);
            if (traceprov_use_join_filter_rewrite){
                const auto _start_time = std::chrono::steady_clock::now();
                layer_string_map = get_sql_mapping(
                    result_spec,
                    options,
                    ddls,
                    added_ddls,
                    info->cached_data->at(result_spec->root_layer_number)
                );
                const auto _end_time = std::chrono::steady_clock::now();
                info->partition_time += (std::chrono::duration_cast<std::chrono::microseconds>(_end_time - _start_time)).count();
            }
        }
        for (auto layer_string_pair: *layer_string_map){
            auto table_extra = make_table_extra();
            uint64_t extra_added = 0;
            auto node_sql = layer_string_pair.second;
            if (traceprov_use_index && log_offset != -1){
                node_sql = get_index_query(layer_string_pair.first, log_offset);
            }
            elog(INFO, "SQL Query: %s", node_sql.c_str());
            if (options->traceprov_materialize_derivation){
                std::string table_name = "traceprov_lineage_" + std::to_string(layer_string_pair.first);
                if (log_offset != -1){
                    table_name += "_" + std::to_string(log_offset);
                }
                node_sql = "create or replace table " + table_name + " as (" + node_sql + ")";
                if (options->dump_base_table){
                    for (auto ddl_string : ddls){
                        std::string base_table_name = "base_table_" + std::to_string(global_counter++);
                        extra_sqls.push_back(ExtraQuery{.sql = ddl_string, .extra = ""});
                        extra_sqls.push_back(ExtraQuery{.sql = "create or replace table " + base_table_name + " as (" + ddl_string + ")", .extra = ""});
                        elog(INFO, "Table: %s", base_table_name.c_str());
                        elog(INFO, "SQL (Table): %s", extra_sqls.back().sql.c_str());
                        extra_added += 2;
                    }
                }
            }
            if (options->traceprov_dry_run_derivation){
                elog(INFO, "SQL Query: %s", node_sql.c_str());
            }else {
                table_extra->pointer_spec = result_spec->p_context;
                table_extra->partition_spec = info;
                for (uint64_t e_idx = 0; e_idx < extra_added + 1; e_idx++)
                    table_func_extra->push_back(table_extra);
                std::string extra_str = "";
                extra_str += "[";
                extra_str += "layer-" + std::to_string(layer_string_pair.first);
                if (log_offset != -1){
                    extra_str += ",";
                    extra_str += "log_offset-" + std::to_string(log_offset);
                }
                if (info){
                    if (info->partition_time != 0){
                        extra_str += ",";
                        extra_str += "partition_time-" + std::to_string(info->partition_time);
                    }
                }
                extra_str += "]";
                extra_sqls.push_back(ExtraQuery{.sql = node_sql, .extra = extra_str});
            }
        }
    }

    return result_spec;
}
