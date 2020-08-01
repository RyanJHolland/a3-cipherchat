/*
Ryan Holland
OSU ID 933636892
Operating Systems class
Assignment 3
*/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
/*
#include <sys/types.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
*/

// Prints a random arr of chars A-Z and space, of length arg[1], with '\n' at the end.
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        printf("USAGE: keygen keylength\n");
        return 1;
    }
    int buf_len = atoi(argv[1]);
    int i;
    char *buf = (char *)malloc(buf_len * sizeof(char));
    if (buf == NULL)
    {
        printf("Memory not allocated.\n");
        return 1;
    }

    // random seed
    srand(time(0));

    // fill the buffer with random chars
    for (i = 0; i < buf_len; i++)
    {
        buf[i] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ "[rand() % 27];
    }
    printf("%s\n", buf);
    free(buf);
    return 0;
}
