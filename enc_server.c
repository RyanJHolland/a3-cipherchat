#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// function to send strings in chunks. Credit: https://stackoverflow.com/questions/39110113/second-recv-call-doesnt-receive-data-halts-the-execution-in-c
int sendstring(int sock, const char *str)
{
    if (!str)
        str = "";
    int len = strlen(str) + 1;

    do
    {
        int ret = send(sock, str, len, 0);
        if (ret <= 0)
            return -1;
        str += ret;
        len -= ret;
    } while (len > 0);

    return 0;
}

// Error function used for reporting issues
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in *address,
                        int portNumber)
{

    // Clear out the address struct
    memset((char *)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

// Mutates the message array by encoding it with the key array.
int encode(char *msg, char *key)
{
    //char *keyset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    int i = 0;
    while (msg[i] != '\0')
    {
        char msg_char;
        if (msg[i] == ' ')
        {
            msg_char = msg[i] - 5; // space is ASCII 32, but we want it to be 27
        }
        else
        {
            msg_char = msg[i] - 64; // the capital alphabet starts at 65 in ASCII, but we want it to be 1-26
        }
        //printf("msg[i] = %d which is %c, so msg_char = %d which is %c\n", msg[i], msg[i], msg_char, msg_char);

        char key_char;
        if (key[i] == ' ')
        {
            key_char = key[i] - 5; // space is ASCII 32, but we want it to be 27
        }
        else
        {
            key_char = key[i] - 64; // the capital alphabet starts at 65 in ASCII, but we want it to be 1-26
        }
        //printf("key[i] = %d which is %c, so key_char = %d which is %c\n", key[i], key[i], key_char, key_char);

        char cipher_char = msg_char + key_char; // this would be in range 0 (A + A) to 52 (space + space)
        //printf("cipher_char = %d which is %c\n", cipher_char, cipher_char);
        while (cipher_char > 27)
        {
            cipher_char -= 27;
        }
        //printf("after mod 27, cipher_char = %d which is %c\n", cipher_char, cipher_char);
        cipher_char += 64; // bring it back up to the ASCII A-Z range
        //printf("after bringing it back to ASCII, cipher_char = %d which is %c\n", cipher_char, cipher_char);
        if (cipher_char == 91) // handle a space
        {
            cipher_char = 32;
        }
        //printf("cipher_char = %d which is %c\n", cipher_char, cipher_char);

        msg[i] = cipher_char;
        i++;
    }
    return 0;
}

///////////////////////////////////////
//              MAIN                //
/////////////////////////////////////
int main(int argc, char *argv[])
{
    int connectionSocket, charsRead, pid;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    // Check usage & args
    if (argc < 2)
    {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0)
    {
        error("ERROR opening socket");
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket,
             (struct sockaddr *)&serverAddress,
             sizeof(serverAddress)) < 0)
    {
        close(listenSocket);
        error("ERROR on binding");
    }

    // Start listening for connections. Allow up to 5 connections to queue up
    listen(listenSocket, 5);

    // Accept a connection, blocking if one is not available until one connects
    while (1)
    {
        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket,
                                  (struct sockaddr *)&clientAddress,
                                  &sizeOfClientInfo);
        if (connectionSocket < 0)
        {
            close(listenSocket);
            close(connectionSocket);
            error("ERROR on accept");
        }

        // fork into a child process to handle the connection and a parent process to keep listening
        pid = fork();
        if (pid < 0) // error
        {
            close(listenSocket);
            close(connectionSocket);
            error("fork() failed!");
        }
        else if (pid > 0) // child. handle connection.
        {
            /*
            printf("SERVER: Connected to client running at host %d port %d\n",
                   ntohs(clientAddress.sin_addr.s_addr),
                   ntohs(clientAddress.sin_port));
            */

            // VERIFY THAT YOU ARE TALKING TO THE RIGHT CLIENT ////////
            /*
            // get introduction
            int correct_client = 0;
            char client[11] = {'\0'};
            charsRead = recv(connectionSocket, client, 10, 0);
            if (charsRead < 0)
            {
                close(connectionSocket);
                error("SERVER: ERROR reading from socket");
            }
            //printf("SERVER: charsRead = %d, client = %s\n", charsRead, client);
            // confirm or deny
            if (strcmp(client, "enc_client") == 0)
            {
                correct_client = 1;
            }
            //printf("SERVER: correct_client = %d\n", correct_client);
            /////////////// Write confirmation

            int confirmation = send(connectionSocket, &correct_client, 1, 0);
            if (confirmation < 0)
            {
                close(connectionSocket);
                error("SERVER: ERROR sending confirmation to client");
            }
            //printf("SERVER: confirmation = %d\n", confirmation);
*/
            // GET MESSAGE LENGTH ///////////////////////////
            int msg_len = 0;

            // Read the client's message SIZE from the socket
            charsRead = recv(connectionSocket, &msg_len, 1, 0);
            if (charsRead < 0)
            {
                close(connectionSocket);
                error("SERVER: ERROR reading to msg_len_buf from socket");
            }
            //printf("SERVER: msg_len = %d\n", msg_len);

            // GET MESSAGE ///////////////////////////
            // Allocate a buf of the anticipated size of the message
            char *buf = (char *)malloc(msg_len);
            if (buf == NULL)
            {
                close(connectionSocket);
                error("SERVER: Unable to allocate memory for message. Client probably requested too large of a buf.");
            }

            // Get the message from the client and display it
            memset(buf, '\0', msg_len);

            // Read the client's message from the socket
            charsRead = recv(connectionSocket, buf, msg_len, 0);
            if (charsRead < 0)
            {
                free(buf);
                close(connectionSocket);
                error("SERVER: ERROR reading from socket");
            }
            printf("SERVER: charsRead = %d, buf = %s\n", charsRead, buf);

            // Find the index of the key within the buf
            int i = 0;
            int key_idx = -1;
            while (1)
            {
                if (buf[i] == '$')
                {
                    key_idx = i + 1;
                    buf[i] = '\0'; // replace the $ with a null, so now we have 2 strings in the buf.
                }
                i++;
                if (buf[i] == '\0')
                {
                    break;
                }
            }
            if (key_idx == -1)
            {
                free(buf);
                close(connectionSocket);
                error("SERVER: ERROR finding key index");
            }
            // Mutate the message array with the key
            encode(buf, &buf[key_idx]);

            //printf("SERVER: Encoded msg: \"%s\"\n", buf);

            // Send a Success message back to the client
            charsRead = send(connectionSocket,
                             buf, msg_len, 0);
            if (charsRead < 0)
            {
                free(buf);
                close(connectionSocket);
                error("SERVER: ERROR writing to socket");
            }
            // Close the connection socket for this client
            free(buf);
            close(connectionSocket);
            return 0;
        }
    }
    // Close the listening socket
    close(listenSocket);
    return 0;
}
