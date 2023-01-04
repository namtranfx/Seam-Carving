#include "Header.h"


template <class T> void conv2D(Mat input, Mat kernel, Mat& output, int channel = 0) {
	if (input.channels() != output.channels()) {
		cout << "Two matrix have different channel" << endl;
		exit(-1);
	}

	int minValue = 1000000;
	int maxValue = 0;
	/*
	* Travel the input matrix
	*/
	for (int y = 0; y < input.rows; y++) {
		for (int x = 0; x < input.cols; x++) {
			/*
			* For each pixel, we calculate convolution operation with the kernel
			*/
			int value = 0;
			/*
			* Travel the kernel to do convoluting
			*/
			for (int kRow = 0; kRow < kernel.rows; kRow++) {    
				for (int kCol = 0; kCol < kernel.cols; kCol++) {
					
					int in_PixelX = x + (kRow - 1);
					int in_PixelY = y + (kCol - 1);

					// ignore input samples which are out of bound
					if (in_PixelY >= 0 && in_PixelY < input.rows && in_PixelX >= 0 && in_PixelX < input.cols) {
						value += input.at<uchar>(input.channels() * (in_PixelY * input.cols + in_PixelX) + channel) * kernel.at<char>(kRow* kernel.cols + kCol);
					}
				}
			}
			if (value < minValue) minValue = value; // check value range of convolution result
			if (value > maxValue) maxValue = value;
			output.at<T>(output.channels() * (y * output.cols + x) + channel) = value; // update result
		}
	}
	cout << "Min-max value in convolution operation= " << minValue << "-" << maxValue << endl;
}

Mat dot(Mat& matrix1, Mat& matrix2) {
	return Mat();
}

template <class T>
T* pnmAt(Mat& image, int& indexX, int& indexY) {

}


void printMat(Mat input) {
	for (uint16_t y_index = 0; y_index < input.rows; y_index++)
	{
		for (uint16_t x_index = 0; x_index < input.cols; x_index++)
		{
			for (uint8_t channel_idx = 0; channel_idx < input.channels(); channel_idx++)
			{
				cout << (int)input.at<char>(input.channels() * (y_index * input.cols + x_index) + channel_idx);
				if (channel_idx < input.channels() - 1) cout << "-";
			}
			if (x_index < input.cols - 1) cout << "  ";
		}
		cout << endl;
	}
}
// Use for grayscale image
template <class T> void min_max_normalization(Mat& image) {
	float min = 1000000, max = 0;
	int i, j;
	for (i = 0; i < image.rows; i++) {
		for (j = 0; j < image.cols; j++) {
			if (image.at<T>(i * image.cols + j) < min) {
				min = image.at<T>(i * image.cols + j);
			}
			else if (image.at<T>(i * image.cols + j) > max) {
				max = image.at<T>(i * image.cols + j);
			}
		}
	}

	for (i = 0; i < image.rows; i++) {
		for (j = 0; j < image.cols; j++) {
			float ratio = (float)(image.at<T>(i * image.cols + j) - min) / (max - min);
			if (ratio < 0.72) ratio = 0;
			image.at<T>(i * image.cols + j) = ratio * 255;
		}
	}
	cout << "norm func: max and min = " << max << ", " << min << endl;
}

template <class I, class O> Mat combineEdge(Mat edgeX, Mat edgeY) {
	Mat out(edgeX.size(), CV_8U);
	for (uint16_t y_index = 0; y_index < out.rows; y_index++)
	{
		for (uint16_t x_index = 0; x_index < out.cols; x_index++)
		{
			int tx = edgeX.at<I>(y_index * edgeX.cols + x_index);
			int ty = edgeY.at<I>(y_index * edgeY.cols + x_index);
			int sqx = tx * tx;
			int sqy = ty * ty;
			//out.at<O>(y_index * out.cols + x_index) = sqrt(sqx + sqy);
			out.at<O>(y_index * out.cols + x_index) = static_cast<unsigned char>((static_cast<float16_t>(
				std::sqrt(static_cast<float>(sqx + sqy))) > 0.72) * 255U);
		}
	}
	return out;
}

Mat detectEdge(Mat input) {
	Mat edgeX(input.size(), CV_16S);
	Mat edgeY(input.size(), CV_16S);

	Mat kernelX = (Mat_<char>(3, 3) << -1, 0, 1, -2, 0, 2, -1, 0, 1);
	Mat kernelY = (Mat_<char>(3, 3) << 1, 2, 1, 0, 0, 0, -1, -2, -1);
	
	imshow("Source Image", input);
	waitKey(0);

	cout << "Source pixel(maybe junk value) = " << (int)edgeX.at<int16_t>(5, 5) << endl;
	conv2D<int16_t>(input, kernelX, edgeX, 0);
	cout << "pixel = " << (int)edgeX.at<int16_t>(5, 5) << endl;
	min_max_normalization<int16_t>(edgeX);
	cout << "After normalization = " << (int)edgeX.at<int16_t>(5, 5) << endl;
	/*imshow("Edge Detection X", edgeX);
	waitKey(0);*/


	conv2D<int16_t>(input, kernelY, edgeY, 0);
	min_max_normalization<int16_t>(edgeY);
	/*imshow("Edge Detection Y", edgeY);
	waitKey(0);*/

	Mat dest = combineEdge<int16_t, uchar>(edgeX, edgeY).clone();
	
	return dest;
}