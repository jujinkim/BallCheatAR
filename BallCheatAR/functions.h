#pragma once
#include <opencv2\imgproc.hpp>
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <algorithm>
using namespace cv;

Point* isIntersection(Point p1, Point p2, Point p3, Point p4);
void morphOps(Mat &thresh);