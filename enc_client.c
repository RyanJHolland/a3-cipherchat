/*
Ryan Holland
OSU ID 933636892
Operating Systems class
Assignment 3
*/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/stat.h>

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arguments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg)
{
  perror(msg);
  exit(0);
}

// Set up the address struct
void setupAddressStruct(struct sockaddr_in *address,
                        int portNumber,
                        char *hostname)
{

  // Clear out the address struct
  memset((char *)address, '\0', sizeof(*address));

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent *hostInfo = gethostbyname(hostname);
  if (hostInfo == NULL)
  {
    fprintf(stderr, "CLIENT: ERROR, no such host\n");
    exit(0);
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char *)&address->sin_addr.s_addr,
         hostInfo->h_addr_list[0],
         hostInfo->h_length);
}

// args: plaintext key port
int main(int argc, char *argv[])
{
  int socketFD, portNumber, charsWritten, charsRead, i, ch;
  struct sockaddr_in serverAddress;
  FILE *fp;
  int buf_size = 256;
  char *buf = malloc(buf_size);

  // Check usage & args
  if (argc < 4)
  {
    fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
    exit(0);
  }

  // Get FILE pointer to plaintext message file
  fp = fopen(argv[1], "r");
  if (fp == NULL)
  {
    fprintf(stderr, "file not found: %s\n", argv[1]);
    exit(0);
  }

  // Read plaintext msg into buffer
  i = 0;
  while (1)
  {
    ch = getc(fp);
    if (ch == EOF)
    {
      break;
    }
    if (i > buf_size - 2)
    {
      buf_size *= 2;
      buf = realloc(buf, buf_size);
      if (buf == NULL)
      {
        printf("error, out of memory!\n");
        exit(1);
      }
    }
    buf[i] = ch;
    i++;
  }

  // Remove trailing newline, if present
  if (buf[i - 1] == '\n')
  {
    buf[i - 1] = '\0';
    i--;
  }

  // Insert divider
  buf[i] = '$';

  i++;
  // Get FILE pointer to key file
  fp = fopen(argv[2], "r");
  if (fp == NULL)
  {
    fprintf(stderr, "file not found: %s\n", argv[2]);
    exit(0);
  }

  // Read key into buffer
  while ((ch = getc(fp)) != EOF)
  {
    if (i < buf_size - 1)
    {
      buf[i] = ch;
      i++;
    }
    else
    {
      buf_size *= 2;
      buf = realloc(buf, buf_size);
      if (buf == NULL)
      {
        printf("error, out of memory!\n");
        exit(1);
      }
    }
  }
  buf[i] = '\0';

  // Remove trailing newline, if present
  if (buf[i - 1] == '\n')
  {
    buf[i - 1] = '\0';
    i--;
  }

  printf("%s\n", buf);

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0)
  {
    error("CLIENT: ERROR opening socket");
  }

  // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
  {
    error("CLIENT: ERROR connecting");
  }

  /*
  // This was the old code for getting cmd line input. We are changing it to read from a file instead of cmd line.
  // Get input message from user
  printf("CLIENT: Enter text to send to the server, and then hit enter: ");
  // Clear out the buffer array
  memset(buf, '\0', sizeof(buf));
  // Get input from the user, trunc to buffer - 1 chars, leaving \0
  fgets(buf, sizeof(buf) - 1, stdin);
  // Remove the trailing \n that fgets adds
  buf[strcspn(buf, "\n")] = '\0'; 
  */

  // Write to the server
  charsWritten = send(socketFD, buf, strlen(buf), 0);
  if (charsWritten < 0)
  {
    error("CLIENT: ERROR writing to socket");
  }
  if (charsWritten < strlen(buf))
  {
    printf("CLIENT: WARNING: Not all data written to socket!\n");
  }

  // Get return message from server
  // Clear out the buffer again for reuse
  memset(buf, '\0', sizeof(buf));
  // Read data from the socket, leaving \0 at end
  charsRead = recv(socketFD, buf, sizeof(buf) - 1, 0);
  if (charsRead < 0)
  {
    error("CLIENT: ERROR reading from socket");
  }
  printf("CLIENT: I received this from the server: \"%s\"\n", buf);

  // Close the socket
  close(socketFD);
  free(buf);
  return 0;
}