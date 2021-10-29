#include "MatrixReaderWriter.h"
#include <stdio.h>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include<thread> 
#include <chrono>


float v = 1.0;
float u = 0.5;
float rad = 1000;

MatrixReaderWriter* mrw;
cv::Mat resImg;

chrono::high_resolution_clock::time_point tZoomStart;
chrono::high_resolution_clock::time_point tCurrent;
chrono::high_resolution_clock::time_point tRotationStart;

bool isAnimRotationIncreasing = true;
int animRotationPeriod = 10000; //milliseconds
float rotationFactor = 0.0f;

bool isAnimZoomIn = true;
bool isAnimZoomIncreasing = true;
int animZoomHalfPeriod = 5000; //milliseconds
float scaleFactor = 1.0f;

bool isMouseRotation = false;
bool isLMouseClicking = false; 

bool isApplicationRunning = true;

int prevPosX, prevPosY;
bool firstTime = true;

//functions:

void drawPoints(MatrixReaderWriter* mrw, float u, float v, float rad, cv::Mat& resImg) {
	int NUM = mrw->rowNum;

	cv::Mat C(3, 3, CV_32F);
	cv::Mat R(3, 3, CV_32F);
	cv::Mat T(3, 1, CV_32F);

	float tx = cos(u) * sin(v);
	float ty = sin(u) * sin(v);
	float tz = cos(v);



	//Intrinsic parameters

	C.at<float>(0, 0) = 3000.0f;
	C.at<float>(0, 1) = 0.0f;
	C.at<float>(0, 2) = 400.0f;

	C.at<float>(1, 0) = 0.0f;
	C.at<float>(1, 1) = 3000.0f;
	C.at<float>(1, 2) = 300.0f;

	C.at<float>(2, 0) = 0.0f;
	C.at<float>(2, 1) = 0.0f;
	C.at<float>(2, 2) = 1.0f;

	T.at<float>(0, 0) = rad * tx;
	T.at<float>(1, 0) = rad * ty;
	T.at<float>(2, 0) = rad * tz;


	//Mirror?
	int HowManyPi = (int)floor(v / 3.1415);


	//Axes:
	cv::Point3f Z(-1.0 * tx, -1.0 * ty, -1.0 * tz);
	cv::Point3f X(sin(u) * sin(v), -cos(u) * sin(v), 0.0f);
	if (HowManyPi % 2)
		X = (1.0 / sqrt(X.x * X.x + X.y * X.y + X.z * X.z)) * X;
	else
		X = (-1.0 / sqrt(X.x * X.x + X.y * X.y + X.z * X.z)) * X;

	cv::Point3f up = X.cross(Z);  //Axis Y	

/*
	printf("%f\n",X.x*X.x+X.y*X.y+X.z*X.z);
	printf("%f\n",up.x*up.x+up.y*up.y+up.z*up.z);
	printf("%f\n",Z.x*Z.x+Z.y*Z.y+Z.z*Z.z);

	printf("(%f,%f)\n",u,v);
*/


	R.at<float>(2, 0) = Z.x;
	R.at<float>(2, 1) = Z.y;
	R.at<float>(2, 2) = Z.z;

	R.at<float>(1, 0) = up.x;
	R.at<float>(1, 1) = up.y;
	R.at<float>(1, 2) = up.z;

	R.at<float>(0, 0) = X.x;
	R.at<float>(0, 1) = X.y;
	R.at<float>(0, 2) = X.z;


	for (int i = 0; i < NUM; i++) {
		cv::Mat vec(3, 1, CV_32F);
		vec.at<float>(0, 0) = mrw->data[3 * i];
		vec.at<float>(1, 0) = mrw->data[3 * i + 1];
		vec.at<float>(2, 0) = mrw->data[3 * i + 2];

		int red = 255;
		int green = 255;
		int blue = 255;

		cv::Mat trVec = R * (vec - T);
		trVec = C * trVec;
		trVec = trVec / trVec.at<float>(2, 0);
		//		printf("(%d,%d)",(int)trVec.at<float>(0,0),(int)trVec.at<float>(1,0));

		circle(resImg, cv::Point((int)trVec.at<float>(0, 0), (int)trVec.at<float>(1, 0)), 2.0, cv::Scalar(blue, green, red), 2, 8);
	}
}

void UI() {
	while (isApplicationRunning) {
		resImg = cv::Mat::zeros(600, 800, CV_8UC3);
		drawPoints(mrw, u + rotationFactor, v + rotationFactor, rad * scaleFactor, resImg);
		imshow("Display window", resImg);
		this_thread::sleep_for(chrono::milliseconds(50));
	}
}

void MouseCallBackFunc(int event, int x, int y, int flags, void* userdata)
{

	if (event == cv::EVENT_MOUSEWHEEL)
	{
		if (cv::getMouseWheelDelta(flags) > 0)
		{
			rad /= 1.1f;
		}
		else if (cv::getMouseWheelDelta(flags) < 0)
		{
			rad *= 1.1f;
		}
	}

	if (!isLMouseClicking)
	{
		if (event == cv::EVENT_LBUTTONDOWN)
		{
			isLMouseClicking = true;
			prevPosX = x;
			prevPosY = y;
		}
	}
	else
	{
		if (event == cv::EVENT_LBUTTONUP)
		{
			isLMouseClicking = false;
			isMouseRotation = false;
			prevPosX = x;
			prevPosY = y;
		}

	}

	if (isLMouseClicking) {
		isMouseRotation = true;

		int difX = x - prevPosX;
		int difY = y - prevPosY;

		if (difX > 0)
		{
			u -= 0.03;
		}
		else if (difX < 0)
		{
			u += 0.03;
		}

		if (difY > 0)
		{
			v -= 0.03;
		}
		else if (difY < 0)
		{
			v += 0.03;
		}
	}

	prevPosX = x;
	prevPosY = y;

}

float LinearInterpolation(float b1, float b2, float t) {
	return b1 * (1 - t) + b2 * t;
}

void Animation() {
	while (isApplicationRunning) {
		tCurrent = chrono::high_resolution_clock::now();

		chrono::duration<double, std::milli> durZoom = tCurrent - tZoomStart;
		chrono::duration<double, std::milli> durRotation = tCurrent - tRotationStart;

		if (durZoom.count() > animZoomHalfPeriod)
		{
			tZoomStart = chrono::high_resolution_clock::now();
			durZoom = chrono::steady_clock::duration::zero();
			if (isAnimZoomIn)
			{
				if (isAnimZoomIncreasing)
				{
					isAnimZoomIncreasing = false;
				}
				else
				{
					isAnimZoomIn = false;
				}
			}
			else
			{
				if (isAnimZoomIncreasing)
				{
					isAnimZoomIn = true;
				}
				else
				{
					isAnimZoomIncreasing = true;
				}
			}
		}

		if (durRotation.count() > animRotationPeriod) {
			durRotation = chrono::steady_clock::duration::zero();
			tRotationStart = chrono::high_resolution_clock::now();
			if (isAnimRotationIncreasing) {
				isAnimRotationIncreasing = false;
			}
			else {
				isAnimRotationIncreasing = true;
			}
		}

		float tZoom = durZoom.count() / animZoomHalfPeriod;
		float tRotation = durRotation.count() / animRotationPeriod;

		if (isAnimZoomIn) {
			if (isAnimZoomIncreasing)
			{
				scaleFactor = LinearInterpolation(1.0f, 2.0f, tZoom);
			}
			else
			{
				scaleFactor = LinearInterpolation(2.0f, 1.0f, tZoom);
			}

		}
		else {
			if (isAnimZoomIncreasing)
			{
				scaleFactor = LinearInterpolation(0.5f, 1.0f, tZoom);
			}
			else
			{
				scaleFactor = LinearInterpolation(1.0f, 0.5f, tZoom);
			}
		}

		if (isAnimRotationIncreasing)
		{
			rotationFactor = LinearInterpolation(0, 2.0f, tRotation);
		}
		else
		{
			rotationFactor = LinearInterpolation(2.0f, 0, tRotation);
		}

		std::cout << "Scale factor: " << scaleFactor << std::endl;
		//std::cout << "Rotation factor: " << rotationFactor << std::endl;
		std::cout << "----------------------------------------------------" << std::endl;
		this_thread::sleep_for(chrono::milliseconds(50));
	}
}

int main(int argc, const char** argv) {
	if (argc != 2) { printf("Usage: FV filename\n"); exit(0); }

	mrw = new MatrixReaderWriter(argv[1]);

	//Setup timers
	tZoomStart = chrono::high_resolution_clock::now();
	tRotationStart = chrono::high_resolution_clock::now();

	cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);// Create a window for display.
	cv::setMouseCallback("Display window", MouseCallBackFunc, NULL);

	std::thread UIThread(UI);
	std::thread animationThread(Animation);

	char key;
	while (true) {
		key = cvWaitKey(100);
		if (key == 27)
		{
			isApplicationRunning = false;
			break;
		}
		switch (key) {
		case 'q':
			u += 0.1;
			break;
		case 'a':
			u -= 0.1;
			break;
		case 'w':
			v += 0.1;
			break;
		case 's':
			v -= 0.1;
			break;
		case 'e':
			rad *= 1.1f;
			break;
		case 'd':
			rad /= 1.1f;
			break;
		}


	}

	UIThread.join();
	animationThread.join();


}
