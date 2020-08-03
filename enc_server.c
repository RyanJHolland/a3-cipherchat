#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


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
    int i;

    while (msg[i] != '\0')
    {
        // remove the newline char from the end of the str (they all have one)
        if (msg[i] == '\n')
        {
            msg[i] = '\0';
            break;
        }
        // if the char is not A-Z or space (ASCII values of A-Z are 65-90)

        if (msg[i] < 65 || msg[i] > 90)
        {
            printf("SERVER: error: input contains bad characters\n");
            break;
        }
        // mutate the array
        msg[i] = (msg[i] + key[i]) % 26;
        i++;
    }
    return 0;
}

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
            printf("SERVER: Connected to client running at host %d port %d\n",
                   ntohs(clientAddress.sin_addr.s_addr),
                   ntohs(clientAddress.sin_port));

            // GET MESSAGE LENGTH ///////////////////////////
            char *msg_len_buf = '\0';

            // Read the client's message SIZE from the socket
            charsRead = recv(connectionSocket, msg_len_buf, 1, 0);
            if (charsRead < 0)
            {
                free(msg_len_buf);
                close(connectionSocket);
                error("SERVER: ERROR reading to msg_len_buf from socket");
            }
            
            printf("SERVER: msg_len_buf = %s\n", msg_len_buf);
            int msg_len = (int)msg_len_buf[0];
            printf("SERVER: msg_len = %d\n", msg_len);

            // GET MESSAGE ///////////////////////////
            // Allocate a buf of the anticipated size of the message
            char *msg_buf = (char *)malloc(msg_len);
            if (msg_buf == NULL)
            {
                free(msg_len_buf);
                close(connectionSocket);
                printf("SERVER: msg_len: %d", msg_len);
                error("SERVER: Unable to allocate memory for message. Client probably requested too large of a buf.");
            }

            // Get the message from the client and display it
            memset(msg_buf, '\0', msg_len);
            printf("SERVER: msg_buf = %s\n", msg_buf);

            // Read the client's message from the socket
            charsRead = recv(connectionSocket, msg_buf, msg_len, 0);
            if (charsRead < 0)
            {
                free(msg_len_buf);
                free(msg_buf);
                close(connectionSocket);
                error("SERVER: ERROR reading from socket");
            }
            printf("SERVER: charsRead = %d, msg_buf = %s\n", charsRead, msg_buf);

            // Mutate the message array with the key
            //encode(msg_buf, key);

            printf("SERVER: I received this from the client: \"%s\"\n", msg_buf);

            // Send a Success message back to the client
            charsRead = send(connectionSocket,
                             "I am the server, and I got your message", 39, 0);
            if (charsRead < 0)
            {
                free(msg_len_buf);
                free(msg_buf);
                close(connectionSocket);
                error("ERROR writing to socket");
            }
            // Close the connection socket for this client
            free(msg_len_buf);
            free(msg_buf);
            close(connectionSocket);
            return 0;
        }
    }
    // Close the listening socket
    close(listenSocket);
    return 0;
}
