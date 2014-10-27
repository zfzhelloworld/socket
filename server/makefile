server : server.o streamsock.o main_logic.o

server.o : server.c
	gcc -c server.c
streamsock.o : ../libs/streamsock.c
	gcc -c ../libs/streamsock.c
main_logic.o : main_logic.c
	gcc -c main_logic.c

clean:
	rm server server.o streamsock.o
