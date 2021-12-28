
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

void print_debug_message( segment &message ){
    cout <<"============================================"<<endl;
    cout << "message.head.length: " << message.head.length << endl;
    cout << "message.head.seqNumber: " << message.head.seqNumber << endl;
    cout << "message.head.ackNumber: " << message.head.ackNumber << endl;
    cout << "message.head.fin: " << message.head.fin << endl;
    cout << "message.head.syn: " << message.head.syn << endl;
    cout << "message.head.ack: " << message.head.syn << endl;
    cout << "message.data: " << message.data << endl;
    cout <<"============================================"<<endl;
}

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

    long long int frame_tol;
    long long int frame_buf;
    long long int buf_num = 0;/*Caculate one frame size buf_num==frame_buf*/
    long long int frame_num = 0;/*Caculate total frame frame_num==frame_amt*/
    long long int height;
    long long int width;

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
                if( message.head.fin!=1 ) expect_seqNumber += 1;
                //print_debug_message( message );
                if( message.head.fin == 1 )
                    cout << "recv\t" <<"fin"<<endl;
                else
                    cout << "recv\t" <<"data\t" <<"#"<<message.head.seqNumber<<endl;
                
                if( message.head.seqNumber == 1 )
                    frame_tol = atoll(message.data);
                else if( message.head.seqNumber == 2 )
                    width = atoll(message.data);
                else if( message.head.seqNumber == 3 )
                    height = atoll(message.data);
                else if( message.head.seqNumber == 4 )
                    frame_buf = atoll(message.data);
            }

            //lost packet 
            else{
                message.head.ackNumber = success_ackNum;
                message.head.ack = 1;
                cout << "drop\t" <<"data\t" <<"#"<<message.head.seqNumber<<endl;
            }
            if( message.head.fin == 1 )
                cout << "send\t" <<"finack\t"<<endl;
            else
                cout << "send\t" <<"ack\t" <<"#"<<message.head.ackNumber<<endl;

            //cout << "You got a message (" <<message.data<<")  from "<< inet_ntoa(agent.sin_addr)<<endl; /* prints client's IP */
            sendto(sockfd,&message,num,0,(struct sockaddr *)&agent,sin_size);
        }
        if( message.head.fin==1 && expect_seqNumber==message.head.seqNumber ){
            //cout<<"expect_ack_num <= message.head.ackNumber"<<expect_seqNumber <<" "<<message.head.seqNumber <<endl;
            break;
        }

    }
    cout<< "Hole frame amt: "<<frame_tol<<endl;
    cout<< "Width: "<<width<<endl;
    cout<< "Height: "<<height<<endl;
    cout<< "One frame packet num: "<<frame_buf<<endl;
    close(sockfd); /* close listenfd */ 
}