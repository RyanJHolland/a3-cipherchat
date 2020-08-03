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
            /*
            // Get an int with the length of the message
            char msg_len_char = '\0';
            int lenRead = recv(connectionSocket, &msg_len_char, sizeof(char), 0);
            if (lenRead < 0)
            {
                close(connectionSocket);
                error("SERVER: Error reading msg length from socket");
            }
            printf("SERVER: lenRead = %d, msg_len_char: %c", lenRead, msg_len_char);
            int msg_len = atoi(msg_len_char);
            printf("SERVER: msg_len: %d", msg_len);
*/

            // GET MESSAGE LENGTH ///////////////////////////
            char *msg_len_buf = (char *)malloc(1);
            if (msg_len_buf == NULL)
            {
                close(connectionSocket);
                printf("SERVER: msg_len_buf: %s", msg_len_buf);
                error("SERVER: Unable to allocate memory for msg_len_buf.");
            }

            // Get the message length from the client and display it
            memset(msg_len_buf, '\0', 1);

            // Read the client's message from the socket
            charsRead = recv(connectionSocket, msg_len_buf, 1, 0);
            if (charsRead < 0)
            {
                close(connectionSocket);
                error("SERVER: ERROR reading to msg_len_buf from socket");
            }
            printf("msg_len_buf = %s\n", msg_len_buf);
            int msg_len = (int)msg_len_buf[0];
            printf("msg_len = %d\n", msg_len);

            // GET MESSAGE ///////////////////////////
            // Allocate a buffer of the anticipated size of the message
            char *buf;
            buf = (char *)malloc(msg_len);
            if (buf == NULL)
            {
                close(connectionSocket);
                printf("SERVER: msg_len: %d", msg_len);
                error("SERVER: Unable to allocate memory for message. Client probably requested too large of a buffer.");
            }

            // Get the message from the client and display it
            memset(buf, '\0', msg_len);
            printf("SERVER: buf = %s\n", buf);
            int received = 0;
            int loopcount = 0;
            while (received < msg_len)
            {
                printf("SERVER: buf = %s, received = %d, msg_len = %d\n", buf, received, msg_len);
                // Read the client's message from the socket
                charsRead = recv(connectionSocket, buf + received, msg_len, 0);
                if (charsRead < 0)
                {
                    close(connectionSocket);
                    error("SERVER: ERROR reading from socket");
                }
                received += charsRead;
                loopcount++;
                if (loopcount > 500)
                {
                    printf("SERVER: looped more than 500 times, probably endless\n");
                    break;
                }
            }
            printf("SERVER: got it all\n");
            printf("SERVER: buf = %s\n", buf);

            // Mutate the message array with the key
            //encode(buf, key);

            printf("SERVER: I received this from the client: \"%s\"\n", buf);

            // Send a Success message back to the client
            charsRead = send(connectionSocket,
                             "I am the server, and I got your message", 39, 0);
            if (charsRead < 0)
            {
                close(connectionSocket);
                error("ERROR writing to socket");
            }
            // Close the connection socket for this client
            close(connectionSocket);
            return 0;
        }
    }
    // Close the listening socket
    close(listenSocket);
    return 0;
}
