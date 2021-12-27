
#include <stdio.h> /* These are the usual header files */
#include <string.h>
#include <unistd.h> /* for close() */
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <stdlib.h>
#include <iostream>
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

#define PORT 8888 /* Port that will be opened */
#define A_PORT 1234
#define MAXDATASIZE 100 /* Max number of bytes of data */

typedef struct {
    int length = 0;
    int seqNumber = 0;
    int ackNumber = 0;
    int fin = 0;
    int syn = 0;
    int ack = 0;
} header;

typedef struct{
    header head;
    char data[1000];
} segment;

int main(int argc, char *argv[])
{

    
    int sockfd; /* socket descriptors */
    struct sockaddr_in server; /* server's address information */
    struct sockaddr_in agent; /* agent's address information */
    socklen_t sin_size;
    segment message;
    int num;
    int expect_seqNumber = 1;
    if (argc != 3) {
        fprintf(stderr,"Usage: %s <receiver port> <agent port>\n", argv[0]);
        fprintf(stderr, "Example: ./receiver 8889 1234\n");
        exit(1);
    }
    /* Creating UDP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        /* handle exception */
        perror("Creating socket failed.");
        exit(1);
    }
    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    bzero(&server,sizeof(server));
    int port = atoi(argv[1]);
    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
        /* handle exception */
        perror("Bind error.");
        exit(1);
    }

    port = atoi(argv[2]);
    agent.sin_family = AF_INET;
    agent.sin_port = htons(port);
    agent.sin_addr.s_addr = inet_addr("local");
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));
    
    sin_size=sizeof(struct sockaddr_in);
    int success_ackNum = 0;
    while (1) {
        num = recvfrom(sockfd,&message,sizeof(message),0,(struct sockaddr *)&agent,&sin_size);
        if (num < 0){
            perror("recvfrom error\n");
            exit(1);
        }

        if( num > 0 ){
            if( expect_seqNumber==message.head.seqNumber ){
                message.head.ackNumber = message.head.seqNumber;
                success_ackNum = message.head.seqNumber;
                message.head.ack = 1;
                expect_seqNumber += 1;
                if( message.head.fin == 1 )
                    cout << "recv\t" <<"fin"<<endl;
                else
                    cout << "recv\t" <<"data\t" <<"#"<<message.head.seqNumber<<endl;
            }

            //lost packet 
            else{
                message.head.ackNumber = success_ackNum;
                message.head.ack = 1;
                cout << "drop\t" <<"data\t" <<"#"<<message.head.seqNumber<<endl;
            }
            if( message.head.fin == 1 )
                cout << "send\t" <<"ack\t" <<"#"<<message.head.ackNumber<<endl;
            else
                cout << "send\t" <<"finack\t"<<endl;
            cout << "You got a message (" <<message.data<<")  from "<< inet_ntoa(agent.sin_addr)<<endl; /* prints client's IP */
            sendto(sockfd,&message,num,0,(struct sockaddr *)&agent,sin_size);
        }
        if( message.head.fin == 1 )break; 
    }
    
    close(sockfd); /* close listenfd */ 
}