clean:
	rm -f *.so
	rm -f *.o

parallel:
	sudo cp /etc/postgresql/16/main/postgresql.conf.parallel /etc/postgresql/16/main/postgresql.conf
	sudo systemctl restart postgresql.service

series:
	sudo cp /etc/postgresql/16/main/postgresql.conf.sequential /etc/postgresql/16/main/postgresql.conf
	sudo systemctl restart postgresql.service

module:
	gcc -fPIC -c -I/usr/include/postgresql/16/server/ $(src).c -o $(src)_$(suff).o
	gcc -shared $(src)_$(suff).o -o $(src)_$(suff).so
	sudo cp $(src)_$(suff).so /usr/lib/postgresql/16/lib
	rm -f *.auto.sql
	sed 's/__FILE__/$(src)_$(suff)/g' parallel_drop_and_create.sql > parallel_drop_and_create_$(src)_$(suff).auto.sql

infer:
	g++ -O3 -I/usr/include/postgresql/16/server/ infer.cpp -o infer.o

infer_dbg:
	g++ -g -O0 -I/usr/include/postgresql/16/server/ infer.cpp -o infer.o

sql:
	sed 's/__FILE__/$(file)/g' parallel_drop_and_create.sql > $(name).auto.sql
	sudo cp $(name).auto.sql /home/postgres/
