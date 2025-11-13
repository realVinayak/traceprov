#include "postgres.h"
#include "rewriter_utils.h"
#include "parser/parse_node.h"
#include "nodes/makefuncs.h"
#include "traceprov_parse_context.h"

void traceProvAssertNoResJunk(const List *targetList){
    ListCell *targetListCursor;
    foreach(targetListCursor, targetList){
        const TargetEntry *entry = (TargetEntry *)lfirst(targetListCursor);
        if (entry->resjunk){
            elog(ERROR, "Expected no resjunk columns!");
        }  
    }
}

void traceProvAssertIsSubquery(const RangeTblEntry *rte){
    if (rte->rtekind != RTE_SUBQUERY){
        elog(ERROR, "Expected rte to be of subquery, got: %d", rte->rtekind);
    }
    if (rte->subquery == NULL){
        elog(ERROR, "Expected subquery to be non-null!");
    }
}

// Helper that also sets the resno of target entry appropriately.
List *_appendAndAdjustResno(List *inList, TargetEntry *toAdd){
    List *newList = lappend(inList, toAdd);
    toAdd->resno = list_length(newList);
    return newList;
}

List *traceProvAppendAtResJunk(List *old, TargetEntry *newTe){
    if (list_length(old) == 0 || newTe->resjunk){
        // Append it to the very end.
        return lappend(old, newTe);
    }

    // Need to find the first resjunk, and then append it there.
    List *newList = NIL;
    ListCell *cursor;
    bool hasSeenResJunk = false;
    foreach(cursor, old){
        TargetEntry *targetEntry = (TargetEntry *)lfirst(cursor);
        if (targetEntry->resjunk && !hasSeenResJunk){
            newList = _appendAndAdjustResno(newList, newTe);
            hasSeenResJunk = true;
        }
        newList = _appendAndAdjustResno(newList, targetEntry);
    }
    if (!hasSeenResJunk){
        newList = _appendAndAdjustResno(newList, newTe);
    }
    Assert(list_length(newList) == (list_length(old) + 1));
    return newList;
}

List *traceProvGetNullList(unsigned count, Oid consttype, int32 consttypmod, Oid constcollid){
    // This might be too strict, but should never get raised. We can still have defined behavior (return NIL)
    if (count == 0) elog(ERROR, "expected to get atleast 1!");
    List *nullList = NIL;
    for (unsigned int i = 0; i < count; i++){
        nullList = lappend(nullList, makeNullConst(consttype, consttypmod, constcollid));
    }
    return nullList;
}

// Checks that the length of the input lists are same.
// The input is a list of lists (to avoid functions for different number of args)
int traceProvAssertEqualLength(List *inLists){
    if (list_length(inLists) < 2){
        elog(ERROR, "expected at least 2 lists to be compared.");
    }

    const int firstLength = list_length((List *)(lfirst(list_head(inLists))));
    ListCell *cursor;
    for_each_from(cursor, inLists, 1){
        const List *inList = (List *)lfirst(cursor);
        if (list_length(inList) != firstLength){
            elog(ERROR, "got list length to be different!");
        }
    }
    return firstLength;
}

// Takes in the head and gets all the leaf refs.
// This is useful when some action needs to be performed on the flattened structure.
// If includeInternals is true, also includes the internal node (rather than just the leaf refs.)
List *traceProvFindUsedRefs(Node *head, bool includeInternals){
    if (head == NULL){
        // Nothing to do.
        return NIL;
    }
    if (!IsA(head, RangeTblRef) && !IsA(head, SetOperationStmt)){
        elog(ERROR, "Expected either range table ref or set operation stmt. Got : %d", head->type);
    }
    // No need to do anything.
    if (IsA(head, RangeTblRef)) return list_make1(head);
    SetOperationStmt *setOp = (SetOperationStmt *)head;

    List *refs = list_concat_copy(
        traceProvFindUsedRefs(setOp->larg, includeInternals), 
        traceProvFindUsedRefs(setOp->rarg, includeInternals)
    );

    if (includeInternals) refs = lappend(refs, head);
    return refs;
}

List *traceProvDupInt(int element, int count){
    List *begin = NIL;
    for (int i = 0; i < count; i++){
        begin = lappend_int(begin, element);
    }
    return begin;
}

List *traceProvDupOid(Oid element, int count){
    List *begin = NIL;
    for (int i = 0; i < count; i++){
        begin = lappend_oid(begin, element);
    }
    return begin;
}
List *traceProvFlatten(List *inList){

    List *newList = NIL;
    
    ListCell *outerCursor;
    foreach(outerCursor, inList){
        List *innerList = lfirst(outerCursor);
        ListCell *innerCursor;
        foreach(innerCursor, innerList){
            newList = lappend(newList, lfirst(innerCursor));
        }
    }
    return newList;
}

List* traceProvAppendTargets(List *traceProvTargets, List *targetList){
    ListCell *targetEntryCursor;
    foreach(targetEntryCursor, traceProvTargets){
        // Add target entry to the parse->targetlist.
        TargetEntry *target = ((TraceProvTarget *)lfirst(targetEntryCursor))->targetEntry;
        // Here is an ugly case.
        // It is possible that the attributes we're grouping over don't appear as resjunk.
        // In that case, we'll need to adjust the references in the sort refs.
        targetList = traceProvAppendAtResJunk(targetList, target);
    }
    return targetList;
}