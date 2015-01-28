CFLAGS = -g -std=c99 -D_XOPEN_SOURCE=700

a:
	gcc $(CFLAGS) -c ./src/library.c
	gcc $(CFLAGS) -pthread library.o ./src/server.c -o ./bin/server
	gcc $(CFLAGS) -pthread library.o  ./src/client.c -o ./bin/client
	rm library.o
clean:
	rm ./bin/server
	rm ./bin/client