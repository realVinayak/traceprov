#ifndef __TRACEPROV_REWRITER_UTILS__
#define __TRACEPROV_REWRITER_UTILS__
#include "c.h"
#include "nodes/pg_list.h"
#include "nodes/parsenodes.h"

void traceProvAssertNoResJunk(const List *);
void traceProvAssertIsSubquery(const RangeTblEntry *);
int traceProvAssertEqualLength(List *);

List *traceProvAppendAtResJunk(List *, TargetEntry *);
List *traceProvGetNullList(unsigned, Oid, int32, Oid);
List *traceProvFindUsedRefs(Node *, bool);

List *traceProvDupInt(int, int);
List *traceProvDupOid(Oid, int);

List *traceProvFlatten(List *);

#endif