/*
** server.c - a stream socket server demo
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#define SELECTPORT 55000 // the port users will be connecting to
#define BACKLOG 10// how many pending connections queue will hold
#define NUM_SERVERS 1

void sigchld_handler(int s)
{
    while(wait(NULL) > 0);
}

int main(void)
{
    int sockfd, new_fd; // listen on sock_fd, new connection on new_fd
    struct sockaddr_in my_addr; // my address information
    struct sockaddr_in their_addr; // connector’s address information
    int sin_size;
    struct sigaction sa;
    int yes=1,i,ctr_code_n;
	char ctr_buff[8];

    //char *serverList[NUM_SERVERS] = {"64.9.225.153","64.9.225.142","64.9.225.166","64.9.225.179","74.63.50.34","74.63.50.40","74.63.50.21","4.71.251.149","4.71.251.175","4.71.251.162","38.102.0.87","38.102.0.85","38.102.0.111"};
    char *serverList[NUM_SERVERS] = {"127.0.0.1"};

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }
    my_addr.sin_family = AF_INET; // host byte order
    my_addr.sin_port = htons(SELECTPORT); // short, network byte order
    my_addr.sin_addr.s_addr = INADDR_ANY; // automatically fill with my IP
    memset(&(my_addr.sin_zero), '\0', 8); // zero the rest of the struct
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr))    == -1) {
        perror("bind");
        exit(1);
    }
    
    if (listen(sockfd, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    while(1) { // main accept() loop

        sin_size = sizeof(struct sockaddr_in);
        if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr,&sin_size)) == -1) {
            perror("accept");
            continue;
        }

        printf("server: got connection from %s\n",
        inet_ntoa(their_addr.sin_addr));
        
        if (!fork()) { // this is the child process
 // child doesn’t need the listener
		close(sockfd);
		ctr_code_n = htonl(NUM_SERVERS);
		memcpy((void*)ctr_buff, &ctr_code_n, sizeof(ctr_code_n));
		if (send(new_fd, ctr_buff, sizeof(int), 0) == -1)
	                perror("send");
		printf("\nSent size");
	    for(i=0;i<NUM_SERVERS;i++)
	    {
				printf("\nSending %d bytes",strlen(serverList[i]));
				ctr_code_n = htonl(strlen(serverList[i]));
				memcpy((void*)ctr_buff, &ctr_code_n, sizeof(ctr_code_n));
				if (send(new_fd, ctr_buff, sizeof(int), 0) == -1)
	                perror("send");
 	            if (send(new_fd, serverList[i], strlen(serverList[i]), 0) == -1)
	                perror("send");
	    }
		printf("\nSent server List");
            close(new_fd);
            exit(0);
        }

        close(new_fd); // parent doesn’t need this
    }
    return 0;
}
