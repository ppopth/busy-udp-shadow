#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>

#define PORT	 8080
#define MAXLINE 1024

#define START 946684800
#define ONE_SECOND (10000000000/17)

pid_t gettid(void);

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

int main() {
    setbuf(stdout, NULL);
    int sockfd;
    char buffer[MAXLINE];
    char *hello = "Hello from server";
    struct sockaddr_in servaddr, cliaddr;

    // Creating socket file descriptor
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    // Filling server information
    servaddr.sin_family = AF_INET; // IPv4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // Bind the socket with the server address
    if ( bind(sockfd, (const struct sockaddr *)&servaddr,
                sizeof(servaddr)) < 0 )
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int len, n;

        len = sizeof(cliaddr); //len is value/result

        n = recvfrom(sockfd, (char *)buffer, MAXLINE,
                MSG_WAITALL, ( struct sockaddr *) &cliaddr,
                &len);
        buffer[n] = '\0';
        tlogf("Client : %s\n", buffer);
        sendto(sockfd, (const char *)hello, strlen(hello),
                MSG_CONFIRM, (const struct sockaddr *) &cliaddr,
                len);
        tlogf("Hello message sent.\n");
    }

    return 0;
}
