// The source for running duckdb (smokedduck) queries.
// This doesn't exist in Makefiles (because it gets compiled during setup)

#include <iostream>
#include <string>
#include <cstring>
#include "duckdb.hpp"
#include <sstream>
#include <fstream>
#include <chrono>
#include <vector>
#include "traceprov_infer.hpp"

// whatever
#define TP_DUCKDB_INCLUDED
#include "traceprov_duckdb_infer.cpp"

#define TP_ENABLE_PROFILING "PRAGMA enable_profiling=json"
#define TP_SET_PROFILE_OUTPUT "PRAGMA profile_output='%s'"
#define TP_DISABLE_PROFILING "PRAGMA disable_profiling;"

#define TP_ENABLE_LINEAGE "PRAGMA enable_lineage;"
#define TP_DISABLE_LINEAGE "PRAGMA disable_lineage;"
#define TP_CLEAR_LINEAGE "PRAGMA clear_lineage;"

#define TP_SET_STATS_OUTPUT "COPY (select * from duckdb_queries_list() where query = ? order by query_id desc limit 1) TO '%s'"
#define TP_DUMP_SETTINGS "copy (select json_group_object(name, value) as settings from duckdb_settings()) TO '%s';"

#undef sprintf

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

    return options;
};

#define DUCKDB_EXIT_ON_ERROR(state) { \
    if (state == DuckDBError){ \
        std::cout << "Received duckdberror state at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::exit(1); \
    } \
}

#define DUCKDB_EXIT_ON_ERROR_MSG(state, msg) { \
    if (state == DuckDBError){ \
        std::cout << "Received duckdberror state at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cout << "error: " << msg << std::endl; \
        std::exit(1); \
    } \
}

#define DUCKDB_EXIT_ON_ERROR_RESULT(state, result) { \
    if (state == DuckDBError){ \
        std::cout << "Received duckdberror state at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cout << duckdb_result_error(&result) << std::endl; \
        std::exit(1); \
    } \
}

#define DUCKDB_RUN_SHORT_QUERY(con, query, msg) { \
    duckdb_result result; \
    std::cout << "QUERY: " << query << std::endl; \
    duckdb_state state = duckdb_query(con, query, &result); \
    DUCKDB_EXIT_ON_ERROR_RESULT(state, result); \
    duckdb_destroy_result(&result); \
    std::cout << "Reached " << msg << " correctly" << std::endl; \
} \

// Simply populates the data, given a chunk.
static void populate_traceprov_data(
    TraceProvData *traceprov_data, 
    duckdb_data_chunk *chunk
){
    const uint64 column_count = duckdb_data_chunk_get_column_count(*chunk);
    const uint64 row_count = duckdb_data_chunk_get_size(*chunk);
    // Unlikely because it'll happen just once, for the first chunk.
    if (unlikely(column_count != traceprov_data->size())){
        if (traceprov_data->size() != 0){
            std::cout << "Attempting to set different number of column entries" << std::endl;
            exit(1);
        }
        for (uint64 col_idx = 0; col_idx < column_count; col_idx++){
            auto column_data = new TraceProvColumnData;
            column_data->data = new std::vector<uint64>();
            traceprov_data->push_back(column_data);
        }
    }
    for (uint64 col_idx = 0; col_idx < column_count; col_idx++){
        duckdb_vector col = duckdb_data_chunk_get_vector(*chunk, col_idx);
        uint64 *col_data = (uint64 *)duckdb_vector_get_data(col);
        uint64_t *col_validity = (uint64_t *)duckdb_vector_get_validity(col);
        auto current_column_data = traceprov_data->at(col_idx)->data;
        for (uint64 row_idx = 0; row_idx < row_count; row_idx++){
            if(unlikely(!duckdb_validity_row_is_valid(col_validity, row_idx))){
                std::cout << "Expected all non-null row, for now" << std::endl;
                exit(1);
            }
            current_column_data->push_back(col_data[row_idx]);
        }
    }
}

typedef struct PerformQueryResult {
    int64_t computed_time;
    TraceProvData *data;
} PerformQueryResult;

PerformQueryResult *make_result(int64_t computed_time, TraceProvData *data){
    auto result = new PerformQueryResult;
    result->computed_time = computed_time;
    result->data = data;
    return result;
}

void perform_query(
    struct Options &options, 
    duckdb_connection &con, 
    std::string &in_sql, 
    int iter,
    std::vector<PerformQueryResult *> &agg_result
){

    if (IS_SET(options.profile_out_path)){
        DUCKDB_RUN_SHORT_QUERY(con, TP_ENABLE_PROFILING, "enable profiling");
        char profile_out[256] = {0};
        sprintf(profile_out, options.profile_out_path.c_str(), iter);
        char final_profile_out[256] = {0};
        sprintf(final_profile_out, TP_SET_PROFILE_OUTPUT, profile_out);
        DUCKDB_RUN_SHORT_QUERY(con, final_profile_out, "set json out");
    }

    if (options.capture_lineage){
        DUCKDB_RUN_SHORT_QUERY(con, TP_ENABLE_LINEAGE, "enable lineage");
        DUCKDB_RUN_SHORT_QUERY(con, TP_CLEAR_LINEAGE, "clear lineage");
    }
    // Need to use both, the pending and the streaming API.
    duckdb_prepared_statement stmt;
    duckdb_result final_result;

    auto start_time = std::chrono::high_resolution_clock::now();

    DUCKDB_EXIT_ON_ERROR_MSG(duckdb_prepare(con, in_sql.c_str(), &stmt), duckdb_prepare_error(stmt));
    if (options.use_pending){
        duckdb_pending_result result;
        DUCKDB_EXIT_ON_ERROR(duckdb_pending_prepared_streaming(stmt, &result));
        DUCKDB_EXIT_ON_ERROR(duckdb_execute_pending(
            result,
            &final_result
        ));
    }else{
        DUCKDB_EXIT_ON_ERROR(duckdb_execute_prepared(stmt, &final_result));
    }

    std::cout << "Is streaming: " << duckdb_result_is_streaming(final_result) << std::endl;

    uint64 chunk_count = 0;
    auto traceprov_data = new TraceProvData;

    if (options.use_pending){
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

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    if (duration.count() == 0){
        std::cout << "Got 0 as the measured time, use a finer granularity..." << std::endl;
        exit(1);
    }

    agg_result.push_back(make_result(duration.count(), traceprov_data));

    // This needs to run before anything else bc of overwrites.
    DUCKDB_RUN_SHORT_QUERY(con, TP_DISABLE_PROFILING, "disable profiling");
    
    if (options.capture_lineage){
        DUCKDB_RUN_SHORT_QUERY(con, TP_DISABLE_LINEAGE, "disable lineage");
    }

    std::cout << "Chunks: " << chunk_count;

    if (IS_SET(options.stats_path)){
        char stats_query[256] = { 0 };
        char final_stats_query[256] = { 0 };
        sprintf(stats_query,  options.stats_path.c_str(), iter);
        sprintf(final_stats_query, TP_SET_STATS_OUTPUT, stats_query);
        std::cout << "STATS QUERY: " << final_stats_query << std::endl;
        if((duckdb_prepare(con, final_stats_query, &stmt)) == DuckDBError){
            std::cout << duckdb_prepare_error(stmt) << std::endl;
        }
        DUCKDB_EXIT_ON_ERROR(duckdb_bind_varchar(stmt, 1, in_sql.c_str()));
        DUCKDB_EXIT_ON_ERROR(duckdb_execute_prepared(stmt, &final_result));
        duckdb_destroy_result(&final_result);
        duckdb_destroy_prepare(&stmt);
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

    duckdb_database db;
    duckdb_connection con;
    char *error_msg;
    DUCKDB_EXIT_ON_ERROR_MSG(duckdb_open_ext(options.db_path.c_str(), &db, nullptr, &error_msg), error_msg);
    DUCKDB_EXIT_ON_ERROR(duckdb_connect(db, &con));

    auto function = setup_func();
    DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, function));
    std::cout << "ran register successfully!" << std::endl;
    DUCKDB_RUN_SHORT_QUERY(con, "ANALYZE;", "run analyze;");

    if (options.dry_run)
        return 0;

    char thread_set_query[256] = {0};
    sprintf(thread_set_query, "SET threads=%d;", options.num_threads);
    DUCKDB_RUN_SHORT_QUERY(con, thread_set_query, "setting threads");

    if (IS_SET(options.index_scan_percentage)){
        std::string indx_set_query = "SET index_scan_percentage=" + options.index_scan_percentage + ";";
        DUCKDB_RUN_SHORT_QUERY(con, indx_set_query.c_str(), "setting index scan percent");
    }

    for (int i = 0; i < options.repeat; i++){
        perform_query(options, con, in_sql, i, agg_result);
        // delete agg_result.back()->data;
    }

    if (IS_SET(options.settings_out_path)){
        char settings_out_query[256] = {0};
        sprintf(settings_out_query, TP_DUMP_SETTINGS, options.settings_out_path.c_str());
        DUCKDB_RUN_SHORT_QUERY(con, settings_out_query, "dumping settings");
    }

    if (IS_SET(options.time_out_path)){
        if (agg_result.size() != (uint64)options.repeat){
            std::cout << "Got inconsistent size of computed time!" << std::endl;
            exit(1);
        }
        std::string time_out_json = "[";
        for (uint64_t computed_time_idx = 0; computed_time_idx < agg_result.size(); computed_time_idx++){
            auto current = agg_result.at(computed_time_idx);
            if (computed_time_idx > 0) time_out_json += ",";
            time_out_json += "{";
            time_out_json += "\"time\":" + std::to_string(current->computed_time);
            time_out_json += ",";
            time_out_json += "\"width\":" + std::to_string(current->data->size());
            time_out_json += ",";
            time_out_json += "\"row_count\": " + std::to_string(current->data->at(0)->data->size());
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