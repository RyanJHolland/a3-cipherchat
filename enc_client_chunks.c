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
  int socketFD, charsWritten, charsRead, i, ch;
  struct sockaddr_in serverAddress;
  FILE *fp;
  int buf_size = 256;
  char *buf = malloc(buf_size);

  // Check usage & args
  if (argc < 4)
  {
    free(buf);
    fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
    exit(0);
  }

  // Get FILE pointer to plaintext message file
  fp = fopen(argv[1], "r");
  if (fp == NULL)
  {
    free(buf);
    fprintf(stderr, "file not found: %s\n", argv[1]);
    exit(0);
  }

  // Read plaintext msg into buffer
  i = 0;
  while (1)
  {
    ch = getc(fp);
    if (ch == '\n' || ch == EOF)
    {
      break;
    }
    if (i > buf_size - 2)
    {
      buf_size *= 2;
      buf = realloc(buf, buf_size);
      if (buf == NULL)
      {
        free(buf);
        error("out of memory!");
        exit(0);
      }
    }
    // validate character
    if ((ch > 64 && ch < 91) || (ch == 32)) // from A-Z or a space
    {
      buf[i] = ch;
      i++;
    }
    else
    {
      free(buf);
      printf("character %c in file %s was invalid.\n", ch, argv[1]);
      error("invalid character input! Exiting...");
      exit(0);
    }
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
  while (1)
  {
    ch = getc(fp);
    if (ch == '\n' || ch == EOF)
    {
      break;
    }
    if (i > buf_size - 2)
    {
      buf_size *= 2;
      buf = realloc(buf, buf_size);
      if (buf == NULL)
      {
        error("out of memory!");
        exit(0);
      }
    }
    // validate character
    if ((ch > 64 && ch < 91) || (ch == 32)) // from A-Z or a space
    {
      buf[i] = ch;
      i++;
    }
    else
    {
      free(buf);
      printf("character %c in file %s was invalid.\n", ch, argv[2]);
      error("invalid character input! Exiting...");
      exit(0);
    }
  }

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
        free(buf);
        printf("error, out of memory!\n");
        exit(0);
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

  //printf("%s\n", buf);

  // SEND TO SERVER /////////////

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0)
  {
    free(buf);
    error("CLIENT: ERROR opening socket");
  }

  // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
  {
    close(socketFD);
    free(buf);
    error("CLIENT: ERROR connecting");
  }

  // Write message length to the server
  int msg_len = i;
  printf("CLIENT: msg_len = %d, strlen(buf) = %d\n", msg_len, strlen(buf));
  int len_written = send(socketFD, &msg_len, sizeof(msg_len), 0);
  if (len_written < 0)
  {
    close(socketFD);
    free(buf);
    error("CLIENT: ERROR writing msg_len to socket");
  }
  else if (len_written < 1)
  {
    close(socketFD);
    free(buf);
    error("CLIENT: ERROR writing full msg_len to socket");
  }
  else
  {
    printf("CLIENT: msg_len sent successfully\n");

    // Write message to the server, breaking it into chunks of 1000 or less
    int chunk_size = 5;
    int written = 0;
    int loopcount = 0;
    while (written < msg_len)
    {
      printf("CLIENT: written = %d, msg_len = %d\n", written, msg_len);
      if (msg_len > chunk_size)
      {
        printf("CLIENT: msg_len > chunk_size.\n");
        charsWritten = send(socketFD, buf + written, chunk_size, 0);
        if (charsWritten < chunkSize)
        {
          
        }
      }
      else
      {
        printf("CLIENT: msg_len <= chunk_size.\n");
        charsWritten = send(socketFD, buf + written, msg_len, 0);
      }
      printf("CLIENT: charsWritten = %d\n", charsWritten);
      if (charsWritten < 0)
      {
        close(socketFD);
        free(buf);
        error("CLIENT: ERROR writing msg to socket");
      }
      written += charsWritten;

      loopcount++;
      if (loopcount > 500)
      {
        printf("CLIENT: looped more than 500 times, probably endless\n");
        break;
      }
    }
    printf("CLIENT: wrote everything.\n");
    // Get return message from server
    // Clear out the buffer again for reuse
    memset(buf, '\0', buf_size);
    // Read data from the socket, leaving \0 at end
    charsRead = recv(socketFD, buf, buf_size - 1, 0);
    if (charsRead < 0)
    {
      error("CLIENT: ERROR reading from socket");
    }
    printf("CLIENT: I received this from the server: \"%s\"\n", buf);
  }
  // Close the socket
  close(socketFD);
  free(buf);
  return 0;
}