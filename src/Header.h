#pragma once
#include <iostream>
#include <filesystem>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <math.h>
#include <limits>

using namespace std;
using namespace cv;

// Ultilities function
void conv2D(Mat input, Mat kernel, Mat& output, int channel);

void printMat(Mat input);
Mat combineEdge(Mat edgeX, Mat edgeY, float threshold);

Mat detectEdge(Mat input, bool detail = false, float threshold = -1);

void min_max_normalization(Mat& image, float threshold);

// SEAM CARVING
