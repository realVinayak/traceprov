// implements table scan functions, for duckdb-based traceprov files.

#include <stdlib.h>
#include <vector>

#include <cstring>
#include <sys/mman.h>

#include <string>
#include <unordered_map>
#include "traceprov_duckdb_infer.hpp"

#include "traceprov.hpp"
#include "duckdb.hpp"
#include "utils.hpp"
#include "file_utils.hpp"

#include <cmath>
#include "traceprov_partition_info.hpp"
#include <traceprov_node.hpp>
#include "traceprov_settings.hpp"
#include "traceprov_derive.hpp"
#include "derivation_utils.hpp"

TraceProvDuckDbGlobalState g_tp_duckdb_state {
    .did_initialize = false,
    .worker_local_contexts = NULL
};


typedef struct TraceProvBindData {
    TraceProvRelationArgs rel_args;
    void *col_layer_ptr;
    void *row_count_layer_ptr;
    uint64_t num_rows;
    uint64_t column_width;
    const struct traceprov_aggregate_layer *col_layer;
    const struct traceprov_aggregate_layer *row_layer;
    bool is_dummy;
    // how many chunks can fit in the first page.
    uint64_t first_page_element_count;
    // how many chunks can fit in each added page.
    uint64_t incr_page_element_count;
    // same as above, but for row layer.
    uint64_t row_first_page_element_count;
    uint64_t row_incr_page_element_count;
    bool is_strict_rows;
    uint64_t start_offset;
    int64_t offset_in_chunk;
    std::vector<TraceProvBindData *> *worker_bind_data;
    bool is_memory_mapping;
    std::mutex *bind_data_mutex;
    uint64_t max_worker_idx;
    uint32_t pointer_column_idx;
    std::vector<uint8_t> *sizes;
    // Needs mask?
    bool first_mask;
    bool is_aggregate;
} TraceProvBindData;

typedef struct TraceProvInitData {
    bool is_single;
    uint64_t current;
    uint64_t offset_in_chunk;
    void *col_layer_ptr;
    void *row_count_layer_ptr;
    void *col_layer_ptr_end;
    void *row_count_layer_ptr_end;
    uint64_t col_page_idx;
    uint64_t row_page_idx;
    bool is_dummy;
    // In cases where we are doing an implicit union, need to go over the worker bind data
    // 1-by-1. This tracks where which worker we are currently at.
    uint64_t worker_bind_idx;
    // Useful to do it this way.
    std::vector<TraceProvInitData *> *worker_init_data;
    uint64_t idx_in_bind;
    TraceProvInitData *null_map_init_data;
    TraceProvBindData *null_map_bind_data;
    // This gets set in an eager way.
    // Done this way so that we don't have to go back an entire page just to check what chunk idx we need to stop at.
    uint64_t next_null_idx;
    uint64_t next_null_col_count;
} TraceProvInitData;

static TraceProvBindData *setup_layers(
    const uint64_t worker_id,
    uint64_t layer_number,
    const int64_t log_offset,
    int64_t *record_count,
    const bool expect_present=true,
    const uint64_t partition_idx=0
);

// We don't need a file version of this, since that cannot happen.
// That is, since we always increment by 8 bytes, we'll never jump pages.
static inline void traceprov_grow_row_count_page_mapping(TraceProvInitData *init, TraceProvBindData *bind){
    if (init->row_count_layer_ptr == init->row_count_layer_ptr_end){
        init->row_count_layer_ptr = bind->row_layer->page_mapping[++init->row_page_idx];
        init->row_count_layer_ptr_end = &((uint8_t *)init->row_count_layer_ptr)[TRACEPROV_INCREMENT_TRACE_BY_PG*TRACEPROV_PAGE_SIZE];
    }
}

static inline void traceprov_grow_col_page_mapping(const uint64_t extra_size, TraceProvInitData *init, TraceProvBindData *bind){
    if (((uint64_t)init->col_layer_ptr + extra_size) > (uint64_t)init->col_layer_ptr_end){
        init->col_layer_ptr = bind->col_layer->page_mapping[++init->col_page_idx];
        init->col_layer_ptr_end = &((uint8_t *)init->col_layer_ptr)[TRACEPROV_INCREMENT_TRACE_BY_PG*TRACEPROV_PAGE_SIZE];
    }
}

static inline void traceprov_grow_col_page_mapping_file(const uint64_t extra_size, TraceProvInitData *init, TraceProvBindData *bind){
    if (((uint64_t)init->col_layer_ptr + extra_size) > (uint64_t)init->col_layer_ptr_end){
        // We don't need to consult any page mapping in that case.
        init->col_layer_ptr = init->col_layer_ptr_end;
        init->col_layer_ptr_end = &((uint8_t *)init->col_layer_ptr_end)[TRACEPROV_INCREMENT_TRACE_BY_PG*TRACEPROV_PAGE_SIZE];
    }
}


// a Nop.
static inline void traceprov_grow_row_count_page_mapping_file(TraceProvInitData *init, TraceProvBindData *bind){

}

TraceProvInitData *allocate_init_data(){
    auto init_data_inst = (TraceProvInitData *)malloc(sizeof(TraceProvInitData));
    memset(init_data_inst, 0, sizeof(TraceProvInitData));
    return init_data_inst;
}

TraceProvInitData *traceprov_make_init_data(TraceProvBindData *bind_data){
    auto init_data_inst = allocate_init_data();
    init_data_inst->current = bind_data->start_offset;
    init_data_inst->is_single = (bind_data->rel_args.offset != -1);
    init_data_inst->offset_in_chunk = 0;
    init_data_inst->col_layer_ptr = bind_data->col_layer_ptr;
    init_data_inst->col_layer_ptr_end = INCR_BY_BYTES(init_data_inst->col_layer_ptr, TRACEPROV_PAGE_SIZE);
    init_data_inst->row_count_layer_ptr = bind_data->row_count_layer_ptr;
    init_data_inst->row_count_layer_ptr_end = INCR_BY_BYTES(bind_data->row_count_layer_ptr, TRACEPROV_PAGE_SIZE);
    init_data_inst->is_dummy = bind_data->is_dummy;
    if (bind_data->col_layer->null_layer_number){
        if (bind_data->rel_args.worker_id == 0){
            elog(ERROR, "Expected the worker id to be always set here!");
        }
        auto null_bind_data_inst = setup_layers(
            bind_data->rel_args.worker_id,
            bind_data->col_layer->null_layer_number,
            -1,
            NULL
        );
        // Need to also make yet another init data.
        // The recursive call automatically sets up most things correctly.
        // Also, it is guaranteed to end too.
        auto null_init_data_inst = traceprov_make_init_data(
            null_bind_data_inst
        );
        const auto grow_func = null_bind_data_inst->is_memory_mapping ? traceprov_grow_col_page_mapping : traceprov_grow_col_page_mapping_file;
        grow_func(
            2*sizeof(uint64_t),
            null_init_data_inst,
            null_bind_data_inst
        );
        init_data_inst->next_null_idx = ((uint64_t *)null_init_data_inst->col_layer_ptr)[0];
        init_data_inst->next_null_col_count = ((uint64_t *)null_init_data_inst->col_layer_ptr)[1];
        null_init_data_inst->col_layer_ptr = INCR_BY_BYTES(null_init_data_inst->col_layer_ptr, sizeof(uint64_t)*2);
        init_data_inst->null_map_bind_data = null_bind_data_inst;
        init_data_inst->null_map_init_data = null_init_data_inst;

    }
    return init_data_inst;
}

// Reads from an offset.
// Done this way because it allows better reusability with the window-read logic.
// Some of the code from the simple read gets copied, but, eh, I'd rather keep that separate and tidy.
static void read_at_offset(
    const uint64_t log_offset,
    TraceProvBindData *bind_data,
    // Values are written here.
    uint64_t *values,
    std::vector<uint8_t> *sizes
){
    auto init_data = traceprov_make_init_data(bind_data);
    const bool can_jump_row_page = bind_data->row_layer->page_mapping != NULL;
    uint64_t accum_size = 0;
    auto col_grow_func = can_jump_row_page ? traceprov_grow_col_page_mapping : traceprov_grow_col_page_mapping_file;
    while (true){
        if (can_jump_row_page){
            traceprov_grow_row_count_page_mapping(init_data, bind_data);
        }
        const uint64_t num_rows = *((uint64_t*)init_data->row_count_layer_ptr);
        init_data->row_count_layer_ptr = INCR_BY_BYTES(init_data->row_count_layer_ptr, sizeof(uint64_t));
        const bool is_in_current = (log_offset >= accum_size) && (log_offset < (num_rows + accum_size));
        const uint64_t local_offset = log_offset - accum_size;
        for (idx_t col_idx = 0; col_idx < bind_data->column_width; col_idx++){
            const uint8_t curr_size = sizes->at(col_idx);
            const uint64_t extra_size = num_rows * curr_size;
            col_grow_func(extra_size, init_data, bind_data);
            if (is_in_current) {
                if (curr_size == sizeof(uint32_t)){
                    values[col_idx] = ((uint32_t*)init_data->col_layer_ptr)[local_offset];
                }else{
                    values[col_idx] = ((uint64_t*)init_data->col_layer_ptr)[local_offset];
                }
            }
            init_data->col_layer_ptr = INCR_BY_BYTES(init_data->col_layer_ptr, extra_size);
        }
        if (is_in_current){
            free(init_data);
            return;
        }
        accum_size += num_rows;
    }
}

std::mutex g_tp_state_mutex;

void initialize_global_context(){
    g_tp_state_mutex.lock();
    if (!g_tp_duckdb_state.did_initialize){
        traceprov_shared_context shared_context;
        if (map_traceprov_shared_context(&shared_context))
            elog(ERROR, "error maping shared context!");

        g_tp_duckdb_state.did_initialize = true;
        g_tp_duckdb_state.worker_local_contexts = traceprov_get_local_contexts(shared_context.worker_count);
    }
    g_tp_state_mutex.unlock();
}

void reset_global_context(){
    g_tp_duckdb_state.did_initialize = false;
    g_tp_duckdb_state.worker_local_contexts = nullptr;
}

static TraceProvBindData *allocate_bind_data(){
    auto my_bind_data = (TraceProvBindData *)malloc(sizeof(TraceProvBindData));
    memset(my_bind_data, 0, sizeof(TraceProvBindData));
    my_bind_data->bind_data_mutex = new std::mutex;
    return my_bind_data;
}

void bp(){

}

TraceProvBindData *setup_layers(
    const uint64_t worker_id,
    uint64_t layer_number,
    const int64_t log_offset,
    int64_t *record_count,
    const bool expect_present,
    const uint64_t partition_idx
){
    // -1 so that it can be ignored, if not set.
    int64_t found_record_count = -1;

    initialize_global_context();
    auto my_bind_data = allocate_bind_data();
    my_bind_data->rel_args.offset = log_offset;
    my_bind_data->offset_in_chunk = -1;

    my_bind_data->rel_args.worker_id = worker_id;
    uint64_t current_worker_id = worker_id;


    bool is_dummy = false;
    if (current_worker_id > g_tp_duckdb_state.worker_local_contexts->size()){
        current_worker_id = 1;
        is_dummy = true;
        // elog(INFO, "Got case where the worker id is greater than recognized cases. Not handling this case anymore.");
        return NULL;
    }

    auto current_local_context =  g_tp_duckdb_state.worker_local_contexts->at(current_worker_id - 1);
    auto current_layer = &current_local_context->cached_layers[layer_number - 1];
    if (partition_idx > 0){ // because == 0 is always current.
        layer_number = current_layer->buckets[partition_idx - 1];
        if (layer_number == 0)
            elog(ERROR, "Got invalid comptued!");
        current_layer = &current_local_context->cached_layers[layer_number - 1];
    }

    my_bind_data->rel_args.layer_number = layer_number;

    if (current_layer->layer_number != layer_number){
        if (current_layer->layer_number != 0){
            // This assertion should hold regardless of what caller expects.
            elog(ERROR, "Expected layer to be 0!");
        }
        if (expect_present){
            elog(ERROR, "Expected the layer number to be filled");
        }else{
            return NULL;
        }
    }


    uint64_t column_count = current_layer->num_pk_records;

    my_bind_data->is_strict_rows = false;

    if (current_layer->page_mapping == NULL){
        map_layer_file(current_layer->layer_number, current_worker_id, &my_bind_data->col_layer_ptr, current_layer->size);
    }else {
        my_bind_data->col_layer_ptr = current_layer->page_mapping[0];
        if (current_layer->rows_layer_number == 0){
            // In this case, have page mapping, but not rows.
            // This will be the case for combine.
            const uint64_t logged_chunk_size = (TRACEPROV_GET_RECORD_SIZE(current_layer));
            my_bind_data->first_page_element_count = (TRACEPROV_PAGE_SIZE / logged_chunk_size);
            my_bind_data->incr_page_element_count = ((TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG) / logged_chunk_size);
            my_bind_data->is_strict_rows = true;
            if (TRACEPROV_PAGE_SIZE % logged_chunk_size)
                elog(ERROR, "expected complete chunks!");
            if (((TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG) % logged_chunk_size))
                elog(ERROR, "expected complete chunks!");
        }
    }

    my_bind_data->col_layer = current_layer;

    uint64_t chunk_count = 0;
    // In duckdb case, the rows layer stores just the chunk size.
    // Otherwise, the rows layer is NULL (which is then inferred as row-based storage)
    if (current_layer->rows_layer_number){
        auto rows_layer = &current_local_context->cached_layers[current_layer->rows_layer_number - 1];
        if (rows_layer->layer_number != current_layer->rows_layer_number)
            elog(ERROR, "Expected the layer number to be filled");
        my_bind_data->row_layer = rows_layer;
        if (rows_layer->page_mapping == NULL){
            map_layer_file(rows_layer->layer_number, current_worker_id, &my_bind_data->row_count_layer_ptr, rows_layer->size);
        }else{
            my_bind_data->row_count_layer_ptr = rows_layer->page_mapping[0];
        }
        chunk_count = rows_layer->num_rows;
        found_record_count = rows_layer->record_count;
    }else{
        const void *final_ptr = get_final_ptr(my_bind_data->col_layer_ptr, current_layer);
        chunk_count = ((uint64_t)final_ptr - (uint64_t)my_bind_data->col_layer_ptr) / TRACEPROV_GET_RECORD_SIZE(current_layer);
        my_bind_data->is_strict_rows = true;
    }

    my_bind_data->column_width = column_count;
    my_bind_data->num_rows = chunk_count;
    my_bind_data->is_dummy = is_dummy;
    if (record_count){
        *record_count = found_record_count;
    }

    return my_bind_data;
}

void extract_partition_info(
    TraceProvTableExtra *extra_info, 
    bool &will_be_dummy, 
    uint64_t &partition_idx, 
    uint64_t table_flags, 
    uint64_t worker_idx, 
    uint32_t layer_number,
    int64_t &log_offset
){
    will_be_dummy = false;
    log_offset = -1;
    if (extra_info->partition_spec != NULL & ((table_flags & TRACEPROV_TABLE_COMBINE) == 0)){
        auto partition_data = extra_info->partition_spec->partition_data;
        if (partition_data != NULL){
            if (partition_data->find(layer_number) != partition_data->end()){
                auto layer_data = partition_data->at(layer_number);
                if (layer_data->find(worker_idx) == layer_data->end()){
                    will_be_dummy = true;
                } else{
                    partition_idx = partition_data->at(layer_number)->at(worker_idx);
                }
            }
        }
        auto layer_log_data = extra_info->partition_spec->layer_log_map;
        if (layer_log_data != NULL){
            if (layer_log_data->find(layer_number) != layer_log_data->end()){
                auto log_entry = layer_log_data->at(layer_number);
                auto log_worker_id = TRACEPROV_GET_WORKER_ID(log_entry);
                if (log_worker_id == 0){
                    elog(ERROR, "Got 0 as the worker id!");
                }
                log_entry = TRACEPROV_STRIP_WORKER_ID(log_entry);
                if (log_worker_id == worker_idx){
                    log_offset = log_entry;
                }
            }
        }
    }
}

void traceprov_duckdb_bind(duckdb_bind_info info){
    initialize_global_context();

    auto param_1 = duckdb_bind_get_parameter(info, 0);
    const uint64_t table_flags = duckdb_get_int64(param_1);
    duckdb_destroy_value(&param_1);

    auto param_2 = duckdb_bind_get_parameter(info, 1);
    const uint64_t current_worker_id = duckdb_get_int64(param_2);
    duckdb_destroy_value(&param_2);

    auto param_3 = duckdb_bind_get_parameter(info, 2);
    const uint64_t layer_number = duckdb_get_int64(param_3);
    duckdb_destroy_value(&param_3);

    int64_t log_offset = -1;
    if (duckdb_bind_get_parameter_count(info) >= 4){
        auto param_4 = duckdb_bind_get_parameter(info, 3);
        log_offset = duckdb_get_int64(param_4);
        duckdb_destroy_value(&param_4);
    }

    uint64_t partition_idx = 0;
    if (duckdb_bind_get_parameter_count(info) == 5){
        auto param_4 = duckdb_bind_get_parameter(info, 4);
        partition_idx = duckdb_get_int64(param_4);
        duckdb_destroy_value(&param_4);
    }

    // This, for now, assumes that the bind infrastructure in DuckDB is correct.
    // That is, if the arguments are different, then this bind gets called multiple times.
    TraceProvTableExtra *extra_info = (TraceProvTableExtra *)duckdb_bind_get_extra_info(info);

    TraceProvBindData *bind_data;
    int64_t total_record_count = -1;
    if (current_worker_id == 0){
        bind_data = allocate_bind_data();
        bind_data->rel_args.offset = log_offset;
        bind_data->worker_bind_data = new std::vector<TraceProvBindData *>;
        const uint64_t worker_count = g_tp_duckdb_state.worker_local_contexts->size();
        std::unordered_map<uint64_t, uint64_t> worker_layer_map;
        if (table_flags & TRACEPROV_TABLE_COMBINE){
            // Need to dynamically determine which tables to select.
            auto pairs = find_combine_layers_across_workers(layer_number, g_tp_duckdb_state.worker_local_contexts);
            for (auto worker_layer_pair : *pairs){
                worker_layer_map.insert({(uint64_t)worker_layer_pair.first, worker_layer_pair.second->layer_number});
            }
        }
        for (uint64_t worker_idx = 0; worker_idx < worker_count; worker_idx++){
            // Cannot expect to find the layer in this case (because, previously, it would have been done at the query generation phase)
            uint64_t layer_number_to_search = layer_number;
            if (worker_layer_map.find(worker_idx + 1) != worker_layer_map.end()){
                layer_number_to_search = worker_layer_map.at(worker_idx + 1);
            }else if (worker_layer_map.size()){
                layer_number_to_search = 0;
            }
            // This assumes that the code before correctly filters those out. Seems 
            if (layer_number_to_search == 0) continue;
            int64_t child_record_count = -1;
            uint64_t local_partition = partition_idx;
            // Don't do this for combine, even though they have the same layer number
            bool will_be_dummy = false;
            int64_t local_offset = log_offset;
            extract_partition_info(extra_info, will_be_dummy, local_partition, table_flags, worker_idx + 1, layer_number, local_offset);
            TraceProvBindData *child_bind_data = setup_layers(worker_idx + 1, layer_number_to_search, log_offset, &child_record_count, false, local_partition);
            if (child_record_count != -1){
                if (total_record_count == -1) total_record_count = 0;
                total_record_count += child_record_count;
            }
            if (child_bind_data == NULL) continue;
            bind_data->worker_bind_data->push_back(child_bind_data);
            bind_data->is_strict_rows |= child_bind_data->is_strict_rows;
            bind_data->is_memory_mapping |= (child_bind_data->col_layer->page_mapping != NULL);
            if (bind_data->column_width != 0 && (bind_data->column_width != child_bind_data->column_width)){
                bp();
                elog(ERROR, "Got inconsitent size!");
            }
            bind_data->column_width = child_bind_data->column_width;
            child_bind_data->is_dummy |= will_be_dummy;
            child_bind_data->rel_args.offset = local_offset;
        }
        bind_data->rel_args.offset = log_offset;
    }else{
        int64_t child_record_count = -1;
        bool will_be_dummy = false;
        uint64_t local_partition = partition_idx;
        int64_t local_offset = log_offset;
        extract_partition_info(extra_info, will_be_dummy, local_partition, table_flags, current_worker_id, layer_number, local_offset);
        bind_data = setup_layers(current_worker_id, layer_number, log_offset, &child_record_count, false, local_partition);
        if (child_record_count != -1){
            if (total_record_count == -1) total_record_count = 0;
            total_record_count += child_record_count;
        }
        if (bind_data != NULL){
            bind_data->worker_bind_data = new std::vector<TraceProvBindData *>;
            bind_data->is_memory_mapping = bind_data->col_layer->page_mapping != NULL;
            bind_data->worker_bind_data->push_back(bind_data);
        }else{
            bind_data = allocate_bind_data();
            bind_data->is_dummy = true;
            // Need to go over all the logs in hopes that we find some,
            for (auto worker_layer_par : *g_tp_duckdb_state.worker_local_contexts){
                const auto candidate_layer = &worker_layer_par->cached_layers[layer_number - 1];
                if(candidate_layer->layer_number == layer_number){
                    bind_data->column_width = candidate_layer->num_pk_records;
                    break;
                }
            }
            if (bind_data->column_width == 0){
                elog(ERROR, "Expected column width to be set!");
            }
        }
        bind_data->is_dummy |= will_be_dummy;
        bind_data->rel_args.offset = local_offset;
    }
    std::vector<uint8_t> *sizes = NULL;
    if (extra_info == NULL || (extra_info->pointer_spec == NULL)){
        sizes = new std::vector<uint8_t>(bind_data->column_width, sizeof(uint64_t));
    }else{
        const TraceProvPointerContext *pc = extra_info->pointer_spec;
        sizes = pc->size_map->at(layer_number);
        const bool is_agg = std::find(pc->aggregate_layers->begin(), pc->aggregate_layers->end(), (uint32_t)layer_number) != pc->aggregate_layers->end();
        if (table_flags & TRACEPROV_TABLE_COMBINE){
            sizes = new std::vector<uint8_t>(bind_data->column_width, sizeof(uint64_t));
        }else{
            // Don't set is agg when dealing with combines (since they are "faked") as an combine.
            bind_data->is_aggregate = is_agg;
            if (traceprov_use_compact && is_agg){
                bind_data->first_mask = true;
            }
            if (sizes->size() != bind_data->column_width){
                elog(ERROR, "Expected the size vector to be of the same length as the column width!");
            }
        }
    }
    bind_data->sizes = sizes;
    for (uint64_t col_count = 0; col_count < bind_data->column_width; col_count++){
        const std::string param = std::string("column_") + std::to_string(col_count);
        const uint8_t column_size = bind_data->sizes->at(col_count);
        // elog(INFO, "[Infer] Layer: %d, Col: %ld, Size: %d", layer_number, col_count, column_size);
        // elog(INFO,)
        duckdb_logical_type type = column_size == sizeof(uint64_t) ? duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT) : duckdb_create_logical_type(DUCKDB_TYPE_UINTEGER);
        duckdb_bind_add_result_column(info, param.c_str(), type);
        duckdb_destroy_logical_type(&type);
    }
    bind_data->rel_args.table_flags = table_flags;

    if (total_record_count != -1){
        duckdb_bind_set_cardinality(info, total_record_count, true);
    }

    duckdb_bind_set_bind_data(info, bind_data, free);
}

void traceprov_duckdb_init(duckdb_init_info info){
    auto bind_data = (TraceProvBindData *)duckdb_init_get_bind_data(info);
    TraceProvInitData *init_data_inst = allocate_init_data();
    duckdb_init_set_init_data(info, init_data_inst, free);
    if (bind_data->is_dummy){
        init_data_inst->is_dummy = true;
        return;
    }
    // TODO: See if it is worth optimization below.
    // Even if we're using 1 thread, we still treat the init data as a "worker" one.
    // This simplifies of the code handling later. But, could, theoreticlaly, be micro-optimized.
    uint64_t max_threads = 1;
    if (bind_data->rel_args.worker_id == 0){
        // Don't make init data yet for this case.
        if (!traceprov_force_seq_scan){
            max_threads = bind_data->worker_bind_data->size();
        }
    }else{
        init_data_inst->worker_init_data = new std::vector<TraceProvInitData *>;
        init_data_inst->worker_init_data->push_back(
            traceprov_make_init_data(bind_data)
        );
    }
    // This way, all the threads will scan each portion of the init data.
    duckdb_init_set_max_threads(info, max_threads);
}

void traceprov_duckdb_local_init(duckdb_init_info info){
    auto bind_data = (TraceProvBindData *)duckdb_init_get_bind_data(info);
    TraceProvInitData *init_data_inst = allocate_init_data();
    init_data_inst->worker_init_data = new std::vector<TraceProvInitData *>;
    duckdb_init_set_init_data(info, init_data_inst, free);
    if (bind_data->is_dummy){
        init_data_inst->is_dummy = true;
        return;
    }
    bool is_dummy = false;
    bind_data->bind_data_mutex->lock();
    const uint64_t self_idx = bind_data->max_worker_idx++;
    bind_data->bind_data_mutex->unlock();
    // TODO: Make this smarter.
    // Specificially, see if this thread has a local context, and try "sticking" to that context
    if ((bind_data->rel_args.table_flags & TRACEPROV_TABLE_SEQ_SCAN) || traceprov_force_seq_scan){
        // In this case, need to over the children ones.
        if (self_idx == 0){
            for (auto worker_bind_data: *bind_data->worker_bind_data){
                init_data_inst->worker_init_data->push_back(
                    traceprov_make_init_data(worker_bind_data)
                );
            }
        }else{
            // Ugh.
            is_dummy = true;
        }
    }else{
        init_data_inst->worker_init_data->push_back(
            traceprov_make_init_data(bind_data->worker_bind_data->at(self_idx))
        );
        init_data_inst->idx_in_bind = self_idx;
    }
    init_data_inst->is_dummy = is_dummy;
}

uint64_t fillup_pointer(
    const struct traceprov_aggregate_layer *layer,
    duckdb_data_chunk chunk,
    void *source_ptr,
    uint64_t current_pos,
    const uint64_t final_num_records,
    const uint64_t total_width,
    void **final_ptr
){
    const uint64_t original_pos = current_pos;
    for (uint64_t i = 0; i < TP_STD_VECTOR_SIZE; i++){
        source_ptr = INCR_BY_BYTES(source_ptr, layer->record_padding);
        if (current_pos >= final_num_records) break;
        uint64_t *canonical_ptr = (uint64_t *)source_ptr;
        for (uint64_t col_idx = 0; col_idx < total_width; col_idx++){
            auto dest_ptr = (uint64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(chunk, col_idx));
            dest_ptr[i] = *canonical_ptr;
            canonical_ptr++;
        }
        current_pos++;
        source_ptr = (void *)canonical_ptr;
    }
    duckdb_data_chunk_set_size(chunk, current_pos - original_pos);
    *final_ptr = source_ptr;
    return current_pos;
}

uint64_t fillup_pointer_huge(
    const struct traceprov_aggregate_layer *layer,
    duckdb_data_chunk chunk,
    uint64_t current_pos,
    TraceProvBindData *bind_data,
    const uint64_t final_num_records,
    const uint64_t total_width,
    TraceProvInitData *init_data
){
    const uint64_t original_pos = current_pos;
    uint64_t final_pos = 0;
    for (uint64_t local_idx = 0; local_idx < TP_STD_VECTOR_SIZE; local_idx++){
        const uint64_t probe_pos = (local_idx + original_pos);
        final_pos = probe_pos;
        if (probe_pos >= final_num_records) break;
        traceprov_grow_col_page_mapping(0, init_data, bind_data);
        uint64_t *canonical_ptr = (uint64_t *)(INCR_BY_BYTES(init_data->col_layer_ptr, layer->record_padding));
        for (uint64_t col_idx = 0; col_idx < total_width; col_idx++){
            auto dest_ptr = (uint64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(chunk, col_idx));
            dest_ptr[local_idx] = *canonical_ptr;
            canonical_ptr++;
        }
        init_data->col_layer_ptr = canonical_ptr;
    }
    duckdb_data_chunk_set_size(chunk, final_pos - original_pos);
    return final_pos;
}


void traceprov_duckdb_func_huge_incremental(duckdb_function_info info, duckdb_data_chunk output){
    auto bind_data_combined = (TraceProvBindData *)duckdb_function_get_bind_data(info);
    auto init_data_combined = (TraceProvInitData *)duckdb_function_get_local_init_data(info);

    // This is the only case that signifies table scan end now.
    if (init_data_combined->worker_bind_idx >= init_data_combined->worker_init_data->size()){
        return duckdb_data_chunk_set_size(output, 0);
    }

    TraceProvBindData *bind_data = bind_data_combined;

    if (bind_data_combined->worker_bind_data){
        if (bind_data->rel_args.table_flags & TRACEPROV_TABLE_SEQ_SCAN || traceprov_force_seq_scan){
            bind_data = bind_data_combined->worker_bind_data->at(init_data_combined->worker_bind_idx);
        }else{
            bind_data = bind_data_combined->worker_bind_data->at(init_data_combined->idx_in_bind);
        }
    }

    auto init_data = init_data_combined->worker_init_data->at(init_data_combined->worker_bind_idx);

    if (bind_data->rel_args.offset != -1){
        uint64_t *values = NULL;
        bool new_allocated = false;
        if (values == NULL){
            // This allows caching the value, if determined at partition pruning time.
            // alloc buffer for the values.
            values = (uint64_t *) malloc(sizeof(uint64_t)*bind_data->column_width);
            read_at_offset(bind_data->rel_args.offset, bind_data, values, bind_data_combined->sizes);
            new_allocated = true;
        }
        for (idx_t col_idx = 0; col_idx < bind_data->column_width; col_idx++){
            const uint8_t col_size = bind_data_combined->sizes->at(col_idx);
            if (col_size == sizeof(uint32_t)){
                uint32_t *dest_ptr = (uint32_t *)(duckdb_vector_get_data(duckdb_data_chunk_get_vector(output, col_idx)));
                dest_ptr[0] = values[col_idx];
            }else{
                uint64_t *dest_ptr = (uint64_t *)(duckdb_vector_get_data(duckdb_data_chunk_get_vector(output, col_idx)));
                dest_ptr[0] = values[col_idx];
            }
        }
        duckdb_data_chunk_set_size(output, 1);
        if (new_allocated)
            free(values);
        init_data->is_dummy = true;
        init_data_combined->is_dummy = true;
        return;
    }

    uint64_t chunk_size = 0;
    const auto _traceprov_grow_col_page_mapping = bind_data_combined->is_memory_mapping ? traceprov_grow_col_page_mapping : traceprov_grow_col_page_mapping_file;
    const auto _traceprov_grow_row_count_page_mapping = bind_data_combined->is_memory_mapping ? traceprov_grow_row_count_page_mapping : traceprov_grow_row_count_page_mapping_file;

    if (!bind_data->is_strict_rows){
        // Haven't emitted all chunks yet.
        uint64_t seek_ahead_chunk_size = 0;
        while (true){
            if (init_data->current < bind_data->num_rows){
                _traceprov_grow_row_count_page_mapping(init_data, bind_data);
                const uint64_t num_rows = *((uint64_t*)init_data->row_count_layer_ptr);
                seek_ahead_chunk_size += num_rows;
                if (seek_ahead_chunk_size > STANDARD_VECTOR_SIZE) break;
                init_data->row_count_layer_ptr = INCR_BY_BYTES(init_data->row_count_layer_ptr, sizeof(uint64_t));
                if (bind_data->col_layer->read_columns_at_once){
                    const uint64_t col_log_size = num_rows * sizeof(uint64_t);
                    const uint64_t extra_size = col_log_size * 2;
                    _traceprov_grow_col_page_mapping(extra_size, init_data, bind_data);
                    for (idx_t col_idx = 0; col_idx < bind_data->column_width; col_idx++){
                        uint64_t *dest_ptr = (uint64_t *)(duckdb_vector_get_data(duckdb_data_chunk_get_vector(output, col_idx)));
                        memcpy(&dest_ptr[chunk_size], init_data->col_layer_ptr, col_log_size);
                        init_data->col_layer_ptr = INCR_BY_BYTES(init_data->col_layer_ptr, col_log_size);
                    }
                }else{
                    idx_t start_idx = 0;
                    if (bind_data_combined->first_mask){
                        // In this case, need to read the first column as 4 bytes, but add the mask in layer.
                        if (unlikely(bind_data->col_layer->mask == NULL)){
                            elog(ERROR, "Expected the mask to be present!!");
                        }
                        start_idx++;
                        const uint64_t extra_size = sizeof(uint32_t)*num_rows;
                        _traceprov_grow_col_page_mapping(extra_size, init_data, bind_data);
                        uint64_t *dest_ptr = &((uint64_t *)(duckdb_vector_get_data(duckdb_data_chunk_get_vector(output, 0))))[chunk_size];
                        for (idx_t row_idx = 0; row_idx < num_rows; row_idx++){
                            dest_ptr[row_idx] = ((bind_data->col_layer->mask << 32) | (uint64_t)(((uint32_t *)(init_data->col_layer_ptr))[row_idx]));
                        }
                        init_data->col_layer_ptr = INCR_BY_BYTES(init_data->col_layer_ptr, extra_size);
                    }
                    for (idx_t col_idx = start_idx; col_idx < bind_data->column_width; col_idx++){
                        const uint8_t unit_size = bind_data_combined->sizes->at(col_idx);
                        const uint64_t extra_size = unit_size*num_rows;
                        _traceprov_grow_col_page_mapping(extra_size, init_data, bind_data);
                        void *dest_ptr = (void *)(duckdb_vector_get_data(duckdb_data_chunk_get_vector(output, col_idx)));
                        memcpy(INCR_BY_BYTES(dest_ptr, chunk_size*unit_size), init_data->col_layer_ptr, extra_size);
                        init_data->col_layer_ptr = INCR_BY_BYTES(init_data->col_layer_ptr, extra_size);
                    }
                    if (bind_data->col_layer->null_layer_number){
                        duckdb_data_chunk_set_size(output, seek_ahead_chunk_size);
                        const bool is_first_write = seek_ahead_chunk_size == num_rows;
                        const idx_t eager_idx = init_data->current + 1;
                        const uint64_t validity_size = ((num_rows - 1) / 64) + 1;
                        if (eager_idx == init_data->next_null_idx){
                            auto null_init_data = init_data->null_map_init_data;
                            auto null_bind_data = init_data->null_map_bind_data;
                            for (uint64_t col_idx = 0; col_idx < init_data->next_null_col_count; col_idx++){
                                _traceprov_grow_col_page_mapping(
                                    (validity_size + 1)*sizeof(uint64_t),
                                    null_init_data,
                                    null_bind_data
                                );
                                const idx_t written_col_idx = ((uint64_t*)null_init_data->col_layer_ptr)[0];
                                null_init_data->col_layer_ptr = INCR_BY_BYTES(null_init_data->col_layer_ptr, sizeof(uint64_t));
                                duckdb_vector col_vector = duckdb_data_chunk_get_vector(output, written_col_idx);
                                uint64_t *written_validity = (uint64_t *)null_init_data->col_layer_ptr;
                                if (is_first_write){
                                    duckdb_vector_ensure_validity_writable(col_vector);
                                    uint64_t *dest_validity = duckdb_vector_get_validity(col_vector);
                                    memcpy(dest_validity, written_validity, sizeof(uint64_t)*validity_size);
                                }else{
                                    // In this case, we cannot, unfortuntaly, not simply copy the validity.
                                    // TODO: Try copying if the size can be reorganized to be of byte or something.
                                    uint64_t *dest_validity = duckdb_vector_get_validity(col_vector);
                                    const idx_t write_start_idx = (seek_ahead_chunk_size - num_rows);
                                    for (uint32_t row_idx = 0; row_idx < num_rows; row_idx++){
                                        const uint64_t validity_block = written_validity[row_idx / 64];
                                        const bool row_validity = (validity_block & ((uint64_t)1 << (row_idx % 64))) != 0;
                                        if (row_validity == 0){
                                            duckdb_validity_set_row_invalid(dest_validity, row_idx + write_start_idx);
                                        }
                                    }
                                }
                                null_init_data->col_layer_ptr = INCR_BY_BYTES(null_init_data->col_layer_ptr, sizeof(uint64_t)*validity_size);
                            }
                            // Need to set the eager idx now.
                            _traceprov_grow_col_page_mapping(
                                2*sizeof(uint64_t),
                                null_init_data,
                                null_bind_data
                            );
                            init_data->next_null_idx = ((uint64_t *)null_init_data->col_layer_ptr)[0];
                            init_data->next_null_col_count = ((uint64_t *)null_init_data->col_layer_ptr)[1];
                            null_init_data->col_layer_ptr = INCR_BY_BYTES(null_init_data->col_layer_ptr, sizeof(uint64_t)*2);
                        }
                    }
                }
                init_data->current++;
                chunk_size += num_rows;
                if (init_data->current >= bind_data->num_rows){
                    // This way, when the last chunk of the any worker's layer is seen, we automatically
                    // shift to the next worker's layer.
                    init_data_combined->worker_bind_idx++;
                }
            }else{
                break;
            }
            // The original behaviour.
            if (!traceprov_use_merge_chunks) break;
        }
        // elog(INFO, "Chunk Sizes: %ld", chunk_size);
        duckdb_data_chunk_set_size(output, chunk_size);
        return;
    }
    // Previously, combine was handled here.
    elog(ERROR, "Didn't expect to get here now!");
}

void traceprov_duckdb_func(duckdb_function_info info, duckdb_data_chunk output){
    auto bind_data = (TraceProvBindData *)duckdb_function_get_bind_data(info);
    auto init_data = (TraceProvInitData *)duckdb_function_get_local_init_data(info);

    if (init_data->is_dummy){
        duckdb_data_chunk_set_size(output, 0);
        return;
    }

    traceprov_duckdb_func_huge_incremental(info, output);
}

struct TraceProvCTableFunctionInfo : public TableFunctionInfo {
    ~TraceProvCTableFunctionInfo() {
        if (extra_info && delete_callback) {
            delete_callback(extra_info);
        }
        extra_info = nullptr;
        delete_callback = nullptr;
    }

    duckdb_table_function_bind_t bind = nullptr;
    duckdb_table_function_init_t init = nullptr;
    duckdb_table_function_init_t local_init = nullptr;
    duckdb_table_function_t function = nullptr;
    void *extra_info = nullptr;
    duckdb_delete_callback_t delete_callback = nullptr;
};

struct TraceProvCTableBindData : public TableFunctionData {
    TraceProvCTableBindData(TraceProvCTableFunctionInfo &info) : info(info) {
    }
    ~TraceProvCTableBindData() {
        if (bind_data && delete_callback) {
            delete_callback(bind_data);
        }
        bind_data = nullptr;
        delete_callback = nullptr;
    }

    TraceProvCTableFunctionInfo &info;
    void *bind_data = nullptr;
    duckdb_delete_callback_t delete_callback = nullptr;
    unique_ptr<NodeStatistics> stats;
};


void handle_pointer_stats(TraceProvStatistics *old_stats, const struct traceprov_aggregate_layer *aggregate_layer, const TraceProvRelationArgs rel_args){
    // Doesn't really matter if the mask is set or not.
    const uint64_t min_value = TRACEPROV_SET_WORKER_ID((uint64_t)1, rel_args.worker_id);
    const uint64_t max_value = TRACEPROV_SET_WORKER_ID((uint64_t)aggregate_layer->num_groups, rel_args.worker_id);
    TraceProvStatistics stats = {
        .is_set = true,
        .min_value = min_value,
        .max_value = max_value
    };
    merge_stats(old_stats, &stats);
}

void bind_data_stats(TraceProvStatistics *stats, TraceProvBindData *bind_data, const bool is_aggregate, const bool column_index){
    if (is_aggregate && (column_index == 0)) {
        handle_pointer_stats(stats, bind_data->col_layer, bind_data->rel_args);
    } else {
        merge_stats(stats, &bind_data->col_layer->stats[column_index]);
    }
}

unique_ptr<BaseStatistics> traceprov_duckdb_table_stats(
    ClientContext &context,
    const FunctionData *bind_data,
    column_t column_index
){
    if (column_index == 0) elog(INFO, "Calling with 0 as col idx!");
    uint64_t distinct_count = 0;
    TraceProvBindData *tp_bind_data = (TraceProvBindData *)((TraceProvCTableBindData *)bind_data)->bind_data;
    TraceProvStatistics stats = {
        .is_set = false,
        .min_value = 0,
        .max_value = 0
    };
    bool is_combine = (tp_bind_data->rel_args.table_flags & TRACEPROV_TABLE_COMBINE) != 0;
    TraceProvLayerNumber layer = tp_bind_data->rel_args.layer_number;
    if (tp_bind_data->col_layer){
        bind_data_stats(&stats, tp_bind_data, tp_bind_data->is_aggregate, column_index);
        if (column_index == 0 && (tp_bind_data->is_aggregate || is_combine)){
            distinct_count += tp_bind_data->col_layer->num_groups;
        }
    }
    for (auto child_bind_data: *tp_bind_data->worker_bind_data){
        bind_data_stats(&stats, child_bind_data, tp_bind_data->is_aggregate, column_index);
        layer = child_bind_data->rel_args.layer_number;
        is_combine |= (child_bind_data->rel_args.table_flags & TRACEPROV_TABLE_COMBINE) != 0;
        if (column_index == 0 && (child_bind_data->is_aggregate || is_combine)){
            distinct_count += child_bind_data->col_layer->num_groups;
        }
    }
    const uint8_t column_size = tp_bind_data->sizes->at(column_index);
    auto result = NumericStats::CreateEmpty(  column_size == sizeof(uint64_t ) ? LogicalType::UBIGINT : LogicalType::UINTEGER );
    elog(INFO, "Asking stats for (%d, %d)", layer, column_index);
    if (g_tp_agg_extra->stats_collector_map != NULL){
        if (g_tp_agg_extra->stats_collector_map->find(layer) != g_tp_agg_extra->stats_collector_map->end() ){
            auto stats_cols = g_tp_agg_extra->stats_collector_map->at(layer);
            for (auto col: *stats_cols){
                elog(INFO, "Spec stats (%d, %d)", layer, col);
            }
        }
    }
    if (stats.is_set && traceprov_use_table_stats){
        elog(INFO, "Setting stats for (%d, %d) -- [%lu, %lu]. Distinct: %lu", layer, column_index, stats.min_value, stats.max_value, distinct_count);
        if (column_size == sizeof(uint64_t)){
            NumericStats::SetMin(result, Value::UBIGINT(stats.min_value));
            NumericStats::SetMax(result, Value::UBIGINT(stats.max_value));
        }else{
            NumericStats::SetMin(result, Value::UINTEGER(stats.min_value));
            NumericStats::SetMax(result, Value::UINTEGER(stats.max_value));
        }

        if (distinct_count != 0){
            result.SetDistinctCount(distinct_count);
        }
    }
	return result.ToUnique();
}

// Taken from DuckDB.
duckdb::TableFunction *GetCTableFunction(duckdb_table_function function) {
    return reinterpret_cast<duckdb::TableFunction *>(function);
}

duckdb_table_function traceprov_create_table_func(){
    auto function = duckdb_create_table_function();
    duckdb_table_function_set_name(function, "traceprov_read_worker_layer");
    duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
    duckdb_table_function_add_parameter(function, type);
    duckdb_table_function_add_parameter(function, type);
    duckdb_table_function_add_parameter(function, type);
    duckdb_destroy_logical_type(&type);

    duckdb_table_function_set_bind(function, traceprov_duckdb_bind);
    duckdb_table_function_set_init(function, traceprov_duckdb_init);
    duckdb_table_function_set_function(function, traceprov_duckdb_func);
    duckdb_table_function_set_local_init(function, traceprov_duckdb_local_init);
    auto duckdb_function = GetCTableFunction(function);
    duckdb_function->statistics = traceprov_duckdb_table_stats;
    return function;
}

duckdb_table_function traceprov_create_table_offset_func(){
    auto function = traceprov_create_table_func();
    duckdb_table_function_set_name(function, "traceprov_read_worker_layer_offset");
    duckdb_logical_type offset_type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
    duckdb_table_function_add_parameter(function, offset_type);
    duckdb_destroy_logical_type(&offset_type);
    duckdb_table_function_set_extra_info(function, NULL, nullptr);
    return function;
}

duckdb_table_function traceprov_create_table_offset_partition_func(){
    auto function = traceprov_create_table_func();
    duckdb_table_function_set_name(function, "traceprov_read_worker_layer_partition");
    duckdb_logical_type offset_type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
    duckdb_table_function_add_parameter(function, offset_type);
    duckdb_table_function_add_parameter(function, offset_type);
    duckdb_destroy_logical_type(&offset_type);
    duckdb_table_function_set_extra_info(function, NULL, nullptr);
    return function;
}


// Worker and Layer will fit in uint32_t. Two of them are unique enough to distinguish any combination.
// So they are combined here togther to form a key into the context map of layers.
// Doesn't really matter which one comes first, as long as we're consistent about it...
#define TRACEPROV_MAKE_WORKER_LAYER_KEY(X, Y) ((uint64_t)(((uint64_t)X << 32) | (uint64_t)Y))

typedef struct TraceProvWindowFuncExtra {
    std::unordered_map<uint64_t, TraceProvBindData *> *key_bind_map;
    uint64_t num_cols;
} TraceProvWindowFuncExtra;

#if TRACEPROV_SD_MODE==0

void traceprov_window_func(duckdb_function_info info, duckdb_data_chunk input, duckdb_vector output){
    TraceProvWindowFuncExtra *window_extra = (TraceProvWindowFuncExtra *)duckdb_scalar_function_get_extra_info(info);
    if (unlikely(window_extra == NULL)){
        elog(ERROR, "Expected window func extra to be set!");
    }
    duckdb_vector worker_id_vector = duckdb_data_chunk_get_vector(input, 0);
    uint32_t *worker_id_data = (uint32_t *)duckdb_vector_get_data(worker_id_vector);
    
    duckdb_vector layer_number_vector = duckdb_data_chunk_get_vector(input, 1);
    uint32_t *layer_number_data = (uint32_t *)duckdb_vector_get_data(layer_number_vector);
    
    duckdb_vector frame_start_vector = duckdb_data_chunk_get_vector(input, 2);
    uint64_t *frame_start_data = (uint64_t *)duckdb_vector_get_data(frame_start_vector);

    duckdb_vector frame_end_vector = duckdb_data_chunk_get_vector(input, 3);
    uint64_t *frame_end_data = (uint64_t *)duckdb_vector_get_data(frame_end_vector);

    const idx_t row_count = duckdb_data_chunk_get_size(input);

    idx_t expected_size = 0;
    for (idx_t row_idx = 0; row_idx < row_count; row_idx++){
        if (unlikely(frame_end_data[row_idx] < frame_start_data[row_idx])){
            elog(ERROR, "Expected end to never be less than start!");
        }
        // Since a frame always includes the current row, also need to add 1.
        // This could be an upper bound in the cases where the worker is simply not present..
        expected_size += (frame_end_data[row_idx] - frame_start_data[row_idx]) + 1;
    }
        
    if(duckdb_list_vector_reserve(output, expected_size) == DuckDBError){
        elog(ERROR, "Error reserving!");
    }
    
    if(duckdb_list_vector_set_size(output, expected_size) == DuckDBError){
        elog(ERROR, "Error setting size!");
    }

    auto entries = (duckdb_list_entry *)duckdb_vector_get_data(output);

    duckdb_vector child_structs = duckdb_list_vector_get_child(output);
    uint64_t generic_idx = 0;
    for (uint64_t row_idx = 0; row_idx < row_count; row_idx++){
        const uint32_t worker_id = worker_id_data[row_idx];
        const uint32_t layer_number = layer_number_data[row_idx];
        TraceProvBindData *curr_bind_data = window_extra->key_bind_map->at(TRACEPROV_MAKE_WORKER_LAYER_KEY(worker_id, layer_number));

        const uint64_t frame_start = frame_start_data[row_idx];
        const uint64_t frame_end = frame_end_data[row_idx];
        const idx_t generic_start_idx = generic_idx;
        // For a given layer, number of cols is predetermined.
        // SO, this is fine being allocated here and reused.
        uint64_t *values = (uint64_t *) malloc(sizeof(uint64_t)*window_extra->num_cols);
        for (uint64_t row_to_return = frame_start; row_to_return < frame_end + 1; row_to_return++, generic_idx++){
            const int64_t logged_row_idx = row_to_return - 1;
            if (unlikely(logged_row_idx < 0))
                elog(ERROR, "any idx can never be < 0!");

            read_at_offset(logged_row_idx, curr_bind_data, values, NULL);            
            for (uint64_t col_idx = 0; col_idx < window_extra->num_cols; col_idx++){
                duckdb_vector member_data = duckdb_struct_vector_get_child(child_structs, col_idx);
                uint64_t *child_data = (uint64_t*) duckdb_vector_get_data(member_data);
                child_data[generic_idx] = values[col_idx];
            }
        }
        free(values);
        const uint64_t generic_end_idx = generic_idx;
        entries[row_idx].offset = generic_start_idx;
        entries[row_idx].length = generic_end_idx - generic_start_idx;
    }
}

// Creates a window function, returning num_args in the struct.
duckdb_scalar_function traceprov_create_table_window_func(
    const uint64_t num_args,
    // The layer numbers that'll contain this many number of args.
    // This is done this way so we don't need to any strict checks later...
    const uint32_t worker_count,
    std::vector<uint32_t> *expected_layers
){
    duckdb_scalar_function func = duckdb_create_scalar_function();
    std::string *func_name = new std::string(("traceprov_window_func" + std::to_string(num_args)).c_str());
    duckdb_scalar_function_set_name(func, func_name->c_str());
    duckdb_logical_type basic_arg_type = duckdb_create_logical_type(DUCKDB_TYPE_INTEGER);
    duckdb_logical_type arg_type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
    // Worker id.
    duckdb_scalar_function_add_parameter(func, basic_arg_type);
    // Layer id.
    duckdb_scalar_function_add_parameter(func, basic_arg_type);
    // Frame start
    duckdb_scalar_function_add_parameter(func, arg_type);
    // Frame end.
    duckdb_scalar_function_add_parameter(func, arg_type);

    duckdb_logical_type *struct_member_types = (duckdb_logical_type *)malloc(sizeof(duckdb_logical_type)*num_args);
    const char **member_names = (const char **)malloc(sizeof(char *)*num_args);
    for (uint64_t col_idx = 0; col_idx < num_args; col_idx++){
        std::string *column_str = new std::string((std::string("column_") + std::to_string(col_idx)).c_str());
        member_names[col_idx] = column_str->c_str();
        struct_member_types[col_idx] = arg_type;
    }
    duckdb_logical_type struct_type = duckdb_create_struct_type(struct_member_types, member_names, num_args);
    duckdb_logical_type list_type = duckdb_create_list_type(struct_type);
    duckdb_scalar_function_set_return_type(func, list_type);
    duckdb_destroy_logical_type(&list_type);
    duckdb_scalar_function_set_function(func, traceprov_window_func);
    duckdb_destroy_logical_type(&arg_type);

    auto window_extra = new TraceProvWindowFuncExtra;
    window_extra->key_bind_map = new std::unordered_map<uint64_t, TraceProvBindData *>;

    for (uint32_t worker_id = 1; worker_id < worker_count + 1; worker_id++){
        for (auto layer_number: *expected_layers){
            const uint64_t worker_layer_key = TRACEPROV_MAKE_WORKER_LAYER_KEY(worker_id, layer_number);
            if (unlikely(window_extra->key_bind_map->find(worker_layer_key) != window_extra->key_bind_map->end())){
                elog(ERROR, "Expected the key to not be present!");
            }
            window_extra->key_bind_map->insert({worker_layer_key, setup_layers(worker_id, layer_number, -1, NULL)});
        }
    }
    window_extra->num_cols = num_args;
    duckdb_scalar_function_set_extra_info(func, window_extra, nullptr);
    return func;
}
#endif

void *traceprov_get_row(
    const uint64_t worker_id,
    const uint64_t layer_number,
    const uint64_t log_probe_value,
    std::vector<uint8_t> *sizes
){
    auto bind_data = setup_layers(worker_id, layer_number, log_probe_value, NULL, true);
    bind_data->sizes = sizes;
    auto value = (uint64_t *)malloc(sizeof(uint64_t)*(bind_data->column_width));
    read_at_offset(log_probe_value, bind_data, value, sizes);
    if (value == 0)
        elog(ERROR, "Expected value to be something!!");
    return value;
}


void traceprov_read_int_vector_func(duckdb_function_info info, duckdb_data_chunk input, duckdb_vector output){

    duckdb_vector ptr_vector = duckdb_data_chunk_get_vector(input, 0);
    uint64_t *ptr_vector_data = (uint64_t *)duckdb_vector_get_data(ptr_vector);

    duckdb_vector idx_vector = duckdb_data_chunk_get_vector(input, 1);
    const uint64_t idx_to_return = ((uint64_t*)duckdb_vector_get_data(idx_vector))[0];

    const idx_t row_count = duckdb_data_chunk_get_size(input);
    idx_t expected_size = 0;
    for (idx_t row_idx = 0; row_idx < row_count; row_idx++){
        auto extended_state = (AggStateExtended *) ptr_vector_data[row_idx];
        if (extended_state == NULL) continue;
        if (idx_to_return == 0){
            expected_size += extended_state->total_size;
        }else{
            expected_size++;
            if (extended_state->extended != NULL){
                if (extended_state->extended->at(idx_to_return - 1)){
                    auto extended_to_append = extended_state->extended->at(idx_to_return - 1);
                    expected_size += extended_to_append->size();
                }
            }
        }
    }
    // elog(INFO, "Row: %ld, Expected: %ld", row_count, expected_size);
    if (duckdb_list_vector_reserve(output, expected_size) == DuckDBError){
        elog(ERROR, "Error reserving!");
    }
    if (duckdb_list_vector_set_size(output, expected_size) == DuckDBError){
        elog(ERROR, "Error setting size!");
    }
    auto entries = (duckdb_list_entry *)duckdb_vector_get_data(output);

    duckdb_vector child_ptr = duckdb_list_vector_get_child(output);
    uint64_t *child_ptr_data = (uint64_t*)duckdb_vector_get_data(child_ptr);

    uint64_t generic_idx = 0;

    for (uint64_t row_idx = 0; row_idx < row_count; row_idx++){
        auto extended_state = (AggStateExtended*) ptr_vector_data[row_idx];
        idx_t vector_size = 0;
        const idx_t generic_start_idx = generic_idx;
        idx_t end_idx = 0;
        if (extended_state != NULL){
            if (idx_to_return == 0){
                vector_size = extended_state->inline_state->size();
                // Done via memcpy to speed this up.
                idx_t cursor = generic_start_idx;
                memcpy(&child_ptr_data[cursor], extended_state->inline_state->data(), extended_state->inline_state->size()*sizeof(uint64_t));
                cursor += extended_state->inline_state->size();
                if (extended_state->extended){
                    for (auto vec: *extended_state->extended){
                        if (vec == NULL) continue;
                        memcpy(&child_ptr_data[cursor], vec->data(), vec->size()*sizeof(uint64_t));
                        vector_size += vec->size();
                        cursor += vec->size();
                    }
                }
            }else{
                std::vector<uint64_t> *extended_to_append = NULL;
                if (extended_state->extended != NULL){
                    if (extended_state->extended->at(idx_to_return - 1)){
                        extended_to_append = extended_state->extended->at(idx_to_return - 1);
                    }
                }
                idx_t cursor = generic_start_idx;
                vector_size++;
                child_ptr_data[generic_start_idx] = extended_state->inline_state->at(idx_to_return - 1);
                cursor++;
                if (extended_to_append){
                    memcpy(&child_ptr_data[cursor], extended_to_append->data(), extended_to_append->size()*sizeof(uint64_t));
                    vector_size += extended_to_append->size();
                }
            }
        }
        generic_idx += vector_size;
        entries[row_idx].offset = generic_start_idx;
        entries[row_idx].length = vector_size;
    }
}

#if TRACEPROV_SD_MODE==0
duckdb_scalar_function traceprov_create_read_vector_func(){
    duckdb_scalar_function func = duckdb_create_scalar_function();
    std::string *func_name = new std::string("traceprov_read_int_vector");
    duckdb_scalar_function_set_name(func, func_name->c_str());
    duckdb_logical_type ubigint_type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
    duckdb_scalar_function_add_parameter(func, ubigint_type);
    duckdb_scalar_function_add_parameter(func, ubigint_type);

    duckdb_logical_type list_type = duckdb_create_list_type(ubigint_type);
    duckdb_scalar_function_set_return_type(func, list_type);
    duckdb_destroy_logical_type(&list_type);
    duckdb_destroy_logical_type(&ubigint_type);

    duckdb_scalar_function_set_function(func, traceprov_read_int_vector_func);
    return func;
}
#endif

static uint8_t dummy = 0;

void traceprov_attempt_prefaults(){
    initialize_global_context();
    auto start_time = std::chrono::steady_clock::now();
    for (auto entry: *g_tp_duckdb_state.worker_local_contexts){
        for (idx_t layer_idx = 0; layer_idx < TRACEPROV_MAX_LAYER_PER_WORKER; layer_idx++){
            const traceprov_aggregate_layer *layer = &entry->cached_layers[layer_idx];
            if (layer->layer_number == 0 || layer->page_mapping == NULL)  continue;
            for (int32_t page_idx = layer->page_mapping_size - 1; page_idx >= 0; page_idx--){
                uint64_t pages_used = 0;
                if (page_idx > 0 && page_idx < layer->page_mapping_size - 1){
                    pages_used = TRACEPROV_INCREMENT_TRACE_BY_PG;
                }else if (page_idx == layer->page_mapping_size - 1){
                    pages_used = (((uint64_t)layer->current_row - (uint64_t)layer->last_mapping) / TRACEPROV_PAGE_SIZE);
                }else{
                    pages_used = 1;
                }
                void *page_ptr = layer->page_mapping[page_idx];
                const size_t page_size = pages_used * TRACEPROV_PAGE_SIZE;
                int rc =  madvise(page_ptr, page_size, MADV_WILLNEED);
                if (rc != 0){
                    elog(ERROR, "Got error madvise!");
                }
                if(mlock(page_ptr, page_size)){
                    elog(ERROR, "Got error mlock!");
                }
                // if (mlockall(MCL_CURRENT | MCL_FUTURE | MCL_ONFAULT)){
                //     elog(ERROR, "Got error mlock all")
                // }
                // Hopefully setup the TLB too.
                const auto value = ((uint8_t *)(page_ptr))[0];
                dummy += value;
            }
        }
    }
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time);
    elog(INFO, "Time for prefaults: %ld", (duration).count());
}

TraceProvLogSize traceprov_get_total_layer_size(){
    TraceProvLogSize log_size = {
        .page_requested_size = 0,
        .page_used_size = 0,
        .bytes_used_size = 0
    };
    uint64_t page_requested = 0;
    uint64_t page_used = 0;
    uint64_t bytes_used = 0;
    initialize_global_context();
    for (auto entry : *g_tp_duckdb_state.worker_local_contexts){
        for (idx_t layer_idx = 0; layer_idx < TRACEPROV_MAX_LAYER_PER_WORKER; layer_idx++){
            const traceprov_aggregate_layer *layer = &entry->cached_layers[layer_idx];
            if (layer->layer_number == 0) continue;
            page_requested += layer->size;
            page_used += 1;
            const uint64_t gap = ((uint64_t)layer->current_row - (uint64_t)layer->last_mapping);
            bytes_used += gap;
            if (layer->size > 1){
                // Need to determine which pages were used.
                bytes_used += TRACEPROV_PAGE_SIZE; // for the first page.
                const int64_t allocate_count = (layer->size - 1) / TRACEPROV_INCREMENT_TRACE_BY_PG;
                if (allocate_count <= 0){
                    elog(ERROR, "Expected allocations to be > 1");
                }
                const uint64_t intermediate_page_used = (allocate_count - 1)*TRACEPROV_INCREMENT_TRACE_BY_PG;
                page_used += intermediate_page_used;
                bytes_used += (intermediate_page_used * TRACEPROV_PAGE_SIZE);
                if (gap == 0){
                    // This can never happen.
                    // Otherwise, it will imply that the increment go through, but no nemory access was performed, which is not possible.
                    elog(ERROR, "Should never expect gap to be 0");
                }
                page_used += ((gap - 1) / TRACEPROV_PAGE_SIZE) + 1;
            }
        }
    }
    log_size.page_requested_size = page_requested * TRACEPROV_PAGE_SIZE;
    log_size.page_used_size = page_used * TRACEPROV_PAGE_SIZE;
    log_size.bytes_used_size = bytes_used;
    return log_size;
}