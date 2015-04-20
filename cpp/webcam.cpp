#include <iostream>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;
using namespace std;

int main(int argc, char** argv){
	
	VideoCapture cap(0);

	if(! cap.isOpened()){
		cout << "Cannot open webcam" << endl;
		return -1;
	}

	namedWindow("MS Finger Paint", CV_WINDOW_AUTOSIZE);
	namedWindow("Control", CV_WINDOW_AUTOSIZE);

	int lowH = 100;
	int highH = 179;

	int lowS = 75; 
	int highS = 210;

	int lowV = 104;
	int highV = 255;

	//Create trackbars in "Control" window
	cvCreateTrackbar("LowH", "Control", &lowH, 179);
	cvCreateTrackbar("HighH", "Control", &highH, 179);

	cvCreateTrackbar("LowS", "Control", &lowS, 255);
	cvCreateTrackbar("HighS", "Control", &highS, 255);

	cvCreateTrackbar("LowV", "Control", &lowV, 255);
	cvCreateTrackbar("HighV", "Control", &highV, 255);
	
	while(true){ //Create infinte loop for live streaming

		Mat frame;
		Mat frameHSV;
		Mat frameThreshold;
		Mat ellipse = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));

		bool bSuccess = cap.read(frame);

		if(!bSuccess){
			cout << "Cannot read stream" << endl;
			break;
		}

		flip(frame, frame, 1); //mirror image
		
		cvtColor(frame, frameHSV, CV_RGB2HSV); //convert RGB -> HSV

		inRange(frameHSV, Scalar(lowH, lowS, lowV), Scalar(highH, highS, highV), frameThreshold);

		//morphological opening (remove small objects from the foreground)
	  	erode(frameThreshold, frameThreshold, ellipse);
	  	dilate( frameThreshold, frameThreshold, ellipse); 

	  	//morphological closing (fill small holes in the foreground)
	  	dilate( frameThreshold, frameThreshold,ellipse); 
	  	erode(frameThreshold, frameThreshold, ellipse);
		
		imshow("MS Finger Paint", frameThreshold); //Show image frames on created window 

		if(waitKey(30) == 27){
			cout << "lowH: " << lowH << endl;
			cout << "highH: " << highH << endl;
			cout << "lowS: " << lowS << endl;
			cout << "highS: " << highS << endl;
			cout << "lowV: " << lowV << endl;
			cout << "highV: " << highV << endl;
			cout << "Initiated quit by user" << endl;
			break;
		}
	}

	return 0;
}
