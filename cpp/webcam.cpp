//#include "stdafx.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <cv.h>
#include <highgui.h>
using namespace cv;
using namespace std;

char key;

int main(int argc, char * argv[]){
	cvNamedWindow("Finger Paint", 1);    //Create window
	CvCapture* capture = cvCaptureFromCAM(CV_CAP_ANY);  //Capture using any camera connected to your system
	
	while(1){ //Create infinte loop for live streaming
		//--

		IplImage* frame = cvQueryFrame(capture); //Create image frames from capture
		IplImage* frameHSV = cvCreateImage(cvGetSize(frame), 8, 3);
		
		cvCvtColor(frame, frameHSV, CV_RGB2HSV);
		IplImage* frameThreshold = cvCreateImage(cvGetSize(frameHSV), 8, 1);

		cvInRangeS(frame, cvScalar(13, 71, 81), cvScalar(255, 255, 47), frameThreshold);

		//--
		
		cvShowImage("Finger Paint", frameThreshold); //Show image frames on created window 
		
		key = cvWaitKey(10); //Capture Keyboard stroke
		if(char (key) == 27){
			break; //If you hit ESC key loop will break.
		}
	}

cvReleaseCapture(&capture); //Release capture.
cvDestroyWindow("Finger Paint"); //Destroy Window

return 0;
}
