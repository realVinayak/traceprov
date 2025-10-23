#CPP := /Users/uicdbgroup/.nix-profile/bin/g++
#CC := /Users/uicdbgroup/.nix-profile/bin/gcc
CPP := /usr/bin/g++
CC := /usr/bin/gcc

all: traceprov infer_set

clean:
	rm -f *.so
	rm -f *.dylib
	rm -f *.o
	rm -f *.auto.sql
	rm -f *.ids

buildSetup:
	@if [ "$(PG_VERSION)" = "" ]; then echo "PG_VERSION not set!" && exit 1; else echo "PG_VERSION: $(PG_VERSION)"; fi
	@if [ "$(USE_DEF)" = "" ]; then echo "USE_DEF not set!" && exit 1; else echo "USE_DEF: $(USE_DEF)"; fi

# ifndef PG_VERSION
# $(error PG Version is not set)
# endif

ifndef suff
suff := default
endif

# If we're using default dirs, simply use the output of pg_config.
# TODO: I think that's also generalizable to different versions...
ifeq ($(USE_DEF), 1)
INCLUDE_DIR := $(shell pg_config --includedir-server)
PKG_LIB_DIR := $(shell pg_config --pkglibdir)
SQL_OUTPUT_DIR := $(shell pwd)/
else
INCLUDE_DIR := /usr/include/postgresql/$(PG_VERSION)/server/
PKG_LIB_DIR := /usr/lib/postgresql/$(PG_VERSION)/lib
SQL_OUTPUT_DIR := /home/postgres/
endif

module: buildSetup
	$(CC) -g -Wall -fPIC -c -I$(INCLUDE_DIR) $(src).c -o $(src)_$(suff).o
	$(CC) -g -Wall -shared $(src)_$(suff).o -o $(src)_$(suff).so
	sudo cp $(src)_$(suff).so $(PKG_LIB_DIR)
	rm -f *.auto.sql
	sed 's/__FILE__/$(src)_$(suff)/g' $(sql).sql > $(sql)_$(src)_$(suff).auto.sql
	sudo cp $(sql)_$(src)_$(suff).auto.sql $(SQL_OUTPUT_DIR)

sql:
	sed 's/__FILE__/$(file)/g' parallel_drop_and_create.sql > $(name).auto.sql
	sudo cp $(name).auto.sql /home/postgres/

binary_search:
	sudo make module src=binary_search suff=$(suff) sql=create_b_search

traceprov: buildSetup
	$(CC) -g -O3 -Wall -fPIC -c -I$(INCLUDE_DIR) traceprov.c -o traceprov_$(suff).o
	$(CC) -g -O3 -Wall -fPIC -c -I$(INCLUDE_DIR) file_utils.c -o file_utils_$(suff).o
	$(CC) -g -O3 -Wall -fPIC -c -I$(INCLUDE_DIR) utils.c -o utils_$(suff).o
	$(CC) -g -O3 -Wall -bundle -flat_namespace -undefined suppress traceprov_$(suff).o file_utils_$(suff).o utils_$(suff).o -o traceprov_$(suff).dylib
	sudo cp traceprov_$(suff).dylib $(PKG_LIB_DIR)
	sed 's/__FILE__/traceprov_$(suff)/g' traceprov.sql > traceprov_$(suff).auto.sql
	@if [ "$(USE_DEF)" = "0" ]; then echo "copying SQL!" && sudo cp traceprov_$(suff).auto.sql $(SQL_OUTPUT_DIR); fi

traceprov_infer:
	g++ -O3 -I/usr/include/postgresql/$(PG_VERSION)/server/ traceprov_infer.cpp file_utils.c -o traceprov_infer.o

traceprov_infer_dbg:
	g++ -g -I/usr/include/postgresql/$(PG_VERSION)/server/ traceprov_infer.cpp file_utils.c -o traceprov_infer.o

infer_set: buildSetup
	$(CC) -g -O3 -Wall -fPIC -c -I$(INCLUDE_DIR) file_utils.c -o file_utils_$(suff).o
	$(CC) -g -O3 -Wall -fPIC -c -I$(INCLUDE_DIR) utils.c -o utils_$(suff).o
	$(CPP) -g -std=c++11 -O3 -Wall -fPIC -c -I$(INCLUDE_DIR) traceprov_return_infer.cpp -o traceprov_return_$(suff).o
	$(CPP) -g -std=c++11 -O3 -Wall -bundle -flat_namespace -undefined suppress  traceprov_return_$(suff).o file_utils_$(suff).o utils_$(suff).o -o traceprov_return_$(suff).dylib
	sudo cp traceprov_return_$(suff).dylib $(PKG_LIB_DIR)
	sed 's/__FILE__/traceprov_return_$(suff)/g' traceprov_return_infer.sql > traceprov_return_infer_$(suff).auto.sql
	sed 's/__FILE__/traceprov_return_$(suff)/g' traceprov_return_infer_template.sql > traceprov_return_infer_template_$(suff).auto.sql
	@if [ "$(USE_DEF)" = "0" ]; then echo "copying SQL!" && sudo cp traceprov_return_infer_$(suff).auto.sql $(SQL_OUTPUT_DIR); fi
