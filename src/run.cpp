#include "Header.h"
#include <vector>

void edgeDetectOpenCV(Mat image) {
	Mat grad, src_gray;
	// Remove noise by blurring with a Gaussian filter ( kernel size = 3 )
	GaussianBlur(image, src_gray, Size(3, 3), 0, 0, BORDER_DEFAULT);

	Mat grad_x, grad_y;
	Mat abs_grad_x, abs_grad_y;
	
	Sobel(src_gray, grad_x, CV_16S, 1, 0, 3, 1, 0, BORDER_DEFAULT);
	Sobel(src_gray, grad_y, CV_16S, 0, 1, 3, 1, 0, BORDER_DEFAULT);
	
	// converting back to CV_8U
	convertScaleAbs(grad_x, abs_grad_x);
	convertScaleAbs(grad_y, abs_grad_y);

	/*Sobel(src_gray, abs_grad_x, CV_8U, 1, 0, 3, 1, 0, BORDER_DEFAULT);
	Sobel(src_gray, abs_grad_y, CV_8U, 0, 1, 3, 1, 0, BORDER_DEFAULT);*/

	addWeighted(abs_grad_x, 0.5, abs_grad_y, 0.5, 0, grad);
	/*imshow("Gradient X", abs_grad_x);
	waitKey(0);
	imshow("Gradient Y", abs_grad_y);
	waitKey(0);*/
	imshow("OPENCV Test Edge Detection", grad);
	waitKey(0);
}


int main() {
	string filename = "test.jpg";
	if (std::filesystem::exists(filename)) {
		cout << "File existance confirmed" << endl;
	}
	Mat srcImg = imread(filename, IMREAD_GRAYSCALE);
	if (!srcImg.data) {
		cout << "Cannot read image" << endl;
		return -1;
	}

	imshow("Source Image", srcImg);
	waitKey(0);
	//===========================

	Mat edgeImg = srcImg.clone();	
	GaussianBlur(edgeImg, edgeImg, cv::Size(3, 3), 0);
	edgeImg = detectEdge(edgeImg);

	imshow("Edge Detection Result", edgeImg);
	waitKey(0);
	//====================
	edgeDetectOpenCV(srcImg);

	//====================

	return 0;
}