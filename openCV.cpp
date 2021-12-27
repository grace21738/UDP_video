#include "opencv2/opencv.hpp"
#include<iostream>

using namespace std;
using namespace cv;

int main(int argc, char *argv[]){
    Mat server_img,client_img;
    VideoCapture cap("./p1f.mpg");

    
    // Get the resolution of the video
    long long int frame_num = cap.get(CV_CAP_PROP_FRAME_COUNT);
    int width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
    int height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
    cout << "Video resolution: " << width << ", " << height << endl;
    
    // Allocate container to load frames 
    server_img = Mat::zeros(height, width, CV_8UC3);    
    client_img = Mat::zeros(height, width, CV_8UC3);
 
    // Ensure the memory is continuous (for efficiency issue.)
    if(!server_img.isContinuous()){
         server_img = server_img.clone();
    }

    if(!client_img.isContinuous()){
         client_img = client_img.clone();
    }

    for( int i=0; i<frame_num-1; ++i ){
        // Get a frame from the video to the container of the server.
        cap >> server_img;
        
        // Get the size of a frame in bytes 
        int imgSize = server_img.total() * server_img.elemSize();
        cout << "imgSize: "<<imgSize <<endl;
        // Allocate a buffer to load the frame (there would be 2 buffers in the world of the Internet)
        uchar buffer[imgSize];
        
        // Copy a frame to the buffer
        memcpy(buffer, server_img.data, imgSize);
        
        // Here, we assume that the buffer is transmitted from the server to the client
        // Copy a fream from the buffer to the container of the client
        uchar *iptr = client_img.data;
        memcpy(iptr, buffer, imgSize);
      
        // show the frame 
        imshow("Video", client_img);  
        
        // Press ESC on keyboard to exit
        // Notice: this part is necessary due to openCV's design.
        // waitKey function means a delay to get the next frame. You can change the value of delay to see what will happen
        char c = (char)waitKey(33.3333);
        if(c==27)
                break;
    }

	cap.release();
	destroyWindow("Video");
	return 0;
}