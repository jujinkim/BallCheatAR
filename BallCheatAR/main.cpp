#include <Windows.h>
#include <iostream>
#include <opencv2\imgproc.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>

#include "setting.h"
#include "button.h"
#include "functions.h"


using namespace cv; 
using namespace std;

#pragma region Button Objs
Button btnResetPoolPos;	//(버튼) 당구대 위치 리셋
Button chkLine;			//(토글버튼) 선 켬끔
Button btnPoolColor;	//(버튼) 당구대 색상 리셋
Button btnCuePointColor;	//(버튼) 큐대 포인트쪽 색상 리셋
Button btnCueStickColor;	//(버튼) 큐대 막대쪽 색상 리셋
#pragma endregion

//메인 윈도우에 그려질 캔버스
Mat3b canvas;

#pragma region Mat
Mat srcImg;	//카메라 이미지
Mat hsvImg;	//HSV변환 이미지
Mat procImg;	//프로세싱용
Mat gryImg;	//그레이스케일 이미지
Mat thdImg;	//이진화 이미지
Mat thdWhiteImg;	//흰공탐색 이진화이미지
Mat proc_cuePImg;	//큐대-포인트쪽
Mat proc_cueSImg;	//큐대-막대쪽
Mat outImg;	//빔프로젝터 출력 이미지
Mat img_lbl, stats, centroids;
#pragma endregion

//화면 위 마우스 위치
int mX, mY;

//선을 그릴까?
bool drawLine = false;

//매개변수들
int cParam1 = 80, cParam2 = 9, cParam2W = 15;
int cTh1 = 80, cTh2 = 20;

//반지름
double radiusMultiply = 2;

//Perspective Transform Parameter (50 -> middle, 0~100)
int perspectiveParameter = 50;

//White ball position
Point WhiteBallPos;
int whiteBallLostCount = 0;
//int whiteBallROIRange = 150;	//흰 공을 중심으로 ROI를 하나 더 만든다. 이것은 흰 공 주변의 '큐대'를 탐지하는 용도이다.

#pragma region 당구대 위치 설정용
bool poolSet = false;	//당구대 위치 수동세팅 완료됐는지
int poolSetNumber = 0;	//0이면 찍을 점이 두 점, 1이면 한 점 남음. 2이면 설정 완료
Point poolPos[2];		//당구대 꼭지점. 
float poolWidth = 0, poolHeight = 0;	//당구대 가로/세로. (poolPos 기반)
int roiRange = 20;		//당구대 위치로부터 roiRange값 만큼 확대해서 ROI(Region of Interest)를 만듬
Point poolPosROI[2];	//당구대 위치에서 조금(roiRange) 확대한거. ROI가 된다.
float poolROIWidth = 0, poolROIHeight = 0;	//ROI 가로/세로
#pragma endregion


#pragma region 당구대/큐대 색상
//당구대 색상
Scalar poolColor = Scalar(255, 255, 255);
int poolRangeH = Range_H_INIT, poolRangeS = Range_S_INIT, poolRangeV = Range_V_INIT;
bool poolColorSet = true;

//큐대 색상
Scalar cuePntColor = Scalar(255, 255, 255);
int cuePntCRangeH = CUE_H_INIT, cuePntCRangeS = CUE_S_INIT, cuePntCRangeV = CUE_V_INIT;
bool cuePntSet = true;
Scalar cueStkColor = Scalar(255, 255, 255);
int cueStkCRangeH = CUE_H_INIT, cueStkCRangeS = CUE_S_INIT, cueStkCRangeV = CUE_V_INIT;
bool cueStkSet = true;
#pragma endregion


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
					poolWidth = abs(poolPos[1].x - poolPos[0].x);
					poolHeight = abs(poolPos[1].y - poolPos[0].y);

					poolSetNumber = 0;
					poolSet = true;
					btnResetPoolPos.setText("Reset Pool Pos");
				}
			}
		}
	}

	//당구대 색상 설정시작버튼
	if (btnPoolColor.isInPos(x, y))
	{
		if (event == EVENT_LBUTTONUP)
		{
			poolColorSet = false;
			btnPoolColor.setText("Setting..");
		}
	}
	//당구대 색상 설정
	if (!poolColorSet)
	{
		try {
			btnPoolColor.setColor(srcImg.at<Vec3b>(y, x));
		}
		catch (Exception e) { cout << "당구대색에러" << e.msg << endl; }
		//클릭하면 그 점을 당구대 색상으로 설정
		if (event == EVENT_LBUTTONDOWN)
		{
			poolColor = Scalar(hsvImg.at<Vec3b>(y, x));
			poolColorSet = true;
			btnPoolColor.setText("Set Pool Color");
		}
	}


	//큐 포인트색상 설정시작버튼
	if (btnCuePointColor.isInPos(x, y))
	{
		if (event == EVENT_LBUTTONUP)
		{
			cuePntSet = false;
			btnCuePointColor.setText("...");
		}
	}
	//큐 포인트색상 설정
	if (!cuePntSet)
	{
		try {
			btnCuePointColor.setColor(srcImg.at<Vec3b>(y, x));
		}
		catch (Exception e) { cout << "큐포인트색에러" << e.msg << endl; }
		//클릭하면 그 점을 큐 포인트색상으로 설정
		if (event == EVENT_LBUTTONDOWN)
		{
			cuePntColor = Scalar(hsvImg.at<Vec3b>(y, x));
			cuePntSet = true;
			btnCuePointColor.setText("C_P_C");
		}
	}

	//큐 막대색상 설정시작버튼
	if (btnCueStickColor.isInPos(x, y))
	{
		if (event == EVENT_LBUTTONUP)
		{
			cueStkSet = false;
			btnCueStickColor.setText("...");
		}
	}
	//큐 막대색상 설정
	if (!cueStkSet)
	{
		try {
			btnCueStickColor.setColor(srcImg.at<Vec3b>(y, x));
		}
		catch (Exception e) { cout << "큐막대색에러" << e.msg << endl; }
		//클릭하면 그 점을 큐 막대색상으로 설정
		if (event == EVENT_LBUTTONDOWN)
		{
			cueStkColor = Scalar(hsvImg.at<Vec3b>(y, x));
			cueStkSet = true;
			btnCueStickColor.setText("C_S_C");
		}
	}
}

int main()
{
	//cerr << cv::getBuildInformation();

	//당구대 꼭지점 초기화
	for each (Point p in poolPos)
	{
		p.x = -1;
		p.y = -1;
	}

	//카메라 열기
	VideoCapture capture(0);

	#pragma region Generate Windows
	namedWindow("Main");
	namedWindow("Canny");
	namedWindow("Threshold");
	namedWindow("Threshold_W");
	namedWindow("Setting");
	namedWindow("Setting HSV Range");
	namedWindow("Display", CV_WINDOW_FREERATIO);

	resizeWindow("Main", IMG_W, IMG_H + PANEL_H);
	resizeWindow("Setting", 400, 170);
	resizeWindow("Setting", 500, 500);
	#pragma endregion

	#pragma region  Generate Trackbars
	createTrackbar("RoI Range", "Setting", &roiRange, 50);
	//createTrackbar("WhiteBall RoIRange", "Setting", &whiteBallROIRange, 150);

	createTrackbar("cParam1", "Setting", &cParam1, 255);
	createTrackbar("cParam2", "Setting", &cParam2, 255);
	createTrackbar("cParam2W", "Setting", &cParam2W, 255);
	/*createTrackbar("cCanny1", "Setting", &cTh1, 255);
	createTrackbar("cCanny2", "Setting", &cTh2, 255);*/
	createTrackbar("output Perspective", "Setting", &perspectiveParameter, 100);

	createTrackbar("pool_range_H", "Setting HSV Range", &poolRangeH, 100);
	createTrackbar("pool_range_S", "Setting HSV Range", &poolRangeS, 100);
	createTrackbar("pool_range_V", "Setting HSV Range", &poolRangeV, 100);

	createTrackbar("cue_p_range_H", "Setting HSV Range", &cuePntCRangeH, 100);
	createTrackbar("cue_p_range_S", "Setting HSV Range", &cuePntCRangeS, 100);
	createTrackbar("cue_p_range_V", "Setting HSV Range", &cuePntCRangeV, 100);

	createTrackbar("cue_s_range_H", "Setting HSV Range", &cueStkCRangeH, 100);
	createTrackbar("cue_s_range_S", "Setting HSV Range", &cueStkCRangeS, 100);
	createTrackbar("cue_s_range_V", "Setting HSV Range", &cueStkCRangeV, 100);
	#pragma endregion
	
	//Mouse Callback method bind.
	setMouseCallback("Main", callBackFunc);

	//Assign Objects.
	try
	{
		canvas = Mat3b(IMG_H+PANEL_H, IMG_W, Vec3b(0, 0, 0));

		//버튼
		btnResetPoolPos = Button(canvas, 0, IMG_H, 260, PANEL_H, "Pool Position Resetting(LT)", Scalar(200, 200, 200));
		chkLine = Button(canvas, IMG_W - 80, IMG_H, 80, PANEL_H, "Line X", Scalar(230, 230, 230));
		btnPoolColor = Button(canvas, 280, IMG_H, 150, PANEL_H, "Reset pool color", poolColor);
		btnCuePointColor = Button(canvas, 450, IMG_H, 50, PANEL_H, "C_P_C", cuePntColor);
		btnCueStickColor = Button(canvas, 500, IMG_H, 50, PANEL_H, "C_S_C", cueStkColor);
	}
	catch (Exception e)	{cout << "GUI에러 : " << e.msg << endl;}
	 
	//FPS표시 문자열 초기화
	char strBuf[STRBUFFER] = { 0, };

	//캡쳐 루프(프레임 가져와서 srcImg에 담는다)
	while (capture.read(srcImg))
	{
		try
		{
			resize(srcImg, srcImg, Size(IMG_W, IMG_H));
			cvtColor(srcImg, hsvImg, COLOR_BGR2HSV);

			//결과만 그릴 이미지
			//outImg = Mat(Size(srcImg.cols, srcImg.rows), srcImg.type(), Scalar(0, 0, 0));

			//당구대 위치와 색깔이 설정 된 상태이면
			if (poolSet && poolColorSet && cuePntSet && cueStkSet)
			{
				#pragma region RoI 설정
				if (poolPos[0].x - roiRange < 0) roiRange = poolPos[0].x;
				if (poolPos[0].y - roiRange < 0) roiRange = poolPos[0].y;
				if (poolPos[1].x + roiRange >= srcImg.cols) roiRange = srcImg.cols - poolPos[1].x;
				if (poolPos[1].y + roiRange >= srcImg.rows) roiRange = srcImg.rows - poolPos[1].y;

				poolPosROI[0].x = poolPos[0].x - roiRange;
				poolPosROI[0].y = poolPos[0].y - roiRange;
				poolPosROI[1].x = poolPos[1].x + roiRange;
				poolPosROI[1].y = poolPos[1].y + roiRange;
				poolROIWidth = abs(poolPosROI[1].x - poolPosROI[0].x);
				poolROIHeight = abs(poolPosROI[1].y - poolPosROI[0].y);
				#pragma endregion
			
				procImg = hsvImg(Rect(poolPosROI[0], poolPosROI[1]));
				outImg = Mat(Size(procImg.cols, procImg.rows), CV_8U, Scalar(0, 0, 0));
				//선명하게
				/*Mat sharpen_kernel = (Mat_<char>(3, 3) << 0, -1, 0,
														-1, 5, -1,
														0, -1, 0);
				filter2D(srcImg, srcImg, srcImg.depth(), sharpen_kernel);*/
				
				//이미지 그레이스케일 변환
				/*cvtColor(srcImg, gryImg, COLOR_BGR2GRAY, 0);
				threshold(gryImg, thdImg, 0, 255, THRESH_BINARY_INV + THRESH_OTSU);*/

				inRange(procImg, Scalar(poolColor[0] - poolRangeH, poolColor[1] - poolRangeS, poolColor[2] - poolRangeV),
								 Scalar(poolColor[0] + poolRangeH, poolColor[1] + poolRangeS, poolColor[2] + poolRangeV), thdImg);
				inRange(procImg, Scalar(0, 0, 180), Scalar(255, 70, 255), thdWhiteImg);
				thdImg = ~thdImg;

				//#pragma region 인식 ROI 설정
				//Point wbPosFull(WhiteBallPos.x + poolPosROI[0].x, WhiteBallPos.y + poolPosROI[0].y);
				//Rect whiteBallRect(wbPosFull.x - whiteBallROIRange, wbPosFull.y - whiteBallROIRange, whiteBallROIRange * 2, whiteBallROIRange * 2);
				//if (wbPosFull.x - whiteBallROIRange < 0) whiteBallRect.x = wbPosFull.x;
				//if (wbPosFull.y - whiteBallROIRange < 0) whiteBallRect.y = wbPosFull.y;
				//if (wbPosFull.x + whiteBallROIRange >= hsvImg.cols) whiteBallRect.width = hsvImg.cols - wbPosFull.x;
				//if (wbPosFull.y + whiteBallROIRange >= hsvImg.rows) whiteBallRect.height = hsvImg.rows - wbPosFull.y;
				//rectangle(srcImg, whiteBallRect, Scalar(0, 255, 255));
				//#pragma endregion 

				inRange(procImg, Scalar(cuePntColor[0] - cuePntCRangeH, cuePntColor[1] - cuePntCRangeS, cuePntColor[2] - cuePntCRangeV),
								 Scalar(cuePntColor[0] + cuePntCRangeH, cuePntColor[1] + cuePntCRangeS, cuePntColor[2] + cuePntCRangeV), proc_cuePImg);
				inRange(procImg, Scalar(cueStkColor[0] - cueStkCRangeH, cueStkColor[1] - cueStkCRangeS, cueStkColor[2] - cueStkCRangeV),
								 Scalar(cueStkColor[0] + cueStkCRangeH, cueStkColor[1] + cueStkCRangeS, cueStkColor[2] + cueStkCRangeV), proc_cueSImg);
				

				//morph -> 빈공간 없애고 작은점 없애고
				morphOpCl(thdImg, 3, 3);
				morphOpCl(thdWhiteImg, 3, 3);
				morphOpCl(proc_cuePImg, 3, 3);
				morphOpCl(proc_cueSImg, 3, 3);
				
				GaussianBlur(thdImg, thdImg, Size(3, 3), 2, 2);

				imshow("Threshold", thdImg);

				//이미지 가우시안블러 적용
				GaussianBlur(thdWhiteImg, thdWhiteImg, Size(3, 3), 2,2);
				
				imshow("Threshold_W", thdWhiteImg);

				//원(당구공) 검출 (circles에 담는다)
				vector<Vec3f> circles;				
				HoughCircles(thdImg, circles, CV_HOUGH_GRADIENT, 1, DIST_BALL, cParam1, cParam2, MAX_BALL_SIZE, 1);
				vector<Vec3f> circlesWhite;
				HoughCircles(thdWhiteImg, circlesWhite, CV_HOUGH_GRADIENT, 1, DIST_BALL, cParam1, cParam2W , MAX_BALL_SIZE, 1);

				//큐대 중심점 검출(라벨링)
				int numOfLables = connectedComponentsWithStats(proc_cuePImg, img_lbl, stats, centroids);
				//가장큰거
				int max = -1, idx = 0;
				for (int i = 0; i < numOfLables; i++)
				{
					int area = stats.at<int>(i, CC_STAT_AREA);
					if (max < area)
					{
						max = area;
						idx = i;
					}
				}
				//stats.at()
				

				//라인(큐대) 검출(lines에 담는다)
				vector<Vec2f> lines;
				if(drawLine) HoughLines(procImg, lines, 1, CV_PI / 180, 150);

				//검출된 원 중 가장자리에 붙어있거나 가장자리 넘어간 원들을 다 지운다
				////////////////////원 그리기///////////////////////
				Point pntGuideline1(roiRange + DIST_BALL / 2.3, roiRange + DIST_BALL / 2.3);
				Point pntGuideline2(roiRange + poolWidth - DIST_BALL / 2.3, roiRange + poolHeight - DIST_BALL / 2.3);
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
					if (norm(curCircleP - Point(roiRange, roiRange)) < DIST_BALL ||
						norm(curCircleP - Point(roiRange + poolWidth, roiRange + poolHeight)) < DIST_BALL ||
						norm(curCircleP - Point(roiRange, roiRange + poolHeight)) < DIST_BALL ||
						norm(curCircleP - Point(roiRange + poolWidth, roiRange)) < DIST_BALL)
					{
						circles.erase(itc);
						itc = circles.begin();
						continue;
					}

					//살아남은 공은 그린다
					circle(outImg, Point((*itc)[0], (*itc)[1]), (*itc)[2] * radiusMultiply, Scalar(255,255,255), 2);
					circle(srcImg, Point((*itc)[0]+poolPosROI[0].x, (*itc)[1]+poolPosROI[0].y), (*itc)[2], Scalar(255,0,0), 2);

					++itc;
				}

				//흰공
				vector<Vec3f>::const_iterator itcW = circlesWhite.begin();
				float maxRadius = 0;
				while (itcW != circlesWhite.end())
				{
					Point curCircleP = Point((*itcW)[0], (*itcW)[1]);
					//당구대 구석에서 검출된건 삭제
					if (norm(curCircleP - Point(roiRange, roiRange)) < DIST_BALL ||
						norm(curCircleP - Point(roiRange + poolWidth, roiRange + poolHeight)) < DIST_BALL ||
						norm(curCircleP - Point(roiRange, roiRange + poolHeight)) < DIST_BALL ||
						norm(curCircleP - Point(roiRange + poolWidth, roiRange)) < DIST_BALL)
					{
						circlesWhite.erase(itcW);
						itcW = circlesWhite.begin();
						continue;
					}

					//최대 반지름을 저장한다
					if((*itcW)[2] > maxRadius)
						maxRadius = (*itcW)[2];
					else
					{
						//최대반지름보다 작으면 지운다.
						circlesWhite.erase(itcW);
						itcW = circlesWhite.begin();
						continue;
					}

					++itcW;
				}
				//흰 공은 하나만 그린다.
				if (circlesWhite.size() > 0)
				{	
					whiteBallLostCount = 0;
					WhiteBallPos = Point(circlesWhite[0][0], circlesWhite[0][1]);
				} 
				else whiteBallLostCount += 1;
				
				if (whiteBallLostCount < WHITE_BALL_TRACK_LIMIT)
				{
					circle(outImg, Point(WhiteBallPos.x, WhiteBallPos.y), MAX_BALL_SIZE + 20, Scalar(255, 255, 255), 15);
					circle(srcImg, Point(WhiteBallPos.x + poolPosROI[0].x, WhiteBallPos.y + poolPosROI[0].y), MAX_BALL_SIZE + 5, Scalar(0, 255, 0), 2);
				}
				else cout << "White Ball Lost" << endl;

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
					   /*if(!isIntersection(pt1, pt2, poolPos[0], Point(poolPos[1].x, poolPos[0].y)) &&
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

						++itc2;
					}

					////살아남은 라인을 그린다. 
					/*Point pt1 = Point((*itc2)[0], (*itc2)[1]);
					Point pt2 = Point((*itc2)[2], (*itc2)[3]);*/
					if (lines.size() == 2)
					{
						itc2 = lines.begin();
						double t1 = (*itc2)[1];
						double a1 = cos(t1), b1 = sin(t1);
						double x01 = a1*((*itc2)[0]), y01 = b1*((*itc2)[0]);
						Point pt1, pt2, pt3, pt4, ptS, ptE, *ptTS = NULL, *ptTE = NULL;
						pt1.x = round(x01 + IMG_W * (-b1));
						pt1.y = round(y01 + IMG_W * (a1));
						pt2.x = round(x01 - IMG_W * (-b1));
						pt2.y = round(y01 - IMG_W * (a1));
						line(srcImg, pt1, pt2, Scalar(255, 255, 255));
						itc2 = lines.end() - 1;
						double t2 = (*itc2)[1];
						double a2 = cos(t2), b2 = sin(t2);
						double x02 = a2*((*itc2)[0]), y02 = b2*((*itc2)[0]);
						pt3.x = round(x02 + IMG_W * (-b2));
						pt3.y = round(y02 + IMG_W * (a2));
						pt4.x = round(x02 - IMG_W * (-b2));
						pt4.y = round(y02 - IMG_W * (a2));
						line(srcImg, pt3, pt4, Scalar(255, 255, 255));

						//최종 선분의 각도(t3)와 좌표(x03, y03)
						double t3 = (t1 + t2) / 2;
						double a3 = cos(t3), b3 = sin(t3);
						double x03 = (x01 + x02) / 2, y03 = (y01 + y02) / 2;
						ptS.x = round(x03 + IMG_W * (-b3));
						ptS.y = round(y03 + IMG_W * (a3));
						ptE.x = round(x03 - IMG_W * (-b3));
						ptE.y = round(y03 - IMG_W * (a3));

						//당구대 안에만
						ptTS			    = isIntersection(ptS, ptE, poolPos[0], Point(poolPos[1].x, poolPos[0].y));

						if (!ptTS)	   ptTS = isIntersection(ptS, ptE, poolPos[0], Point(poolPos[0].x, poolPos[1].y));
						else if(!ptTE) ptTE = isIntersection(ptS, ptE, poolPos[0], Point(poolPos[0].x, poolPos[1].y));

						if (!ptTS)		ptTS = isIntersection(ptS, ptE, Point(poolPos[0].x, poolPos[1].y), poolPos[1]);
						else if (!ptTE) ptTE = isIntersection(ptS, ptE, Point(poolPos[0].x, poolPos[1].y), poolPos[1]);

						if (!ptTS)		ptTS = isIntersection(ptS, ptE, Point(poolPos[1].x, poolPos[0].y), poolPos[1]);
						else if (!ptTE) ptTE = isIntersection(ptS, ptE, Point(poolPos[1].x, poolPos[0].y), poolPos[1]);

						if (ptTS && ptTE) {
							line(outImg, *ptTS, *ptTE, Scalar(255, 255, 255), 3);
							line(srcImg, *ptTS, *ptTE, Scalar(0, 0, 255));

							//벽에 튕긴 후 예상경로
							/*Point ptNE;
							Point ptNS;
							double t4 = -t3;
							double a4 = cos(t4), b4 = sin(t4);

							if (abs(norm(pt1 - pt3)) < abs(norm(pt2 - pt4)))
							{
								ptNS = *ptTS;
								ptNE.x = round(ptTE->x - IMG_W * (-b4));
								ptNE.y = round(ptTE->y - IMG_W * (a3));
							}
							else
							{
								ptNS = *ptTE;
								ptNE.x = round(ptTS->x - IMG_W * (-b4));
								ptNE.y = round(ptTS->y - IMG_W * (a3));
							}

							line(outImg, ptNS, ptNE, Scalar(255, 255, 255), 3);
							line(srcImg, ptNS, ptNE, Scalar(0, 0, 255));*/
						}
					}
				}

				//당구대 가장자리 선을 그린다
				rectangle(srcImg, Rect(poolPosROI[0], poolPosROI[1]), Scalar(0, 0, 255), 1);
				rectangle(srcImg, Rect(poolPos[0], poolPos[1]), Scalar(255, 255, 255), 2);
				rectangle(srcImg, Rect(pntGuideline1 + poolPosROI[0], pntGuideline2 + poolPosROI[0]), Scalar(0, 255, 255), 1);
				rectangle(outImg, Rect(roiRange,roiRange, poolWidth, poolHeight), Scalar(255, 255, 255), 2);

				#pragma region perspectiveTransform 변환
				/*Point2f p1[4], p2[4];
				p1[0] = Point2f(0, 0);
				p1[1] = Point2f(outImg.cols, 0);
				p1[2] = Point2f(outImg.cols, outImg.rows);
				p1[3] = Point2f(0, outImg.rows);

				p2[0] = Point2f(perspectiveParameter>50 ? perspectiveParameter - 50 : 0, 0);
				p2[1] = Point2f(outImg.cols - (perspectiveParameter>50 ? perspectiveParameter - 50 : 0), 0);
				p2[2] = Point2f(outImg.cols - perspectiveParameter<50 ? 50 - perspectiveParameter : 0, outImg.rows);
				p2[3] = Point2f(perspectiveParameter<50 ? 50 - perspectiveParameter : 0, outImg.rows);
				Mat m = getPerspectiveTransform(p1, p2);
				warpPerspective(outImg, outImg, m, Size(outImg.cols, outImg.rows));*/
				#pragma endregion
			}
			//현재 당구대 위치 설정중이면
			else if((poolColorSet || cuePntSet || cueStkSet) && !poolSet)
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
	gryImg.release();
	procImg.release();
	thdImg.release();
	thdWhiteImg.release();
	capture.release();
	cvDestroyAllWindows();
	
	return 0;
}
