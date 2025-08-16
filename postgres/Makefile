clean:
	rm -f *.so
	rm -f *.o
	rm -f *.auto.sql
	rm -f *.ids

parallel:
	sudo cp /etc/postgresql/14/main/postgresql.conf.parallel /etc/postgresql/14/main/postgresql.conf
	sudo systemctl restart postgresql.service

series:
	sudo cp /etc/postgresql/14/main/postgresql.conf.sequential /etc/postgresql/14/main/postgresql.conf
	sudo systemctl restart postgresql.service

module:
	gcc -g -Wall -fPIC -c -I/usr/include/postgresql/14/server/ $(src).c -o $(src)_$(suff).o
	gcc -g -Wall -shared $(src)_$(suff).o -o $(src)_$(suff).so
	sudo cp $(src)_$(suff).so /usr/lib/postgresql/14/lib
	rm -f *.auto.sql
	sed 's/__FILE__/$(src)_$(suff)/g' parallel_drop_and_create.sql > parallel_drop_and_create_$(src)_$(suff).auto.sql
	sudo cp parallel_drop_and_create_$(src)_$(suff).auto.sql /home/postgres/

infer:
	g++ -O3 -I/usr/include/postgresql/14/server/ infer.cpp -o infer.o

infer_dbg:
	g++ -g -O0 -I/usr/include/postgresql/14/server/ infer.cpp -o infer.o

sql:
	sed 's/__FILE__/$(file)/g' parallel_drop_and_create.sql > $(name).auto.sql
	sudo cp $(name).auto.sql /home/postgres/