#pragma once
#include <iostream>
#include <filesystem>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <math.h>

using namespace std;
using namespace cv;

// Ultilities function
void conv2D(Mat input, Mat kernel, Mat& output, int channel);
Mat dot(Mat& matrix1, Mat& matrix2);
template <class T> T* pnmAt(Mat& image, int& indexX, int& indexY);

void printMat(Mat input);
Mat combineEdge(Mat edgeX, Mat edgeY);
Mat detectEdge(Mat input);
template <class T> void min_max_normalization(Mat& image);