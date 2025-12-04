lib_LTLIBRARIES = libtraceprov.la libtraceprov_infer.la librewriter.la
libtraceprov_la_SOURCES = traceprov.c file_utils.c utils.c traceprov_type.c hash.c
libtraceprov_infer_la_SOURCES = traceprov_return_infer.cpp file_utils.c utils.c traceprov_parse_context.c
librewriter_la_SOURCES = rewriter.c traceprov_parse_context.c rewriter_utils.c rewriter_sets.c rewriter_sublinks.c plan_analyzer.c
AM_CFLAGS = -I${TRACEPROV_PG_INCLUDE_DIR} -DTRACEPROV_PAGE_SIZE_RAW=${TRACEPROV_PAGE_SIZE} -UPACKAGE_NAME -UPACKAGE_VERSION -UPACKAGE_STRING -UPACKAGE_BUGREPORT -UPACKAGE_URL -UPACKAGE_TARNAME -UPACKAGE_URL
AM_CPPFLAGS = -I${TRACEPROV_PG_INCLUDE_DIR} -DTRACEPROV_PAGE_SIZE_RAW=${TRACEPROV_PAGE_SIZE} -UPACKAGE_NAME -UPACKAGE_VERSION -UPACKAGE_STRING -UPACKAGE_BUGREPORT -UPACKAGE_URL -UPACKAGE_TARNAME -UPACKAGE_URL
libtraceprov_la_LDFLAGS = -avoid-version
libtraceprov_infer_la_LDFLAGS = -avoid-version
librewriter_la_LDFLAGS = -avoid-version
