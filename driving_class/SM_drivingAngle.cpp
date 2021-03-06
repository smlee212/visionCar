#include "SM_drivingAngle.h"
#include "ImageProcessing_Constants.h"
#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core/matx.hpp>
#include <iostream>
using namespace cv;
using namespace std;

CheckStart::CheckStart() {
	lower_white = Scalar(200,200,200);
	upper_white = Scalar(255,255,255);
	lower_black = Scalar(0, 0, 0);
	upper_black = Scalar(80, 80, 80);
	flag_start = -1;
	flag_tunnel = -1;
	check_start = -1;
}

bool CheckStart::isWhite(Mat& frame, double percent) {

	bool returnVal;
	Mat frame_white;

	//cvtColor(frame, frame_hsv, COLOR_BGR2HSV);
	inRange(frame, lower_white, upper_white, frame_white);

	int whitePixel(0);
	for (int i = 0; i < frame_white.cols; i += 10)				// 10픽셀마다 하나씩 검사함 속도를 위해
	{
		for (int j = 0; j < frame_white.rows; j += 10)
		{
			if (frame_white.at<uchar>(j, i))	whitePixel++;		// 흰색이 나오는 픽셀 개수 계산
		}
	}

	double whiteRatio = ((double)whitePixel / ((frame.cols / 10) * (frame.rows / 10)));	//검출된 픽셀수를 전체 픽셀수로 나눈 비율
	whiteRatio *= 100;

	if (whiteRatio > percent)	{	returnVal = true;	}
	else	{	returnVal = false;	}

	return returnVal;
}

bool CheckStart::isBlack(Mat& frame, double percent) {

	bool returnVal;
	Mat frame_black;

	//cvtColor(frame, frame_hsv, COLOR_BGR2HSV);
	inRange(frame, lower_black, upper_black, frame_black);

	int blackPixel(0);
	for (int i = 0; i < frame_black.cols; i += 10)				// 10픽셀마다 하나씩 검사함 속도를 위해
	{
		for (int j = 0; j < frame_black.rows; j += 10)
		{
			if (frame_black.at<uchar>(j, i))	blackPixel++;		// 검은색이 나오는 픽셀 개수 계산
		}
	}


	double blackRatio = ((double)blackPixel / ((frame.cols / 10) * (frame.rows / 10)));	//검출된 픽셀수를 전체 픽셀수로 나눈 비율
	blackRatio *= 100;

	if (blackRatio > percent) { returnVal = true; }
	else { returnVal = false; }

	return returnVal;
}

bool CheckStart::isStop(Mat& frame, double percent) {
	if (check_start == 0) { // 4. 최종 상황 : 한번이라도 출발했으면 계속 출발상태이다.
		return false;
	}
	else { // 출발하기 전 상황
		if (flag_start > 0) {// 2. 흰색 카드가 검출 된 후 사라졌을 때 1 프레임 당 flag 감소
			flag_start--;
			return true;
		}
		else if (flag_start == 0) { // 3. flag가 0이 될 경우 출발
			putText(frame, "Go!", Point(frame.cols / 4, frame.rows * 0.65), FONT_HERSHEY_COMPLEX, 1, Scalar(255), 2);
			if (check_start != 0) {
				check_start = 0; // 출발했다는 표시
			}
			return false;
		}
		else { // 1. 초기 상황
			if (isWhite(frame, percent)) { // 흰색 카드가 검출되면 flag 활성화
				flag_start = 13; // 해당 frame 이후 출발
			}
			return true;
		}
	}
}

bool CheckStart::isTunnel(Mat& frame, double percent) {

	if (isBlack(frame, percent)) { // 어두워지면 flag 증가
		if(flag_tunnel < 15) // 최대 임계값
			flag_tunnel++;
	}
	else { // 밝으면 flag 감소
		if (flag_tunnel > 0)
			flag_tunnel--;
	}

	if (flag_tunnel >= 7) { // 최소 임계값. flag가 이보다 크면 터널 안에 있다고 인식
		putText(frame, "Tunnel!", Point(frame.cols / 4, frame.rows * 0.65), FONT_HERSHEY_COMPLEX, 1, Scalar(255), 2);
		return true;
	}
	else { // 최소 임계값보다 flag가 작으면 터널 밖이라고 인식
		return false;
	}
}

int CheckStart::GetFlag_start() {
	return flag_start;
}

int CheckStart::GetFlag_tunnel() {
	return  flag_tunnel;
}

//////////////////////////////////////////////////////////////////////////////////////////////

RoundAbout::RoundAbout() {
	flag1_start = -1;
	check1_start = -1;
	lower1_distance = 30;
	uper1_distance = 30;

	flag2_start = -1;
	check2_start = -1;
	lower2_distance = 15;
	uper2_distance = 40;
}

bool RoundAbout::isStop(const double Distance) {
	if (check1_start == 0) { // 4. 최종 상황 : 한 번이라도 출발했을 경우 출발을 유지한다.
		return false;
	}
	else { // 정지선에서 대기 상태
		if (flag1_start > 0){ // 2. 앞의 차량이 일정 거리 이상 멀어질 경우 1프레임 당 flag 감소
			if (Distance >= uper1_distance) {
				flag1_start--;		
			}
			return true;
		}
		else if (flag1_start == 0) { // 3. flag가 0이 될 경우 출발
			if (check1_start != 0) { 
				check1_start = 0; // 출발했다는 표시
			}
			return false; // 출발
		}
		else // 1. 초기 상황
		{
			if (Distance < lower1_distance) { // 앞의 차량이 나타났을 때 flag 활성화
				flag1_start = 15; // 1초당 4프레임정도 처리한다고 가정하면, 4초 뒤에 출발
			}	
			return true;
		}
	}
}

bool RoundAbout::isDelay(const double Distance) { 
	if (Distance < lower2_distance) { // 앞의 차량이 나타났을 때 flag 활성화
		flag2_start = 15;
		return true; // 정지
	}
	else { // 앞의 차량이 가깝지 않을 때
		if (flag2_start < 0) { // flag가 비활성화 되었을 때
			return false; // 출발
		}
		else { // flag가 활성화 되어 있을 때
			if (Distance >= uper2_distance) { // 앞의 차량이 일정 거리 이상으로 멀어질 경우				
				flag2_start--;
			}
			return true;
		}
	}
}

/*

void drivingAngle_SM(Mat& inputImg, vector<Vec4i> lines, double& steering, double& steering_Before, int& flag) {
	Vec4f params;
	Point pt1, pt2;
	int x1, y1, x2, y2;
	vector<Vec4i> newLines;//후에 왼쪽오른쪽 하나만 남기기
	const int width = inputImg.size().width;
	const int height = inputImg.size().height;
	vector<float> slopeDegrees;
	float slopeDegree;//항상 라디안으로 만들것
	double preSteering = steering;//이전 값불러오기 최종 조향각도 조절용
	float slopeThreshold = 0.3;//항상 라디안으로 만들 것
	float headingAngle;
	//임시로 1rad(56.6도 정도) 으로해서 모든 라인 다 검출

	//vector point로 선언해보자
	vector<Point> newPoint;
	for (int k = 0; k < lines.size(); k++) {
		params = lines[k];
		x1 = params[0];
		y1 = params[1];
		//x1,y1의 점
		x2 = params[2];
		y2 = params[3];
		//x2,y2의 점

		pt1 = Point(x1, y1);
		pt2 = Point(x2, y2);

		if (x2 - x1 == 0)
			slopeDegree = 999;//x의 변화량이 없는 경우 각도 90도로 만들기
		else slopeDegree = (y2 - y1) / (float)(x2 - x1);
		//slope degree 에 따라 넣을지말지 결정
		if (abs(slopeDegree) > slopeThreshold) {
			newLines.push_back(params);
			slopeDegree = atan(slopeDegree);
			slopeDegrees.push_back(slopeDegree);
		}
	}
	// Split lines into right_lines and left_lines, representing the right and left lane lines
	// Right / left lane lines must have positive / negative slope, and be on the right / left half of the image
	vector<Vec4i> right_lines;
	vector<Vec4i> left_lines;

	for (int i = 0; i < newLines.size(); i++)
	{
		Vec4i line = newLines[i];
		float slope = slopeDegrees[i];
		x1 = line[0];
		y1 = line[1];
		x2 = line[2];
		y2 = line[3];

		float cx = width * 0.5; //x coordinate of center of image
		if (slope > 0 && x1 > cx && x2 > cx)
			right_lines.push_back(line);
		//slope가 0보다 크면 pi/2+a rad에서 온 것이므로 오른쪽일 것
		else if (slope < 0 && x1 < cx && x2 < cx)
			left_lines.push_back(line);
		//slope가 0보다 작으면 pit/2-a rad에서 온 것이므로 왼쪽일 것임
	}

	//Run linear regression to find best fit line for right and left lane lines
	//Right lane lines
	double right_lines_x[1000];
	double right_lines_y[1000];
	vector<Point> rightLines;

	int right_index = 0;
	for (int i = 0; i < right_lines.size(); i++) {

		Vec4i line = right_lines[i];

		x1 = line[0];
		y1 = line[1];
		x2 = line[2];
		y2 = line[3];

		right_lines_x[right_index] = x1;
		right_lines_y[right_index] = y1;
		rightLines.push_back(Point(x1, y1));//
		right_index++;
		right_lines_x[right_index] = x2;
		right_lines_y[right_index] = y2;
		rightLines.push_back(Point(x2, y2));//
		right_index++;
	}

	double left_lines_x[1000];
	double left_lines_y[1000];
	vector<Point> leftLines;

	int left_index = 0;
	for (int i = 0; i < left_lines.size(); i++) {

		Vec4i line = left_lines[i];
		x1 = line[0];
		y1 = line[1];
		leftLines.push_back(Point(x1, y1));//
		x2 = line[2];
		y2 = line[3];
		leftLines.push_back(Point(x2, y2));//
		left_lines_x[left_index] = x1;
		left_lines_y[left_index] = y1;
		left_index++;
		left_lines_x[left_index] = x2;
		left_lines_y[left_index] = y2;
		left_index++;
	}

	Vec4f fitLeft, fitRight;

	Point rp1, rp0;//오른쪽
	Point lp1, lp0;//왼쪽
	float s = 1000;//값 
	double dydxLeft, dydxRight;//각 축별 기울기 값

	//double left_interP = 0, right_interP = 0;
	//double left_b, right_b;
	//방향 벡터 구하는 곳임
	if (left_index > 0) {
		fitLine(leftLines, fitLeft, DIST_L2, 0, 0.01, 0.01);
		lp1.x = cvRound(fitLeft[0] * (+s) + fitLeft[2]);
		lp1.y = cvRound(fitLeft[1] * (+s) + fitLeft[3]);
		lp0.x = cvRound(fitLeft[0] * (-s) + fitLeft[2]);
		lp0.y = cvRound(fitLeft[1] * (-s) + fitLeft[3]);

		dydxLeft = double(-fitLeft[1]) / double(fitLeft[0]);
		//left_b = dydxLeft * lp1.x - lp1.y; // y = ax + b 에서 b를 구하는 식
		//left_interP = dydxLeft * width / 2 + left_b; // 차선의 방정식에서 x값이 x축 중심일때 y의 값
	}
	else { dydxLeft = 0; }//한쪽라인 인식 안되는 예외 처리 부분

	if (right_index > 0) {
		fitLine(rightLines, fitRight, DIST_L2, 0, 0.01, 0.01);
		rp1.x = cvRound(fitRight[0] * s + fitRight[2]);//[0]은 방향 벡터 dx 오른쪽에 있는 좌표
		rp1.y = cvRound(fitRight[1] * s + fitRight[3]);//[1]은 방향 벡터 dy 아래에 있는 좌표
		rp0.x = cvRound(fitRight[0] * (-s) + fitRight[2]);
		rp0.y = cvRound(fitRight[1] * (-s) + fitRight[3]);

		dydxRight = double(-fitRight[1]) / double(fitRight[0]);
		//right_b = dydxRight * rp1.x - rp1.y; // y = ax + b 에서 b를 구하는 식
		//right_interP = dydxRight * width / 2 + right_b; // 라인의 방정식에서 x값이 x축 중심일때 y의 값
	}
	else { dydxRight = 0; } // 한쪽라인 인식 안되는 예외 처리 부분
	//값저장

	//steering값은 각도로 나오며 정면기준 0도임
	////////////////////////////////////////////////////////////////////
	// 수정된 부분
	////////////////////////////////////////////////////////////////////
	double angleThreshold = 3;// 절대값 2.5도 이하는 0으로만들기
	if (abs(atan(dydxLeft) + atan(dydxRight)) <= (angleThreshold * CV_PI / 180)) {
		headingAngle = 0;
	}
	else {
		headingAngle = -180 / CV_PI * (atan((dydxLeft)) + atan((dydxRight)));
	}

	double weight = 1.65; // steering에 가중치를 줘서 조향각을 맞춰줌. Mode1에서 사용

		// 범위를 지정해서 해당 범위마다 일정한 조향각을 설정해둠.(헤딩각이 해당 범위에 들어오면 조향각이 설정됨)
		// 차선의 개수에 따라 조향각의 방향을 잡아줌.
	if ((flag == 111)||(flag == 121)) {
		cout << "flag " << flag << " !" << endl;
		steering = steering_Before;
		if (headingAngle <= 0) {
			flag = 0;
			steering = 0;
		}
	}
	else if ((flag == 112) || (flag == 122)) {
		cout << "flag " << flag << " !" << endl;
		steering = steering_Before;
		if (headingAngle >= 0) {
			flag = 0;
			steering = 0;
		}
	}
	else if (flag == 21) {
		cout << "flag 21 !" << endl;
		steering = steering_Before;
		if (headingAngle > -20) {
			steering = -10;
			steering *= weight;
			flag = 0;
		}
	}
	else if (flag == 22) {
		cout << "flag 22 !" << endl;
		steering = steering_Before;
		if (headingAngle < 20) {
			steering = 10;
			steering *= weight;
			flag = 0;
		}
	}
	else if (flag == 31) {
		cout << "flag 31 !" << endl;
		steering = steering_Before;
		if (headingAngle >= 0) {
			flag = 0;
			steering = 0;
		}
	}
	else if (flag == 32) {
		cout << "flag 32 !" << endl;
		steering = steering_Before;
		if (headingAngle <= 0) {
			flag = 0;
			steering = 0;
		}
	}
	else {
		cout << "flag 0 !" << endl;
		if ((right_index != 0) && (left_index != 0)) { // 차선이 두 개일 때
			// steering 각 조절
			if (abs(headingAngle) <= 5) {
				steering = 0;
				flag = 0;
			}
			else if ((headingAngle >= 5)&&((headingAngle <= 20))) {
				steering = -5;
				flag = 111;
			}
			else if ((headingAngle <= -5) && ((headingAngle >= -20))) {
				steering = 5;
				flag = 112;
			}
			else if (headingAngle > 20){
				steering = -10;
				flag = 111;
			}
			else {
				steering = 10;
				flag == 112;
			}
			// steering 방향 조절 : 기본적으로 heading 방향과 반대 방향임
		
			// 위에는 기본적인 경우
			//////////////////////////////////////////////////////
			// 아래는 special case -> 한 쪽 차선에 붙어있을 경우, 곡선 차선이 나올 경우

			// 곡선
			if ((abs(rp1.y - rp0.y) < height * 1 / 6) && (abs(lp1.y - lp0.y) > height * 1 / 6)) { // 오른쪽 차선이 곡선으로 나올 때 (좌회전)
				steering = -steering;
				flag = 21;
			}
			else if ((abs(rp1.y - rp0.y) > height * 1 / 6) && (abs(lp1.y - lp0.y) < height * 1 / 6)) { // 왼쪽 차선이 곡선으로 나올 때 (우회전)
				steering = -steering;
				flag = 22;
			}
			else {

			}

			// 한 쪽 차선에 붙어 있을 경우 heading 방향이 steering 방향이 됨
			if ((flag != 21) || (flag != 22)) {
				if (abs(rp1.y - rp0.y) > abs(lp1.y - lp0.y)) {// 오른쪽 차선에 붙어있을 경우 
					if (headingAngle <= 0) {
						steering = -steering;
						flag = 121;
					}
				}
				else {// 왼쪽 차선에 붙어있을 경우
					if (headingAngle >= 0) {
						steering = -steering;
						flag = 122;
					}
				}
			}
			steering *= weight;
		}
		// 30은 임계값
		//if ((atan(dydxRight) > -30) && (atan(dydxLeft) > 30)) { // 좌회전 구간 (곡선인 오른쪽 차선 나올 때) 
			// heading > 0 이다.
		//	steering =
		//}
		else if ((right_index != 0) && (left_index == 0)) { // 오른쪽 차선만 보일 때
			// steering 그대로		
			if (headingAngle < -70) {
				steering = -5;
				flag = 0;
			}
			else if (headingAngle <= -30) {
				steering = -10;
				flag = 31;
			}
			else {
				steering = -20;
				flag = 0;
			}
			steering *= weight;
			
		}
		else if ((right_index == 0) && (left_index != 0)) { // 왼쪽 차선만 보일 때
			if (headingAngle > 70) {
				steering = 5;
				flag = 0;
			}
			else if (headingAngle >= 30) {
				steering = 10;
				flag = 32;
			}
			else {
				steering = 20;
				flag = 0;
			}
			steering *= weight;
		}
		else { // 차선이 없을 때
			steering = steering_Before;
			flag = 0;
		}
	}

	// 아주 기본적인 알고리즘 상steering = -headingAngle;
	//right_index=0일때 오른선 검출X
	//left_index=0일때 왼선 검출
	//heading Angle은 차량이 바라보는 방향
	cout << "headingAngle: " << headingAngle << endl;
	cout << "steering: " << steering << endl << endl;
	
	slopeDegrees.clear();
	leftLines.clear();
	rightLines.clear();
	right_lines.clear();
	left_lines.clear();
	newPoint.clear();
	newLines.clear();
}

void regionOfInterest(Mat& src, Mat& dst, Point* points) {// points의 포인터인 이유-> 여러개의 꼭짓점 경우

	Mat maskImg = Mat::zeros(src.size(), CV_8UC1);

	Scalar ignore_mask_color = Scalar(255, 255, 255);
	const Point* ppt[1] = { points };//개의 꼭짓점 :n vertices
	int npt[] = { 4 };

	fillPoly(maskImg, ppt, npt, 1, Scalar(255, 255, 255), LINE_8);
	Mat maskedImg;
	bitwise_and(src, maskImg, maskedImg);
	dst = maskedImg;
}

void imgBlur(Mat& src, Mat& dst, int processingCode) {
	if (processingCode == 1) {//gaussian Blur
		GaussianBlur(src, dst, Size(3, 3), 0, 0);
	}
	else if (processingCode == 2) {//Canny edge
		Canny(src, dst, 50, 150);
	}

}

bool extractLines(Mat& src, vector<Vec4i>& lines) {
	Mat filterImg;
	Mat grayImg, blurImg, edgeImg, roiImg, dstImg;
	int width = src.size().width;
	int height = src.size().height;
	filter_colors(src, filterImg);
	cvtColor(filterImg, grayImg, COLOR_BGR2GRAY);
	imgBlur(grayImg, blurImg, 1);
	imgBlur(blurImg, edgeImg, 2);
	Point pt[4] = { Point(0,height * 1 / 2),Point(width,height * 1 / 2),Point(width,height),Point(0,height) };
	//roi point 설정

	regionOfInterest(edgeImg, roiImg, pt);
	vector<Vec4i> extractLines;

	HoughLinesP(roiImg, extractLines, 1, CV_PI / 180.0, 30, 10, 20);
	lines = extractLines;
	return true;

}

void filter_colors(Mat& src, Mat& img_filtered) {
	//
	UMat bgrImg;
	UMat hsvImg;
	//UMat maskWhite, whiteImg;
	UMat maskYellow, yellowImg;
	UMat imgCombined;
	src.copyTo(bgrImg);

	//white 변경
	//inRange(bgrImg, lower_white, upper_white, maskWhite);
	//lower와 upper사이의 값을 1로 나머지는 0으로 저장
	//bitwise_and(bgrImg, bgrImg, whiteImg, maskWhite);
	Scalar lower_w = Scalar(120, 120, 120); //흰색 차선 (RGB)
	Scalar upper_w = Scalar(255, 255, 255);
	Scalar lower_y(14, 30, 35);
	Scalar upper_y(46, 255, 255);


	cvtColor(bgrImg, hsvImg, COLOR_BGR2HSV);
	inRange(hsvImg, lower_y, upper_y, maskYellow);
	bitwise_and(bgrImg, bgrImg, yellowImg, maskYellow);
	//addWeighted(whiteImg, 1.0, yellowImg, 1.0, 0.0, imgCombined);//두 이미지 합치기
	yellowImg.copyTo(imgCombined);;//노란색만 검출할때까지 사용
	imgCombined.copyTo(img_filtered);
}
*/