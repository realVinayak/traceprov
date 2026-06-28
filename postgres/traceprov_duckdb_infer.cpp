
#include <string>
#include "traceprov_ext_utils.hpp"
#include "traceprov_infer.hpp"
#include "traceprov_infer_essentials.hpp"
#include <traceprov_node.hpp>
#include <mutex>
#include "traceprov_duckdb_stats.hpp"
#include "traceprov_bind.hpp"

// Duckdb integration for inference.
// Duckdb doesn't technically do anything smart (just runs the query)
// The "leaf" are table scan functions for this mess.

#ifndef STANDARD_VECTOR_SIZE
#define STANDARD_VECTOR_SIZE 2048
#endif

#define TP_ENABLE_PROFILING "PRAGMA enable_profiling=json"
#define TP_ENABLE_DETAILED_PROFILING "PRAGMA profiling_mode = 'detailed';"
#define TP_ENABLE_PROFILING_QUERY_TREE "PRAGMA enable_profiling=query_tree"
#define TP_SET_PROFILE_OUTPUT "PRAGMA profile_output='%s'"

extern "C"
{
#include "duckdb.h"
#include <stdlib.h>
#include "funcapi.h"
#include "traceprov_graph.h"

    // This is a bit different from duckdb's global state.
    // We manage the lifecycle of this ourselves.
    typedef struct TraceProvDuckDbGlobalState
    {
        bool did_initialize;
        std::vector<struct local_context *> *worker_local_contexts;
    } TraceProvDuckDbGlobalState;

    static TraceProvDuckDbGlobalState g_tp_duckdb_state{
        .did_initialize = false,
        .worker_local_contexts = NULL};

    typedef struct TraceProvInitData
    {
        uint64_t current;
        void *col_layer_ptr;
        void *row_layer_ptr;
        uint64_t number_of_records;
        void *col_null_layer_ptr;
        void *row_null_layer_ptr;
        uint64_t number_of_col_records;
        uint64_t current_col_idx;
        // The current child that's being queried.
        uint64_t current_child_idx;
        std::vector<TraceProvInitData *> *child_init_data;
        uint64_t idx_in_bind;
        bool is_dummy;
        void *scratch_space;
    } TraceProvInitData;

#define TRACEPROV_MAKE_WORKER_LAYER_KEY(X, Y) ((uint64_t)(((uint64_t)X << 32) | (uint64_t)Y))

    void traceprov_duckdb_cleanup(struct traceprov_inference_context *p_ctxt)
    {
        duckdb_disconnect(&p_ctxt->con);
        duckdb_close(&p_ctxt->db);
        g_tp_duckdb_state.did_initialize = false;
        if (g_tp_duckdb_state.worker_local_contexts)
            delete g_tp_duckdb_state.worker_local_contexts;
        g_tp_duckdb_state.worker_local_contexts = nullptr;
    }

    void initialize_g_tp_duckdb_state()
    {
        if (g_tp_duckdb_state.did_initialize)
            return;
        traceprov_shared_context shared_context;
        if (map_traceprov_shared_context(&shared_context))
            elog(ERROR, "error maping shared context!");

        g_tp_duckdb_state.did_initialize = true;
        g_tp_duckdb_state.worker_local_contexts = traceprov_get_local_contexts(shared_context.worker_count);
    }

    bool traceprov_populate_bind_data(
        const uint32 worker_id,
        const uint32 layer_number,
        TraceProvBindData *bind_data,
        const bool expect_present)
    {
        bind_data->rel_args.worker_id = worker_id;
        bind_data->rel_args.layer_number = layer_number;

        // Need to figure out the number of columns and everything here.
        initialize_g_tp_duckdb_state();

        auto current_local_context = g_tp_duckdb_state.worker_local_contexts->at(worker_id - 1);
        auto current_layer = &current_local_context->cached_layers[layer_number - 1];

        // We should always crash here (because the top-level should have detected this case...)
        if (current_layer->layer_number != layer_number)
        {
            if (current_layer->layer_number != 0)
                elog(ERROR, "Invalid state!");

            if (expect_present)
            {
                elog(ERROR, "Expected the layer number to be filled");
            }
            return false;
        }
        if (current_layer->size == 0)
            return false;

        bind_data->col_layer_info = current_layer;
        bind_data->column_width = current_layer->num_pk_records;

        if (unlikely(current_layer->null_map_layer_number != 0))
        {
            bind_data->col_null_map_layer_info = &current_local_context->cached_layers[current_layer->null_map_layer_number - 1];
            if (bind_data->col_null_map_layer_info->layer_number != current_layer->null_map_layer_number)
                elog(ERROR, "Expected the layer number to be filled");
        }

        if (current_layer->rows_layer_number)
        {
            auto rows_layer = &current_local_context->cached_layers[current_layer->rows_layer_number - 1];
            if (rows_layer->layer_number != current_layer->rows_layer_number)
                elog(ERROR, "Expected the layer number to be filled");
            bind_data->row_layer_info = rows_layer;
            bind_data->row_width = rows_layer->num_pk_records;

            if (unlikely(rows_layer->null_map_layer_number != 0))
            {
                bind_data->row_null_map_layer_info = &current_local_context->cached_layers[rows_layer->null_map_layer_number - 1];
                if (bind_data->row_null_map_layer_info->layer_number != rows_layer->null_map_layer_number)
                    elog(ERROR, "Expected the layer number to be filled");
            }
        }
        bind_data->number_of_records = (uint64_t)get_final_ptr(NULL, current_layer) / TRACEPROV_GET_RECORD_SIZE(current_layer);
        if (current_layer->aggregate_strategy == AGG_SORTED){
            const void *final_ptr = get_final_ptr(NULL, bind_data->row_layer_info);
            bind_data->number_of_records = ((uint64)final_ptr) / TRACEPROV_GET_RECORD_SIZE(current_layer);
            const void *final_col_ptr = get_final_ptr(NULL, current_layer);
            bind_data->number_of_col_records = ((uint64)final_col_ptr) / TRACEPROV_GET_RECORD_SIZE(current_layer);
        }else{
            const void *final_ptr = get_final_ptr(NULL, current_layer);
            bind_data->number_of_records = ((uint64)final_ptr) / TRACEPROV_GET_RECORD_SIZE(current_layer);
        }
        return true;
    }

    void traceprov_populate_init_data(TraceProvBindData *bind_data, TraceProvInitData *init_data_inst)
    {
        init_data_inst->current = 0;
        const auto current_layer = bind_data->col_layer_info;
        map_layer_file(bind_data->rel_args.layer_number, bind_data->rel_args.worker_id, &init_data_inst->col_layer_ptr, current_layer->size);

        if (unlikely(current_layer->null_map))
        {
            map_layer_file(current_layer->null_map_layer_number, bind_data->rel_args.worker_id, &init_data_inst->col_null_layer_ptr, bind_data->col_null_map_layer_info->size);
        }

        if (bind_data->row_layer_info)
        {
            const auto rows_layer = bind_data->row_layer_info;
            map_layer_file(rows_layer->layer_number, bind_data->rel_args.worker_id, &init_data_inst->row_layer_ptr, rows_layer->size);
            if (unlikely(rows_layer->null_map))
            {
                map_layer_file(rows_layer->null_map_layer_number, bind_data->rel_args.worker_id, &init_data_inst->row_null_layer_ptr, bind_data->row_null_map_layer_info->size);
            }
        }

        if (current_layer->aggregate_strategy == AGG_SORTED)
        {
            if (bind_data->offset != -1)
            {
                elog(ERROR, "didn't expect offset version for this.");
            }
            const void *final_ptr = get_final_ptr(init_data_inst->row_layer_ptr, bind_data->row_layer_info);
            init_data_inst->number_of_records = ((uint64)final_ptr - (uint64)init_data_inst->row_layer_ptr) / TRACEPROV_GET_RECORD_SIZE(current_layer);
            const void *final_col_ptr = get_final_ptr(init_data_inst->col_layer_ptr, current_layer);
            init_data_inst->number_of_col_records = ((uint64)final_col_ptr - (uint64)init_data_inst->col_layer_ptr) / TRACEPROV_GET_RECORD_SIZE(current_layer);
        }
        else
        {
            const void *final_ptr = get_final_ptr(init_data_inst->col_layer_ptr, current_layer);
            init_data_inst->number_of_records = ((uint64)final_ptr - (uint64)init_data_inst->col_layer_ptr) / TRACEPROV_GET_RECORD_SIZE(current_layer);
        }
        if (bind_data->offset != -1)
        {
            // Need to adjust the read offset.
            init_data_inst->current = bind_data->offset;
            init_data_inst->col_layer_ptr = &((uint8_t *)init_data_inst->col_layer_ptr)[bind_data->offset * TRACEPROV_GET_RECORD_SIZE(current_layer)];
            if (bind_data->row_layer_info)
            {
                init_data_inst->row_layer_ptr = &((uint8_t *)init_data_inst->row_layer_ptr)[bind_data->offset * TRACEPROV_GET_RECORD_SIZE(bind_data->row_layer_info)];
            }
            // Limit the number of records.
            init_data_inst->number_of_records = init_data_inst->current + 1;
        }
    }

    TraceProvBindData *allocate_bind_data()
    {
        auto bind_data = (TraceProvBindData *)calloc(1, sizeof(TraceProvBindData));
        bind_data->child_bind_data = new std::vector<TraceProvBindData *>;
        bind_data->bind_data_mutex = new std::mutex;
        bind_data->offset = -1;
        return bind_data;
    }

    TraceProvInitData *allocate_init_data()
    {
        auto init_data = (TraceProvInitData *)calloc(1, sizeof(TraceProvInitData));
        init_data->child_init_data = new std::vector<TraceProvInitData *>;
        return init_data;
    }

    TraceProvBindData *traceprov_duckdb_bind_core(const TraceProvRelationArgs *core_rel_args, uint64_t &total_record_count){
        initialize_g_tp_duckdb_state();

        const auto current_worker_id = core_rel_args->worker_id;
        const auto layer_number = core_rel_args->layer_number;
        const auto offset = core_rel_args->offset;
        const auto table_flags = core_rel_args->table_flags;

        auto bind_data = allocate_bind_data();

        const uint32_t end_idx = current_worker_id == 0 ? g_tp_duckdb_state.worker_local_contexts->size() : current_worker_id;

        // Just means we need to do this for all the workers with this layer.
        for (uint32_t worker_id = current_worker_id == 0 ? current_worker_id : current_worker_id - 1; worker_id < end_idx; worker_id++)
        {
            auto child_bind_data = allocate_bind_data();
            if (traceprov_populate_bind_data(worker_id + 1, layer_number, child_bind_data, current_worker_id != 0))
            {
                bind_data->child_bind_data->push_back(child_bind_data);
                total_record_count += child_bind_data->number_of_records;
            }
            else
            {
                // Free-up the child bind data if we didn't populate it.
                free(child_bind_data);
            }
        }

        if (bind_data->child_bind_data->size() == 0)
        {
            elog(ERROR, "Expected at least 1 child bind data!");
        }

        auto child_bind_data = bind_data->child_bind_data->at(0);
        bind_data->rel_args = child_bind_data->rel_args;
        if (child_bind_data->col_layer_info->rows_layer_number)
        {
            // In this case, it is either aggregate or combine.
            if (child_bind_data->col_layer_info->flags & TRACEPROV_LAYER_COMBINE_FLAG)
            {
                bind_data->is_combine = true;
            }
            else
            {
                bind_data->is_aggregate = true;
            }
        }
        if (offset != -1)
        {
            if (bind_data->child_bind_data->size() != 1)
            {
                // TODO: Make this smart like DuckDB.
                elog(ERROR, "Expected only one child in this case, for now.");
            }
            bind_data->child_bind_data->at(0)->offset = offset;
            bind_data->child_bind_data->at(0)->number_of_records = offset + 1;
            bind_data->offset = offset;
            total_record_count = 1;
        }
        return bind_data;
    }

    void traceprov_duckdb_bind(duckdb_bind_info info)
    {
        if (duckdb_bind_get_parameter_count(info) != 2 && (duckdb_bind_get_parameter_count(info) != 3))
        {
            elog(ERROR, "Expected 2 or 3 params!");
        }
        initialize_g_tp_duckdb_state();
        auto param_1 = duckdb_bind_get_parameter(info, 0);
        const uint32_t current_worker_id = duckdb_get_int32(param_1);
        auto param_2 = duckdb_bind_get_parameter(info, 1);
        const uint32_t layer_number = duckdb_get_int32(param_2);
        int64_t offset = -1;
        if (duckdb_bind_get_parameter_count(info) == 3)
        {
            auto param_3 = duckdb_bind_get_parameter(info, 3);
            offset = duckdb_get_int64(param_3);
            duckdb_destroy_value(&param_3);
        } else if (g_tp_relation_infer_extra.log_offset)
        {
            if (g_tp_relation_infer_extra.log_offset->find(layer_number) != g_tp_relation_infer_extra.log_offset->end())
            {
                offset = g_tp_relation_infer_extra.log_offset->at(layer_number);
            }
        }

        duckdb_destroy_value(&param_1);
        duckdb_destroy_value(&param_2);

        TraceProvRelationArgs core_rel_args = {
            .worker_id = current_worker_id,
            .layer_number = layer_number,
            .table_flags = 0,
            .offset = offset
        };

        uint64_t total_record_count = 0;
        auto bind_data = traceprov_duckdb_bind_core(&core_rel_args, total_record_count);
        auto child_bind_data = bind_data->child_bind_data->at(0);
        duckdb_bind_set_bind_data(info, bind_data, free);
        for (uint64_t col_count = 0; col_count < child_bind_data->column_width + child_bind_data->row_width; col_count++)
        {
            const std::string param = std::string("column_") + std::to_string(col_count);
            duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
            duckdb_bind_add_result_column(info, param.c_str(), type);
            duckdb_destroy_logical_type(&type);
        }
        duckdb_bind_set_cardinality(info, total_record_count, true);
    }

    TraceProvInitData *traceprov_duckdb_init_core(TraceProvBindData *bind_data){
        auto init_data_inst = allocate_init_data();
        init_data_inst->child_init_data = new std::vector<TraceProvInitData *>;

        for (auto child_bind_data : *bind_data->child_bind_data)
        {
            auto child_data_inst = allocate_init_data();
            traceprov_populate_init_data(child_bind_data, child_data_inst);
            init_data_inst->child_init_data->push_back(child_data_inst);
        }
        return init_data_inst;
    }

    void traceprov_duckdb_init(duckdb_init_info info)
    {
        auto bind_data = (TraceProvBindData *)duckdb_init_get_bind_data(info);
        auto init_data_inst = traceprov_duckdb_init_core(bind_data);
        duckdb_init_set_init_data(info, init_data_inst, free);
        if (!traceprov_force_seq_scan)
            duckdb_init_set_max_threads(info, init_data_inst->child_init_data->size());
    }

    TraceProvInitData *traceprov_duckdb_local_init_core(
        TraceProvBindData *bind_data
    ){
        TraceProvInitData *init_data_inst = allocate_init_data();
        bind_data->bind_data_mutex->lock();
        const uint64_t self_idx = bind_data->max_worker_idx++;
        bind_data->bind_data_mutex->unlock();
        bool is_dummy = false;
        if (bind_data->rel_args.table_flags & TRACEPROV_TABLE_SEQ_SCAN || traceprov_force_seq_scan)
        {
            if (self_idx == 0)
            {
                for (auto child_bind_data : *bind_data->child_bind_data)
                {
                    auto child_data_inst = allocate_init_data();
                    traceprov_populate_init_data(child_bind_data, child_data_inst);
                    init_data_inst->child_init_data->push_back(child_data_inst);
                }
            }
            else
            {
                is_dummy = true;
            }
        }
        else
        {
            auto child_data_inst = allocate_init_data();
            traceprov_populate_init_data(bind_data->child_bind_data->at(self_idx), child_data_inst);
            init_data_inst->child_init_data->push_back(
                child_data_inst);
            init_data_inst->idx_in_bind = self_idx;
        }
        init_data_inst->is_dummy = is_dummy;
        return init_data_inst;
    }

    void traceprov_duckdb_local_init(duckdb_init_info info)
    {
        auto bind_data = (TraceProvBindData *)duckdb_init_get_bind_data(info);
        auto init_data_inst = traceprov_duckdb_local_init_core(bind_data);
        duckdb_init_set_init_data(info, init_data_inst, free);
    }

    uint64_t fillup_pointer_compressed(
        const struct traceprov_aggregate_layer *layer,
        duckdb_data_chunk chunk,
        void *source_ptr,
        uint64_t current_pos,
        const uint64_t final_num_records,
        const uint64_t final_col_records,
        TraceProvInitData *init_data,
        TraceProvBindData *bind_data)
    {
        const uint64_t original_pos = current_pos;
        for (uint64_t i = 0; i < STANDARD_VECTOR_SIZE; i++)
        {
            if (current_pos >= final_num_records)
                break;
            auto ptr = (uint64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(chunk, 0));
            ptr[i] = TRACEPROV_SET_WORKER_ID((init_data->current_col_idx + 1), bind_data->rel_args.worker_id);
            current_pos++;
            if (init_data->current_col_idx < final_col_records)
            {
                if (((uint64 *)source_ptr)[init_data->current_col_idx] <= current_pos)
                {
                    init_data->current_col_idx++;
                }
            }
        }
        duckdb_data_chunk_set_size(chunk, current_pos - original_pos);
        return current_pos;
    }

    uint64_t fillup_predicate(
        duckdb_data_chunk chunk,
        TraceProvInitData *state,
        TraceProvBindData *bind_data,
        const uint64_t filter_value,
        std::vector<uint64_t> *sel_vec
    ){
        const uint64_t original_pos = state->current;
        const uint64_t final_num_records = bind_data->number_of_records;
        const uint64_t *data_ptr = (uint64_t *)(state->col_layer_ptr);
        uint64_t sel_size = 0;
        uint64_t current_pos = original_pos;
        while (sel_size < STANDARD_VECTOR_SIZE){
            if (current_pos >= final_num_records) break;
            const auto value = data_ptr[current_pos];
            if (value == filter_value){
                sel_vec->push_back(current_pos);
                sel_size++;
            }
            current_pos++;
        }
        // While we are here, also populate the first column.
        // This is fine being done all at once here.
        // after all, it is a constant :)
        auto data_vec = (uint64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(chunk, 0));
        for (uint16_t idx = 0; idx < sel_size; idx++){
            data_vec[idx] = filter_value;
        }
        duckdb_data_chunk_set_size(chunk, sel_vec->size());
        return current_pos;
    }


    void fillup_pointer_selection(
        const struct traceprov_aggregate_layer *layer,
        duckdb_data_chunk chunk,
        const void *orig_source_ptr,
        const uint64 start_width,
        const uint64 total_width,
        const void *null_bit_vector,
        std::vector<uint64_t> *sel_vector
    ){
        const uint64_t record_size = TRACEPROV_GET_RECORD_SIZE(layer);
        for (uint16_t sel_idx = 0; sel_idx < sel_vector->size(); sel_idx++){
            const auto slice_idx = sel_vector->at(sel_idx);
            auto source_ptr = &(((uint8_t *)orig_source_ptr)[slice_idx*record_size + layer->record_padding]);
            uint64_t *canonical_ptr = (uint64_t*)source_ptr;
            for (uint64_t col_idx = 0; col_idx < total_width; col_idx++, canonical_ptr++){
                auto ptr = (uint64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(chunk, start_width + col_idx));
                ptr[sel_idx] = *canonical_ptr;
            }
            // Also populate the null-bit vector.
            if (unlikely(layer->null_map != 0))
            {
                const uint64_t null_bit_map = layer->null_map;
                const uint64_t null_bit_set = ((uint64_t *)null_bit_vector)[slice_idx];
                for (uint64_t col_idx = 0; col_idx < total_width; col_idx++)
                {
                    duckdb_vector col_vector = duckdb_data_chunk_get_vector(chunk, start_width + col_idx);
                    duckdb_vector_ensure_validity_writable(col_vector);
                    if ((null_bit_map & (((uint64_t)1) << col_idx)) != 0)
                    {
                        if (null_bit_set & (((uint64_t)1) << col_idx))
                        {
                            auto validity = duckdb_vector_get_validity(col_vector);
                            duckdb_validity_set_row_invalid(validity, sel_idx);
                        }
                    }
                }
            }
        }
    }
    uint64 fillup_pointer(
        const struct traceprov_aggregate_layer *layer,
        duckdb_data_chunk chunk,
        void *source_ptr,
        uint64_t current_pos,
        const uint64 final_num_records,
        const uint64 start_width,
        const uint64 total_width,
        void **final_ptr,
        const void *null_bit_vector,
        const bool use_filter=false,
        const uint64_t filter_value=0
    )
    {
        // elog(LOG, "Getting called with %ld", final_num_records);
        const uint64_t original_pos = current_pos;
        uint16_t new_size = 0;
        for (int64_t i = 0; i < STANDARD_VECTOR_SIZE; i++)
        {
            source_ptr = &((uint8_t *)source_ptr)[layer->record_padding];
            if (current_pos >= final_num_records)
                break;

            uint64 *canonical_ptr = (uint64 *)source_ptr;
            const bool is_valid = !use_filter || (canonical_ptr[0] == filter_value);
            if (is_valid){
                new_size++;
                for (uint64 col_idx = 0; col_idx < total_width; col_idx++, canonical_ptr++)
                {
                    auto ptr = (uint64_t *)duckdb_vector_get_data(duckdb_data_chunk_get_vector(chunk, start_width + col_idx));
                    ptr[i] = *canonical_ptr;
                }
                // Also populate the null-bit vector.
                if (unlikely(layer->null_map != 0))
                {
                    const uint64_t null_bit_map = layer->null_map;
                    const uint64_t null_bit_set = ((uint64_t *)null_bit_vector)[current_pos];
                    for (uint64_t col_idx = 0; col_idx < total_width; col_idx++)
                    {
                        duckdb_vector col_vector = duckdb_data_chunk_get_vector(chunk, start_width + col_idx);
                        duckdb_vector_ensure_validity_writable(col_vector);
                        if ((null_bit_map & (((uint64_t)1) << col_idx)) != 0)
                        {
                            if (null_bit_set & (((uint64_t)1) << col_idx))
                            {
                                auto validity = duckdb_vector_get_validity(col_vector);
                                duckdb_validity_set_row_invalid(validity, i);
                            }
                        }
                    }
                }
            }else{
                // Shift by -1.
                canonical_ptr += total_width;
                i--;
            }

            source_ptr = (void *)canonical_ptr;
            current_pos++;
        }

        duckdb_data_chunk_set_size(chunk, new_size);
        *final_ptr = source_ptr;
        return current_pos;
    }

    // main handler.
    // done this way so that the logic can be reused in places
    // where we bypass DuckDB.
    void traceprov_duckdb_func_core(
        TraceProvBindData *bind_data_combined,
        TraceProvInitData *init_data_combined,
        duckdb_data_chunk output
    ){
        // End of the scan.
        if ((init_data_combined->current_child_idx >= init_data_combined->child_init_data->size()) || (init_data_combined->is_dummy))
        {
            duckdb_data_chunk_set_size(output, 0);
            return;
        }

        uint32 bind_data_idx = init_data_combined->idx_in_bind;
        if (bind_data_combined->rel_args.table_flags & TRACEPROV_TABLE_SEQ_SCAN || traceprov_force_seq_scan)
        {
            bind_data_idx = init_data_combined->current_child_idx;
        }

        auto bind_data = bind_data_combined->child_bind_data->at(bind_data_idx);
        auto init_data = init_data_combined->child_init_data->at(init_data_combined->current_child_idx);

        uint64 final_state = 0;
        std::vector<uint64_t> *sel_vector = NULL;
        if (bind_data->col_layer_info->aggregate_strategy == AGG_SORTED)
        {
            final_state = fillup_pointer_compressed(
                bind_data->col_layer_info,
                output,
                init_data->col_layer_ptr,
                init_data->current,
                bind_data->number_of_records,
                bind_data->number_of_col_records,
                init_data,
                bind_data);
        }
        else
        {
            if (traceprov_use_filter_pushdown && bind_data_combined->filter_value_set){
                if (bind_data->row_layer_info != NULL){
                    if (init_data->scratch_space == NULL){
                        auto sel_vector = new std::vector<uint64_t>;
                        sel_vector->reserve(STANDARD_VECTOR_SIZE);
                        init_data->scratch_space = sel_vector;
                    };
                    sel_vector = (std::vector<uint64_t>*)init_data->scratch_space;
                    sel_vector->clear();
                    final_state = fillup_predicate(
                        output,
                        init_data,
                        bind_data,
                        bind_data_combined->filter_value,
                        sel_vector
                    );
                }else{
                    // In this case, need to filter and read in one shot, rather than bothering to separate it.
                    final_state = fillup_pointer(
                        bind_data->col_layer_info,
                        output,
                        init_data->col_layer_ptr,
                        init_data->current,
                        bind_data->number_of_records,
                        0,
                        bind_data->column_width,
                        &init_data->col_layer_ptr,
                        init_data->col_null_layer_ptr,
                        true,
                        bind_data_combined->filter_value
                    );
                }
            }else{
                final_state = fillup_pointer(
                    bind_data->col_layer_info,
                    output,
                    init_data->col_layer_ptr,
                    init_data->current,
                    bind_data->number_of_records,
                    0,
                    bind_data->column_width,
                    &init_data->col_layer_ptr,
                    init_data->col_null_layer_ptr
                );
            }
        }
        if (bind_data->row_layer_info)
        {
            if (traceprov_use_filter_pushdown && bind_data_combined->filter_value_set){
                fillup_pointer_selection(
                    bind_data->row_layer_info,
                    output,
                    init_data->row_layer_ptr,
                    bind_data->column_width,
                    bind_data->row_width,
                    init_data->row_null_layer_ptr,
                    sel_vector
                );
            }else{
                fillup_pointer(
                    bind_data->row_layer_info,
                    output,
                    init_data->row_layer_ptr,
                    init_data->current,
                    bind_data->number_of_records,
                    bind_data->column_width,
                    bind_data->row_width,
                    &init_data->row_layer_ptr,
                    init_data->row_null_layer_ptr
                );
            }
        }
        init_data->current = final_state;
        if (final_state >= bind_data->number_of_records)
        {
            // Switch to the next worker data if we've read through all for this layer.
            init_data_combined->current_child_idx++;
        }
        if (bind_data->offset != -1 || bind_data_combined->offset != -1)
        {
            // Make it a dummy so that the next scan yields 0 rows.
            init_data_combined->is_dummy |= true;
        }
    }

    void traceprov_duckdb_func(duckdb_function_info info, duckdb_data_chunk output)
    {

        auto bind_data_combined = (TraceProvBindData *)duckdb_function_get_bind_data(info);
        auto init_data_combined = (TraceProvInitData *)duckdb_function_get_local_init_data(info);
        traceprov_duckdb_func_core(bind_data_combined, init_data_combined, output);
    }

#define PG_DUCKDB_RUN_SHORT_QUERY(con, query, msg)              \
    {                                                           \
        duckdb_result result;                                   \
        elog(INFO, "QUERY: %s", query);                         \
        duckdb_state state = duckdb_query(con, query, &result); \
        PG_DUCKDB_EXIT_ON_ERROR_RESULT(state, result);          \
        duckdb_destroy_result(&result);                         \
        elog(INFO, "Reached: %s correctly", msg);               \
    }

    void traceprov_window_func(duckdb_function_info info, duckdb_data_chunk input, duckdb_vector output)
    {
        TraceProvWindowFuncExtra *window_extra = (TraceProvWindowFuncExtra *)duckdb_scalar_function_get_extra_info(info);
        if (unlikely(window_extra == NULL))
        {
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
        for (idx_t row_idx = 0; row_idx < row_count; row_idx++)
        {
            if (unlikely(frame_end_data[row_idx] < frame_start_data[row_idx]))
            {
                elog(ERROR, "Expected end to never be less than start!");
            }
            // Since a frame always includes the current row, also need to add 1.
            // This could be an upper bound in the cases where the worker is simply not present..
            expected_size += (frame_end_data[row_idx] - frame_start_data[row_idx]) + 1;
        }

        if (duckdb_list_vector_reserve(output, expected_size) == DuckDBError)
        {
            elog(ERROR, "Error reserving!");
        }

        if (duckdb_list_vector_set_size(output, expected_size) == DuckDBError)
        {
            elog(ERROR, "Error setting size!");
        }

        auto entries = (duckdb_list_entry *)duckdb_vector_get_data(output);

        duckdb_vector child_structs = duckdb_list_vector_get_child(output);
        uint64_t generic_idx = 0;
        for (uint64_t row_idx = 0; row_idx < row_count; row_idx++)
        {
            const uint32_t worker_id = worker_id_data[row_idx];
            const uint32_t layer_number = layer_number_data[row_idx];
            TraceProvWindowPack *window_pack = window_extra->key_bind_map->at(TRACEPROV_MAKE_WORKER_LAYER_KEY(worker_id, layer_number));

            const uint64_t frame_start = frame_start_data[row_idx];
            const uint64_t frame_end = frame_end_data[row_idx];
            const uint64_t generic_start_idx = generic_idx;
            for (uint64_t row_to_return = frame_start; row_to_return < (frame_end + 1); row_to_return++, generic_idx++)
            {
                const int64_t logged_row_idx = row_to_return - 1;
                if (unlikely(logged_row_idx < 0))
                    elog(ERROR, "any idx can never be < 0!");
                for (uint64_t col_idx = 0; col_idx < window_extra->num_cols; col_idx++)
                {
                    duckdb_vector member_data = duckdb_struct_vector_get_child(child_structs, col_idx);
                    uint64_t *child_data = (uint64_t *)duckdb_vector_get_data(member_data);
                    traceprov_aggregate_layer *curr_layer = window_pack->bind_data->row_layer_info;
                    ;
                    void *data_ptr = window_pack->init_data->row_layer_ptr;
                    child_data[generic_idx] = ((uint64_t *)(&(((uint8_t *)data_ptr)[(TRACEPROV_GET_RECORD_SIZE(curr_layer) * logged_row_idx) + (curr_layer->record_padding)])))[col_idx];
                }
            }
            const uint64_t generic_end_idx = generic_idx;
            // TODO: get rid of extra vars.
            // Helpful for readability ig.
            entries[row_idx].offset = generic_start_idx;
            entries[row_idx].length = generic_end_idx - generic_start_idx;
        }
    }

    static duckdb_scalar_function traceprov_create_read_window_func(
        const uint64_t num_args,
        const uint32_t worker_count,
        std::vector<uint32_t> *expected_layers)
    {
        duckdb_scalar_function func = duckdb_create_scalar_function();
        std::string *func_name = new std::string(("traceprov_read_window_" + std::to_string(num_args)).c_str());
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

        duckdb_logical_type *struct_member_types = (duckdb_logical_type *)malloc(sizeof(duckdb_logical_type) * num_args);
        const char **member_names = (const char **)malloc(sizeof(char *) * num_args);
        for (uint64_t col_idx = 0; col_idx < num_args; col_idx++)
        {
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
        window_extra->key_bind_map = new std::unordered_map<uint64_t, TraceProvWindowPack *>;

        for (uint32_t worker_id = 1; worker_id < worker_count + 1; worker_id++)
        {
            for (auto layer_number : *expected_layers)
            {
                const uint64_t worker_layer_key = TRACEPROV_MAKE_WORKER_LAYER_KEY(worker_id, layer_number);
                if (unlikely(window_extra->key_bind_map->find(worker_layer_key) != window_extra->key_bind_map->end()))
                {
                    elog(ERROR, "Expected the key to not be present!");
                }
                TraceProvBindData *bind_data = new TraceProvBindData;
                TraceProvInitData *init_data = new TraceProvInitData;
                memset(bind_data, 0, sizeof(TraceProvBindData));
                memset(init_data, 0, sizeof(TraceProvInitData));
                TraceProvWindowPack *window_pack = nullptr;
                // In the case where we dont' find it, it'll just be null.
                // This simplifies checking for it later, or at least makes the code more readable.
                if (traceprov_populate_bind_data(worker_id, layer_number, bind_data, false))
                {
                    window_pack = new TraceProvWindowPack;
                    window_pack->bind_data = bind_data;
                    window_pack->init_data = init_data;
                    traceprov_populate_init_data(bind_data, init_data);
                }
                window_extra->key_bind_map->insert({worker_layer_key, window_pack});
            }
        }
        window_extra->num_cols = num_args;
        // We don't care about the extra being "regenerated".
        // This is becuase the input layers are always disjoint.
        // So, the same worker+layer will never occur again, across the calls.
        duckdb_scalar_function_set_extra_info(func, window_extra, nullptr);
        return func;
    }

    void handle_pointer_stats(
        TraceProvStatistics *old_stats,
        const struct traceprov_aggregate_layer *aggregate_layer,
        const TraceProvRelationArgs rel_args)
    {
        const uint64_t min_value = TRACEPROV_SET_LAYER(TRACEPROV_SET_WORKER_ID((uint64_t)1, rel_args.worker_id), rel_args.layer_number);
        const uint64_t max_value = TRACEPROV_SET_LAYER(TRACEPROV_SET_WORKER_ID((aggregate_layer->num_groups), rel_args.worker_id), rel_args.layer_number);
        TraceProvStatistics stats = {
            .is_set = true,
            .min_value = min_value,
            .max_value = max_value};
        merge_stats(old_stats, &stats);
    }

    void bind_data_stats(TraceProvStatistics *stats, TraceProvBindData *bind_data, const bool is_aggregate, const int column_index)
    {
        if (is_aggregate && (column_index == 0))
        {
            handle_pointer_stats(stats, bind_data->col_layer_info, bind_data->rel_args);
        }
        else
        {
            merge_stats(stats, &bind_data->col_layer_info->stats[column_index]);
        }
    }

    bool traceprov_infer_stats(void *bind_data, const int column_idx, uint64_t *min_value, uint64_t *max_value, uint64_t *p_distinct_count)
    {
        // If not capturing stats, don't do anything.
        if (!traceprov_use_table_stats)
            return false;
        TraceProvStatistics stats = {
            .is_set = false,
            .min_value = 0,
            .max_value = 0};
        uint64_t distinct_count = 0;
        TraceProvBindData *tp_bind_data = (TraceProvBindData *)bind_data;
        if (tp_bind_data->col_layer_info)
        {
            bind_data_stats(&stats, tp_bind_data, tp_bind_data->is_aggregate, column_idx);
            if (column_idx == 0 && (tp_bind_data->is_aggregate || tp_bind_data->is_combine))
            {
                distinct_count += tp_bind_data->col_layer_info->num_groups;
            }
        }
        for (auto child_bind_data : *tp_bind_data->child_bind_data)
        {
            bind_data_stats(&stats, child_bind_data, tp_bind_data->is_aggregate, column_idx);
            if (column_idx == 0 && (tp_bind_data->is_aggregate || tp_bind_data->is_combine))
            {
                distinct_count += child_bind_data->col_layer_info->num_groups;
            }
        }
        if (stats.is_set)
        {
            elog(LOG, "Setting stats for (%d, %d) -- [%lu, %lu]. Distinct: %lu", (uint32_t)tp_bind_data->rel_args.layer_number, column_idx, stats.min_value, stats.max_value, distinct_count);
            *min_value = stats.min_value;
            *max_value = stats.max_value;
            if (distinct_count)
            {
                *p_distinct_count = distinct_count;
            }
        }
        return stats.is_set;
    }

    static duckdb_table_function setup_func()
    {
        auto function = duckdb_create_table_function();
        duckdb_table_function_set_name(function, "traceprov_read_worker_layer");
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_INTEGER);
        duckdb_table_function_add_parameter(function, type);
        duckdb_table_function_add_parameter(function, type);
        duckdb_destroy_logical_type(&type);

        duckdb_table_function_set_bind(function, traceprov_duckdb_bind);
        duckdb_table_function_set_init(function, traceprov_duckdb_init);
        duckdb_table_function_set_function(function, traceprov_duckdb_func);
        duckdb_table_function_set_local_init(function, traceprov_duckdb_local_init);
        traceprov_duckdb_table_set_stats_function(function, traceprov_infer_stats);
        return function;
    }

    static duckdb_table_function setup_offset_func()
    {
        auto function = setup_func();
        duckdb_table_function_set_name(function, "traceprov_read_worker_layer_offset");
        duckdb_logical_type type = duckdb_create_logical_type(DUCKDB_TYPE_BIGINT);
        duckdb_table_function_add_parameter(function, type);
        duckdb_destroy_logical_type(&type);
        return function;
    }

    static void populate_traceprov_data(
        TraceProvData *traceprov_data,
        duckdb_data_chunk *chunk)
    {
        const uint64 column_count = duckdb_data_chunk_get_column_count(*chunk);
        const uint64 row_count = duckdb_data_chunk_get_size(*chunk);
        // Unlikely because it'll happen just once, for the first chunk.
        if (unlikely(column_count != traceprov_data->size()))
        {
            if (traceprov_data->size() != 0)
            {
                elog(ERROR, "Attempting to set different number of column entries");
            }
            for (uint64 col_idx = 0; col_idx < column_count; col_idx++)
            {
                auto column_data = new TraceProvColumnData;
                column_data->data = new std::vector<uint64_t>();
                column_data->validity = new std::vector<bool>();
                traceprov_data->push_back(column_data);
            }
        }
        for (uint64 col_idx = 0; col_idx < column_count; col_idx++)
        {
            duckdb_vector col = duckdb_data_chunk_get_vector(*chunk, col_idx);
            uint64 *col_data = (uint64 *)duckdb_vector_get_data(col);
            uint64_t *col_validity = (uint64_t *)duckdb_vector_get_validity(col);
            auto current_column_data = traceprov_data->at(col_idx)->data;
            auto current_column_data_validity = traceprov_data->at(col_idx)->validity;
            current_column_data->reserve(row_count + current_column_data->size() + 50);
            current_column_data_validity->reserve(row_count + current_column_data_validity->size() + 50);
            for (uint64 row_idx = 0; row_idx < row_count; row_idx++)
            {
                if (col_validity != NULL)
                {
                    current_column_data_validity->push_back(duckdb_validity_row_is_valid(col_validity, row_idx));
                }
                else
                {
                    current_column_data_validity->push_back(true);
                }
                current_column_data->push_back(col_data[row_idx]);
            }
        }
    }

    void traceprov_duckdb_setup_context(
        struct traceprov_inference_context **p_ctxt,
        void (**p_cleanup)(struct traceprov_inference_context *),
        TraceProvInferSetupExtra *setup_extra)
    {
        struct traceprov_inference_context *context = (struct traceprov_inference_context *)malloc(sizeof(struct traceprov_inference_context));
        duckdb_database db;
        duckdb_connection con;
        char *error_msg;
        PG_DUCKDB_EXIT_ON_ERROR_MSG(duckdb_open_ext(":memory:", &db, nullptr, &error_msg), error_msg);
        PG_DUCKDB_EXIT_ON_ERROR(duckdb_connect(db, &con));
        context->db = db;
        context->con = con;
        *p_ctxt = context;

        for (auto entry : *setup_extra->size_layer_map)
        {
            duckdb_scalar_function read_window_func = traceprov_create_read_window_func(entry.first, setup_extra->worker_count, entry.second);
            PG_DUCKDB_EXIT_ON_ERROR(duckdb_register_scalar_function(con, read_window_func));
        }

        auto function = setup_func();
        PG_DUCKDB_EXIT_ON_ERROR(duckdb_register_table_function(con, function));
        // Use whatever the default, should be max anyways.
        // TODO: Make this configurable??
        // PG_DUCKDB_RUN_SHORT_QUERY(con, "SET threads=1;", "setting threads");
        if (traceprov_duckdb_profile_out && (strlen(traceprov_duckdb_profile_out) > 1)){
            char *profile_out = psprintf(TP_SET_PROFILE_OUTPUT, traceprov_duckdb_profile_out);
            PG_DUCKDB_RUN_SHORT_QUERY(con, TP_ENABLE_PROFILING, TP_ENABLE_PROFILING);
            // PG_DUCKDB_RUN_SHORT_QUERY(con, TP_ENABLE_DETAILED_PROFILING, TP_ENABLE_DETAILED_PROFILING);
            PG_DUCKDB_RUN_SHORT_QUERY(con, profile_out, profile_out);
        }
        *p_cleanup = traceprov_duckdb_cleanup;
    }

    // guts of all the inference.
    TraceProvData *traceprov_perform_duckdb_inference(
        const char *generated_sql,
        struct traceprov_inference_context *context)
    {
        duckdb_connection con = context->con;
        duckdb_prepared_statement stmt;
        duckdb_result final_result;
        std::string final_sql_str = std::string(generated_sql);
        // final_sql_str = "copy (" + final_sql_str + ") to '/tmp/temp.csv'";
        PG_DUCKDB_EXIT_ON_ERROR_MSG(duckdb_prepare(con, final_sql_str.c_str(), &stmt), duckdb_prepare_error(stmt));
        PG_DUCKDB_EXIT_ON_ERROR(duckdb_execute_prepared(stmt, &final_result));

        uint64_t total_chunk_count = duckdb_result_chunk_count(final_result);
        auto traceprov_data = new TraceProvData;
        for (idx_t chunk_idx = 0; chunk_idx < total_chunk_count; chunk_idx++)
        {
            duckdb_data_chunk data_chunk = duckdb_result_get_chunk(final_result, chunk_idx);
            populate_traceprov_data(traceprov_data, &data_chunk);
            duckdb_destroy_data_chunk(&data_chunk);
        }
        duckdb_destroy_result(&final_result);

        // PG_DUCKDB_RUN_SHORT_QUERY(con, generated_sql, "inference query");
        return traceprov_data;
    }

    // Technically, traceprov_perform_duckdb_inference can be generalized (a "handler" function can be passed)
    // But, that'll have its own overhead (a new function call).
    // Most of the stuff copied from that function is trivial anyways.
    TraceProvInferResult traceprov_perform_duckdb_inference_pg_copy(
        const char *generated_sql,
        struct traceprov_inference_context *context,
        FunctionCallInfo fcinfo,
        const uint32 expected_col_width)
    {
        duckdb_connection con = context->con;
        duckdb_prepared_statement stmt;
        duckdb_result final_result;
        TupleDesc tupdesc = NULL;
        Tuplestorestate *tupstore = NULL;

        TP_EVALUATE_START();

        PG_DUCKDB_EXIT_ON_ERROR_MSG(duckdb_prepare(con, generated_sql, &stmt), duckdb_prepare_error(stmt));
        PG_DUCKDB_EXIT_ON_ERROR(duckdb_execute_prepared(stmt, &final_result));

        uint64_t total_chunk_count = duckdb_result_chunk_count(final_result);

        Datum *record = palloc0_array(Datum, expected_col_width);
        bool *nulls = palloc0_array(bool, expected_col_width);

        uint64_t total_row_count = 0;

        for (idx_t chunk_idx = 0; chunk_idx < total_chunk_count; chunk_idx++)
        {
            duckdb_data_chunk data_chunk = duckdb_result_get_chunk(final_result, chunk_idx);
            if (DEBUG_MODE)
            {
                const uint32 column_count = duckdb_data_chunk_get_column_count(data_chunk);
                if (column_count != expected_col_width)
                    elog(ERROR, "Got mismatching col counts: %d, %d", column_count, expected_col_width);
            }

            const uint64_t row_count = duckdb_data_chunk_get_size(data_chunk);
            total_row_count += row_count;

            ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;

            if (rsinfo->returnMode == SFRM_Materialize)
            {

                tupdesc = rsinfo->setDesc;
                tupstore = rsinfo->setResult;

                for (uint64_t row_idx = 0; row_idx < row_count; row_idx++)
                {
                    memset(nulls, 0, sizeof(bool) * expected_col_width);
                    for (uint32 col_idx = 0; col_idx < expected_col_width; col_idx++)
                    {
                        duckdb_vector col = duckdb_data_chunk_get_vector(data_chunk, col_idx);
                        uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col);
                        uint64_t *col_validity = (uint64_t *)duckdb_vector_get_validity(col);
                        record[col_idx] = col_data[row_idx];
                        if (col_validity)
                            nulls[col_idx] = !duckdb_validity_row_is_valid(col_validity, row_idx);
                    }
                    // Directly store the value in Postgres rather than storing them
                    // in an intermediate step.
                    tuplestore_putvalues(tupstore, tupdesc, record, nulls);
                }
            }
            duckdb_destroy_data_chunk(&data_chunk);
        }
        duckdb_destroy_result(&final_result);
#if (PG_MAJORVERSION_NUM != 18)
        if (tupstore)
        {
            tuplestore_donestoring(tupstore);
        }
#endif

        TP_EVALUATE_END();
        const uint64_t duration = TP_EVALUATE_DURATION();

        return TraceProvInferResult{
            .width = expected_col_width,
            .time = duration,
            .row_count = total_row_count,
        };
    }

    void static read_at_offset(
        const uint64_t log_offset,
        TraceProvBindData *bind_data,
        // Values are written here.
        uint64_t **values
    ){
        auto init_data = allocate_init_data();
        traceprov_populate_init_data(bind_data, init_data);
        const uint64_t total_record_width = bind_data->row_width + bind_data->column_width;
        uint64_t *value_buff = (uint64_t*)malloc(sizeof(uint64)*total_record_width);
        if (bind_data->column_width){
            void *col_data_ptr = init_data->col_layer_ptr;
            for (uint32_t idx = 0; idx < bind_data->column_width; idx++){
                auto value = ((uint64_t *)(&(((uint8_t *)col_data_ptr)[(TRACEPROV_GET_RECORD_SIZE(bind_data->col_layer_info) * log_offset) + (bind_data->col_layer_info->record_padding)])))[idx];
                value_buff[idx] = value;
            }
        }
        if (bind_data->row_width){
            void *row_layer_ptr = init_data->row_layer_ptr;
            for (uint32_t idx = 0; idx < bind_data->row_width; idx++){
                auto value = ((uint64_t *)(&(((uint8_t *)row_layer_ptr)[(TRACEPROV_GET_RECORD_SIZE(bind_data->row_layer_info) * log_offset) + (bind_data->row_layer_info->record_padding)])))[idx];
                value_buff[idx + bind_data->column_width] = value;
            }
        }
        free(init_data);
        *values = value_buff;
    }


    void *traceprov_get_row(
        const uint64_t layer_number,
        const uint64_t log_probe_value
    ){
        auto child_bind_data = allocate_bind_data();
        g_tp_duckdb_state.worker_local_contexts;
        auto layer_workers = find_layers_across_workers(layer_number, g_tp_duckdb_state.worker_local_contexts, 0, false);
        if (layer_workers->size() != 1){
            elog(ERROR, "Got unexpected size: %ld", layer_workers->size());
        }
        traceprov_populate_bind_data(layer_workers->at(0).first, layer_number, child_bind_data, true);
        uint64_t *values = NULL;
        read_at_offset(log_probe_value, child_bind_data, &values);
        return values;
    }

    void traceprov_prepare_foldable(
        TraceProvRelationInferExtraItem *item,
        TraceProvBindData **bind_data_core,
        TraceProvInitData **init_data_core,
        TraceProvInitData **local_init_data
    ){
        uint64_t total_record_count = 0;
        auto bind_data = traceprov_duckdb_bind_core(item->rel_args, total_record_count);
        if (item->use_filter_value){
            bind_data->filter_value = item->filter_value;
            bind_data->filter_value_set = true;
        }
        *init_data_core = traceprov_duckdb_init_core(bind_data);
        *local_init_data = traceprov_duckdb_init_core(bind_data);
        *bind_data_core = bind_data;
    }

}
