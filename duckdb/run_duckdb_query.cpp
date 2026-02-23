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

#define TP_ENABLE_PROFILING "PRAGMA enable_profiling=json"
#define TP_ENABLE_PROFILING_QUERY_TREE "PRAGMA enable_profiling=query_tree"
#define TP_SET_PROFILE_OUTPUT "PRAGMA profile_output='%s'"
#define TP_DISABLE_PROFILING "PRAGMA disable_profiling;"

// For OLD SmokedDuck.
#define TP_ENABLE_LINEAGE "PRAGMA enable_lineage;"
#define TP_DISABLE_LINEAGE "PRAGMA disable_lineage;"
#define TP_CLEAR_LINEAGE "PRAGMA clear_lineage;"

// For new SD.
#define TP_ENABLE_LINEAGE_NEW "PRAGMA set_lineage(True)"
#define TP_DISABLE_LINEAGE_NEW "PRAGMA set_lineage(False)"
#define TP_CLEAR_LINEAGE_NEW TP_CLEAR_LINEAGE

#define TP_SET_STATS_OUTPUT "COPY (select * from duckdb_queries_list() where query = ? order by query_id desc limit 1) TO '%s'"
#define TP_DUMP_SETTINGS "copy (select json_group_object(name, value) as settings from duckdb_settings()) TO '%s';"

#define TP_SET_STATS_OUTPUT_NEW "copy (select * from lineage_meta()) to '%s'"

#undef sprintf

#include "duckdb.hpp"
#include "traceprov.hpp"
#include "utils.hpp"

#include "traceprov_settings.hpp"

typedef uint32_t TraceProvLayerNumber;

typedef struct TraceProvDescriptor {
    TraceProvLayerNumber layer_number;
    void *entry;
} TraceProvDescriptor;

typedef struct {
    TraceProvDescriptor *descriptor;
    std::vector<uint64_t> *data;
} TraceProvColumnData;

typedef std::vector<TraceProvColumnData*> TraceProvData;

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
    int num_threads;
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
    // via --top_log_num
    uint32_t top_level_log_layer_number;
    // via --log_offset
    int64_t log_offset;
    /** TraceProv Settings */
    // Note that the values are not repeated here (the update is inlined for these.)
    // via --traceprov_use_partition_in_agg
    // via --traceprov_use_partition_in_log
    // via --traceprov_use_row_in_agg_partition
};

#define IS_OPTION(X) (strcmp(argv[i], X) == 0)
#define IS_SET(X) (X.size() != 0)

struct Options parse_args(int argc, char **argv){
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
        .top_level_log_layer_number = 0,
        .log_offset = -1
    };
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
            for (std::string extra_file_path; std::getline(extra_file_stream, extra_file_path);){
                if (extra_file_path.size() > 0){
                    options.extra_query_paths->push_back(extra_file_path);
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
        } else if (IS_OPTION("--top_log_num")){
            options.top_level_log_layer_number = std::atoi(argv[++i]);
            continue;
        } else if (IS_OPTION("--log_offset")){
            options.log_offset = std::atoi(argv[++i]);
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

    if (options.top_level_log_layer_number == 0)
        options.top_level_log_layer_number = options.min_layer_number - 1;

    return options;
};

// Simply populates the data, given a chunk.
static void populate_traceprov_data(
    TraceProvData *traceprov_data, 
    duckdb_data_chunk *chunk
){
    const uint64_t column_count = duckdb_data_chunk_get_column_count(*chunk);
    const uint64_t row_count = duckdb_data_chunk_get_size(*chunk);
    // Unlikely because it'll happen just once, for the first chunk.
    if ((column_count != traceprov_data->size())){
        if (traceprov_data->size() != 0){
            std::cout << "Attempting to set different number of column entries" << std::endl;
            exit(1);
        }
        for (uint64_t col_idx = 0; col_idx < column_count; col_idx++){
            auto column_data = new TraceProvColumnData;
            column_data->data = new std::vector<uint64_t>();
            traceprov_data->push_back(column_data);
        }
    }
    for (uint64_t col_idx = 0; col_idx < column_count; col_idx++){
        duckdb_vector col = duckdb_data_chunk_get_vector(*chunk, col_idx);
        uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col);
        uint64_t *col_validity = (uint64_t *)duckdb_vector_get_validity(col);
        auto current_column_data = traceprov_data->at(col_idx)->data;
        for (uint64_t row_idx = 0; row_idx < row_count; row_idx++){
            if((!duckdb_validity_row_is_valid(col_validity, row_idx))){
                std::cout << "Expected all non-null row, for now" << std::endl;
                exit(1);
            }
            current_column_data->push_back(col_data[row_idx]);
        }
    }
}

static void populate_log_offset(Options *option, std::string extra_sql);
static std::string serialize_option(Options *option);

typedef struct PerformQueryResult {
    int64_t computed_time;
    TraceProvData *data;
    Options option;
} PerformQueryResult;

PerformQueryResult *make_result(int64_t computed_time, TraceProvData *data, Options *options){
    auto result = new PerformQueryResult;
    result->computed_time = computed_time;
    result->data = data;
    result->option = *options;
    return result;
}

void perform_query(
    struct Options *options, 
    duckdb_connection &con, 
    std::string &in_sql,
    std::vector<PerformQueryResult *> &agg_result,
    const char *final_profile_out,
    const char *final_stats_query
){

    #if TRACEPROV_SD_MODE==0
    if (!options->no_reinit_state){
        DUCKDB_RUN_SHORT_QUERY(con, "select reinit_state();", "reinit-state");
        reset_global_context();
    }
    #endif

    if (options->min_layer_number){
        traceprov_current.maximum_local_layer_used = options->min_layer_number;
    }


    if (options->capture_lineage){
        DUCKDB_RUN_SHORT_QUERY(con, (options->is_new_sd ? TP_ENABLE_LINEAGE_NEW : TP_ENABLE_LINEAGE), "enable lineage");
        DUCKDB_RUN_SHORT_QUERY(con, (options->is_new_sd ? TP_CLEAR_LINEAGE_NEW : TP_CLEAR_LINEAGE), "clear lineage");
    }

    if (IS_SET(options->profile_out_path)){
        if (final_profile_out == NULL)
            elog(ERROR, "Expected profile out to be set!");
        DUCKDB_RUN_SHORT_QUERY(con, (options->query_tree ? TP_ENABLE_PROFILING_QUERY_TREE : TP_ENABLE_PROFILING), "enable profiling");
        DUCKDB_RUN_SHORT_QUERY(con, final_profile_out, "set json out");
    }


    // Need to use both, the pending and the streaming API.
    duckdb_prepared_statement stmt;
    duckdb_result final_result;

    auto start_time = std::chrono::steady_clock::now();

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

    std::cout << "Is streaming: " << duckdb_result_is_streaming(final_result) << std::endl;

    uint64_t chunk_count = 0;
    auto traceprov_data = new TraceProvData;

    if (options->use_pending){
        while (true) {
            duckdb_data_chunk data_chunk = duckdb_stream_fetch_chunk(final_result);
            if (!data_chunk) break;
            chunk_count++;
            populate_traceprov_data(traceprov_data, &data_chunk);
            duckdb_destroy_data_chunk(&data_chunk);
        }
    }else{
        uint64_t total_chunk_count = duckdb_result_chunk_count(final_result);
        for (idx_t chunk_idx = 0; chunk_idx < total_chunk_count; chunk_idx++){
            duckdb_data_chunk data_chunk = duckdb_result_get_chunk(final_result, chunk_idx);
            populate_traceprov_data(traceprov_data, &data_chunk);
            duckdb_destroy_data_chunk(&data_chunk);
            chunk_count++;
        }
    }

    duckdb_destroy_result(&final_result);
    duckdb_destroy_prepare(&stmt);

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    if (duration.count() == 0){
        std::cout << "Got 0 as the measured time, use a finer granularity..." << std::endl;
        exit(1);
    }

    agg_result.push_back(make_result(duration.count(), traceprov_data, options));

    // This needs to run before anything else bc of overwrites.
    DUCKDB_RUN_SHORT_QUERY(con, TP_DISABLE_PROFILING, "disable profiling");

    if (options->capture_lineage){
        DUCKDB_RUN_SHORT_QUERY(con, (options->is_new_sd ? TP_DISABLE_LINEAGE_NEW : TP_DISABLE_LINEAGE), "disable lineage");
    }

    std::cout << "Chunks: " << chunk_count;

    if (IS_SET(options->stats_path)){
        if (final_stats_query == NULL)
            elog(ERROR, "Expected final stats query to be set!");

        if (options->is_new_sd){
            DUCKDB_RUN_SHORT_QUERY(con, final_stats_query, "new sd result dump");
        }else{
            if((duckdb_prepare(con, final_stats_query, &stmt)) == DuckDBError){
                std::cout << duckdb_prepare_error(stmt) << std::endl;
            }
            DUCKDB_EXIT_ON_ERROR(duckdb_bind_varchar(stmt, 1, in_sql.c_str()));
            DUCKDB_EXIT_ON_ERROR(duckdb_execute_prepared(stmt, &final_result));
            duckdb_destroy_result(&final_result);
            duckdb_destroy_prepare(&stmt);
        }
    }
}

int main(int argc, char **argv){
    struct Options options = parse_args(argc, argv);

    std::ifstream in_sql_stream(options.input_path.c_str());
    std::stringstream buffer;
    buffer << in_sql_stream.rdbuf();
    std::string in_sql(buffer.str());
    std::cout << "From file: " << in_sql << std::endl;
    std::vector<PerformQueryResult *> agg_result;

    std::vector<std::string> extra_sqls;

    for (auto extra_sql_path: *options.extra_query_paths){
        std::string extra_sql = "";
        std::ifstream extra_sql_stream(extra_sql_path.c_str());
        std::stringstream extra_buffer;
        extra_buffer << extra_sql_stream.rdbuf();
        extra_sql = extra_buffer.str();
        // std::cout << "From file (extra): " << extra_sql << std::endl;
        extra_sqls.push_back(extra_sql);
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

    if (options.is_new_sd){
        if (!IS_SET(options.sd_extension_path))
            elog(ERROR, "Expected extenstion path to be set!");
        std::string load_str = "LOAD '" + options.sd_extension_path + "';";
        DUCKDB_RUN_SHORT_QUERY(con, load_str.c_str(), "load new sd extension");
    }

    #if TRACEPROV_SD_MODE==0
    const uint32_t num_args = 12;
    duckdb_aggregate_function *funcs = traceprov_create_funcs(num_args, false, false);
    duckdb_aggregate_function *ignore_gn_funcs = traceprov_create_funcs(num_args, false, true);
    duckdb_aggregate_function *window_funcs = traceprov_create_window_funcs(num_args);
    duckdb_scalar_function *log_funcs = traceprov_create_log_function(num_args, false);
    duckdb_scalar_function *volatile_log_funcs = traceprov_create_log_function(num_args, true);
    for (uint32_t farg_idx = 0; farg_idx < num_args; farg_idx++){
        DUCKDB_EXIT_ON_ERROR(duckdb_register_aggregate_function(con, funcs[farg_idx]));
        std::cout << "ran aggregate register successfully!" << std::endl;
    
        DUCKDB_EXIT_ON_ERROR(duckdb_register_aggregate_function(con, ignore_gn_funcs[farg_idx]));
        std::cout << "ran aggregate register ignore group nums successfully!" << std::endl;

        DUCKDB_EXIT_ON_ERROR(duckdb_register_aggregate_function(con, window_funcs[farg_idx]));
        std::cout << "ran aggregate register window successfully!" << std::endl;

        DUCKDB_EXIT_ON_ERROR(duckdb_register_scalar_function(con, log_funcs[farg_idx]));
        std::cout << "ran top-level log register successfully!" << std::endl;

        DUCKDB_EXIT_ON_ERROR(duckdb_register_scalar_function(con, volatile_log_funcs[farg_idx]));
        std::cout << "ran top-level volatile log register successfully!" << std::endl;
    }

    duckdb_scalar_function reinit_func = traceprov_create_reinit_state();
    DUCKDB_EXIT_ON_ERROR(duckdb_register_scalar_function(con, reinit_func));

    duckdb_table_function tp_read_func = traceprov_create_table_func();
    DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, tp_read_func));

    duckdb_table_function tp_read_offset_func = traceprov_create_table_offset_func();
    DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, tp_read_offset_func));
    
    duckdb_scalar_function tp_table_window_func = traceprov_create_table_window_func(2, 0, NULL);
    DUCKDB_EXIT_ON_ERROR(duckdb_register_scalar_function(con, tp_table_window_func));

    if (options.load_micro_benchmarks){
        traceprov_create_vary_chunk_funcs(con);
    }

    #endif
        
    if (options.disable_column_optimizer){
        DUCKDB_RUN_SHORT_QUERY(con, "SET disabled_optimizers = 'unused_columns';", "run disable optimizer..;");
        //DUCKDB_RUN_SHORT_QUERY(con, "SET disabled_optimizers = 'COLUMN_LIFETIME,unused_columns';", "run disable optimizer..;");
    }

    DUCKDB_RUN_SHORT_QUERY(con, "ANALYZE;", "run analyze;");

    #if TRACEPROV_SD_MODE==0
    if (!options.no_reinit_state){
        DUCKDB_RUN_SHORT_QUERY(con, "select reinit_state();", "reinit-state");
        reset_global_context();
    }
    #endif

    if (options.min_layer_number){
        traceprov_current.maximum_local_layer_used = options.min_layer_number;
    }

    if (options.dry_run)
        return 0;

    char thread_set_query[256] = {0};
    sprintf(thread_set_query, "SET threads=%d;", options.num_threads);
    DUCKDB_RUN_SHORT_QUERY(con, thread_set_query, "setting threads");

    if (IS_SET(options.index_scan_percentage)){
        std::string indx_set_query = "SET index_scan_percentage=" + options.index_scan_percentage + ";";
        DUCKDB_RUN_SHORT_QUERY(con, indx_set_query.c_str(), "setting index scan percent");
    }

    if (!options.main_once_extra_all){
        for (int i = 0; i < options.repeat; i++){
            char final_profile_out[256] = {0};
            char final_stats_query[256] = {0};
            char profile_out[256] = {0};
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
            
            perform_query(&options, con, in_sql, agg_result, final_profile_out, final_stats_query);
            uint32_t extra_idx = 0;
            for (auto extra_sql: extra_sqls){
                extra_idx++;
                Options new_options = options;
                new_options.no_reinit_state = true;
                new_options.capture_lineage = false;
                new_options.stats_path = "";
                std::string *extra_profile_str = new std::string((std::string(profile_out) + "_" + std::to_string(extra_idx) + "_extra.json"));
                memset(final_profile_out, 0, sizeof(char)*256);
                sprintf(final_profile_out, TP_SET_PROFILE_OUTPUT, extra_profile_str->c_str());
                perform_query(
                    &new_options, con, extra_sql, agg_result, 
                    final_profile_out,
                    NULL
                );
            }
        }
    }else{
        // run main once.
        {
            Options new_options = options;
            new_options.profile_out_path = "";
            char final_stats_query[256] = {0};
            if (IS_SET(new_options.stats_path)){
                char stats_query[256] = {0};
                sprintf(stats_query,  new_options.stats_path.c_str(), 0);
                sprintf(final_stats_query, (options.is_new_sd ? TP_SET_STATS_OUTPUT_NEW : TP_SET_STATS_OUTPUT), stats_query);
                std::cout << "STATS QUERY: " << final_stats_query << std::endl;
            }
            perform_query(&new_options, con, in_sql, agg_result, NULL, final_stats_query);
        }
        int extra_sql_idx = 0;

        // Run extra all ;)
        for (auto extra_sql: extra_sqls){

            populate_log_offset(&options, extra_sql);

            #if TRACEPROV_SD_MODE == 0

            int64_t partition_idx = -1;
            void *extra_cntxt = NULL;
            // Figure out the 
            if (options.log_offset != -1){
                // Try partition pruning.
                extra_cntxt = traceprov_get_partition(1, options.top_level_log_layer_number, options.log_offset, &partition_idx);
            }
            duckdb_table_function_set_extra_info(tp_read_func, (void *)partition_idx, nullptr); // whatever
            if (extra_cntxt)
                duckdb_table_function_set_extra_info(tp_read_offset_func, extra_cntxt, free);

            #endif
            extra_sql_idx++;
            Options extra_options = options;
            extra_options.no_reinit_state = true;
            extra_options.capture_lineage = false;
            extra_options.stats_path = "";
            char final_profile_out[256] = {0};
            for (int i = 0; i < extra_options.repeat; i++){
                if (IS_SET(extra_options.profile_out_path)){
                    char profile_out[256] = {0};
                    sprintf(profile_out, options.profile_out_path.c_str(), extra_sql_idx, i);
                    sprintf(final_profile_out, TP_SET_PROFILE_OUTPUT, profile_out);
                }
                perform_query(&extra_options, con, extra_sql, agg_result, final_profile_out, NULL);
            }
            #if TRACEPROV_SD_MODE == 0
            // eh, so that the state is still consistent later.
            duckdb_table_function_set_extra_info(tp_read_offset_func, NULL, nullptr);
            duckdb_table_function_set_extra_info(tp_read_func, NULL, nullptr);
            #endif
        }
    }

    if (IS_SET(options.settings_out_path)){
        char settings_out_query[256] = {0};
        sprintf(settings_out_query, TP_DUMP_SETTINGS, options.settings_out_path.c_str());
        DUCKDB_RUN_SHORT_QUERY(con, settings_out_query, "dumping settings");
    }

    if (IS_SET(options.time_out_path)){
        // if (agg_result.size() != (uint64_t)options.repeat){
        //     std::cout << "Got inconsistent size of computed time!" << std::endl;
        //     exit(1);
        // }
        std::string time_out_json = "[";
        for (uint64_t computed_time_idx = 0; computed_time_idx < agg_result.size(); computed_time_idx++){
            auto current = agg_result.at(computed_time_idx);
            if (computed_time_idx > 0) time_out_json += ",";
            const uint64_t width = (current->data->size());
            uint64_t row_count = 0;
            if (width > 0){
                row_count = (current->data->at(0)->data->size());
            }
            time_out_json += "{";
            time_out_json += "\"time\":" + std::to_string(current->computed_time);
            time_out_json += ",";
            time_out_json += "\"width\":" + std::to_string(width);
            time_out_json += ",";
            time_out_json += "\"row_count\": " + std::to_string(row_count);
            time_out_json += ",";
            time_out_json += "\"option\": " + serialize_option(&current->option);
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

#define LOG_TICKER "/*(traceprov_log_offset): "
#define LOG_TICKER_REST "%d:%ld*/"

#define LOG_TICKER_ALL (LOG_TICKER LOG_TICKER_REST)
// To allow for running the experiments multiple times, on the same offset, we try parsing it from the SQL itself.
static void populate_log_offset(Options *option, std::string extra_sql){
    if (extra_sql.find(LOG_TICKER) == std::string::npos){
        // Maybe throw error here??
        return;
    }
    if (sscanf(extra_sql.c_str(), LOG_TICKER_ALL, &option->top_level_log_layer_number, &option->log_offset) == EOF)
        elog(ERROR, "Expected full conversion!!");
}

// This doesn't do all of option (that'll be too much)
static std::string serialize_option(Options *option){
    std::string serialized = "{";
    serialized += "\"top_level_log_layer_number\": " + std::to_string(option->top_level_log_layer_number) + ",";
    serialized += "\"log_offset\": " + std::to_string(option->log_offset);
    serialized += "}";
    return serialized;
}