#pragma once

#include <string>
#include <opencv/cv.h>
#include <opencv/highgui.h>

using namespace std;
using namespace cv;

class Ball
{
public:
	Ball();
	~Ball();
	Ball(string name);

	int getXPos() { return xPos; }
	void setXPos(int x) { xPos = x; }
	int getYPos() { return yPos; }
	void setYPos(int y) { yPos = y; }

	Scalar getHSVmin();
	Scalar getHSVmax();
	void setHSVmin(Scalar min);
	void setHSVmax(Scalar max);

	Scalar getColor() {
		return Color;
	}
	void setColor(Scalar c) {
		Color = c;
	}

	void setName(string name) { this->name = name; }
	string getName() { return name; }

private:
	int xPos, yPos;
	string name;
	Scalar HSVmin, HSVmax;
	Scalar Color;
};
