
run: keygen.c enc_client.c enc_server.c
	make compile
	enc_server 14141 &
	enc_client cipher key 14141
	dec_server 14142 &
	dec_client cipher key 14142
	
compile: keygen.c enc_client.c enc_server.c dec_client.c dec_server.c
	gcc -g -lm -Wall -std=c99 keygen.c -o keygen
	gcc -g -lm -Wall -std=c99 enc_client.c -o enc_client
	gcc -g -lm -Wall -std=c99 enc_server.c -o enc_server
	gcc -g -lm -Wall -std=c99 dec_client.c -o dec_client
	gcc -g -lm -Wall -std=c99 dec_server.c -o dec_server

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
