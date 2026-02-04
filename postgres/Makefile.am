lib_LTLIBRARIES = libtraceprov.la libtraceprov_infer.la librewriter.la
libtraceprov_la_SOURCES = traceprov.c file_utils.c utils.c traceprov_type.c hash.c traceprov_settings.c traceprov_utils.cpp
libtraceprov_infer_la_SOURCES = traceprov_return_infer.cpp file_utils.c utils.c traceprov_parse_context.c rewriter_utils.c traceprov_utils.cpp traceprov_settings.c
librewriter_la_SOURCES = rewriter.c traceprov_parse_context.c rewriter_utils.c rewriter_sets.c rewriter_sublinks.c plan_analyzer.c rewriter_window.c traceprov_settings.c
AM_CFLAGS = -I${TRACEPROV_PG_INCLUDE_DIR} -DTRACEPROV_PAGE_SIZE_RAW=${TRACEPROV_PAGE_SIZE} -UPACKAGE_NAME -UPACKAGE_VERSION -UPACKAGE_STRING -UPACKAGE_BUGREPORT -UPACKAGE_URL -UPACKAGE_TARNAME -UPACKAGE_URL
AM_CPPFLAGS = -I${TRACEPROV_PG_INCLUDE_DIR} -DTRACEPROV_PAGE_SIZE_RAW=${TRACEPROV_PAGE_SIZE} -UPACKAGE_NAME -UPACKAGE_VERSION -UPACKAGE_STRING -UPACKAGE_BUGREPORT -UPACKAGE_URL -UPACKAGE_TARNAME -UPACKAGE_URL
libtraceprov_la_LDFLAGS = -avoid-version
libtraceprov_infer_la_LDFLAGS = -avoid-version
librewriter_la_LDFLAGS = -avoid-version
if ENABLE_SMOKEDDUCK
libtraceprov_infer_la_SOURCES += traceprov_duckdb_infer.cpp
AM_CFLAGS += -I${SMOKEDDUCK_INCUDE_PATH} -DTRACEPROV_BUILD_WITH_DUCKDB=1
AM_CPPFLAGS += -I${SMOKEDDUCK_INCUDE_PATH} -DTRACEPROV_BUILD_WITH_DUCKDB=1
libtraceprov_la_LDFLAGS += -L${SMOKEDDUCK_PATH} -Wl,-rpath,${SMOKEDDUCK_PATH}
libtraceprov_infer_la_LDFLAGS += -L${SMOKEDDUCK_PATH} -Wl,-rpath,${SMOKEDDUCK_PATH}
libtraceprov_la_LIBADD = -lduckdb
libtraceprov_infer_la_LIBADD = -lduckdb
bin_PROGRAMS = run_smokedduck
run_smokedduck_SOURCES = run_duckdb_query.cpp
run_smokedduck_CPPFLAGS = -I${SMOKEDDUCK_INCUDE_PATH} -I${TRACEPROV_PG_INCLUDE_DIR} -UPACKAGE_NAME -UPACKAGE_VERSION -UPACKAGE_STRING -UPACKAGE_BUGREPORT -UPACKAGE_URL -UPACKAGE_TARNAME -UPACKAGE_URL
run_smokedduck_LDFLAGS = -L${SMOKEDDUCK_PATH} -Wl,-rpath,${SMOKEDDUCK_PATH}
run_smokedduck_LDADD = -lduckdb 
endif
