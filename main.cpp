#include <iostream>//
#include <opencv2/opencv.hpp>

#include "Calibration.h"
#include "CustomPicar.h"
#include "DetectColorSign.h"
#include "Driving_DH.h"
#include "SM_drivingAngle.h"

using namespace std;
using namespace auto_car;
using namespace cv;

int main()
{
	//Board, Servo, DCmotor configuration-------------------------
	PCA9685 pca{};
	pca.set_pwm_freq(60.0);
	Servo steering(pca, Steering);
	Servo cam_tilt(pca, Tilt);
	Servo cam_pan(pca, Pan);
	Wheel DCmotor(pca, LeftWheel, RightWheel);
	allServoReset(pca);				// 3 Servo motor center reset
	UltraSonic firstSonic(28, 27);	// 초음파센서 객체
	UltraSonic secondSonic(26, 25);
	PicarLED whiteLed(24);
	PicarLED rightLed(23);
	PicarLED leftLed(22);
	whiteLed.on();
	cout << "[Sensor and motor setting complete]" << endl
		<< endl;

	//OpenCV setting----------------------------------------------
	VideoCapture videocap(0); //camera obj
	if (!videocap.isOpened())
	{
		cerr << endl
			<< "video capture fail!" << endl;
		return -1;
	}
	whiteLed.off();
	cout << "[VideoCapture loading complete]" << endl
		<< endl;

	//Calibration setting-----------------------------------------
	Size videoSize = Size(640, 480);
	Mat map1, map2, distCoeffs;
	Mat cameraMatrix = Mat(3, 3, CV_32FC1);
	int numBoards = 5;
	DoCalib(distCoeffs, cameraMatrix, numBoards);
	initUndistortRectifyMap(cameraMatrix, distCoeffs, Mat(), cameraMatrix, videoSize, CV_32FC1, map1, map2);
	Mat distortedFrame;
	Mat frame;
	whiteLed.on();
	cout << "[calibration complete]" << endl
		<< endl;

	//mode selection----------------------------------------------
	cout << "[visionCar] program start" << endl
		<< endl;

	cout << "Test 1 : Basic test" << endl;
	cout << "Test 2 : Manual test" << endl
		<< endl;

	cout << "Mode 3 : Signal detection(대희)" << endl;
	cout << "Mode 4 : Driving(대희)" << endl;
	cout << "Mode 5 : Parking(석준)" << endl;
	cout << "Mode 6 : Rotary(상민)" << endl;
	cout << "Mode 7 : Overtaking(민수)" << endl;
	cout << "Mode 8 : Tunnel(대희)" << endl
		<< endl;

	cout << "Select mode : ";
	int mode;
	cin >> mode;
	whiteLed.off();

	if (mode == 1) //Test 1 : Basic test------------------------------------------------------
	{
		//steering.setRatio(100); //바퀴 우측
		//steering.setRatio(0);	//바퀴 좌측
		//steering.resetCenter(); //바퀴 좌우정렬

		//cam_tilt.setRatio(100); //카메라 상향
		//cam_tilt.setRatio(0);	//카메라 하향
		//cam_tilt.resetCenter(); //카메라 상하정렬

		//cam_pan.setRatio(100); //카메라 우향
		//cam_pan.setRatio(0);   //카메라 좌향
		//cam_pan.resetCenter(); //카메라 좌우정렬

		//DCmotor.go();  //dc모터 전진
		//waitKey(1500); //wait 1.5sec

		//DCmotor.backward(); //dc모터 후진
		//waitKey(1500);		//wait 1.5sec

		//DCmotor.stop(); //dc모터 멈춤
		while (true)
		{
			cout << "1번센서 거리 = " << firstSonic.distance() << endl;
			cout << "2번센서 거리 = " << secondSonic.distance() << endl;
			waitKey(33);
		}
	}
	//End basic test

	else if (mode == 2) //Test 2 : Manual test------------------------------------------------
	{
		//ManualMode class & basic speed rate
		ManualMode Manual(pca, 25);
		Manual.guide();

		//메인루프
		int key(-1);
		while (key != 27) //if not ESC
		{
			videocap >> distortedFrame;
			remap(distortedFrame, frame, map1, map2, INTER_LINEAR);

			namedWindow("frame", WINDOW_NORMAL);
			imshow("frame", frame);
			resizeWindow("frame", 480, 360);
			moveWindow("frame", 320, 80 + 240);

			key = waitKey(33); //if you not press, return -1
			if (key == 27)
				break;
			else if (key != -1)
				Manual.input(key); //movement by keyboard
			rewind(stdin);
		}
	}
	//End manual test

	else if (mode == 3) //Mode 3 : Signal detection(대희) ------------------------------------
	{
		//color detecting class ganerate
		DetectColorSign detectColorSign(true);
		int flicker(4);

		while (true)
		{
			videocap >> distortedFrame;
			remap(distortedFrame, frame, map1, map2, INTER_LINEAR);
			if (!flicker--)
				flicker = 4;

			whiteLed.off();
			rightLed.off();
			leftLed.off();

			if (detectColorSign.priorityStop(frame, 1.2))
			{
				if ((flicker % 2) == 1)
				{
					whiteLed.on();
				}
				else
				{
					leftLed.on();
					rightLed.on();
				}
				cout << "A priority stop signal was detected." << '\n';
			}
			else if (detectColorSign.isRedStop(frame, 1.2, true)) //빨간색 표지판 감지
			{
				if ((flicker % 3) == 1)
				{
					whiteLed.on();
					leftLed.on();
					rightLed.on();
				}
				cout << "A red stop sign was detected." << '\n';
			}
			else if (detectColorSign.isYellow(frame, 1.2)) //노란색 표지판 감지
			{
				if ((flicker % 3) == 1)
				{
					leftLed.on();
					rightLed.on();
				}
				cout << "A yellow sign was detected." << '\n';
			}
			else if (detectColorSign.isGreenTurnSignal(frame, 0.9) == 1) //초록색 표지판 감지
			{
				if ((flicker % 3) == 1) leftLed.on();
				cout << "<----- signal was detected." << '\n';
			}
			else if (detectColorSign.isGreenTurnSignal(frame, 1.2) == 2) //초록색 표지판 감지
			{
				if ((flicker % 3) == 1) rightLed.on();
				cout << "-----> signal was detected." << '\n';
			}
			else
			{
				whiteLed.off();
			}
			namedWindow("frame", WINDOW_NORMAL);
			imshow("frame", frame);
			resizeWindow("frame", 480, 360);
			moveWindow("frame", 320, 80 + 240);
			if (waitKey(33) == 27)
				break; //프로그램 종료 ESC(아스키코드 = 27)키.
		}
	}
	//End Signal detection mode


	else if (mode == 4) //Mode 4 : Driving(대희) --------------------------------------------
	{
		//ManualMode class & basic speed rate
		ManualMode Manual(pca, 40);

		//Self-driving class configuration
		Driving_DH DH(false, 1.00);
		bool cornerFlag(false);
		bool manualFlag(false);
		int detectedLineCnt(-1);
		double steerVal(50.0);			//초기 각도(50이 중심)
		DH.mappingSet(cornerFlag);		//조향수준 맵핑값 세팅

		bool rotaryFlag(false);
		int flicker(4);

		//color detecting class ganerate
		DetectColorSign detectColorSign(true);
		DetectColorSign startCheck(true);
		bool waitingFlag(true);

		//메인동작 루프
		while (true)
		{
			videocap >> distortedFrame;
			remap(distortedFrame, frame, map1, map2, INTER_LINEAR);
			if (waitingFlag)
			{
				if (detectColorSign.waitingCheck(frame, 20))
					flicker = 2;
				else if (startCheck.waitingCheck(frame, 15))
					flicker = 4;
				else
					waitingFlag = false;
			}
			else if (detectColorSign.priorityStop(frame,1.5))
			{
				DCmotor.stop();
			}
			else //정상주행
			{
				DH.driving(frame, steerVal, detectedLineCnt, rotaryFlag);

				if (!cornerFlag && (steerVal == 90 || steerVal == 10))	//최대 각 검출되면 cornerFlag ON
				{
					cornerFlag = true;
					DH.mappingSet(cornerFlag);
					cout << "cornerFlag ON" << '\n';
				}
				//else if (cornerFlag && detectedLineCnt == 2)				//직선 두개 검출되면 cornerFlag OFF
				else if (cornerFlag && (steerVal >= 40 && steerVal <= 60))	//각도가 좁아지면 cornerFlag OFF
				{
					cornerFlag = false;
					DH.mappingSet(cornerFlag);
					cout << "cornerFlag OFF" << '\n';
				}
				if (!manualFlag) steering.setRatio(steerVal);

				DCmotor.go(37);
			}

			namedWindow("frame", WINDOW_NORMAL);
			imshow("frame", frame);
			resizeWindow("frame", 480, 360);
			moveWindow("frame", 320, 80 + 240);

			// LED 관리코드
			rightLed.off();
			leftLed.off();
			if (steerVal > 60)
			{
				rightLed.on();
				whiteLed.off();
			}
			else if (steerVal < 40)
			{
				leftLed.on();
				whiteLed.off();
			}
			else
			{
				if (!flicker)
					flicker = 4;
				if (2 < flicker--)
					whiteLed.on();
				else
					whiteLed.off();
			}

			//키입력 관리코드 ( 0 = 수동모드, w = 전진, x = 후진, s = 멈춤, ESC 탈출 )
			int key = waitKey(33);
			if (key == 27)
				break;
			else if (key == '0')
			{
				manualFlag = !manualFlag;
				Manual.guide();
				cout << "Auto driving start" << endl;
			}
			else if (manualFlag && key != -1)
			{
				Manual.input(key);
				rewind(stdin);
			}
			else if (key == 'w')
				DCmotor.go(37);
			else if (key == 'x')
				DCmotor.backward(40);
			else if (key == 's')
				DCmotor.stop();

		}
	}
	//End Driving mode


	else if (mode == 5) //Mode 5 : Parking(석준) ---------------------------------------------
	{
		//주차 기본세팅
		double sideDistance = 0;	  // 측면 거리센서 값
		double backDistance = 0;	  // 후방 거리센서 값
		int caseNum = 0;			  // Switch - Case 문 변수
		bool parkingComplete = false; // 주차 완료를 나타내는 플래그
		bool sensingFlag(false);
		TickMeter tm;

		//주행 기본세팅
		Driving_DH DH(true, 1.00);
		bool cornerFlag(false);
		bool manualFlag(false);
		bool rotaryFlag(false);
		int detectedLineCnt(-1);
		double steerVal(50.0);			//초기 각도(50이 중심)
		DH.mappingSet(cornerFlag);		//조향수준 맵핑값 세팅
		int flicker(4);

		steering.setRatio(52);		//조향 우측으로 보정
		DCmotor.go(40);


		while (!parkingComplete)
		{
			videocap >> distortedFrame;
			remap(distortedFrame, frame, map1, map2, INTER_LINEAR); //캘리된 영상 frame

			//LED관리 코드
			if (caseNum == 0)
			{
				DH.driving(frame, steerVal, detectedLineCnt, rotaryFlag);
				if (!flicker)
					flicker = 4;
				if (2 < flicker--)
					whiteLed.on();
				else
					whiteLed.off();
			}
			else
			{
				whiteLed.off();
				if (!flicker)
					flicker = 4;
				if (2 < flicker--)
				{
					putText(frame, "~~Parking~~", Point(frame.cols / 4, frame.rows * 0.65), FONT_HERSHEY_COMPLEX, 1, Scalar(255, 123, 0), 2);
					leftLed.on();
					rightLed.on();
				}
				else
				{
					leftLed.off();
					rightLed.off();
				}
			}
			//ㄷ

			namedWindow("frame", WINDOW_NORMAL);
			imshow("frame", frame);
			resizeWindow("frame", 480, 360);
			moveWindow("frame", 320, 80 + 240);

			sideDistance = secondSonic.distance();  //초음파 거리측정.
			waitKey(50);
			backDistance = firstSonic.distance(); //초음파 거리측정.

			cout << "sideDistance : " << sideDistance << endl;
			cout << "backDistance : " << backDistance << endl;


			cout << "caseNum : " << caseNum << endl;

			switch (caseNum)
			{
				//주차 상황 판단전
			case 0:
				cout << "기본 주행 코드" << endl;
				if (sideDistance < 30) // 처음 벽을 만나면 다음 분기로 이동
					caseNum = 1;
				break;

			case 1:
				cout << "벽 처음 감지" << endl;
				if (sideDistance > 30) // 벽을 지나 주차공간을 만나면 다음 분기로 이동
					caseNum = 2;
				break;

			case 2:
				if (!sensingFlag)	//주차공간의 폭을 측정
				{
					sensingFlag = true;
					tm.start();
				}
				cout << "주차 공간 감지" << endl;
				if (sideDistance < 30) //
				{
					DCmotor.stop();
					tm.stop();
					double widthTime = tm.getTimeMilli();
					cout << "폭 감지 시간 = " << widthTime << endl;
					//sfsf
					//if (widthTime > 600)	//폭 길 경우 -> 수평
					if (true)
					{
						cout << "수평 주차로 판단한다." << endl;
						DCmotor.stop();
						waitKey(500);
						DCmotor.go();
						waitKey(1100);
						DCmotor.stop();
						waitKey(500);
						steering.setRatio(90); // 바퀴를 오른쪽으로 돌린 후 후진
						DCmotor.backward(40);
						caseNum = 103;
					}
					else 	//폭 짧음 -> 수직
					{
						cout << "수직 주차로 판단한다." << endl;
						DCmotor.stop();
						waitKey(500);
						DCmotor.backward();
						waitKey(550);
						DCmotor.stop();
						steering.setRatio(20);
						DCmotor.go(40);
						waitKey(1000);
						DCmotor.stop();
						steering.setRatio(95); // 바퀴를 오른쪽으로 돌린 후 후진
						DCmotor.backward(40);
						caseNum = 203;
					}
				}
				break;
				//주차 상황 판단 완료.


				//수평 주차 시작---------------------------------------------
			case 103:
				cout << "수평) 후진 진행 - 1 -" << endl;
				if ((backDistance < 14))
				{ // 후진 중 어느정도 주차공간에 진입하였으면 다음 분기로 이동
					DCmotor.stop();
					waitKey(500);
					DCmotor.go();
					waitKey(800);
					DCmotor.stop();
					waitKey(500);
					steering.setRatio(10); // 바퀴를 왼쪽으로 돌린 후 후진
					DCmotor.backward(40);
					caseNum = 104;
				}
				break;
			case 104:
				cout << "수평) 후진 진행 - 2 -" << endl;
				if (/*(sideDistance < 12) ||*/ (backDistance < 10))
				{
					DCmotor.stop(); // 3초 정도 대기, sleep 함수 이용 or clock 함수로 시간 측정하여 이용
					steering.setRatio(50);
					waitKey(3000);
					steering.setRatio(0);
					caseNum = 105;
				}
				break;
			case 105:
				cout << "수평) 주차 완료 및 차량 복귀" << endl;
				DCmotor.go(); // 바퀴 조향은 그대로 탈출
				waitKey(1000);
				steering.setRatio(90);
				waitKey(1500);
				DCmotor.stop();
				if (1)
				{ // 주차 분기 탈출 구문으로 차선이 검출되면 주차 분기를 탈출한다.
					waitKey(2000);
					cout << "Detect line and keep going" << endl;
					caseNum = 106;
				}
				break;
				//End 수평 주차---------------------------------------------



				//수직 주차 시작---------------------------------------------
			case 203:
				cout << "수직) 후진 진행 - 1 -" << endl;
				if (sideDistance < 12)
				{ // 후진 중 어느정도 주차공간에 진입하였으면 다음 분기로 이동
					DCmotor.stop();
					steering.setRatio(50); // 바퀴를 왼쪽으로 돌린 후 후진
					DCmotor.backward();
					caseNum = 205;
				}
				break;
			case 205:
				cout << "수직) 후진 진행 - 2 -" << endl;
				if (backDistance < 8)
				{
					DCmotor.stop(); // 3초 정도 대기, sleep 함수 이용 or clock 함수로 시간 측정하여 이용
					waitKey(3000);
					caseNum = 206;
				}
				break;
			case 206:
				DCmotor.go(); // 바퀴 조향은 그대로 탈출
				waitKey(1000);
				steering.setRatio(100);
				waitKey(1700);
				steering.setRatio(50);
				waitKey(300);
				DCmotor.stop();
				if (true)
				{ // 주차 분기 탈출 구문으로 차선이 검출되면 주차 분기를 탈출한다.
					waitKey(1000);
					cout << "Detect line and keep going" << endl;
					caseNum = 207;
				}
				break;
				//End 수직주차 case---------------------------------------------


			default:
				parkingComplete = true;
				if (parkingComplete)
					cout << "Parking branck is done" << endl;
				DCmotor.stop();
				break;
			}
			//waitKey(150);
			waitKey(100);
		}

		while (parkingComplete)//주행코드
		{
			videocap >> distortedFrame;
			remap(distortedFrame, frame, map1, map2, INTER_LINEAR);

			DH.driving(frame, steerVal, detectedLineCnt, rotaryFlag);

			if (!cornerFlag && (steerVal == 90 || steerVal == 10))	//최대 각 검출되면 cornerFlag ON
			{
				cornerFlag = true;
				DH.mappingSet(cornerFlag);
				cout << "cornerFlag ON" << '\n';
			}
			//else if (cornerFlag && detectedLineCnt == 2)				//직선 두개 검출되면 cornerFlag OFF
			else if (cornerFlag && (steerVal >= 40 && steerVal <= 60))	//각도가 좁아지면 cornerFlag OFF
			{
				cornerFlag = false;
				DH.mappingSet(cornerFlag);
				cout << "cornerFlag OFF" << '\n';
			}
			steering.setRatio(steerVal);
			DCmotor.go(37);


			namedWindow("frame", WINDOW_NORMAL);
			imshow("frame", frame);
			resizeWindow("frame", 480, 360);
			moveWindow("frame", 320, 80 + 240);
			waitKey(33);

			// LED 관리코드
			rightLed.off();
			leftLed.off();
			if (steerVal > 60)
			{
				rightLed.on();
				whiteLed.off();
			}
			else if (steerVal < 40)
			{
				leftLed.on();
				whiteLed.off();
			}
			else
			{
				if (!flicker)
					flicker = 4;
				if (2 < flicker--)
					whiteLed.on();
				else
					whiteLed.off();
			}

		}

	}
	//End Parking mode


	else if (mode == 6)//Mode 6 : Rotary(상민) ----------------------------------------------
	{
		//Self-driving class configuration
		Driving_DH DH(true, 1.00);
		bool cornerFlag(false);
		int detectedLineCnt(-1);
		DH.mappingSet(cornerFlag);	//조향수준 맵핑값 세팅
		double steerVal(50.0);	//초기 각도(50이 중심)
		double speedVal(40.0);	//초기 속도(0~100)
		double speedVal_rotary(38.0);
		double Distance;	//거리값

		bool rotaryFlag(true);
		int rotaryDelayFlag = 0;
		RoundAbout Rotary;
		//메인동작 루프
		while (true)
		{
			videocap >> distortedFrame;
			remap(distortedFrame, frame, map1, map2, INTER_LINEAR);

			Distance = firstSonic.distance();	//초음파 거리측정.

			cout << "distance = " << Distance << endl;	//거리출력

			if (!Rotary.isStop(Distance)) // 회전 교차로 진입 (흰색 차선에서 멈춰있다고 가정)
			{
				if (Rotary.isDelay(Distance)) { // 앞의 차량과 가까워졌을 시 정지
					DCmotor.stop();
					cout << "<<< stop! >>>" << endl;
					putText(frame, "Stop!", Point(frame.cols / 4, frame.rows * 0.65), FONT_HERSHEY_COMPLEX, 3.5, Scalar(255), 2);
				}
				else { // 
					DH.driving(frame, steerVal, detectedLineCnt, rotaryFlag);

					if (!cornerFlag && steerVal == 90 || steerVal == 10)	//최대 각 검출되면 cornerFlag ON
					{
						cornerFlag = true;
						DH.mappingSet(cornerFlag);
						cout << "cornerFlag ON" << '\n';
					}
					else if (cornerFlag && detectedLineCnt == 2)			//직선 두개 검출되면 cornerFlag OFF
					{
						cornerFlag = false;
						DH.mappingSet(cornerFlag);
						cout << "cornerFlag OFF" << '\n';
					}

					steering.setRatio(steerVal);

					DCmotor.go(speedVal_rotary);

					//
					// LED 관리코드
					rightLed.off();
					leftLed.off();
					if (steerVal > 60)
					{
						rightLed.on();
						whiteLed.off();
					}
					else if (steerVal < 40)
					{
						leftLed.on();
						whiteLed.off();
					}
					else
					{
						whiteLed.on();
					}
				}
			}
			else {
				putText(frame, "Waiting!!", Point(frame.cols / 4, frame.rows * 0.65), FONT_HERSHEY_COMPLEX, 3.5, Scalar(255), 2);
			}
			namedWindow("frame", WINDOW_NORMAL);
			imshow("frame", frame);
			resizeWindow("frame", 480, 360);
			moveWindow("frame", 320, 80 + 240);

			int key = waitKey(33);
			if (key == 27) break;	//프로그램 종료 ESC(아스키코드 = 27)키.
		}

	}
	//End Rotary mode


	else if (mode == 7) //Mode 7 : Overtaking(민수) ------------------------------------------
	{
		Driving_DH DH(true, 1.00);
		bool cornerFlag(false);
		int detectedLineCnt(-1);
		DH.mappingSet(cornerFlag); //조향수준 맵핑값 세팅
		double steerVal(50.0);								   //초기 각도(50이 중심)
		double speedVal(40.0);								   //초기 속도(0~100)
		bool rotaryFlag(false);
		double Distance_first; //거리값
		double Distance_second;
		const double MAX_ULTRASONIC = 35; //30CM 최대
		const double MAX_SIDE_ULTRASONIC = 50;
		const double MIN_ULTRASONIC = 5;  //4CM 최소

		//초음파 센서 하나인 경우
		int delay = 900;
		cout << "delay = 900";
		int switchCase = 0;//0은 기본주행
		bool delayFlag = false;//상태유지 flag
		const int MAX_holdFlag = 3;
		int holdFlag = 0;
		while (true)
		{
			DCmotor.go();
			videocap >> distortedFrame;
			remap(distortedFrame, frame, map1, map2, INTER_LINEAR); //캘리된 영상 frame

			Distance_first = firstSonic.distance();	  //초음파 거리측정 1번센서.
			Distance_second = secondSonic.distance(); //초음파 거리측정 2번센서.
			cout << "전방 센서 거리 : " << Distance_first << endl;
			cout << "측면 센서 거리 : " << Distance_second << endl;

			DH.driving(frame, steerVal, detectedLineCnt, rotaryFlag);
			if (switchCase != 0)	//추월중일때 frame에 표시용도
			{
				putText(frame, "~~Overtaking~~", Point(frame.cols / 4, frame.rows * 0.80), FONT_HERSHEY_COMPLEX, 1, Scalar(0, 255, 0), 2);
			}

			switch (switchCase) {
			case 0:
				cout << "직진중" << endl;
				leftLed.off();
				rightLed.off();
				whiteLed.off();

				if (Distance_first < MAX_ULTRASONIC) {
					switchCase = 1;//회전부분으로 이동
				}
				if (steerVal > 60) { rightLed.on(); }
				else if (steerVal < 40) { leftLed.on(); }
				else { whiteLed.on(); }
				rotaryFlag = false;
				break;

			case 1: //좌회전 중
				cout << "1) 추월 시작 및 좌회전 중" << endl;
				steerVal = 0;
				delayFlag = true;
				switchCase = 2;
				leftLed.on();
				break;

			case 2: //각도 다시 변환
				cout << "2) 각도 조정중" << endl;
				leftLed.on();
				steerVal = 90;
				rotaryFlag = true;
				//차선인식되서 돌아가는지 확인 필요
				delayFlag = true;
				if (steerVal <= 60 || steerVal >= 40) {
					switchCase = 3;
				}
				break;

			case 3:
				cout << "3) 추월 직진 중" << endl;
				//차선이 생기면 여기에 driving넣으면됨
				leftLed.off();
				whiteLed.on();
				holdFlag++;
				delayFlag = false;
				cout << "HoldFlag : " << holdFlag << endl;
				if (Distance_second > MAX_SIDE_ULTRASONIC && holdFlag >= MAX_holdFlag) {
					holdFlag = 0;
					switchCase = 4;
				}

				break;

			case 4:
				steerVal = 90;
				cout << "4) 추월 후 복귀중" << endl;
				whiteLed.off();
				rightLed.on();
				delayFlag = true;
				rotaryFlag = false;
				switchCase = 5;
				break;

			case 5:
				cout << "5) 복귀 후 각도조정중" << endl;
				rightLed.on();
				delayFlag = false;
				switchCase = 0;
				break;
			}
			//switch문 종료

			namedWindow("frame", WINDOW_NORMAL);
			imshow("frame", frame);
			resizeWindow("frame", 480, 360);
			moveWindow("frame", 320, 80 + 240);

			steering.setRatio(steerVal);
			if (delayFlag)
			{
				delayFlag = false;
				waitKey(delay);
				if (switchCase == 3) { waitKey(delay / 2); }//유의
			}

			if (waitKey(50) == 27) {
				break; //프로그램 종료 ESC키.
			}
		}

		//waitKey(100);
		//0.3초당 1frame 처리
		// steering.setRatio(50);	//바퀴조향
		// DCmotor.go();			//dc모터 전진 argument로 속도전달가능
		// DCmotor.backward();		//dc모터 후진 argument로 속도전달가능
		// DCmotor.stop();			//정지
	}
	//End Overtaking mode

   else if (mode == 8) //Mode 8 : Tunnel(대희) ----------------------------------------------------
   {
   double leftDistance; //좌측 거리값
   double rightDistance; //우측 거리값

   ManualMode Manual(pca, 40);

   Driving_DH DH(false, 1.00);
   bool cornerFlag(false);
   bool manualFlag(false);
   int detectedLineCnt(-1);
   double steerVal(50.0);         //초기 각도(50이 중심)
   DH.mappingSet(cornerFlag);      //조향수준 맵핑값 세팅

   bool rotaryFlag(false);
   int flicker(4);

   //color detecting class ganerate
   bool waitingFlag(true);

   DetectColorSign detectColorSign(true);
   CheckStart cs;
   bool check_tunnel_start(false);
   while (true)
   {
	   videocap >> distortedFrame;
	   remap(distortedFrame, frame, map1, map2, INTER_LINEAR); //캘리된 영상 frame

	   if(!check_tunnel_start)check_tunnel_start = cs.isTunnel(frame, 73);
	   if(check_tunnel_start)check_tunnel_start = cs.isTunnel(frame, 45);
	   //check_tunnel = detectColorSign.detectTunnel(frame, 50);
	   if (!cs.isStop(frame, 65)) {
		   if (check_tunnel_start) // 터널 입장
		   {
			   whiteLed.on();   //전조등 킨다.
			   steerVal = 51;
			   DCmotor.go(37);

			   //leftDistance = firstSonic.distance();   //좌측 거리측정.
			   //rightDistance = secondSonic.distance(); //우측 거리측정.
			   //double angle = rightDistance - leftDistance;
			   //angle *= 2;	//민감도
			   //if (angle > 10) angle = 10;   //최대 15으로 제한.
			   //else if (angle < -10)angle = -10;
			   //angle = 50 + angle;
			   //steering.setRatio(angle);
			   DCmotor.go(30);
		   }
		   else   //기본주행
		   {
			   cs.GetFlag_tunnel();
			   whiteLed.off();

			   DH.driving(frame, steerVal, detectedLineCnt, rotaryFlag);
			   steerVal = 51;
			   if (!cornerFlag && (steerVal == 90 || steerVal == 10))   //최대 각 검출되면 cornerFlag ON
			   {
				   cornerFlag = true;
				   DH.mappingSet(cornerFlag);
				   cout << "cornerFlag ON" << '\n';
			   }
			   //else if (cornerFlag && detectedLineCnt == 2)            //직선 두개 검출되면 cornerFlag OFF
			   else if (cornerFlag && (steerVal >= 40 && steerVal <= 60))   //각도가 좁아지면 cornerFlag OFF
			   {
				   cornerFlag = false;
				   DH.mappingSet(cornerFlag);
				   cout << "cornerFlag OFF" << '\n';
			   }
			   if (!manualFlag) steering.setRatio(steerVal);

			   DCmotor.go(37);
		   }
	   }
	   namedWindow("frame", WINDOW_NORMAL);
	   imshow("frame", frame);
	   resizeWindow("frame", 480, 360);
	   moveWindow("frame", 320, 80 + 240);
	   if (waitKey(33) == 27)
		   break; //프로그램 종료 ESC키.
   }
   }
	//End Tunnel mode

	else if (mode == 9) //Mode 9 : Tunnel(대희) ----------------------------------------------------
	{


	}
	//End Tunnel mode

	whiteLed.off();
	rightLed.off();
	leftLed.off();
	allServoReset(pca);
	cout << "-------------[program finished]-------------" << endl
		<< endl;
	return 0;
}
