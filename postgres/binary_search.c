#include "postgres.h"
#include "fmgr.h"
#include "utils/array.h"

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(binary_search);
PG_FUNCTION_INFO_V1(binary_search_value);

int64 binary_search_last(
    int64 *input_array, 
    int start, 
    int end, 
    const int value
){
    if (start > end){
        return end;
    }

    int mid = (start + end) / 2;

    if (input_array[mid] == value){
        return mid;
    }

    if (value < input_array[mid]){
        return binary_search_last(input_array, start, mid - 1, value);
    }else{
        return binary_search_last(input_array, mid + 1, end, value);
    }
}

Datum binary_search(PG_FUNCTION_ARGS){

    ArrayType *array = PG_GETARG_ARRAYTYPE_P(0);
    int64 search_value = PG_GETARG_INT64(1);
    int64 *data =  (int64*)ARR_DATA_PTR(array);

    int64 num_elements = ARR_DIMS(array)[0];
    PG_RETURN_INT64(binary_search_last(data, 0, num_elements - 1, search_value));
}

Datum binary_search_value(PG_FUNCTION_ARGS){

    ArrayType *array = PG_GETARG_ARRAYTYPE_P(0);
    int64 search_value = PG_GETARG_INT64(1);
    int64 *data =  (int64*)ARR_DATA_PTR(array);

    int64 num_elements = ARR_DIMS(array)[0];

    int64 idx = binary_search_last(data, 0, num_elements - 1, search_value);
    PG_RETURN_INT64(data[idx]);
}