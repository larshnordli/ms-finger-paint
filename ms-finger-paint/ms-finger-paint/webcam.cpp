#include "stdafx.h"
#include <windows.h>
#include <winuser.h>
#include <thread>
#include <chrono>
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

	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	if (!cap.isOpened()){
		cout << "Cannot open webcam" << endl;
		return -1;
	}

	namedWindow("MS Finger Paint", CV_WINDOW_NORMAL);
	namedWindow("Control", CV_WINDOW_NORMAL);

	int minYCC1 = 86;
	int minYCC2 = 140;
	int minYCC3 = 97;

	int maxYCC1 = 255;
	int maxYCC2 = 176;
	int maxYCC3 = 136;

	int gauss_blur = 7;
	int depth = 65;
	int ellipse_size = 3;

	//Create trackbars in "Control" window
	cvCreateTrackbar("minYCC1", "Control", &minYCC1, 255);
	cvCreateTrackbar("minYCC2", "Control", &minYCC2, 255);
	cvCreateTrackbar("minYCC3", "Control", &minYCC3, 255);

	cvCreateTrackbar("maxYCC1", "Control", &maxYCC1, 255);
	cvCreateTrackbar("maxYCC2", "Control", &maxYCC2, 255);
	cvCreateTrackbar("maxYCC3", "Control", &maxYCC3, 255);

	cvCreateTrackbar("Gaussian", "Control", &gauss_blur, 20);
	cvCreateTrackbar("Depth", "Control", &depth, 300);
	cvCreateTrackbar("EllipseSize", "Control", &ellipse_size, 20);

	while (true){ //Create infinte loop for live streaming

		Mat frame;
		Mat frameYCC;
		Mat frameThreshold;
		Mat ellipse = getStructuringElement(MORPH_ELLIPSE, Size(ellipse_size, ellipse_size));
		RNG rng(12345);
		vector<Vec4i> hierarchy;

		bool bSuccess = cap.read(frame);

		if (!bSuccess){
			cout << "Cannot read stream" << endl;
			break;
		}
		//mirror image
		flip(frame, frame, 1);
		//gaussian blur
		Size kSize;
		kSize.height = gauss_blur;
		kSize.width = gauss_blur;
		double sigma = 0.3*(3 / 2 - 1) + 0.8;
		GaussianBlur(frame, frame, kSize, sigma, 0.0, 4);
		//convert RGB -> VCC
		cvtColor(frame, frameYCC, CV_BGR2YCrCb);
		//threshold
		inRange(frameYCC, Scalar(minYCC1, minYCC2, minYCC3), Scalar(maxYCC1, maxYCC2, maxYCC3), frameThreshold); //CV_THRESH_OTSU?
		//open
		erode(frameThreshold, frameThreshold, ellipse);
		dilate(frameThreshold, frameThreshold, ellipse);
		//close
		dilate(frameThreshold, frameThreshold, ellipse);
		erode(frameThreshold, frameThreshold, ellipse);

		//find contours
		vector<vector <Point> > contours;
		findContours(frameThreshold, contours, CV_RETR_LIST, CV_CHAIN_APPROX_NONE);

		//hull convex and convex defects
		vector<vector<Point> > hull(contours.size());
		vector<vector<Point> > hull_indice_points(contours.size());
		vector<vector<int> > hull_indices(contours.size());
		vector<vector<Vec4i> > defects(contours.size());

		vector<double> areas(contours.size());
		int num_fingers = 0;
		Point cursor;
			
		Mat drawing = Mat::zeros(frame.size(), CV_8UC3);

		for (int i = 0; i < contours.size(); i++){
			convexHull(Mat(contours[i]), hull[i], false); //for drawing
			convexHull(Mat(contours[i]), hull_indices[i], false); // for calculations
			if (hull_indices[i].size() > 3) // for start, end and furthest points
				convexityDefects(Mat(contours[i]), hull_indices[i], defects[i]);

			//find defects
			for (int j = 0; j < hull[i].size(); j++){
				int indice = hull_indices[i][j];
				hull_indice_points[i].push_back(contours[i][indice]);
			}

			//find end points
			for (int j = 0; j < defects[i].size(); j++){

				if (defects[i][j][3] > depth * 256){
					num_fingers++;
					int start = defects[i][j][0];
					int end = defects[i][j][1];
					int furthest = defects[i][j][2];
					hull_indice_points[i].push_back(contours[i][furthest]);

					circle(drawing, contours[i][start], 10, Scalar(255, 0, 0), -1); //defect
					circle(drawing, contours[i][end], 10, Scalar(0, 255, 0), -1); //second finger
					circle(drawing, contours[i][furthest], 10, Scalar(0, 0, 255), -1); //first finger

					cursor = contours[i][end];

					line(drawing, contours[i][furthest], contours[i][start], Scalar(0, 0, 255), 1);
					line(drawing, contours[i][furthest], contours[i][end], Scalar(0, 0, 255), 1);
				}

			}

				areas.push_back( floor( arcLength(hull[i], true) + 10));
		}

		for (int i = 0; i < contours.size(); i++){
			Scalar color = Scalar(rng.uniform(0, 255), rng.uniform(0, 255), rng.uniform(0, 255));
			//draw contours
			drawContours(drawing, hull, i, color, 1, 8, vector<Vec4i>(), 0, Point());
		}

		//cursor operations
		if (num_fingers == 1){
			SetCursorPos(cursor.x, cursor.y);

			for (int i = 0; i < areas.size() - 1; i++){
				/*if (areas[i] > areas[i + 1])
					mouseLeftClick(cursor.x, cursor.y, 0);
				else if (areas[i] < areas[i + 1])
					mouseLeftClick(cursor.x, cursor.y, 1); */
				cout << i << ":    " << areas[i] << endl;
			}
		}
		else if (num_fingers == 2){
			//keyboard shortcut to paint bucket tool
		}
		else if (num_fingers == 3){
			//keyboard shortcut to line tool
		}
		else if (num_fingers == 4){
			//keyboard shortcut to eye dropper tool
		}

		imshow("MS Finger Paint", drawing); //Show image frames on created window 

		//Pressing escape closes the program
		if (waitKey(30) == 27){
			cout << "Initiated quit by user" << endl;
			break;
		}
	}

	return 0;
}
