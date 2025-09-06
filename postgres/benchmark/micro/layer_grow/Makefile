run:
	gcc -O3 -DTRACEPROV_STANDALONE -DTRACEPROV_INCREMENT_TRACE_BY_PG=$(inc) -I../../../ -I/usr/include/postgresql/14/server/ ../../../utils.c run.c -o run.o

clean:
	rm -f *.o
	rm -f *.tp