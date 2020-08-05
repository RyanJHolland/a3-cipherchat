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

///////////////////////////////////////
//              MAIN                //
/////////////////////////////////////
int main(int argc, char *argv[])
{
  int socketFD, charsRead, i, ch;
  struct sockaddr_in serverAddress;
  FILE *fp;
  int buf_size = 256;
  char *buf = malloc(buf_size);

  // Validate cmd line args
  if (argc < 4)
  {
    free(buf);
    fprintf(stderr, "USAGE: %s plaintext key port\n", argv[0]);
    exit(0);
  }

  ///////////// READ MESSAGE FILE /////////////
  // Get FILE pointer to plaintext message file
  fp = fopen(argv[1], "r");
  if (fp == NULL)
  {
    free(buf);
    fprintf(stderr, "file not found: %s\n", argv[1]);
    exit(0);
  }

  // Read plaintext msg into buf
  int msg_size = 0;
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
      msg_size++;
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

  ///////////// READ KEY FILE /////////////
  // Get FILE pointer to key file
  fp = fopen(argv[2], "r");
  if (fp == NULL)
  {
    fprintf(stderr, "file not found: %s\n", argv[2]);
    exit(0);
  }

  // Read key into buf -- also record its size
  int key_size = 0;
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
        error("CLIENT: out of memory!");
      }
    }
    // validate character
    if ((ch > 64 && ch < 91) || (ch == 32)) // from A-Z or a space
    {
      buf[i] = ch;
      i++;
      key_size++;
    }
    else
    {
      free(buf);
      printf("CLIENT: character %c in file %s was invalid.\n", ch, argv[2]);
      error("CLIENT: invalid character input! Exiting...");
    }
  }

  // Remove trailing newline, if present
  if (buf[i - 1] == '\n')
  {
    buf[i - 1] = '\0';
    i--;
  }
  else
  {
    buf[i] = '\0';
  }

  if (key_size < msg_size)
  {
    free(buf);
    printf("CLIENT: Error: key %s is too short\n", argv[2]);
    error("CLIENT: Exiting...");
  }
  //printf("%s\n", buf);

  ////////////// SEND TO SERVER /////////////

  //////////// Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0)
  {
    free(buf);
    perror("CLIENT: ERROR opening socket");
    exit(2);
  }

  // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
  {
    close(socketFD);
    free(buf);
    perror("CLIENT: ERROR connecting to enc_server");
    exit(2);
  }

  /////////////// Write introduction to server  (tell server which client this is)
  /*
  int intro = sendstring(socketFD, "enc_client");
  printf("CLIENT: intro = %d\n", intro);
  if (intro < 0)
  {
    close(socketFD);
    free(buf);
    perror("CLIENT: ERROR introducing self to server");
    exit(2);
  }
  // Listen for confirmation
  int confirmation = -1;
  int correct_server = -1;
  confirmation = recv(socketFD, &correct_server, sizeof(int), 0);
  //printf("CLIENT: confirmation = %d\n", confirmation);
  //printf("CLIENT: correct_server = %d\n", correct_server);
  if (confirmation < 1)
  {
    close(socketFD);
    free(buf);
    perror("CLIENT: ERROR, getting no response to intro from server");
    exit(2);
  }
  else if (correct_server != -255)
  {
    close(socketFD);
    free(buf);
    printf("correct_server = %d\n", correct_server);
    perror("CLIENT: ERROR, not connecting to the right server");
    exit(2);
  }
  //printf("%s\n", buf);
*/
  /////////////// Write message length to the server
  int msg_len = i;
  //printf("CLIENT: sending msg_len, which is: %d\n", msg_len);
  int sent_msg_len = write(socketFD, &msg_len, 1);
  if (sent_msg_len < 0)
  {
    close(socketFD);
    free(buf);
    error("CLIENT: ERROR sending msg_len");
  }

  /////////////// Write message to the server
  //printf("CLIENT: sending buf\n");
  sendstring(socketFD, buf);

  /////////////// Get return message from server
  // Clear out the buf again for reuse
  memset(buf, '\0', buf_size);
  // Read data from the socket, leaving \0 at end
  charsRead = recv(socketFD, buf, buf_size - 1, 0);
  if (charsRead < 0)
  {
    error("CLIENT: ERROR reading from socket");
  }
  printf("%s\n", buf);

  /////////////// Close the socket
  close(socketFD);
  free(buf);
  return 0;
}