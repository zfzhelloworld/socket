server : server.o streamsock.o

server.o : server.c
	gcc -c server.c
streamsock.o : ../libs/streamsock.c
	gcc -c ../libs/streamsock.c

clean:
	rm server server.o streamsock.o
