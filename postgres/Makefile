
word-dot = $(word $2,$(subst ., ,$1))
word-dot-rest = $(word $2,$(subst ., ,$2))

ifndef PG_VERSION
# latest version, at the moment.
PG_VERSION := 18
endif

ifndef suff
suff := default
endif

all: traceprov infer_set

clean:
	rm -f *.so
	rm -f *.o
	rm -f *.auto.sql
	rm -f *.ids

module:
	gcc -g -Wall -fPIC -c -I/usr/include/postgresql/$(PG_VERSION)/server/ $(src).c -o $(src)_$(suff).o
	gcc -g -Wall -shared $(src)_$(suff).o -o $(src)_$(suff).so
	sudo cp $(src)_$(suff).so /usr/lib/postgresql/$(PG_VERSION)/lib
	rm -f *.auto.sql
	sed 's/__FILE__/$(src)_$(suff)/g' $(sql).sql > $(sql)_$(src)_$(suff).auto.sql
	sudo cp $(sql)_$(src)_$(suff).auto.sql /home/postgres/

sql:
	sed 's/__FILE__/$(file)/g' parallel_drop_and_create.sql > $(name).auto.sql
	sudo cp $(name).auto.sql /home/postgres/

binary_search:
	sudo make module src=binary_search suff=$(suff) sql=create_b_search

traceprov:
	gcc -g -O3 -Wall -fPIC -c -I/usr/include/postgresql/$(PG_VERSION)/server/ traceprov.c -o traceprov_$(suff).o
	gcc -g -O3 -Wall -fPIC -c -I/usr/include/postgresql/$(PG_VERSION)/server/ file_utils.c -o file_utils_$(suff).o
	gcc -g -O3 -Wall -fPIC -c -I/usr/include/postgresql/$(PG_VERSION)/server/ utils.c -o utils_$(suff).o
	gcc -g -O3 -Wall -shared traceprov_$(suff).o file_utils_$(suff).o utils_$(suff).o -o traceprov_$(suff).so
	sudo cp traceprov_$(suff).so /usr/lib/postgresql/$(PG_VERSION)/lib
	sed 's/__FILE__/traceprov_$(suff)/g' traceprov.sql > traceprov_$(suff).auto.sql
	sudo cp traceprov_$(suff).auto.sql /home/postgres/

traceprov_infer:
	g++ -O3 -I/usr/include/postgresql/$(PG_VERSION)/server/ traceprov_infer.cpp file_utils.c -o traceprov_infer.o

traceprov_infer_dbg:
	g++ -g -I/usr/include/postgresql/$(PG_VERSION)/server/ traceprov_infer.cpp file_utils.c -o traceprov_infer.o

infer_set:
	gcc -g -O3 -Wall -fPIC -c -I/usr/include/postgresql/$(PG_VERSION)/server/ file_utils.c -o file_utils_$(suff).o
	gcc -g -O3 -Wall -fPIC -c -I/usr/include/postgresql/$(PG_VERSION)/server/ utils.c -o utils_$(suff).o
	g++ -g -std=c++11 -O3 -Wall -fPIC -c -I/usr/include/postgresql/$(PG_VERSION)/server/ traceprov_return_infer.cpp -o traceprov_return_$(suff).o
	g++ -g -std=c++11 -O3 -Wall -shared traceprov_return_$(suff).o file_utils_$(suff).o utils_$(suff).o -o traceprov_return_$(suff).so
	sudo cp traceprov_return_$(suff).so /usr/lib/postgresql/$(PG_VERSION)/lib
	sed 's/__FILE__/traceprov_return_$(suff)/g' traceprov_return_infer.sql > traceprov_return_infer_$(suff).auto.sql
	sed 's/__FILE__/traceprov_return_$(suff)/g' traceprov_return_infer_template.sql > traceprov_return_infer_template_$(suff).auto.sql
	sudo cp traceprov_return_infer_$(suff).auto.sql /home/postgres/
