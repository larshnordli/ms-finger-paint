#include "stdafx.h"
#include <windows.h>
#include <winuser.h>
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <iostream>
#include <cv.h>
#include <highgui.h>

using namespace cv;
using namespace std;

const double XSCALEFACTOR = 65535 / (GetSystemMetrics(SM_CXSCREEN) - 1);
const double YSCALEFACTOR = 65535 / (GetSystemMetrics(SM_CYSCREEN) - 1);

void mouseLeftClick(int x, int y, int command)
{
	POINT cursorPos;
	GetCursorPos(&cursorPos);

	double cx = cursorPos.x * XSCALEFACTOR;
	double cy = cursorPos.y * YSCALEFACTOR;

	double nx = x * XSCALEFACTOR;
	double ny = y * YSCALEFACTOR;

	INPUT Input = { 0 };
	Input.type = INPUT_MOUSE;

	Input.mi.dx = (LONG)nx;
	Input.mi.dy = (LONG)ny;

	if (command == 0) //down
		Input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
	else if (command == 1) //up
		Input.mi.dwFlags = MOUSEEVENTF_LEFTUP;

	SendInput(1, &Input, sizeof(INPUT));
	return;
}

int main(int argc, char** argv){

	VideoCapture cap(0);

	if (!cap.isOpened()){
		cout << "Cannot open webcam" << endl;
		return -1;
	}

	namedWindow("MS Finger Paint", CV_WINDOW_NORMAL);
	//setMouseCallback("MS Finger Paint", CallBackFunc, NULL);
	namedWindow("Control", CV_WINDOW_NORMAL);

	int lowH = 111;
	int highH = 123;

	int lowS = 98;
	int highS = 255;

	int lowV = 62;
	int highV = 255;

	int avgBlur = 17;
	int depth = 145;

	//Create trackbars in "Control" window
	cvCreateTrackbar("LowH", "Control", &lowH, 179);
	cvCreateTrackbar("HighH", "Control", &highH, 179);

	cvCreateTrackbar("LowS", "Control", &lowS, 255);
	cvCreateTrackbar("HighS", "Control", &highS, 255);

	cvCreateTrackbar("LowV", "Control", &lowV, 255);
	cvCreateTrackbar("HighV", "Control", &highV, 255);

	cvCreateTrackbar("Averaging", "Control", &avgBlur, 101);
	cvCreateTrackbar("Depth", "Control", &depth, 300);

	while (true){ //Create infinte loop for live streaming

		Mat frame;
		Mat frameHSV;
		Mat frameThreshold;
		Mat ellipse = getStructuringElement(MORPH_ELLIPSE, Size(5, 5));
		RNG rng(12345);
		vector<Vec4i> hierarchy;

		bool bSuccess = cap.read(frame);

		if (!bSuccess){
			cout << "Cannot read stream" << endl;
			break;
		}

		//frame = imread("image.jpg", CV_LOAD_IMAGE_COLOR);

		//mirror image
		flip(frame, frame, 1);

		//convert RGB -> HSV
		cvtColor(frame, frameHSV, CV_RGB2HSV);

		//average blur
		blur(frameHSV, frameHSV, Size(avgBlur, avgBlur));

		//threshold
		inRange(frameHSV, Scalar(lowH, lowS, lowV), Scalar(highH, highS, highV), frameThreshold);

		//open and close
		erode(frameThreshold, frameThreshold, ellipse);
		dilate(frameThreshold, frameThreshold, ellipse);
		dilate(frameThreshold, frameThreshold, ellipse);
		erode(frameThreshold, frameThreshold, ellipse);

		//find contours
		vector<vector <Point> > contours;
		Mat contourOutput = frameThreshold.clone();

		findContours(contourOutput, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

		//hull convex and convex defects
		vector<vector<Point> > hull(contours.size());
		vector<vector<Point> > hull_indice_points(contours.size());
		vector<vector<int> > hull_indices(contours.size());
		int num_fingers = 0;
		Point cursor;
		vector<vector<Vec4i> > defects(contours.size());
		Mat drawing = Mat::zeros(contourOutput.size(), CV_8UC3);

		for (int i = 0; i < contours.size(); i++){
			convexHull(Mat(contours[i]), hull[i], false);
			convexHull(Mat(contours[i]), hull_indices[i], false);
			if (hull_indices[i].size() > 3)
				convexityDefects(Mat(contours[i]), hull_indices[i], defects[i]);

			//find defects
			for (int j = 0; j < hull[i].size(); j++){
				int indice = hull_indices[i][j];
				hull_indice_points[i].push_back(contours[i][indice]);
			}

			for (int j = 0; j < defects[i].size(); j++){

				if (defects[i][j][3] > depth * 256){
					num_fingers++;
					int start = defects[i][j][0];
					int end = defects[i][j][1];
					int furthest = defects[i][j][2];
					//cursor.x = start;
					//cursor.y = end;
					hull_indice_points[i].push_back(contours[i][furthest]);

					//circle(drawing, contours[i][start], 5, Scalar(0,255, 100), -1);
					cursor = contours[i][end];
					circle(drawing, contours[i][end], 5, Scalar(0, 255, 0), -1);
					circle(drawing, contours[i][furthest], 5, Scalar(0, 0, 255), -1);

					line(drawing, contours[i][furthest], contours[i][start], Scalar(0, 0, 255), 1);
					line(drawing, contours[i][furthest], contours[i][end], Scalar(0, 0, 255), 1);
				}
			}
		}

		for (int i = 0; i < contours.size(); i++){
			Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			//draw contours
			drawContours(drawing, contours, i, color, 1, 8, vector<Vec4i>(), 0, Point());
		}

		//cursor operations
		if (num_fingers == 1){
			mouseLeftClick(cursor.x, cursor.y, 1);
			SetCursorPos(cursor.x, cursor.y);
		}
		//if (num_fingers == 2){
		//	usleep(500);
		//	mouseLeftClick(cursor.x, cursor.y, 0);
		//}

		imshow("MS Finger Paint", drawing); //Show image frames on created window 

		if (waitKey(30) == 27){
			cout << "lowH: " << lowH << endl;
			cout << "highH: " << highH << endl;
			cout << "lowS: " << lowS << endl;
			cout << "highS: " << highS << endl;
			cout << "lowV: " << lowV << endl;
			cout << "highV: " << highV << endl;
			cout << "blur: " << avgBlur << endl;
			cout << "depth: " << depth << endl;
			cout << "Initiated quit by user" << endl;
			break;
		}
	}

	return 0;
}
