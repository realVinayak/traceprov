// Because Postgres' SRF aren't _that_ good.

#include "traceprov_infer_essentials.hpp"
#include "traceprov_ext_utils.hpp"
#include "traceprov_node.hpp"
#include "traceprov_infer.hpp"

extern "C"
{
#include "postgres.h"
#include "fmgr.h"
#include "access/tableam.h"
#include "access/heapam.h"
#include "nodes/execnodes.h"
#include "catalog/index.h"
#include "commands/vacuum.h"
#include "utils/builtins.h"
#include "executor/tuptable.h"
#include "duckdb.h"
#include "traceprov.h"
#include "traceprov_parse_context.h"

    // This gets used to determine which SQL to run.
    TraceProvRelationInferExtra g_tp_relation_infer_extra = {
        .map = NULL,
        .log_offset = NULL};

// #define TP_TAP_LOCATION elog(INFO, "Got here: %s, %d", __FILE__, __LINE__)
#define TP_TAP_LOCATION 0
    static TraceProvLayerNumber get_layer_from_rel(Relation rel);

    static const TupleTableSlotOps *tp_am_slot_callbacks(
        Relation relation)
    {
        TP_TAP_LOCATION;
        return &TTSOpsVirtual;
    }

    struct TraceProvScanDescData
    {
        // Base class from access/relscan.h.
        TableScanDescData rs_base;

        duckdb_result final_result;
        duckdb_prepared_statement stmt;

        // The current chunk.
        duckdb_data_chunk current_chunk;
        // The index in current chunk
        idx_t current_idx_chunk;
        // The end of the chunk. (cached.)
        idx_t end_idx_chunk;
        TraceProvBindData *bind_data;
        TraceProvInitData *init_data;
        TraceProvInitData *local_init_data;
        TraceProvInferType scan_tag;
        // Some values that need to be artificially inserted.
        uint64_t *foldable_values;
        uint64_t foldable_value_count;
    };

    
    static TableScanDesc tp_am_beginscan(
        Relation relation,
        Snapshot snapshot,
        int nkeys,
        struct ScanKeyData *key,
        ParallelTableScanDesc parallel_scan,
        uint32 flags)
    {
        TraceProvScanDescData *scan;
        TP_TAP_LOCATION;
        scan = (TraceProvScanDescData *)malloc(sizeof(TraceProvScanDescData));
        memset(scan, 0, sizeof(TraceProvScanDescData));
        scan->rs_base.rs_rd = relation;
        scan->rs_base.rs_snapshot = snapshot;
        scan->rs_base.rs_nkeys = nkeys;
        scan->rs_base.rs_flags = flags;
        scan->rs_base.rs_parallel = parallel_scan;
        const TraceProvLayerNumber relation_layer = get_layer_from_rel(relation);
        if (g_tp_relation_infer_extra.map == NULL)
        {
            elog(ERROR, "Expected map to be set!");
        }
        // Figure out the query to execute, and run it :)
        if (g_tp_relation_infer_extra.map->find(relation_layer) == g_tp_relation_infer_extra.map->end())
        {
            elog(ERROR, "Expected the rel to be found!");
        }
        auto relation_item = g_tp_relation_infer_extra.map->at(relation_layer);
        if (relation_item.tag == TraceProvInferType::FOLDABLE){
            // In this case, we don't use DuckDB because the table is going to be a simple scan.
            // Instead, we hold three pointers: TraceProvBindData, TraceProvInitData, TraceProvLocalInitData
            // During the scan, we just iterate till we get no rows.
            // Basically, the only thing that changes between this and SQL version is the method to get the next chunk.
            // TODO: Implement this via callbacks? Not done right now for performance, but maybe that won't be too bad..
            traceprov_prepare_foldable(&relation_item, &scan->bind_data, &scan->init_data, &scan->local_init_data);
            auto col_logical_type = duckdb_create_logical_type(DUCKDB_TYPE_UBIGINT);
            if (relation_item.expected_col_width <= relation_item.foldable_value_count){
                elog(ERROR, "Expected some gap between total column width and the foldable value count!");
            }
            const uint64_t canonical_chunk_width = relation_item.expected_col_width - relation_item.foldable_value_count;
            auto col_types = (duckdb_logical_type*)malloc(sizeof(duckdb_logical_type)*canonical_chunk_width);
            for (uint32_t idx = 0; idx < canonical_chunk_width; idx++){
                col_types[idx] = col_logical_type;
            }
            auto data_chunk = duckdb_create_data_chunk(col_types, canonical_chunk_width);
            duckdb_destroy_logical_type(&col_logical_type);
            free(col_types);
            scan->current_chunk = data_chunk;
        }else if (relation_item.tag == TraceProvInferType::SQL){
            duckdb_connection con = traceprov_current.infer_context->con;
            {
                TP_EVALUATE_START();
                PG_DUCKDB_EXIT_ON_ERROR_MSG(duckdb_prepare(con, relation_item.sql, &scan->stmt), duckdb_prepare_error(scan->stmt));
                TP_EVALUATE_END();
                elog(INFO, "Prepare timing: %ld", TP_EVALUATE_DURATION());
            }
            duckdb_pending_result result;
            duckdb_result final_result;
            PG_DUCKDB_EXIT_ON_ERROR(duckdb_pending_prepared_streaming(scan->stmt, &result));
            PG_DUCKDB_EXIT_ON_ERROR(duckdb_execute_pending(
                result,
                &final_result));
            if (!duckdb_result_is_streaming(final_result))
            {
                elog(ERROR, "Expected the final result to be pending!");
            }
            scan->final_result = final_result;
        }else{
            elog(ERROR, "Got unexpected type: %ld", relation_item.tag);
        }
        scan->foldable_value_count = relation_item.foldable_value_count;
        scan->foldable_values = relation_item.foldable_values;
        scan->scan_tag = relation_item.tag;

        return (TableScanDesc)scan;
    }

    static void tp_am_rescan(
        TableScanDesc sscan,
        struct ScanKeyData *key,
        bool set_params,
        bool allow_strat,
        bool allow_sync,
        bool allow_pagemode)
    {
        TP_TAP_LOCATION;
    }

    static void tp_am_endscan(TableScanDesc sscan)
    {
        TP_TAP_LOCATION;
        TraceProvScanDescData *scan_desc = (TraceProvScanDescData *)sscan;
        if (scan_desc->current_chunk)
        {
            duckdb_destroy_data_chunk(&scan_desc->current_chunk);
        }
        duckdb_destroy_result(&scan_desc->final_result);
        if (scan_desc->stmt)
        {
            duckdb_destroy_prepare(&scan_desc->stmt);
        }
        free(sscan);
    }

    static bool tp_am_getnextslot(
        TableScanDesc sscan,
        ScanDirection direction,
        TupleTableSlot *slot)
    {

        TraceProvScanDescData *scan_desc = (TraceProvScanDescData *)sscan;
        if (scan_desc->current_chunk == NULL || (scan_desc->end_idx_chunk == scan_desc->current_idx_chunk))
        {
            if (scan_desc->current_chunk != NULL)
            {
                if (scan_desc->scan_tag == TraceProvInferType::FOLDABLE){
                    duckdb_data_chunk_reset(scan_desc->current_chunk);
                }else{
                    duckdb_destroy_data_chunk(&scan_desc->current_chunk);
                }
            }
            duckdb_data_chunk data_chunk = nullptr;
            // Right of the bat, execute the query.
            if (scan_desc->scan_tag == TraceProvInferType::SQL){
                data_chunk = duckdb_stream_fetch_chunk(scan_desc->final_result);
                if (!data_chunk)
                {
                    return false;
                }
                scan_desc->current_chunk = data_chunk;
            }else{
                // Run it via the callback func.
                traceprov_duckdb_func_core(scan_desc->bind_data, scan_desc->local_init_data, scan_desc->current_chunk);
                if ( duckdb_data_chunk_get_size(scan_desc->current_chunk) == 0 ) return false;
                data_chunk = scan_desc->current_chunk;
            }
            scan_desc->current_idx_chunk = 0;
            scan_desc->end_idx_chunk = duckdb_data_chunk_get_size(data_chunk);
        }
        auto curr_chunk = scan_desc->current_chunk;

        ExecClearTuple(slot);

        const idx_t column_count = duckdb_data_chunk_get_column_count(curr_chunk);

        auto row_idx = scan_desc->current_idx_chunk;
        for (idx_t col_idx = 0; col_idx < column_count; col_idx++)
        {
            duckdb_vector col = duckdb_data_chunk_get_vector(curr_chunk, col_idx);
            const uint64_t *col_data = (uint64_t *)duckdb_vector_get_data(col);
            const uint64_t true_column_index = col_idx + scan_desc->foldable_value_count;
            slot->tts_values[true_column_index] = Int64GetDatum(col_data[row_idx]);
            uint64_t *col_validity = (uint64_t *)duckdb_vector_get_validity(col);
            if (col_validity)
                slot->tts_isnull[true_column_index] = !duckdb_validity_row_is_valid(col_validity, row_idx);
        }
        if (scan_desc->foldable_value_count){
            if (scan_desc->foldable_values == NULL){
                elog(ERROR, "Expected foldable values to be set!");
            }
            // Need to insert the foldable const values now.
            for (idx_t col_idx = 0; col_idx < scan_desc->foldable_value_count; col_idx++){
                slot->tts_values[col_idx] = Int64GetDatum(scan_desc->foldable_values[col_idx]);
            }
        }
        scan_desc->current_idx_chunk++;
        ExecStoreVirtualTuple(slot);
        return true;
    }

    static IndexFetchTableData *tp_am_index_fetch_begin(Relation rel)
    {
        TP_TAP_LOCATION;
        return NULL;
    }

    static void tp_am_index_fetch_reset(IndexFetchTableData *scan) {}

    static void tp_am_index_fetch_end(IndexFetchTableData *scan) {}

    static bool tp_am_index_fetch_tuple(
        struct IndexFetchTableData *scan,
        ItemPointer tid,
        Snapshot snapshot,
        TupleTableSlot *slot,
        bool *call_again,
        bool *all_dead)
    {
        TP_TAP_LOCATION;
        return false;
    }

    static void tp_am_tuple_insert(
        Relation relation,
        TupleTableSlot *slot,
        CommandId cid,
        int options,
        BulkInsertState bistate)
    {
        elog(INFO, "Did not expect insert to be called!");
    }

    static void tp_am_tuple_insert_speculative(
        Relation relation,
        TupleTableSlot *slot,
        CommandId cid,
        int options,
        BulkInsertState bistate,
        uint32 specToken)
    {
        TP_TAP_LOCATION;
    }

    static void tp_am_tuple_complete_speculative(
        Relation relation,
        TupleTableSlot *slot,
        uint32 specToken,
        bool succeeded)
    {
        TP_TAP_LOCATION;
    }

    static void tp_am_multi_insert(
        Relation relation,
        TupleTableSlot **slots,
        int ntuples,
        CommandId cid,
        int options,
        BulkInsertState bistate)
    {
        TP_TAP_LOCATION;
    }

    static TM_Result tp_am_tuple_delete(
        Relation relation,
        ItemPointer tid,
        CommandId cid,
        Snapshot snapshot,
        Snapshot crosscheck,
        bool wait,
        TM_FailureData *tmfd,
        bool changingPart)
    {
        TM_Result result;
        TP_TAP_LOCATION;
        return result;
    }

    static TM_Result tp_am_tuple_update(
        Relation relation,
        ItemPointer otid,
        TupleTableSlot *slot,
        CommandId cid,
        Snapshot snapshot,
        Snapshot crosscheck,
        bool wait,
        TM_FailureData *tmfd,
        LockTupleMode *lockmode,
        TU_UpdateIndexes *update_indexes)
    {
        TM_Result result;
        TP_TAP_LOCATION;
        return result;
    }

    static TM_Result tp_am_tuple_lock(
        Relation relation,
        ItemPointer tid,
        Snapshot snapshot,
        TupleTableSlot *slot,
        CommandId cid,
        LockTupleMode mode,
        LockWaitPolicy wait_policy,
        uint8 flags,
        TM_FailureData *tmfd)
    {
        TM_Result result;
        TP_TAP_LOCATION;
        return result;
    }

    static bool tp_am_fetch_row_version(
        Relation relation,
        ItemPointer tid,
        Snapshot snapshot,
        TupleTableSlot *slot)
    {
        TP_TAP_LOCATION;
        return false;
    }

    static void tp_am_get_latest_tid(
        TableScanDesc sscan,
        ItemPointer tid)
    {
        TP_TAP_LOCATION;
    }

    static bool tp_am_tuple_tid_valid(TableScanDesc scan, ItemPointer tid)
    {
        TP_TAP_LOCATION;
        return false;
    }

    static bool tp_am_tuple_satisfies_snapshot(
        Relation rel,
        TupleTableSlot *slot,
        Snapshot snapshot)
    {
        TP_TAP_LOCATION;
        return false;
    }

    static TraceProvLayerNumber get_layer_from_rel(Relation rel)
    {
        TraceProvLayerNumber layer = 0;
        auto table_name = NameStr(rel->rd_rel->relname);
        sscanf(table_name, TRACEPROV_RELATION_INFER_NAME, &layer);
        if (layer == 0)
        {
            elog(ERROR, "Expected the layer to be set in name!");
        }
        return layer;
    }

    static TransactionId tp_am_index_delete_tuples(
        Relation rel,
        TM_IndexDeleteOp *delstate)
    {
        TransactionId id = 0;
        TP_TAP_LOCATION;
        return id;
    }

    static void tp_am_relation_set_new_filelocator(
        Relation rel,
        const RelFileLocator *newrlocator,
        char persistence,
        TransactionId *freezeXid,
        MultiXactId *minmulti)
    {
        get_layer_from_rel(rel);
    }

    static void tp_am_relation_nontransactional_truncate(
        Relation rel)
    {
        TP_TAP_LOCATION;
    }

    static void tp_am_relation_copy_data(
        Relation rel,
        const RelFileLocator *newrlocator)
    {
        TP_TAP_LOCATION;
    }

    static void tp_am_relation_copy_for_cluster(
        Relation OldHeap,
        Relation NewHeap,
        Relation OldIndex,
        bool use_sort,
        TransactionId OldestXmin,
        TransactionId *xid_cutoff,
        MultiXactId *multi_cutoff,
        double *num_tuples,
        double *tups_vacuumed,
        double *tups_recently_dead)
    {
        TP_TAP_LOCATION;
    }

    static void tp_am_vacuum_rel(
        Relation rel,
        VacuumParams *params,
        BufferAccessStrategy bstrategy)
    {
        TP_TAP_LOCATION;
    }

    static bool tp_am_scan_analyze_next_block(
        TableScanDesc scan,
        BlockNumber blockno,
        BufferAccessStrategy bstrategy)
    {
        TP_TAP_LOCATION;
        return false;
    }

    static bool tp_am_scan_analyze_next_tuple(
        TableScanDesc scan,
        TransactionId OldestXmin,
        double *liverows,
        double *deadrows,
        TupleTableSlot *slot)
    {
        TP_TAP_LOCATION;
        return false;
    }

    static double tp_am_index_build_range_scan(
        Relation heapRelation,
        Relation indexRelation,
        IndexInfo *indexInfo,
        bool allow_sync,
        bool anyvisible,
        bool progress,
        BlockNumber start_blockno,
        BlockNumber numblocks,
        IndexBuildCallback callback,
        void *callback_state,
        TableScanDesc scan)
    {
        TP_TAP_LOCATION;
        return 0;
    }

    static void tp_am_index_validate_scan(
        Relation heapRelation,
        Relation indexRelation,
        IndexInfo *indexInfo,
        Snapshot snapshot,
        ValidateIndexState *state)
    {
        TP_TAP_LOCATION;
    }

    static bool tp_am_relation_needs_toast_table(Relation rel)
    {
        TP_TAP_LOCATION;
        return false;
    }

    static Oid tp_am_relation_toast_am(Relation rel)
    {
        Oid oid = 0;
        TP_TAP_LOCATION;
        return oid;
    }

    static void tp_am_fetch_toast_slice(
        Relation toastrel,
        Oid valueid,
        int32 attrsize,
        int32 sliceoffset,
        int32 slicelength,
        struct varlena *result)
    {
        TP_TAP_LOCATION;
    }

    static void tp_am_estimate_rel_size(
        Relation rel,
        int32 *attr_widths,
        BlockNumber *pages,
        double *tuples,
        double *allvisfrac)
    {
        TP_TAP_LOCATION;
    }

    static bool tp_am_scan_sample_next_block(
        TableScanDesc scan, SampleScanState *scanstate)
    {
        TP_TAP_LOCATION;
        return false;
    }

    static bool tp_am_scan_sample_next_tuple(
        TableScanDesc scan,
        SampleScanState *scanstate,
        TupleTableSlot *slot)
    {
        TP_TAP_LOCATION;
        return false;
    }

    const TableAmRoutine tp_am_methods = {
        .type = T_TableAmRoutine,

        .slot_callbacks = tp_am_slot_callbacks,

        .scan_begin = tp_am_beginscan,
        .scan_end = tp_am_endscan,
        .scan_rescan = tp_am_rescan,
        .scan_getnextslot = tp_am_getnextslot,

        .parallelscan_estimate = table_block_parallelscan_estimate,
        .parallelscan_initialize = table_block_parallelscan_initialize,
        .parallelscan_reinitialize = table_block_parallelscan_reinitialize,

        .index_fetch_begin = tp_am_index_fetch_begin,
        .index_fetch_reset = tp_am_index_fetch_reset,
        .index_fetch_end = tp_am_index_fetch_end,
        .index_fetch_tuple = tp_am_index_fetch_tuple,

        .tuple_fetch_row_version = tp_am_fetch_row_version,
        .tuple_tid_valid = tp_am_tuple_tid_valid,
        .tuple_get_latest_tid = tp_am_get_latest_tid,
        .tuple_satisfies_snapshot = tp_am_tuple_satisfies_snapshot,
        .index_delete_tuples = tp_am_index_delete_tuples,

        .tuple_insert = tp_am_tuple_insert,
        .tuple_insert_speculative = tp_am_tuple_insert_speculative,
        .tuple_complete_speculative = tp_am_tuple_complete_speculative,
        .multi_insert = tp_am_multi_insert,
        .tuple_delete = tp_am_tuple_delete,
        .tuple_update = tp_am_tuple_update,
        .tuple_lock = tp_am_tuple_lock,

        .relation_set_new_filelocator = tp_am_relation_set_new_filelocator,
        .relation_nontransactional_truncate = tp_am_relation_nontransactional_truncate,
        .relation_copy_data = tp_am_relation_copy_data,
        .relation_copy_for_cluster = tp_am_relation_copy_for_cluster,
        .relation_vacuum = tp_am_vacuum_rel,
        .scan_analyze_next_block = tp_am_scan_analyze_next_block,
        .scan_analyze_next_tuple = tp_am_scan_analyze_next_tuple,
        .index_build_range_scan = tp_am_index_build_range_scan,
        .index_validate_scan = tp_am_index_validate_scan,

        .relation_size = table_block_relation_size,
        .relation_needs_toast_table = tp_am_relation_needs_toast_table,
        .relation_toast_am = tp_am_relation_toast_am,
        .relation_fetch_toast_slice = tp_am_fetch_toast_slice,

        .relation_estimate_size = tp_am_estimate_rel_size,

        .scan_sample_next_block = tp_am_scan_sample_next_block,
        .scan_sample_next_tuple = tp_am_scan_sample_next_tuple};

    PG_FUNCTION_INFO_V1(traceprov_tableam_handler);

    Datum traceprov_tableam_handler(PG_FUNCTION_ARGS)
    {
        PG_RETURN_POINTER(&tp_am_methods);
    }
}