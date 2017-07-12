default: cliser

cliser: ./src/client_chat_tcp.c ./src/server_chat_tcp_multi.c
	gcc -g -o ./Client/client_chat_tcp.o ./src/client_chat_tcp.c
	gcc -g -pthread -o ./Server/server_chat_tcp_multi.o ./src/server_chat_tcp_multi.c

cdf: ./src/cdf_tools.c
	gcc -g -o ./Server/cdf_tools.o ./src/cdf_tools.c

clean: rm ./bin/*.o 
