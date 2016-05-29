#include "stdafx.h"
#include "setting.h"
#include <Windows.h>
#include <iostream>
#include <opencv2\imgproc.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>

using namespace cv;
using namespace std;


//당구대 위치 수동세팅 완료됐는지
bool poolSet = false;
int poolSetNumber = 0;
//당구대 꼭지점
Point poolPos[2];

//개체들
Rect btnResetPoolPos;


//메인 윈도우에 그려질 캔버스
Mat3b canvas;

//카메라 프레임을 저장할 변수
Mat srcImg;
Mat procImg;
Mat outImg;

//화면 위 마우스 위치
int mX, mY;

//화면에 마우스 조작을 했을 경우
void callBackFunc(int event, int x, int y, int flags, void* userdata)
{
	mX = x;
	mY = y;

	//당구대 리셋버튼
	if (btnResetPoolPos.contains(Point(x, y)))
	{
		if (poolSet)	//당구대가 설정되어있는 경우에만 리셋버튼이 눌린다
		{
			if (event == EVENT_LBUTTONDOWN)
			{

			}
			if (event == EVENT_LBUTTONUP)
			{
				cout << "reset Pool Position" << endl;
				for (int i = 0; i < 2; i++)
					poolPos[i] = Point(-1, -1);
				poolSet = false;
				canvas(btnResetPoolPos) = Vec3b(200, 200, 200);
				putText(canvas(btnResetPoolPos), "Pool Position Resetting(LT)",
					Point(btnResetPoolPos.width*0.02, btnResetPoolPos.height*0.7),
					FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 0));
			}
		}
	}
	
	else
	{
		
	}

	//당구대 위치 설정
	if (!poolSet)
	{
		//클릭하면 그 점을 당구대 꼭지점으로 설정한다.
		if (event == EVENT_LBUTTONDOWN)
		{
			if (x >= 0 && x < IMG_W && y >= 0 && y < IMG_H)
			{
				poolPos[poolSetNumber].x = x;
				poolPos[poolSetNumber].y = y;
				poolSetNumber += 1;
				canvas(btnResetPoolPos) = Vec3b(200, 200, 200);
				putText(canvas(btnResetPoolPos), "Pool Position Resetting(RD)",
					Point(btnResetPoolPos.width*0.02, btnResetPoolPos.height*0.7),
					FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 0));
				if (poolSetNumber == 2)
				{
					poolSetNumber = 0;
					poolSet = true;
					canvas(btnResetPoolPos) = Vec3b(200, 200, 200);
					putText(canvas(btnResetPoolPos), format("Reset Pool Pos", poolSetNumber + 1),
						Point(btnResetPoolPos.width*0.1, btnResetPoolPos.height*0.7),
						FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 0));
				}
			}
		}
	}

}

int main()
{
	cerr << cv::getBuildInformation();

	//당구대 꼭지점 초기화
	for each (Point p in poolPos)
	{
		p.x = -1;
		p.y = -1;
	}

	//카메라 열기
	VideoCapture capture(0);

	//윈도우 생성
	namedWindow("Main");
	namedWindow("Display", CV_WINDOW_FULLSCREEN);

	resizeWindow("Main", IMG_W, IMG_H + PANEL_H);

	//마우스 콜백함수 연결
	setMouseCallback("Main", callBackFunc);

	//개체들 생성
	try
	{
		canvas = Mat3b(IMG_H+PANEL_H, IMG_W, Vec3b(0, 0, 0));

		//버튼
		btnResetPoolPos = Rect(0, IMG_H, 230, PANEL_H);
		canvas(btnResetPoolPos) = Vec3b(200, 200, 200);
		putText(canvas(btnResetPoolPos), "Pool Position Resetting(1)",
			Point(btnResetPoolPos.width*0.02, btnResetPoolPos.height*0.7),
			FONT_HERSHEY_PLAIN, 1, Scalar(0, 0, 0));

	}
	catch (Exception e)
	{
		cout << "GUI에러 : " << e.msg << endl;
	}

	//FPS표시 문자열 초기화
	char strBuf[STRBUFFER] = { 0, };

	//캡쳐 루프(프레임 가져와서 srcImg에 담는다)
	while (capture.read(srcImg))
	{
		try
		{
			//결과만 그릴 이미지
			outImg = Mat(Size(srcImg.cols, srcImg.rows), srcImg.type(), Scalar(0, 0, 0));

			//당구대 위치가 설정 된 상태이면
			if (poolSet)
			{
				//선명하게
				/*Mat sharpen_kernel = (Mat_<char>(3, 3) << 0, -1, 0,
														-1, 5, -1,
														0, -1, 0);
				filter2D(srcImg, srcImg, srcImg.depth(), sharpen_kernel);*/

				//이미지 그레이스케일 변환
				cvtColor(srcImg, procImg, COLOR_BGR2GRAY, 0);

				//이미지 가우시안블러 적용
				//GaussianBlur(procImg, procImg, Size(3, 3), 0);

				//원 배열
				vector<Vec3f> circles;

				//원 검출 (circles에 담는다)
				HoughCircles(procImg, circles, CV_HOUGH_GRADIENT, 1, DIST_BALL, 60, 10, MAX_BALL_SIZE, MIN_BALL_SIZE);

				//검출된 원 중 가장자리에 붙어있거나 가장자리 넘어간 원들을 다 지운다
				//그리고 지워지지 않는 원을 
				Point pntGuideline1(poolPos[0].x + DIST_BALL / 2.3, poolPos[0].y + DIST_BALL / 2.3);
				Point pntGuideline2(poolPos[1].x - DIST_BALL / 2.3, poolPos[1].y - DIST_BALL / 2.3);
				vector<Vec3f>::const_iterator itc = circles.begin();
				while (itc != circles.end())
				{
					Point curCircleP = Point((*itc)[0], (*itc)[1]);
					//당구대 밖에 있는 공은 지운다.
					if (!curCircleP.inside(Rect(pntGuideline1, pntGuideline2)))
					{
						circles.erase(itc);
						itc = circles.begin();
						continue;
					}
					//당구대 구석에 근접한 공도 지운다.
					if (norm(curCircleP - poolPos[0]) < DIST_BALL ||
						norm(curCircleP - poolPos[1]) < DIST_BALL ||
						norm(curCircleP - Point(poolPos[0].x, poolPos[1].y)) < DIST_BALL ||
						norm(curCircleP - Point(poolPos[1].x, poolPos[0].y)) < DIST_BALL)
					{
						circles.erase(itc);
						itc = circles.begin();
						continue;
					}

					//살아남은 공은 그린다
					circle(outImg, Point((*itc)[0], (*itc)[1]), (*itc)[2] * 2, Scalar(255,255,255), 2);
					circle(srcImg, Point((*itc)[0], (*itc)[1]), (*itc)[2], Scalar(255,0,0), 2);

					++itc;
				}

				//당구대 가장자리 선을 그린다
				rectangle(srcImg, Rect(poolPos[0], poolPos[1]), Scalar(255, 255, 255), 2);
				rectangle(srcImg, Rect(pntGuideline1, pntGuideline2), Scalar(0, 255, 255), 2);
				rectangle(outImg, Rect(poolPos[0], poolPos[1]), Scalar(255, 255, 255), 2);
				/*for (int i = 0; i < 4; i++)
				{
					line(srcImg, poolPos[i], poolPos[i + 1 >= 4 ? 0 : i + 1], Scalar(0, 255, 0), 2);
					line(outImg, poolPos[i], poolPos[i + 1 >= 4 ? 0 : i + 1], Scalar(255,255,255), 2);
				}*/
			}
			//현재 당구대 위치 설정중이면
			else
			{
				//십자선(가이드라인)을 그린다.
				line(srcImg, Point(0, mY), Point(IMG_W, mY), Scalar(100, 100, 100));
				line(srcImg, Point(mX, 0), Point(mX, IMG_H), Scalar(100, 100, 100));
			}

			//당구대 꼭지점을 그린다. 값이 비어있으면 안그린다.
			for (int i = 0; i < 2; i++)
			{
				if(poolPos[i].x >= 0 && poolPos[i].y >= 0)
					circle(srcImg, poolPos[i], 3, Scalar(0,0,255));
			}
			

			//적용된 이미지 출력
			srcImg.copyTo(canvas(Rect(0, 0, srcImg.cols, srcImg.rows)));

			//출력
			imshow("Main", canvas);
			imshow("Display", outImg);
		}
		catch(Exception e)
		{
			cout << "에러 : " << e.msg << endl;
		}

		//ESC to escape program
		if (cvWaitKey(33) >= 27) break;
	}
	srcImg.release();
	outImg.release();
	procImg.release();
	capture.release();
	cvDestroyAllWindows();
	
	return 0;
}

