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
void printMat(Mat input);

void conv2D(Mat input, Mat kernel, Mat& output, int channel);

Mat combineEdge(Mat edgeX, Mat edgeY, float threshold);

Mat detectEdge(Mat input, bool detail = false, float threshold = -1);

void min_max_normalization(Mat& image, float threshold);

// SEAM CARVING
template <class MyMat> int* findCostArr(MyMat* matrix, const int& nrow, const int& ncol);
int* findSeam(int* cost, const int& nrow, const int& ncol);
template <class MyMat> void removeSeam(MyMat*& pixels, const int& nrow, int& ncol, int* path, const int& nchannel);
void SeamCarving(Mat input, int npixels);
