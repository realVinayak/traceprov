/**
 * Custom type that's used in traceprov.
 * Most of the time, bigint is enough. But, in some cases (set difference), need to be able to
 * do this casting.
 */

#include "postgres.h"
#include "fmgr.h"
#include "libpq/pqformat.h"

#define LOG_LOCATION() elog(INFO, "at: %s", __func__);

PG_FUNCTION_INFO_V1(traceprov_ptr_type_in);

Datum 
traceprov_ptr_type_in(PG_FUNCTION_ARGS)
{
    char *str = PG_GETARG_CSTRING(0);
    uint64 input_ptr;
    if ((sscanf(str, "(%ld)", &input_ptr)) != 1) elog(ERROR, "Got invalid str: %s", str);
    PG_RETURN_DATUM(input_ptr);
}

PG_FUNCTION_INFO_V1(traceprov_ptr_type_out);

Datum
traceprov_ptr_type_out(PG_FUNCTION_ARGS)
{
    const int64 ptr = PG_GETARG_INT64(0);
    char *result;
    result = psprintf("(%ld)", ptr);
    PG_RETURN_CSTRING(result);
}

PG_FUNCTION_INFO_V1(traceprov_ptr_type_recv);

Datum
traceprov_ptr_type_recv(PG_FUNCTION_ARGS)
{
    StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
    int64 ptr = pq_getmsgint64(buf);
    PG_RETURN_POINTER((void*)ptr);
}

PG_FUNCTION_INFO_V1(traceprov_ptr_type_send);

Datum
traceprov_ptr_type_send(PG_FUNCTION_ARGS)
{
    int64 ptr = PG_GETARG_INT64(0);
    StringInfoData buf;
    pq_begintypsend(&buf);
    pq_sendint64(&buf, ptr);
    PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}


// For comparison functions, all the operations that don't involve equality are false.
// So, <, >, != are all false.
// On the other hand, >=, <=, = are all true.

// All the below are false.
PG_FUNCTION_INFO_V1(traceprov_ptr_type_lt);

Datum
traceprov_ptr_type_lt(PG_FUNCTION_ARGS)
{
    LOG_LOCATION();
    PG_RETURN_BOOL(false);
}

PG_FUNCTION_INFO_V1(traceprov_ptr_type_gt);

Datum
traceprov_ptr_type_gt(PG_FUNCTION_ARGS)
{
    LOG_LOCATION();
    PG_RETURN_BOOL(false);
}

// All the below are true.
PG_FUNCTION_INFO_V1(traceprov_ptr_type_le);

Datum
traceprov_ptr_type_le(PG_FUNCTION_ARGS)
{
    LOG_LOCATION();
    PG_RETURN_BOOL(true);
}

PG_FUNCTION_INFO_V1(traceprov_ptr_type_eq);

Datum
traceprov_ptr_type_eq(PG_FUNCTION_ARGS)
{
    LOG_LOCATION();
    PG_RETURN_BOOL(true);
}

PG_FUNCTION_INFO_V1(traceprov_ptr_type_ge);

Datum
traceprov_ptr_type_ge(PG_FUNCTION_ARGS)
{
    LOG_LOCATION();
    PG_RETURN_BOOL(true);
}

PG_FUNCTION_INFO_V1(traceprov_ptr_type_cmp);

Datum
traceprov_ptr_type_cmp(PG_FUNCTION_ARGS)
{
    LOG_LOCATION();
    // Always return 0 (meaning, equal.)
    PG_RETURN_INT32(0);
}