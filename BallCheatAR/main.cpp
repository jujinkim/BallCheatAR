#include "stdafx.h"
#include "setting.h"
#include "button.h"
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
Button btnResetPoolPos;
Button btnCParam1Add, btnCParam1Sub;
Button btnCParam2Add, btnCParam2Sub;
Button btnCTh1Add, btnCTh1Sub;
Button btnCTh2Add, btnCTh2Sub;

Button lblCparam1, lblCparam2, lblCTh1, lblCTh2;
Button lblCparam1lbl, lblCparam2lbl, lblCTh1lbl, lblCTh2lbl;

Button chkLine;

//메인 윈도우에 그려질 캔버스
Mat3b canvas;

//카메라 프레임을 저장할 변수
Mat srcImg;
Mat procImg;
Mat outImg;

//화면 위 마우스 위치
int mX, mY;

bool drawLine = false;
double cParam1 = 80, cParam2 = 10;
double cTh1 = 80, cTh2 = 10;

//화면에 마우스 조작을 했을 경우
void callBackFunc(int event, int x, int y, int flags, void* userdata)
{
	mX = x;
	mY = y;

	//당구대 리셋버튼
	if (btnResetPoolPos.isInPos(x, y))
	{
		if (poolSet)	//당구대가 설정되어있는 경우에만 리셋버튼이 눌린다
		{
			if (event == EVENT_LBUTTONUP)
			{
				cout << "reset Pool Position" << endl;
				for (int i = 0; i < 2; i++)
					poolPos[i] = Point(-1, -1);
				poolSet = false;
				btnResetPoolPos.setText("Pool Position Resetting(LT)");
			}
		}
		return;
	}

	//라인 검출 체크버튼
	if (chkLine.isInPos(x, y))
	{
		if (event == EVENT_LBUTTONUP)
		{
			drawLine = !drawLine;
			chkLine.setText(drawLine?"Line O":"Line X");
		}
	}
	
#pragma region 매개변수 설정 버튼 함수
	//cParam1
	if (btnCParam1Add.isInPos(x, y)) 
	{
		if (event == EVENT_LBUTTONUP)
		{
			cParam1 += 5;
			lblCparam1.setText(format("%d", (int)cParam1));
		}
	}
	if (btnCParam1Sub.isInPos(x, y))
	{
		if (event == EVENT_LBUTTONUP)
		{
			cParam1 -= cParam1>5?5:0;
			lblCparam1.setText(format("%d", (int)cParam1));
		}
	}

	//cParam2
	if (btnCParam2Add.isInPos(x, y))
	{
		if (event == EVENT_LBUTTONUP)
		{
			cParam2 += 5;
			lblCparam2.setText(format("%d", (int)cParam2));
		}
	}
	if (btnCParam2Sub.isInPos(x, y))
	{
		if (event == EVENT_LBUTTONUP)
		{
			cParam2 -= cParam2>5?5:0;
			lblCparam2.setText(format("%d", (int)cParam2));
		}
	}

	//cTh1
	if (btnCTh1Add.isInPos(x, y))
	{
		if (event == EVENT_LBUTTONUP)
		{
			cTh1 += 5;
			lblCTh1.setText(format("%d", (int)cTh1));
		}
	}
	if (btnCTh1Sub.isInPos(x, y))
	{
		if (event == EVENT_LBUTTONUP)
		{
			cTh1 -= cTh1>5 ? 5 : 0;
			lblCTh1.setText(format("%d", (int)cTh1));
		}
	}

	//cTh2
	if (btnCTh2Add.isInPos(x, y))
	{
		if (event == EVENT_LBUTTONUP)
		{
			cTh2 += 5;
			lblCTh2.setText(format("%d", (int)cTh2));
		}
	}
	if (btnCTh2Sub.isInPos(x, y))
	{
		if (event == EVENT_LBUTTONUP)
		{
			cTh2 -= cTh2>5 ? 5 : 1;
			lblCTh2.setText(format("%d", (int)cTh2));
		}
	}
	
#pragma endregion

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
				btnResetPoolPos.setText("Pool Position Resetting(RD)");
				if (poolSetNumber == 2)
				{
					poolSetNumber = 0;
					poolSet = true;
					btnResetPoolPos.setText("Reset Pool Pos");
				}
			}
		}
	}

}

Point* isIntersection(Point p1, Point p2, Point p3, Point p4) {
	// Store the values for fast access and easy
	// equations-to-code conversion
	float x1 = p1.x, x2 = p2.x, x3 = p3.x, x4 = p4.x;
	float y1 = p1.y, y2 = p2.y, y3 = p3.y, y4 = p4.y;

	float d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
	// If d is zero, there is no intersection
	if (d == 0) return NULL;

	// Get the x and y
	float pre = (x1*y2 - y1*x2), post = (x3*y4 - y3*x4);
	float x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
	float y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

	// Check if the x and y coordinates are within both lines
	if (x < min(x1, x2) || x > max(x1, x2) ||
		x < min(x3, x4) || x > max(x3, x4)) return NULL;
	if (y < min(y1, y2) || y > max(y1, y2) ||
		y < min(y3, y4) || y > max(y3, y4)) return NULL;

	// Return the point of intersection
	Point* ret = new Point();
	ret->x = x;
	ret->y = y;
	return ret;
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
	namedWindow("Canny");
	namedWindow("Display", CV_WINDOW_FULLSCREEN);

	resizeWindow("Main", IMG_W, IMG_H + PANEL_H*3+30);

	//마우스 콜백함수 연결
	setMouseCallback("Main", callBackFunc);

	//개체들 생성
	try
	{
		canvas = Mat3b(IMG_H+PANEL_H*3+30, IMG_W, Vec3b(0, 0, 0));

		//버튼
		btnResetPoolPos = Button(canvas, 0, IMG_H, 260, PANEL_H, "Pool Position Resetting(LT)", Scalar(200, 200, 200));

		btnCParam1Add = Button(canvas, 80, IMG_H+PANEL_H+10, 20, PANEL_H, "+", Scalar(230, 230, 230));
		btnCParam1Sub = Button(canvas, 60+80, IMG_H+PANEL_H+10, 20, PANEL_H, "-", Scalar(230, 230, 230));
		btnCTh1Add = Button(canvas, 100+160, IMG_H+PANEL_H+10, 20, PANEL_H, "+", Scalar(230, 230, 230));
		btnCTh1Sub = Button(canvas, 160+160, IMG_H+PANEL_H+10, 20, PANEL_H, "-", Scalar(230, 230, 230));
		btnCParam2Add = Button(canvas, 80, IMG_H + PANEL_H * 2 + 20, 20, PANEL_H, "+", Scalar(230, 230, 230));
		btnCParam2Sub = Button(canvas, 60+80, IMG_H+PANEL_H*2+20, 20, PANEL_H, "-", Scalar(230, 230, 230));
		btnCTh2Add = Button(canvas, 100+160, IMG_H+PANEL_H*2+20, 20, PANEL_H, "+", Scalar(230, 230, 230));
		btnCTh2Sub = Button(canvas, 160+160, IMG_H+PANEL_H*2+20, 20, PANEL_H, "-", Scalar(230, 230, 230));

		//레이블
		lblCparam1 = Button(canvas, 20+80, IMG_H+PANEL_H+10, 40, PANEL_H, format("%d", (int)cParam1), Scalar(255, 255, 255));
		lblCTh1 = Button(canvas, 200+80, IMG_H+PANEL_H+10, 40, PANEL_H, format("%d", (int)cTh1), Scalar(255, 255, 255));
		lblCparam2 = Button(canvas, 20+80, IMG_H+PANEL_H*2+20, 40, PANEL_H, format("%d", (int)cParam2), Scalar(255, 255, 255));
		lblCTh2 = Button(canvas, 200+80, IMG_H + PANEL_H*2+20, 40, PANEL_H, format("%d", (int)cTh2), Scalar(255, 255, 255));
		lblCparam1lbl = Button(canvas, 0, IMG_H + PANEL_H+10, 80, PANEL_H, "cParam1", Scalar(255, 255, 255));
		lblCTh1lbl = Button(canvas, 180, IMG_H + PANEL_H+10, 80, PANEL_H, "cTh1", Scalar(255, 255, 255));
		lblCparam2lbl = Button(canvas, 0, IMG_H + PANEL_H * 2+20, 80, PANEL_H, "cParam2", Scalar(255, 255, 255));
		lblCTh2lbl = Button(canvas, 180, IMG_H + PANEL_H * 2+20, 80, PANEL_H, "cTh2", Scalar(255, 255, 255));

		//체크박스
		chkLine = Button(canvas, IMG_W - 80, IMG_H, 80, PANEL_H, "Line X", Scalar(230, 230, 230));
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
				GaussianBlur(procImg, procImg, Size(3, 3), 0);

				//원(당구공) 검출 (circles에 담는다)
				vector<Vec3f> circles;				
				HoughCircles(procImg, circles, CV_HOUGH_GRADIENT, 1, DIST_BALL, cParam1, cParam2, MAX_BALL_SIZE, MIN_BALL_SIZE);

				//캐니 변환
				Canny(procImg, procImg, cTh1, cTh2);
				imshow("Canny", procImg);

				//라인(큐대) 검출(lines에 담는다)
				vector<Vec2f> lines;
				if(drawLine) HoughLines(procImg, lines, 1, CV_PI / 180, 220);

				//검출된 원 중 가장자리에 붙어있거나 가장자리 넘어간 원들을 다 지운다
				////////////////////원 그리기///////////////////////
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

				////////////////////선 그리기///////////////////////
				if (drawLine) {
					//당구대 바깥을 겨냥한 선들은 다 지운다
					vector<Vec2f>::const_iterator itc2 = lines.begin();
					while (itc2 != lines.end())
					{
						double a = cos((*itc2)[1]), b = sin((*itc2)[1]);
						double x0 = a*((*itc2)[0]), y0 = b*((*itc2)[0]);
						Point pt1, pt2;
						pt1.x = round(x0 + IMG_W * (-b));
						pt1.y = round(y0 + IMG_W * (a));
						pt2.x = round(x0 - IMG_W * (-b));
						pt2.y = round(y0 - IMG_W * (a));


						//사각형의 각 변과의 교점을 찾는다. 교점이 하나도 없으면 당구대 밖이므로 지운다.
						/*if (!isIntersection(pt1, pt2, poolPos[0], Point(poolPos[1].x, poolPos[0].y)) &&
							!isIntersection(pt1, pt2, poolPos[0], Point(poolPos[0].x, poolPos[1].y)) &&
							!isIntersection(pt1, pt2, Point(poolPos[0].x, poolPos[1].y), poolPos[1]) &&
							!isIntersection(pt1, pt2, Point(poolPos[1].x, poolPos[0].y), poolPos[1]))*/
						if (!isIntersection(pt1, pt2, pntGuideline1, Point(pntGuideline2.x, pntGuideline1.y)) &&
							!isIntersection(pt1, pt2, pntGuideline1, Point(pntGuideline1.x, pntGuideline2.y)) &&
							!isIntersection(pt1, pt2, Point(pntGuideline1.x, pntGuideline2.y), pntGuideline2) &&
							!isIntersection(pt1, pt2, Point(pntGuideline2.x, pntGuideline1.y), pntGuideline2))
						{
							lines.erase(itc2);
							itc2 = lines.begin();
							continue;
						}

						////살아남은 라인을 그린다. 
						/*Point pt1 = Point((*itc2)[0], (*itc2)[1]);
						Point pt2 = Point((*itc2)[2], (*itc2)[3]);*/

						line(outImg, pt1, pt2, Scalar(255, 255, 255));
						line(srcImg, pt1, pt2, Scalar(0, 0, 255));

						++itc2;
					}
				}

				//당구대 가장자리 선을 그린다
				rectangle(srcImg, Rect(poolPos[0], poolPos[1]), Scalar(255, 255, 255), 2);
				rectangle(srcImg, Rect(pntGuideline1, pntGuideline2), Scalar(0, 255, 255), 2);
				rectangle(outImg, Rect(poolPos[0], poolPos[1]), Scalar(255, 255, 255), 2);
			
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
