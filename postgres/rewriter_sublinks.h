#ifndef __TRACEPROV_REWRITER_SUBLINKS__
#define __TRACEPROV_REWRITER_SUBLINKS__
#include "rewriter_utils.h"
// The main entry point, to perform rewrites in the sublink.
void traceprov_rewrite_sublinks(Query *, TraceProvParseContext*, const List *, const bool process_later);
#endif
