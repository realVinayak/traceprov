// implements table scan functions, for duckdb-based traceprov files.


#include <stdlib.h>
#include <vector>

#include <cstring>
#include <sys/mman.h>

#include <string>

extern "C" {

#include "traceprov.h"
#include "duckdb.h"
#include "utils.h"
#include "file_utils.h"

typedef struct TraceProvDuckDbGlobalState {
    bool did_initialize;
    std::vector<struct local_context *> *worker_local_contexts;
} TraceProvDuckDbGlobalState;

static TraceProvDuckDbGlobalState g_tp_duckdb_state {
    .did_initialize = false,
    .worker_local_contexts = NULL
};

// void reinit_traceprov_infer_state(){
//     g_tp_duckdb_state.did_initialize = false;
//     g_tp_duckdb_state.worker_local_contexts = NULL;
// }

typedef struct TraceProvRelationArgs {
    uint64_t worker_id;
    uint64_t layer_number;
} TraceProvRelationArgs;

typedef struct TraceProvBindData {
    TraceProvRelationArgs rel_args;
    void *col_layer_ptr;
    void *row_count_layer_ptr;
    uint64_t num_rows;
    uint64_t column_width;
    struct traceprov_aggregate_layer *col_layer;
    struct traceprov_aggregate_layer *row_layer;
    bool is_dummy;
    // how many chunks can fit in the first page.
    uint64_t first_page_element_count;
    // how many chunks can fit in each added page.
    uint64_t incr_page_element_count;
    // same as above, but for row layer.
    uint64_t row_first_page_element_count;
    uint64_t row_incr_page_element_count;
    bool is_strict_rows;
} TraceProvBindData;

typedef struct TraceProvInitData {
    uint64_t current;
    uint64_t current_page;
} TraceProvInitData;

static uint64_t get_idx(const uint64_t first_page_count, const int64_t incr_page_count, const uint64_t idx);
static uint64_t get_local_idx(const uint64_t first_page_count, const int64_t incr_page_count, const uint64_t page_idx, const uint64_t chunk_idx);

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
    }
    return worker_local_contexts;
}


void traceprov_duckdb_bind(duckdb_bind_info info){
    if (duckdb_bind_get_parameter_count(info) != 2){
        elog(ERROR, "Expected 2 params!");
    }

    auto my_bind_data = (TraceProvBindData *)malloc(sizeof(TraceProvBindData));
    memset(my_bind_data, 0, sizeof(TraceProvBindData));
    auto param_1 = duckdb_bind_get_parameter(info, 0);
    uint64_t current_worker_id = duckdb_get_int64(param_1);
    auto param_2 = duckdb_bind_get_parameter(info, 1);
    const uint64_t layer_number = duckdb_get_int64(param_2);

    duckdb_destroy_value(&param_1);
    duckdb_destroy_value(&param_2);

    my_bind_data->rel_args.worker_id = current_worker_id;
    my_bind_data->rel_args.layer_number = layer_number;

    if (!g_tp_duckdb_state.did_initialize){
        traceprov_shared_context shared_context;
        if (map_traceprov_shared_context(&shared_context))
            elog(ERROR, "error maping shared context!");

        g_tp_duckdb_state.did_initialize = true;
        g_tp_duckdb_state.worker_local_contexts = traceprov_get_local_contexts(shared_context.worker_count);
    }
    bool is_dummy = false;
    if (current_worker_id > g_tp_duckdb_state.worker_local_contexts->size()){
        current_worker_id = 1;
        is_dummy = true;
    }

    auto current_local_context =  g_tp_duckdb_state.worker_local_contexts->at(current_worker_id - 1);
    auto current_layer = &current_local_context->cached_layers[layer_number - 1];

    if (current_layer->layer_number != layer_number)
        elog(ERROR, "Expected the layer number to be filled");

    uint64_t column_count = current_layer->num_pk_records;

     my_bind_data->is_strict_rows = false;

    if (current_layer->page_mapping == NULL){
        map_layer_file(current_layer->layer_number, current_worker_id, &my_bind_data->col_layer_ptr, current_layer->size);
    }else if (current_layer->rows_layer_number) {
        const uint64_t logged_chunk_size = (current_layer->num_pk_records * (sizeof(uint64_t)) * TP_STD_VECTOR_SIZE);
        my_bind_data->first_page_element_count = (TRACEPROV_PAGE_SIZE / logged_chunk_size);
        if (TRACEPROV_PAGE_SIZE % logged_chunk_size)
            elog(ERROR, "Expected complete chunks!");
        my_bind_data->incr_page_element_count = ((TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG) / logged_chunk_size);
        if ((TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG) % logged_chunk_size)
            elog(ERROR, "Expected complete chunks!");
    }else{
        // In this case, have page mapping, but not rows.
        // This will be the case for combine.
        const uint64_t logged_chunk_size = (current_layer->num_pk_records * sizeof(uint64_t));
        if (current_layer->record_padding != 0)
            elog(ERROR, "Expected 0 padding..");
        my_bind_data->first_page_element_count = (TRACEPROV_PAGE_SIZE / logged_chunk_size);
        my_bind_data->incr_page_element_count = ((TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG) / logged_chunk_size);
        my_bind_data->is_strict_rows = true;
        if (TRACEPROV_PAGE_SIZE % logged_chunk_size)
            elog(ERROR, "expected complete chunks!");
        if (((TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG) % logged_chunk_size))
            elog(ERROR, "expected complete chunks!");
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
            const uint64_t logged_chunk_size = (rows_layer->num_pk_records * sizeof(uint64_t));
            my_bind_data->row_first_page_element_count = (TRACEPROV_PAGE_SIZE / logged_chunk_size);
            my_bind_data->row_incr_page_element_count = ((TRACEPROV_PAGE_SIZE*TRACEPROV_INCREMENT_TRACE_BY_PG) / logged_chunk_size);
        }

        chunk_count = rows_layer->num_rows;
        // move past the first page.
        if (((current_layer->num_pk_records)*TP_STD_VECTOR_SIZE) > TRACEPROV_PAGE_SIZE)
            my_bind_data->col_layer_ptr += TRACEPROV_PAGE_SIZE;
    }else{
        const void *final_ptr = get_final_ptr(my_bind_data->col_layer_ptr, current_layer);
        chunk_count = ((uint64_t)final_ptr - (uint64_t)my_bind_data->col_layer_ptr) / TRACEPROV_GET_RECORD_SIZE(current_layer);
        // HACKY. Doesn't belong here.
        column_count++;
    }

    my_bind_data->column_width = column_count;
    my_bind_data->num_rows = chunk_count;


    for (uint64_t col_count = 0; col_count < column_count; col_count++){
        const std::string param = std::string("column_") + std::to_string(col_count);
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
        duckdb_bind_add_result_column(info, param.c_str(), type);
        duckdb_destroy_logical_type(&type);
    }

    my_bind_data->is_dummy = is_dummy;
    duckdb_bind_set_bind_data(info, my_bind_data, free);
}

void traceprov_duckdb_init(duckdb_init_info info){
    auto init_data_inst = (TraceProvInitData *)malloc(sizeof(TraceProvInitData));
    init_data_inst->current = 0;
    init_data_inst->current_page = 0;
    duckdb_init_set_init_data(info, init_data_inst, free);
}

uint64_t fillup_pointer(
    struct traceprov_aggregate_layer *layer,
    duckdb_data_chunk chunk,
    void *source_ptr,
    uint64_t current_pos,
    const uint64_t final_num_records,
    const uint64_t total_width,
    void **final_ptr
){
    const uint64_t original_pos = current_pos;
    for (uint64_t i = 0; i < TP_STD_VECTOR_SIZE; i++){
        source_ptr += layer->record_padding;
        if (current_pos >= final_num_records) break;
        uint64_t *canonical_ptr = (uint64_t *)source_ptr;
        for (uint64_t col_idx = 0; col_idx < total_width; col_idx++){
            auto dest_ptr = (uint64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(chunk, col_idx));
            if (col_idx == 1){
                // icky hacky.
                // TODO: Be more smart than this. good enough for capture study ig.
                dest_ptr[i] = 1;
                continue;
            }
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
    struct traceprov_aggregate_layer *layer,
    duckdb_data_chunk chunk,
    uint64_t current_pos,
    TraceProvBindData *bind_data,
    const uint64_t final_num_records,
    const uint64_t total_width
){
    const uint64_t original_pos = current_pos;
    uint64_t final_pos = 0;
    for (uint64_t local_idx = 0; local_idx < TP_STD_VECTOR_SIZE; local_idx++){
        const uint64_t probe_pos = (local_idx + original_pos);
        final_pos = probe_pos;
        if (probe_pos >= final_num_records) break;
        // Very inefficient.
        const uint64_t page_idx = get_idx(
            bind_data->first_page_element_count,
            bind_data->incr_page_element_count,
            probe_pos
        );

        const uint64_t idx_in_page = get_local_idx(
            bind_data->first_page_element_count,
            bind_data->incr_page_element_count,
            page_idx,
            probe_pos
        );
        uint64_t *canonical_ptr = (uint64_t *)&((uint8_t *)(layer->page_mapping[page_idx]))[idx_in_page * sizeof(uint64_t)*layer->num_pk_records];
        for (uint64_t col_idx = 0; col_idx < total_width; col_idx++){
            auto dest_ptr = (uint64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(chunk, col_idx));
            if (col_idx == 1){
                dest_ptr[local_idx] = 1;
                continue; 
            }
            dest_ptr[local_idx] = *canonical_ptr;
            canonical_ptr++;
        }

    }
    duckdb_data_chunk_set_size(chunk, final_pos - original_pos);
    return final_pos;
}

static uint64_t get_idx(const uint64_t first_page_count, const int64_t incr_page_count, const uint64_t idx){
    const uint64_t canonical_idx = idx + 1;
    if (canonical_idx < first_page_count)
        return 0;
    int64_t remaining = canonical_idx - first_page_count;
    uint64_t return_idx = 1;
    while (remaining > 0){
        return_idx++;
        remaining -= incr_page_count;
    }
    return return_idx - 1;
}

static uint64_t get_local_idx(const uint64_t first_page_count, const int64_t incr_page_count, const uint64_t page_idx, const uint64_t chunk_idx){
    if (page_idx == 0)
        return chunk_idx;
    const uint64_t before = first_page_count + ((page_idx - 1)*incr_page_count);
    return chunk_idx - before;
}

// It's different enough to warrant a new call path.
void traceprov_duckdb_func_huge(duckdb_function_info info, duckdb_data_chunk output){
    auto bind_data = (TraceProvBindData *)duckdb_function_get_bind_data(info);
    auto init_data = (TraceProvInitData *)duckdb_function_get_init_data(info);
    uint64_t chunk_size = 0;
    if (!bind_data->is_strict_rows){
        if (init_data->current < bind_data->num_rows){
            // First, need to figure out which page we actually are in.
            // It'll be different for each layer.
            // This is the page that contains the chunk.
            const uint64_t col_page_idx = get_idx(bind_data->first_page_element_count, bind_data->incr_page_element_count, init_data->current);
            // This is the local idx in that page where the chunk is.
            const uint64_t idx_in_col_page = get_local_idx(bind_data->first_page_element_count, bind_data->incr_page_element_count, col_page_idx, init_data->current);
            void *chunk_data_ptr = (void *)&((uint8_t*)bind_data->col_layer->page_mapping[col_page_idx])[idx_in_col_page*(sizeof(uint64_t)*bind_data->column_width*TP_STD_VECTOR_SIZE)];
            const uint64_t row_page_idx = get_idx(bind_data->row_first_page_element_count, bind_data->row_incr_page_element_count, init_data->current);
            const uint64_t idx_in_row_page = get_local_idx(bind_data->row_first_page_element_count, bind_data->row_incr_page_element_count, row_page_idx, init_data->current);
            uint64_t num_rows = ((uint64_t *)bind_data->row_layer->page_mapping[row_page_idx])[idx_in_row_page];
            for (idx_t col_idx = 0; col_idx < bind_data->column_width; col_idx++){
                uint64_t *dest_ptr = (uint64_t *)(duckdb_vector_get_data(duckdb_data_chunk_get_vector(output, col_idx)));
                const uint64_t *source_ptr = &((uint64_t *)chunk_data_ptr)[col_idx*TP_STD_VECTOR_SIZE];
                memcpy(dest_ptr, source_ptr, num_rows *sizeof(uint64_t));
            }
            init_data->current++;
            chunk_size = num_rows;
        }
        duckdb_data_chunk_set_size(output, chunk_size);
        return;
    }
    // The normal case.
    // TODO: cleanup
    init_data->current = fillup_pointer_huge(
        bind_data->col_layer,
        output,
        init_data->current,
        bind_data,
        bind_data->num_rows,
        bind_data->column_width
    );
}

void traceprov_duckdb_func(duckdb_function_info info, duckdb_data_chunk output){
    auto bind_data = (TraceProvBindData *)duckdb_function_get_bind_data(info);
    auto init_data = (TraceProvInitData *)duckdb_function_get_init_data(info);
    if (bind_data->is_dummy){
        duckdb_data_chunk_set_size(output, 0);
        return;
    }
    if (bind_data->col_layer->page_mapping != NULL){
        traceprov_duckdb_func_huge(info, output);
        return;
    }
    if (bind_data->row_count_layer_ptr){
        uint64_t chunk_size = 0;
        if (init_data->current < bind_data->num_rows){
            const uint64_t num_rows = ((uint64_t *)bind_data->row_count_layer_ptr)[init_data->current];
            const uint64_t col_width = bind_data->column_width;
            for (idx_t col_idx = 0; col_idx < col_width; col_idx++){
                uint64_t *dest_ptr = (uint64_t *)(duckdb_vector_get_data(duckdb_data_chunk_get_vector(output, col_idx)));
                const uint64_t *source_ptr = &(((uint64_t *)(bind_data->col_layer_ptr))[(((init_data->current * col_width) + col_idx) * TP_STD_VECTOR_SIZE)]);
                memcpy(dest_ptr, source_ptr, num_rows * sizeof(uint64_t));
            }
            init_data->current++;
            chunk_size = num_rows;
        }
        duckdb_data_chunk_set_size(output, chunk_size);
        return;
    }
    // The row case.
    init_data->current = fillup_pointer(bind_data->col_layer, output, bind_data->col_layer_ptr, init_data->current, bind_data->num_rows, bind_data->column_width, &bind_data->col_layer_ptr);
}

duckdb_table_function traceprov_create_table_func(){
    auto function = duckdb_create_table_function();
    duckdb_table_function_set_name(function, "traceprov_read_worker_layer");
    duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
    duckdb_table_function_add_parameter(function, type);
    duckdb_table_function_add_parameter(function, type);
    duckdb_destroy_logical_type(&type);

    duckdb_table_function_set_bind(function, traceprov_duckdb_bind);
    duckdb_table_function_set_init(function, traceprov_duckdb_init);
    duckdb_table_function_set_function(function, traceprov_duckdb_func);
    return function;
}
}