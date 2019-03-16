/*
** client.c -- a stream socket client demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <pthread.h>
#include <unistd.h>

#include "queue.c"

#define PORT "3490" // the port client will be connecting to 

#define MAXDATASIZE 100 // max number of bytes we can get at once 

#define QUEUEDATANUM 100

#define THREADNUM 10

pthread_mutex_t queueMutex=PTHREAD_MUTEX_INITIALIZER;

Queue q;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void *sendSomething(void *arg)
{

    int sockfd, numbytes;  
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN];

    ElementType message;

    char *hostname = (char *)arg;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostname, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return NULL;
    }


    while(1)
    {
        pthread_mutex_lock(&queueMutex);
        if (!IsEmpty( q ))
        {
            message = Front( q );
            Dequeue( q );
            pthread_mutex_unlock(&queueMutex);
        }
        else
        {
            pthread_mutex_unlock(&queueMutex);
            break;
        }

        // loop through all the results and connect to the first we can
        for(p = servinfo; p != NULL; p = p->ai_next) {
            if ((sockfd = socket(p->ai_family, p->ai_socktype,
                    p->ai_protocol)) == -1) {
                perror("client: socket");
                continue;
            }

            if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
                close(sockfd);
                perror("client: connect");
                continue;
            }
            break;
        }

        if (p == NULL) {
            fprintf(stderr, "client: failed to connect\n");
            continue;
        }

        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
                s, sizeof s);
        printf("client thread %ld: connecting to %s\n", pthread_self(), s);

        memset(buf, 0, sizeof(buf));
        sprintf(buf, "thread %ld with message %d", pthread_self(), message);

        if (send(sockfd, buf, strlen(buf), 0) == -1)
        {
            perror("send");
            close(sockfd);
            continue;
        }

        if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1) 
        {
            perror("recv");
            close(sockfd);
            continue;
        }

        buf[numbytes] = '\0';

        printf("client: received '%s'\n",buf);

        close(sockfd);

    }

    freeaddrinfo(servinfo); // all done with this structure
    return NULL;
}



int main(int argc, char *argv[])
{

    pthread_t sendThread[THREADNUM];

    int queueIsEmpty;

    q = CreateQueue(QUEUEDATANUM);

    for (int i = 0; i < QUEUEDATANUM; ++i)
    {
        ElementType message = QUEUEDATANUM - i;

        Enqueue( message, q );
    }


    if (argc != 2) {
        fprintf(stderr,"usage: client hostname\n");
        exit(1);
    }

    for (int i = 0; i < THREADNUM; ++i)
    {

        if (! pthread_create( &sendThread[i], NULL, sendSomething, argv[1]) ) 
        {
            printf("%ld\n", &sendThread[i]);
        }
        else
        {
            printf("err to create thread\n");
        }
    }

    for (int i = 0; i < THREADNUM; ++i)
    {
        pthread_join ( sendThread[i], NULL );
    }

    return 0;
}


