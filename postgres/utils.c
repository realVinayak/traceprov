#include "traceprov_utils.h"

int get_error_no(){
    int err_no = errno;
    return err_no;
}


void print_layer(struct traceprov_aggregate_layer *layer){
    elog(INFO, "traceprov_aggregate_layer {");
    elog(INFO, "\t->num_pk_records: %d", layer->num_pk_records);
    elog(INFO, "\t->last_mapping: %p", layer->last_mapping);
    elog(INFO, "\t->mapping_count: %d", layer->mapping_count);
    elog(INFO, "\t->current_row: %p", layer->current_row);
    elog(INFO, "\t->num_groups: %d", layer->num_groups);
    elog(INFO, "\t->layer_number: %d", layer->layer_number);
    elog(INFO, "\t->record_padding: %d", layer->record_padding);
    elog(INFO, "}");
}