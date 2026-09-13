#include "duckdb.hpp"
#define TP_ENABLE_PROFILING "PRAGMA enable_profiling=json"
#define TP_SET_PROFILE_OUTPUT "PRAGMA profile_output='%s'"
#define TP_DISABLE_PROFILING "PRAGMA disable_profiling;"

#include <sstream>
#include <fstream>

#include <iostream>

struct Options {
    // via --db
    std::string db_path;
    // via --profile
    // If empty, no profiling is done.
    std::string profile_out_path;
    // via --i
    std::string input_path;
    uint32_t num_threads;
    uint32_t repeat;
};

struct Options get_base_option(){
    struct Options options {
        .db_path = "",
        .profile_out_path = "",
        .input_path = "",
        .num_threads = 1,
        .repeat = 1
    };
    return options;
}

#define IS_OPTION(X) (strcmp(argv[i], X) == 0)
#define IS_SET(X) (X.size() != 0)

struct Options parse_args(int argc, char **argv){
    auto options = get_base_option();
    for (int i = 1; i < argc; i++){
        if (IS_OPTION("--db")){
            options.db_path = std::string(argv[++i]);
            continue;
        } else if (IS_OPTION("--profile")){
            options.profile_out_path = std::string(argv[++i]);
            continue;
        }  else if (IS_OPTION("--threads")){
            options.num_threads = std::atoi(argv[++i]);
            continue;
        } else if (IS_OPTION("--i")){
            options.input_path = std::string(argv[++i]);
            continue;
        } else if (IS_OPTION("--repeat")){
            options.repeat = std::atoi(argv[++i]);
            continue;
        }
    }
    return options;
}

#define DUCKDB_EXIT_ON_ERROR(state) { \
    if (state == DuckDBError){ \
        std::cout << "Received duckdberror state at " << __FILE__ << ":" << __LINE__ <<  std::endl; \
        std::exit(1); \
    } \
}

#define DUCKDB_EXIT_ON_ERROR_MSG(state, msg) { \
    if (state == DuckDBError){ \
        std::cout << "Received duckdberror state at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cout << "error: " << msg << std::endl; \
        std::cerr << "error: " << msg << std::endl; \
        std::exit(1); \
    } \
}

#define DUCKDB_EXIT_ON_ERROR_RESULT(state, result) { \
    if (state == DuckDBError){ \
        std::cout << "Received duckdberror state at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::cout << duckdb_result_error(&result) << std::endl; \
        std::cerr << duckdb_result_error(&result) << std::endl; \
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



int main(int argc, char **argv){

    struct Options options = parse_args(argc, argv);
    std::ifstream in_sql_stream(options.input_path.c_str());
    std::stringstream buffer;
    buffer << in_sql_stream.rdbuf();
    std::string in_sql(buffer.str());
    std::cout << "From file: " << in_sql << std::endl;

    duckdb_database db;
    duckdb_connection con;
    char *error_msg;
    duckdb_config db_config;

    DUCKDB_EXIT_ON_ERROR(duckdb_create_config(&db_config));

    DUCKDB_EXIT_ON_ERROR_MSG(duckdb_open_ext(options.db_path.c_str(), &db, nullptr, &error_msg), error_msg);
    DUCKDB_EXIT_ON_ERROR(duckdb_connect(db, &con));

    // DUCKDB_RUN_SHORT_QUERY(con, "install JSON;", "install json");
    DUCKDB_RUN_SHORT_QUERY(con, "load JSON;", "load json");

    char thread_set_query[256] = {0};
    sprintf(thread_set_query, "SET threads=%d;", options.num_threads);
    DUCKDB_RUN_SHORT_QUERY(con, thread_set_query, "setting threads");



    for (int i = 0; i < options.repeat; i++){
        char final_profile_out[256] = {0};
        char profile_out[256] = {0};
        std::string layer_stats_out_str = "";
        if (IS_SET(options.profile_out_path)){
            DUCKDB_RUN_SHORT_QUERY(con, (TP_ENABLE_PROFILING), "enable profiling");
            #if TRACEPROV_DEBUG_PERF==1
            DUCKDB_RUN_SHORT_QUERY(con, TP_ENABLE_DETAILED_PROFILING, "enable detailed profiling");
            #endif
        }
        if (IS_SET(options.profile_out_path)){
            sprintf(profile_out, options.profile_out_path.c_str(), i);
            sprintf(final_profile_out, TP_SET_PROFILE_OUTPUT, profile_out);
        }
        DUCKDB_RUN_SHORT_QUERY(con, final_profile_out, "set json out");

        duckdb_prepared_statement stmt = NULL;
        duckdb_result final_result;

        DUCKDB_EXIT_ON_ERROR_MSG(duckdb_prepare(con, in_sql.c_str(), &stmt), duckdb_prepare_error(stmt));
        DUCKDB_EXIT_ON_ERROR_MSG(duckdb_execute_prepared(stmt, &final_result), duckdb_result_error(&final_result));

        uint64_t total_chunk_count = duckdb_result_chunk_count(final_result);
        std::cout << "total count: " << total_chunk_count << std::endl;
        uint32_t chunk_count;
        for (idx_t chunk_idx = 0; chunk_idx < total_chunk_count; chunk_idx++){
            duckdb_data_chunk data_chunk = duckdb_result_get_chunk(final_result, chunk_idx);
            chunk_count++;
            duckdb_destroy_data_chunk(&data_chunk);
        }
        duckdb_destroy_result(&final_result);
        duckdb_destroy_prepare(&stmt);
        DUCKDB_RUN_SHORT_QUERY(con, TP_DISABLE_PROFILING, "disable profiling");
        std::cout << "Finished: " << i << std::endl;
    }

    duckdb_disconnect(&con);
    duckdb_close(&db);   
}