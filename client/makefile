client : client.o streamsock.o

server.o : client.c
	gcc -c client.c
streamsock.o : ../libs/streamsock.c
	gcc -c ../libs/streamsock.c

clean:
	rm client client.o streamsock.o
