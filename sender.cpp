#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> /* netbd.h is needed for struct hostent =) */
#include <arpa/inet.h>
#include <iostream>
#include <opencv2/core/mat.hpp>
#include "opencv2/opencv.hpp"
#include <opencv2/core/types_c.h>
#include <opencv/cv.h>
#include <pthread.h>
#include <fcntl.h>
#include<unistd.h> 
#include <vector>

#define PORT 7777 /* Open Port on Remote Host */
#define A_PORT 1234
#define IP_ADDR "127.0.0.1"
#define MAXDATASIZE 1000 /* Max number of bytes of data */

using namespace std;
using namespace cv;

typedef struct {
	int length;
	int seqNumber;
	int ackNumber;
	int fin;
	int syn;
	int ack;
} header;

typedef struct{
	header head;
	char data[MAXDATASIZE];
} segment;

typedef struct {
	bool isTimeOut;
	bool stop_timer;
} Timer_flag;


void init_message(  segment &message){

	message.head.length = 0;
	message.head.seqNumber = 0;
	message.head.ackNumber = -1;
	message.head.fin = 0;
	message.head.syn = 0;
	message.head.ack = 0;
	memset( message.data, '\0', sizeof(char)*MAXDATASIZE );
	
}

void set_packet( segment &message, long long int data, int seqNum ,bool isLast){
	memset( message.data, '\0', sizeof(char)*MAXDATASIZE );
	sprintf(message.data,"%lld",data);
	message.head.length = strlen(message.data);
	message.head.seqNumber = seqNum;
	message.head.fin = 0;
	message.head.ack = 0;
	if(isLast) message.head.fin = 1;
}

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

void print_debug_vector(vector<segment> v){
	for( int i=0; i<v.size(); ++i )
		print_debug_message( v[i] );
}

void push_packet(vector<segment> &all_message,int &seqNum,long long int &data,int window_size,long long int packet_tol){

	segment message;
	for( int k=all_message.size(); k<window_size ; ++k ){
    	seqNum += 1;
    	if( seqNum == packet_tol ) 
    		set_packet( message, data, seqNum, true );
    	else 
    		set_packet( message, data, seqNum, false );
    	data += 1;
    	all_message.push_back(message);
    }



}

void *timer( void *main_timer_flag){


	Timer_flag *time_flag;
	time_flag = (Timer_flag *)main_timer_flag;
	long long int msec = 0, trigger = 3000; /* 10ms */
	clock_t before = clock()/100000;
	time_flag -> stop_timer = false;
	//cout<<"NEW THREAD"<<endl;
	do {
	  clock_t difference = clock()/100000 - before;
	  msec = difference * 100000000/ CLOCKS_PER_SEC;
	  if( time_flag->stop_timer ){
	  	//cout << "time stop" <<endl;
	  	pthread_exit(NULL); 
	  }
	  //iterations++;
	} while ( msec < trigger );

	printf("Time taken %lld seconds %lld milliseconds\n",msec/1000, msec%1000);
	time_flag->isTimeOut = true;
	pthread_exit(NULL); 
}

void init_timer_flag( Timer_flag *time_flag){

	time_flag->stop_timer = false;
	time_flag->isTimeOut = false;
}

int main(int argc, char *argv[])
{
    
    int sockfd, numbytes =0 ; /* files descriptors */
    int window_size = 1;
    vector<segment> all_message;
    char file_name[MAXDATASIZE] = "video.mpg";
    char file_path[2*MAXDATASIZE]; 
    segment message;
    char agent_ip[50];
    struct hostent *he; /* structure that will get information about remote host */
    struct sockaddr_in server,agent; /* server's address information */
    int expect_ack_num =  0;
    int seqNum = 0;
    int threshold = 2;
    int ack_buf_size = 0;
    bool right_ack = true;

    Timer_flag *time_flag;
    time_flag = new Timer_flag();
    
    /* this is used because our program will need two argument (IP address and a message */
    
    if (argc != 3) {
        fprintf(stderr,"Usage: %s <sender port> <agent port>\n", argv[0]);
        fprintf(stderr, "Example: ./sender 8889 1234\n");
        exit(1);
    }
    
    
    if ((sockfd=socket(AF_INET, SOCK_DGRAM, 0))==-1){ // calls socket()
        printf("socket() error\n");
        exit(1);
    }
    
    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    
    bzero(&server,sizeof(server));
    int port = atoi(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(port); /* htons() is needed again */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
        /* handle exception */
        perror("Bind error.");
        exit(1);
    }

    agent.sin_family = AF_INET;
    port = atoi(argv[2]);
    agent.sin_port = htons(port);
    agent.sin_addr.s_addr = inet_addr(IP_ADDR);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));

    socklen_t len;
    len=sizeof(struct sockaddr_in);
    init_message(message);
 
    sprintf(file_path,"./%s",file_name);

//=========VIDEO SENDING=============
    VideoCapture cap(file_path); // open the default camera
    if( !cap.isOpened() ){
        cout << "Read video Failed." << endl;
    }
    Mat frame;
    long long int frame_num = cap.get(CV_CAP_PROP_FRAME_COUNT);
    long long int packet_tol = 11;
    //give frame_num -> frame spilt to packets;
/*
    vector<uchar> data_encode;
    cap.read(frame);
    for( int i=0; i<frame_num-1; ++i ){
    	cap.read(frame);
    	imshow("video_test",frame);
        waitKey(10);
    }
    destroyWindow("video_test");
    cap.release();

  */ 
    //Set timer
	pthread_t t;
	init_timer_flag( time_flag);
    long long int tmp = 100;
    push_packet(all_message,seqNum,tmp,window_size,packet_tol);
    expect_ack_num = all_message[0].head.seqNumber;
	frame_num = 100;
	//==========LOOP===================
    while(1) {

   	//++++++++++++RETRANSMIT+++++++++++++++++
   		if( time_flag->isTimeOut == true || (ack_buf_size == 0 && right_ack == true)){
   			//TIME OUT CHANGE WINDOW SIZE
   			if( time_flag->isTimeOut == true  ){
	   			if( window_size > 1 ) threshold  = window_size/2;
	   			else if( window_size == 1 ) threshold  = 1;
	   			window_size = 1;
	   			ack_buf_size = 0;
	   			cout << "time \tout,\t\t"<<"threshold = "<<threshold << endl;
	   		}
	   		//transmit
	   		for( int j=0; j<all_message.size()&&j<window_size; ++j ){
	   			//Set Timer
	   			if( j==0 ) 
	   			{
	   				pthread_create(&t, NULL, timer, time_flag);
	    			pthread_tryjoin_np(t,NULL);
	   			}

	   			if( message.head.fin != 1 ){
		   			if( time_flag->isTimeOut == false )
		   				cout << "send\t" <<"data\t" <<"#"<<all_message[j].head.seqNumber<<",\t"<<"winSize = "<<window_size<<endl;
		   			else if
		   				cout << "resnd\t" <<"data\t" <<"#"<<all_message[j].head.seqNumber<<",\t"<<"winSize = "<<window_size<<endl;
		   		}
		   		else{
		   			if( time_flag->isTimeOut == false )
		   				cout << "send\t" <<"fin\t" <<endl;
		   			else if
		   				cout << "resnd\t" <<"fin\t" <<endl;

		   		}
	        	sendto(sockfd,&all_message[j],sizeof(all_message[j]),0,(struct sockaddr *)&agent,len);
	        }
	        right_ack = false;
	        time_flag -> isTimeOut = false;
	    }
	//===========RETRANSMIT==============
	//****Settings of recvfrom****
	  	fd_set readfds;
    	fcntl(sockfd,F_SETFL, O_NONBLOCK);
	    FD_ZERO(&readfds);
	    FD_SET(sockfd,&readfds);
	    unsigned int sec = 3;
	    unsigned int usec = 0;
	    struct timeval tv;
	    tv.tv_sec = sec;
	    tv.tv_usec = usec;
	    
	    int rv = select(sockfd+1, &readfds, NULL,NULL,&tv);
	    if( FD_ISSET(sockfd, &readfds) ){
	    	
	        if ((numbytes=recvfrom(sockfd,&message,sizeof(message),0,(struct sockaddr *)&agent,&len)) == -1){ 
	            //printf("recvfrom() error\n");
	            perror("recvfrom() error\n");
	            exit(1);
	        }
	        cout << "recv\t" <<"ack\t" <<"#"<<message.head.ackNumber<<endl;
	    }
	//========Settings of recvfrom===========
       
    //********Receiver get expected packet********
        if( expect_ack_num <= message.head.ackNumber){

        	//Erase ack packet
        	vector<segment>::iterator it;
        	it = all_message.begin();
        	all_message.erase(it);
        	//Change expected ack num
        	expect_ack_num += 1;
        	ack_buf_size += 1;
        	right_ack = true;
        	if( seqNum < packet_tol ) push_packet(all_message,seqNum,tmp,window_size,packet_tol);
        	//Reset Timer
        	time_flag -> stop_timer = true;
        	pthread_create(&t, NULL, timer, time_flag);
    		pthread_tryjoin_np(t,NULL);
        }

        //Whether set high window size
        if( window_size == ack_buf_size ){
        	//Change window size
        	if( window_size>threshold ) window_size += 1;
        	else window_size *= 2;
        	ack_buf_size = 0;
        	//Push new packet in all_message
        	if( seqNum < packet_tol ) push_packet(all_message,seqNum,tmp,window_size,packet_tol);
        }
       
       //FINISHED
        if( message.head.fin==1 ) break;
    }
    //========Receiver get expected packet=========

    close(sockfd); /* close fd */
}