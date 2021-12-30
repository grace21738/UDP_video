
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
#include <fcntl.h>
#include "opencv2/opencv.hpp"

using namespace std;
using namespace cv;

#define PORT 8888 /* Port that will be opened */
#define A_PORT 1234
#define MAXDATASIZE 1000 /* Max number of bytes of data */

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
    char data[MAXDATASIZE];
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

void print_debug_string( string str , long long int buf_num, long long int frame_num){
    cout<<"str size: "<< str.size() << endl;
    cout<<"buf_num: "<< buf_num << endl;
    cout<<"frame_num: "<< frame_num << endl;
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
    string frame_data;

    int info_num = 0;
    long long int frame_amt;
    long long int frame_buf;
    long long int buf_num = 0;/*Caculate one frame size buf_num==frame_buf*/
    long long int frame_num = 0;/*Caculate total frame frame_num==frame_amt*/
    long long int height;
    long long int width;

    Mat frame;
    FILE *pf;
    uchar *iptr; 

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
                
                if( message.head.seqNumber == 1 ){
                    char *token;
                    printf("packet %s\n",message.data);
                    token = strtok(message.data," ");
                    while( token != NULL ){
                        if( info_num == 0 ) frame_amt = atoll(token);
                        if( info_num == 1 ) width = atoll(token);
                        if( info_num == 2 ) height = atoll(token);
                        if( info_num == 3 ) frame_buf = atoll(token);
                        info_num ++;
                        token = strtok(NULL, " ");
                    }
                    /*
                    cout<< "Hole frame amt: "<<frame_amt<<endl;
                    cout<< "Width: "<<width<<endl;
                    cout<< "Height: "<<height<<endl;
                    cout<< "One frame packet num: "<<frame_buf<<endl;*/

                    frame = Mat::zeros(height, width, CV_8UC3);
                    
                    if(!frame.isContinuous()) frame = frame.clone();
                    iptr = frame.data;
                }
                
                else{
                    string tmp;
                    for( int i=0; i<message.head.length; ++i )tmp.push_back(message.data[i]);
                    frame_data += tmp;
                    buf_num ++;
                    if( buf_num == 800 ){
                        pf = fopen("rcv.txt","wb");
                        fwrite(tmp.c_str(),1,tmp.size(),pf);
                        fclose(pf);
                    }
                    //cout<<"tmp size: "<< tmp.size() << endl;
                    //cout<<"buf_num: "<< buf_num << endl;
                    if( buf_num > frame_buf ) cout << "drop\t" <<"data\t" <<"#"<<message.head.seqNumber<<endl;
                }
            }

            //lost packet 
            else{
                message.head.ackNumber = success_ackNum;
                message.head.ack = 1;
                cout << "drop\t" <<"data\t" <<"#"<<message.head.seqNumber<<endl;
            }

            if( buf_num == frame_buf ){
                print_debug_string( frame_data,buf_num,frame_num );
                cout << "flush"<<endl;
                buf_num = 0;
                frame_num += 1;
                if( frame_num == 1 ){
                    pf = fopen("test_rcv.txt","wb");
                    fwrite(frame_data.c_str(),1,frame_data.size(),pf);
                    fclose(pf);
                }
                memcpy(iptr, frame_data.c_str(), frame_data.size());
                frame_data.clear();
                cout <<"iptr: "<<sizeof(iptr)<<endl;
            }
            
            if( message.head.fin == 1 ) cout << "send\t" <<"finack\t"<<endl;
            else cout << "send\t" <<"ack\t" <<"#"<<message.head.ackNumber<<endl;
            if( message.head.seqNumber == 1 )cout << "flush"<<endl;
           // if( frame_num >= 1 )imshow("Video", frame);
            //cout << "You got a message (" <<message.data<<")  from "<< inet_ntoa(agent.sin_addr)<<endl; /* prints client's IP */
            sendto(sockfd,&message,num,0,(struct sockaddr *)&agent,sin_size);
            
            if( frame_num >= 1 ){
                imshow("Video", frame);  
                waitKey(10);
            }

        }

        if( message.head.fin==1 && frame_num == frame_amt ){
            //cout<<"expect_ack_num <= message.head.ackNumber"<<expect_seqNumber <<" "<<message.head.seqNumber <<endl;
            destroyWindow("Video");
            break;
        }


    }
    /*
    cout<< "Hole frame amt: "<<frame_amt<<endl;
    cout<< "Width: "<<width<<endl;
    cout<< "Height: "<<height<<endl;
    cout<< "One frame packet num: "<<frame_buf<<endl;
    */
    close(sockfd); /* close listenfd */ 
}