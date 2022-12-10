#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>

#define PORT	 8080
#define MAXLINE 1024
#define str(s) #s
#define xstr(s) str(s)

#define START 946684800
#define ONE_SECOND (10000000000/17)

pid_t gettid(void);

struct sockaddr_in servaddr;
char *hello = "Hello from client";

void burn_cpu(uint64_t max_counter) {
    uint64_t counter = 0;
    while(counter < max_counter) counter++;
}

int tlogf(const char *format, ...) {
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);

    uint64_t us = time.tv_sec * 1000000 + time.tv_nsec / 1000;
    printf("(thread: %d, time: %lf) ", gettid(), (double)us / 1000000 - START);

    va_list args;
    va_start(args, format);
    int rv = vprintf(format, args);
    va_end(args);
    return rv;
}

void *func(void *vargp) {
    int sockfd;
    char buffer[MAXLINE];

    burn_cpu(ONE_SECOND * 10);
    tlogf("After burning cpu.\n");

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    int n, len;
    sendto(sockfd, (const char *)hello, strlen(hello),
            MSG_CONFIRM, (const struct sockaddr *) &servaddr,
            sizeof(servaddr));
    tlogf("Hello message sent.\n");

    burn_cpu(ONE_SECOND * 10);
    tlogf("After burning cpu.\n");

    n = recvfrom(sockfd, (char *)buffer, MAXLINE,
            MSG_WAITALL, (struct sockaddr *) &servaddr,
            &len);
    buffer[n] = '\0';
    tlogf("Server : %s\n", buffer);

    close(sockfd);
}

int main(int argc, char *argv[]) {
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    if (argc < 2) {
        printf("no server is provided\n");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    struct addrinfo *addrs = NULL;
    struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_DGRAM,
    };
    getaddrinfo(argv[1], xstr(PORT), &hints, &addrs);
    servaddr = *((struct sockaddr_in*)addrs->ai_addr);
    freeaddrinfo(addrs);

    int num_threads = 2;
    pthread_t *tids = malloc(num_threads * sizeof(pthread_t));

    for(int i = 0; i < num_threads; i++) {
        pthread_create(&tids[i], NULL, func, NULL);
    }
    for(int i = 0; i < num_threads; i++) {
        pthread_join(tids[i], NULL);
    }
    return 0;
}
