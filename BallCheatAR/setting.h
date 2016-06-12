/*
BallCheatAR
Projection mapping project.
Detect balls and cue shaft, draw some effects and expected path using OpenCV
jk012345@gmail.com
Jujin Kim, Jaehyun Sim, Yeongjin Lee.
*/

#pragma once

#define STRBUFFER 64
#define IMG_W 853
#define IMG_H 480
#define PANEL_H 30

#define Range_H_INIT 10
#define Range_S_INIT 55
#define Range_V_INIT 90
#define CUE_H_INIT 40
#define CUE_S_INIT 40
#define CUE_V_INIT 40

#define MIN_BALL_SIZE 13
#define MAX_BALL_SIZE 15
#define DIST_BALL (MIN_BALL_SIZE+MAX_BALL_SIZE)

#define NUMBER_OF_LINE 3

#define DEFAULT_SATURATION 255

#define WHITE_BALL_TRACK_LIMIT 5