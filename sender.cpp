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
#include "opencv2/opencv.hpp"
#include <pthread.h>
#include <fcntl.h>
#include<unistd.h> 
#include <vector>
#include <queue>

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

void set_packet( segment &message, char data[MAXDATASIZE], int seqNum , int length,bool isLast){
	//cout << "BBBBB: " <<endl;
	memset( message.data, '\0', sizeof(char)*MAXDATASIZE );
	memcpy(message.data,data,length);
	//cout << "CCCCCC: " <<endl;
	message.head.length = length;
	message.head.seqNumber = seqNum;
	message.head.ack = 0;
	if(isLast) message.head.fin = 1;
	else message.head.fin = 0;
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

void push_packet(vector<segment> &all_message,int &seqNum,char data[MAXDATASIZE],int frame_data_byte){

	segment message;
	seqNum += 1;
	set_packet( message, data, seqNum,frame_data_byte, false );
	all_message.push_back(message);
    

}

void *timer( void *main_timer_flag){


	Timer_flag *time_flag;
	time_flag = (Timer_flag *)main_timer_flag;
	long long int msec = 0, trigger = 1000; /* 10ms */
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

	//printf("Time taken %lld seconds %lld milliseconds\n",msec/1000, msec%1000);
	time_flag->isTimeOut = true;
	pthread_exit(NULL); 
}

void init_timer_flag( Timer_flag *time_flag){

	time_flag->stop_timer = false;
	time_flag->isTimeOut = false;
}

void print_send_message( int seqNumber,bool isFinish,  bool isTimeOut,int window_size,queue<int> &resnd_seq){
	if( ! isFinish ){
		if( isTimeOut == true || (resnd_seq.size()>0 && resnd_seq.back()>=seqNumber )){
			cout << "resnd\t" <<"data\t\t" <<"#"<<seqNumber<<",\t\t"<<"winSize = "<<window_size<<endl;
		}
		else
			cout << "send\t" <<"data\t\t" <<"#"<<seqNumber<<",\t\t"<<"winSize = "<<window_size<<endl;
	}
	else{
		cout << "send\t" <<"fin\t" <<endl;
	}
}

int main(int argc, char *argv[])
{
    
    int sockfd, numbytes =0 ; /* files descriptors */
    int window_size = 1;
    vector<segment> all_message;
    char file_name[MAXDATASIZE] = "p1f.mpg";
    char file_path[2*MAXDATASIZE];
    char packet[MAXDATASIZE];
    segment message;
    char agent_ip[50];
    struct sockaddr_in server,agent; /* server's address information */
    int expect_ack_num =  0;
    int seqNum = 0;
    int last_num = 0;
    int threshold = 2;
    int ack_buf_size = 0;
    bool right_ack = true;
    bool isFinish = false;
    bool isLast = false;
    int time_out_num = 0;
    long long int frame_rate = 1;
    queue<int> resnd_seq ;
    bool isRetransmit = false;

    Timer_flag *time_flag;
    time_flag = new Timer_flag();
    
    /* this is used because our program will need two argument (IP address and a message */
    
    if (argc != 4 && argc != 5) {
        fprintf(stderr,"Usage: %s <video name> <sender port> <agent port>\n", argv[0]);
        fprintf(stderr, "Example: ./sender 8889 1234\n");
        exit(1);
    }
    else if(argc == 5) {
        frame_rate = atoll(argv[4]);
        if( frame_rate> 200 || frame_rate< 1 ){
        	fprintf(stderr,"Usage: %s <video name> <sender port> <agent port> <frame rate>\n", argv[0]);
        	fprintf(stderr,"<frame rate> should be in range of 1~200.\n");
        	fprintf(stderr, "Example: ./sender video.mpg 8889 1234 3\n");
        	exit(1);
        }
    }
    
    memcpy(file_name,argv[1],strlen(argv[1]));
    if ((sockfd=socket(AF_INET, SOCK_DGRAM, 0))==-1){ // calls socket()
        printf("socket() error\n");
        exit(1);
    }
    
    int yes = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    
    bzero(&server,sizeof(server));
    int port = atoi(argv[2]);
    server.sin_family = AF_INET;
    server.sin_port = htons(port); /* htons() is needed again */
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    

    if (bind(sockfd, (struct sockaddr *)&server, sizeof(struct sockaddr)) == -1) {
        /* handle exception */
        perror("Bind error.");
        exit(1);
    }

    agent.sin_family = AF_INET;
    port = atoi(argv[3]);
    agent.sin_port = htons(port);
    agent.sin_addr.s_addr = inet_addr(IP_ADDR);
    memset(agent.sin_zero, '\0', sizeof(agent.sin_zero));

    socklen_t len;
    len=sizeof(struct sockaddr_in);
    init_message(message);
 
    sprintf(file_path,"./%s",file_name);
//=====================================================
    //
//=========VIDEO SENDING===============================
    VideoCapture cap(file_path); // open the default camera
    if( !cap.isOpened() ){
        cout << "Read video Failed." << endl;
    }

    // Get the size of a frame in bytes
    Mat frame;
    long long int frame_amt = cap.get(CV_CAP_PROP_FRAME_COUNT) - 1;
    int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    int height= cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    frame = Mat::zeros(width, height, CV_8UC3);
    if(!frame.isContinuous()) frame = frame.clone();
	long long frame_size = frame.total() * frame.elemSize();
	long long int frame_buf = ceil(double(frame_size)/double(MAXDATASIZE));
    long long int buf_num = 0;/*Caculate one frame size buf_num==frame_buf*/
    long long int frame_num = 0;/*Caculate total frame frame_num==frame_amt*/
    long long int packet_num = 0;

    int send_frame_data = 0;
    int frame_data_byte = 0;
    char frame_data[frame_size];
    //Set frame amount
    frame_amt /= frame_rate;
    //cout << "frame_amt: " << frame_amt<<endl;


 
    //Set timer
	pthread_t t;
	init_timer_flag( time_flag);
	
	//memset(packet,'\0',MAXDATASIZE);
    sprintf(packet,"%lld %d %d %lld",frame_amt,width,height,frame_buf);
    seqNum += 1;
	set_packet( message, packet, seqNum, strlen(packet) , false );
	all_message.push_back(message);
	//printf("message.data: %s\n",message.data);
    expect_ack_num = all_message[0].head.seqNumber;

	cap >> frame;
	memcpy(frame_data, frame.data, frame_size);
	right_ack = true;
	//==========LOOP===================
    while(1) {

   	//++++++++++++Transmit and RETRANSMIT+++++++++++++++++
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
	   				//time_out_num = all_message[j].head.seqNumber;
	   				pthread_create(&t, NULL, timer, time_flag);
	    			pthread_detach(t);
	   			}
	   			print_send_message( all_message[j].head.seqNumber,isFinish,  time_flag->isTimeOut,window_size,resnd_seq);
	        	sendto(sockfd,&all_message[j],sizeof(all_message[j]),0,(struct sockaddr *)&agent,len);
	        	if( right_ack == true)resnd_seq.push(all_message[j].head.seqNumber);
	        	//if( isRetransmit && all_message[j].head.seqNumber>resnd_seq.back() ) resnd_seq.push(all_message[j].head.seqNumber);
	        	//if( isRetransmit ) resnd_seq.pop();
	        }

	        right_ack = false;
	        time_flag -> isTimeOut = false;
	    }
	//===========Transmit and RETRANSMIT==============
	    //cout <<"all_message.size(): "<<all_message.size()<<endl;
	    //cout <<"all_message.size(): "<<all_message.size()<<endl;
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
	    //cout <<"BBBBBB"<<endl;
	    if( FD_ISSET(sockfd, &readfds) ){
	    	
	        if ((numbytes=recvfrom(sockfd,&message,sizeof(message),0,(struct sockaddr *)&agent,&len)) == -1){ 
	            //printf("recvfrom() error\n");
	            perror("recvfrom() error\n");
	            exit(1);
	        }
	        if( message.head.fin == 1 )
	        	cout << "recv\t" <<"finack\t" <<endl;
	        else
	        	cout << "recv\t" <<"ack\t\t" <<"#"<<message.head.ackNumber<<endl;
	    }
	//========Settings of recvfrom===========
    //********Receiver get expected packet********
        if( expect_ack_num <= message.head.ackNumber){

        	//Erase ack packet
        	vector<segment>::iterator it;
        	it = all_message.begin();
        	all_message.erase(it);
        	if(resnd_seq.size()>0)resnd_seq.pop();
        	//Change expected ack num
        	if( !isFinish && message.head.fin != 1 )expect_ack_num = message.head.ackNumber + 1;
        	ack_buf_size += 1;  //right_ack = true;
        	if( frame_num <= frame_amt ){
        		for( int k=all_message.size(); k<window_size ; ++k ){
        			if( buf_num < frame_buf ){
        				buf_num += 1;
	        			if( send_frame_data + MAXDATASIZE <= frame_size ) frame_data_byte = MAXDATASIZE;
	        			else frame_data_byte = frame_size - send_frame_data;
	        			memcpy( packet,frame_data + send_frame_data,frame_data_byte );
	        			push_packet(all_message,seqNum,packet,frame_data_byte);
	        			send_frame_data += frame_data_byte;
	        		}


	        		if( buf_num == frame_buf ){
	        			buf_num = 0;send_frame_data = 0;
	        			frame_num += 1;
	        			if( frame_num > frame_amt ) { last_num = seqNum;break;}
	        			for( long long int s = 0; s<frame_rate; ++s) cap >> frame;
						memcpy(frame_data, frame.data, frame_size);
	        		}

        		}
        		
        	}
        	//Reset Timer
	        time_flag -> stop_timer = true;
        	if( all_message.size()!=0 ){
        		//pthread_cancel(t);
        		//time_out_num = message.head.ackNumber + 1;
        		
	        	if( pthread_create(&t, NULL, timer, time_flag)!=0){
	        		perror("Error: pthread_create\n");
	        	}
	    		pthread_detach(t); 
	    	}
	    	if( resnd_seq.size()==0 )isRetransmit = false;
	    	if( !isLast ){
        		segment tmp_mes;
		    	tmp_mes = all_message[window_size-1];
			    sendto(sockfd,&tmp_mes,sizeof(tmp_mes),0,(struct sockaddr *)&agent,len);
		       	print_send_message( tmp_mes.head.seqNumber,isFinish,  time_flag->isTimeOut,window_size,resnd_seq);
		        if( resnd_seq.size() == 0 || resnd_seq.back()<tmp_mes.head.seqNumber)resnd_seq.push(tmp_mes.head.seqNumber);
    			if( last_num==tmp_mes.head.seqNumber ) isLast = true;
    		}

        }
        else{
        	isRetransmit = true;
        }
    //========Receiver get expected packet=========

        //Whether set high window size
        if( window_size == ack_buf_size ){
        	int pre_window_size = window_size;
        	//Change window size
        	if( window_size >= threshold ) window_size += 1;
        	else window_size *= 2;
        	ack_buf_size = 0;
        	if( frame_num <= frame_amt ){
        		for( int k=all_message.size(); k<window_size ; ++k ){
        			if( buf_num < frame_buf ){
        				buf_num += 1;
	        			if( send_frame_data + MAXDATASIZE <= frame_size ) frame_data_byte = MAXDATASIZE;
	        			else frame_data_byte = frame_size - send_frame_data;
	        			memcpy( packet,frame_data + send_frame_data,frame_data_byte );
	        			push_packet(all_message,seqNum,packet,frame_data_byte);
	        			send_frame_data += frame_data_byte;
	        		}


	        		if( buf_num == frame_buf ){
	        			buf_num = 0;send_frame_data = 0;
	        			frame_num += 1; 
	        			if( frame_num > frame_amt ){ last_num = seqNum;break;}
	        			for( long long int s = 0; s<frame_rate; ++s) cap >> frame;
						memcpy(frame_data, frame.data, frame_size);
	        		}

        		}
        	}
        	if( !isLast ){
        		for( int k=pre_window_size; k<all_message.size()&&k<window_size; ++k ){
        		
	        		sendto(sockfd,&all_message[k],sizeof(all_message[k]),0,(struct sockaddr *)&agent,len);
		        	//cout << "WIDOW: send\t" ;
		        	print_send_message( all_message[k].head.seqNumber,isFinish,  time_flag->isTimeOut,window_size,resnd_seq);
		        	if( resnd_seq.size() == 0 ||resnd_seq.back()<all_message[k].head.seqNumber)resnd_seq.push(all_message[k].head.seqNumber);
	        		if( last_num==all_message[k].head.seqNumber ) isLast = true;
	        	}
	        }
        	
        }
        //FINISHED Need set finish
        if( !isFinish && frame_num > frame_amt && all_message.size()==0){
        	time_flag -> stop_timer = true;
        	isFinish = true; seqNum += 1; ack_buf_size = 0;
        	//cout << "FINISH" <<endl;
        	right_ack = true;
	        set_packet( message, "fin", seqNum ,4,true);
	        all_message.push_back(message);
        }

        if( all_message.size()==0 && isFinish  &&  expect_ack_num == message.head.ackNumber){
        	//cout << "expect_ack_num: " << expect_ack_num<<endl;
        	break;
     	}
    }
    

    close(sockfd); /* close fd */
}

