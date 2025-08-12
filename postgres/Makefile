clean:
	rm *.so
	rm *.o

parallel:
	sudo cp /etc/postgresql/14/main/postgresql.conf.parallel /etc/postgresql/14/main/postgresql.conf
	sudo systemctl restart postgresql.service

series:
	sudo cp /etc/postgresql/14/main/postgresql.conf.sequential /etc/postgresql/14/main/postgresql.conf
	sudo systemctl restart postgresql.service

module:
	gcc -fPIC -c -I/usr/include/postgresql/14/server/ $(src).c -o $(src)_$(suff).o
	gcc -shared $(src)_$(suff).o -o $(src)_$(suff).so
	sudo cp $(src)_$(suff).so /usr/lib/postgresql/14/lib