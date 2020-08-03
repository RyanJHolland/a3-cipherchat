compile: keygen.c enc_client.c enc_server.c
	gcc -g -lm -Wall -std=c99 keygen.c -o keygen
	gcc -g -lm -Wall -std=c99 enc_client.c -o enc_client
	gcc -g -lm -Wall -std=c99 enc_server.c -o enc_server

run: keygen.c enc_client.c enc_server.c
	make compile
	keygen

mem: enc_client.c
	valgrind --leak-check=full \
	--show-leak-kinds=all \
    --track-origins=yes \
    --verbose \
    --log-file=valgrind-out.txt \
    ./enc_client enc_client_valgrind

server: enc_server.c

def_compile: keygen.c default_enc_client.c default_enc_server.c
	gcc -g -lm -Wall -std=c99 keygen.c -o keygen
	gcc -g -lm -Wall -std=c99 default_enc_client.c -o default_enc_client
	gcc -g -lm -Wall -std=c99 default_enc_server.c -o default_enc_server
