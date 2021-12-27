#include <iostream>
#include <time.h>
#include <pthread.h>

using namespace std;
typedef struct {
	bool isTimeOut;
	bool stop_timer;
} Timer_flag;

void *timer( void *main_timer_flag){


	Timer_flag *time_flag;
	time_flag = (Timer_flag *)main_timer_flag;
	int msec = 0, trigger = 3000; /* 10ms */
	clock_t before = clock()/100000;
	cout<<"before: "<<before<<endl;
	time_flag->isTimeOut = false;
	cout<<"NEW THREAD"<<endl;
	do {
	  clock_t difference = clock()/100000 - before;
	  msec = difference * 100000000/ CLOCKS_PER_SEC;
	  if( time_flag->stop_timer ){
	  	cout << "Timer stop" <<endl;
	  	pthread_exit(NULL); 
	  }
	  //iterations++;
	} while ( msec < trigger );

	printf("Time taken %d seconds %d milliseconds\n",
	  msec/1000, msec%1000);
	time_flag->isTimeOut = true;
	pthread_exit(NULL); 
}

void init_timer_flag(  Timer_flag &time_flag){

	time_flag.stop_timer = false;
	time_flag.isTimeOut = false;
}

int main(){
	//Set timer
	Timer_flag *time_flag;
	int n = 0;

	init_timer_flag(  *time_flag);
	pthread_t t;
  	pthread_create(&t, NULL, timer, time_flag);
  	//Start timer
    pthread_tryjoin_np(t,NULL);
    //printf("Enter number: ");
    //printf("isTimeOut: \n",isTimeOut);
    while( n <= 10 ){
    	//printf("Enter number: ");
    	if( n == 10){
    		time_flag -> stop_timer = true;
    		//printf("AAA\n");
    	}
    	//printf("isTimeOut: %d\n",time_flag -> isTimeOut);
    	//cin >> n;
    	if( time_flag -> isTimeOut == true ){
    		init_timer_flag(  *time_flag);
    		printf("AAA\n");
    		pthread_create(&t, NULL, timer, time_flag);
    		pthread_tryjoin_np(t,NULL);

    	}

    }
    printf("isTimeOut: %d\n",time_flag -> isTimeOut);


	return 0;
}