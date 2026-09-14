#include "duckdb.hpp"
#define TP_ENABLE_PROFILING "PRAGMA enable_profiling=json"
#define TP_SET_PROFILE_OUTPUT "PRAGMA profile_output='%s'"
#define TP_DISABLE_PROFILING "PRAGMA disable_profiling;"

#include <sstream>
#include <fstream>

#include <iostream>
#include "traceprov.hpp"
#include "utils.hpp"
#include <sys/mman.h>

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
    bool emulate_mmap;
};

struct Options get_base_option(){
    struct Options options {
        .db_path = "",
        .profile_out_path = "",
        .input_path = "",
        .num_threads = 1,
        .repeat = 1,
        .emulate_mmap = false
    };
    return options;
}

#define IS_OPTION(X) (strcmp(argv[i], X) == 0)
#define IS_SET(X) (X.size() != 0)


void portable_elog(int level){
    if (level == INFO) return;
    if (level == ERROR){
        //  Recursive call is safe
        elog(INFO, "Error no: %d", errno);
        elog(INFO, "Error: %s", strerror(errno));
        exit(1);
    }
}


static bool should_write = false;

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
        } else if (IS_OPTION("--should_write")){
            should_write = true;
            continue;
        } else if (IS_OPTION("--emulate_mmap")){
            options.emulate_mmap = true;
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


static inline void fill_space(void *ptr, const uint64_t size){
    if (!should_write) return;
    memset(ptr, size, 1);
}

int map_traceprov_shared_context(struct traceprov_shared_context *ptr){
    const size_t size_shared_context_filename = sizeof(TRACEPROV_SHARED_CONTEXT) + strlen(DataDir) + 1;
    int rc = 0;
    struct traceprov_shared_context *temp_ptr;
    char *shared_context_filename = (char*)malloc(size_shared_context_filename);
    if (shared_context_filename == NULL){
        elog(ERROR, "Couldn't allocate memory to hold shared context file");
        return 1;
    }
    memset(shared_context_filename, 0, size_shared_context_filename);
    sprintf(shared_context_filename, TRACEPROV_SHARED_CONTEXT, DataDir);

    int shared_context_fd = open(shared_context_filename, O_RDONLY);
    if (shared_context_fd < 0){
        PRINT_ON_DEBUG("Error opening the scratch file");
        goto exit_map;
    }

    temp_ptr = (struct traceprov_shared_context *)mmap(
        NULL,
        TRACEPROV_SHARED_CONTEXT_SIZE,
        PROT_READ,
        MAP_SHARED,
        shared_context_fd,
        0
    );

    if (temp_ptr == MAP_FAILED){
        PRINT_ON_DEBUG("Error mapping the scratch file");
        goto exit_map;
    }

    PRINT_ON_DEBUG("Map shared context succesful!");

    memcpy(ptr, temp_ptr, sizeof(struct traceprov_shared_context));

exit_map:
    if (shared_context_fd > 0) close(shared_context_fd);
    if (shared_context_filename) free(shared_context_filename);
    return rc;
}

std::vector<struct local_context *> *traceprov_get_local_contexts(const uint32_t worker_count){
    auto worker_local_contexts = new std::vector<struct local_context *>;
    for (uint8_t worker_id = 0; worker_id < worker_count; worker_id++){
        char buff[256] = {0};
        sprintf(buff, TRACEPROV_WORKER_LAYER_MAP, DataDir, worker_id + 1);
        int fd = open(buff, O_RDONLY);
        if (fd < 0) elog(ERROR, "Error opening the worker laye rmap!");
        void *ptr = mmap(
            NULL,
            sizeof(struct local_context),
            PROT_READ,
            MAP_SHARED,
            fd,
            0
        );
        if (ptr == MAP_FAILED){
            elog(ERROR, "Error mmaping the layer file!");
        }
        struct local_context *worker_local_context = (struct local_context *)ptr;
        worker_local_contexts->push_back(worker_local_context);
        close(fd);
    }
    return worker_local_contexts;
}

// Figure out the gap between last ptr and head and fill that gap.
static inline void fill_gap(void *ptr, const traceprov_aggregate_layer *layer){
    if (!should_write) return;
    const int64_t gap = (uint64_t)layer->current_row - (uint64_t)layer->last_mapping;
    if (gap < 0) elog(ERROR, "Expected gap to always be > 0");
    memset(ptr, gap, 1);
}
// Just emulates the writes that have been done in the mapping.
void emulate_file_writes(){

    traceprov_shared_context shared_context;
    if (map_traceprov_shared_context(&shared_context))
        elog(ERROR, "error maping shared context!");

    auto worker_local_contexts = traceprov_get_local_contexts(shared_context.worker_count);
    uint64_t mock_sum = 0;
    std::vector<void *> page_list;
    for (auto entry: *worker_local_contexts){
        for (idx_t layer_idx = 0; layer_idx < TRACEPROV_MAX_LAYER_PER_WORKER; layer_idx++){
            const traceprov_aggregate_layer *layer = &entry->cached_layers[layer_idx];
            if (layer->layer_number == 0) continue;
            // Need to always set the initial ptr.
            void *initial_ptr = mmap(
                NULL,
                TRACEPROV_PAGE_SIZE,
                PROT_WRITE,
                TRACEPROV_MMAP_FLAGS,
                0,
                0
            );
            page_list.push_back(initial_ptr);
            void *last_ptr = NULL;
            if (layer->size > 1) { fill_space(initial_ptr, TRACEPROV_PAGE_SIZE); }
            else{
                last_ptr = initial_ptr;
            }
            // Need to figure out the last page, and fill out all the intermediate pages.
            if (layer->size > 1){
                const uint64_t alloc_count = (layer->size - 1) / TRACEPROV_INCREMENT_TRACE_BY_PG;
                for (uint64_t alloc_idx = 0; alloc_idx < alloc_count; alloc_idx){
                    void *incr_ptr = mmap(
                        NULL,
                        TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG,
                        PROT_WRITE,
                        TRACEPROV_MMAP_FLAGS,
                        0,
                        0
                    );
                    page_list.push_back(incr_ptr);
                    // Fillup any intermediate region.
                    if (alloc_idx < alloc_count - 1){
                        fill_space(incr_ptr, TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG);
                    }
                    last_ptr = incr_ptr;
                }
            }
            // Now need to fill up any gap.
            fill_gap(last_ptr, layer);
        }
    }
    for (auto ptr_head : page_list){
        mock_sum += (uint64_t)ptr_head;
    }
}

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

    if (options.emulate_mmap){
        emulate_file_writes();
    }

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