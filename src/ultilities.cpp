#include "Header.h"


template <class In, class Kern, class Out> void conv2D(Mat input, Mat kernel, Mat& output, int channel = 0) {
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
			float value = 0;
			/*
			* Travel the kernel to do convoluting
			*/
			for (int kRow = 0; kRow < kernel.rows; kRow++) {
				for (int kCol = 0; kCol < kernel.cols; kCol++) {

					int in_PixelX = x + (kRow - 1);
					int in_PixelY = y + (kCol - 1);

					// ignore input samples which are out of bound
					if (in_PixelY >= 0 && in_PixelY < input.rows && in_PixelX >= 0 && in_PixelX < input.cols) {
						value += input.at<In>(input.channels() * (in_PixelY * input.cols + in_PixelX) + channel) * kernel.at<Kern>(kRow * kernel.cols + kCol);
					}
				}
			}
			if ((int)value < minValue) minValue = value; // check value range of convolution result
			if ((int)value > maxValue) maxValue = value;
			output.at<Out>(output.channels() * (y * output.cols + x) + channel) = (int)value; // update result
		}
	}
	cout << "Min-max value in convolution operation= " << minValue << "-" << maxValue << endl;
}


void printMat(Mat input) {
	for (uint16_t y_index = 0; y_index < input.rows; y_index++)
	{
		for (uint16_t x_index = 0; x_index < input.cols; x_index++)
		{
			for (uint8_t channel_idx = 0; channel_idx < input.channels(); channel_idx++)
			{
				cout << (int)input.at<int>(input.channels() * (y_index * input.cols + x_index) + channel_idx);
				if (channel_idx < input.channels() - 1) cout << "-";
			}
			if (x_index < input.cols - 1) cout << "  ";
		}
		cout << endl;
	}
}
// Use for grayscale image
template <class Before, class After> void min_max_normalization(Mat& image, float threshold) {
	float min = 1000000, max = 0;
	int i, j;
	for (i = 0; i < image.rows; i++) {
		for (j = 0; j < image.cols; j++) {
			if (image.at<Before>(i * image.cols + j) < min) {
				min = image.at<Before>(i * image.cols + j);
			}
			else if (image.at<Before>(i * image.cols + j) > max) {
				max = image.at<Before>(i * image.cols + j);
			}
		}
	}

	for (i = 0; i < image.rows; i++) {
		for (j = 0; j < image.cols; j++) {
			float ratio = (float)(image.at<Before>(i * image.cols + j) - min) / (max - min);
			//if (ratio < threshold) ratio = 0;
			image.at<Before>(i * image.cols + j) = ratio * numeric_limits<After>::max();
		}
	}
	cout << "norm func: max and min = " << max << ", " << min << endl;
	cout << "norm for max value of output = " << (int)numeric_limits<After>::max() << endl;
}


template <class I, class O> Mat combineEdge(Mat edgeX, Mat edgeY, float threshold) {

	if (edgeX.cols != edgeY.cols || edgeX.rows != edgeY.rows) {
		cout << "Cannot combine two Mat with different size" << endl;
		exit(-5);
	}
	int min = INT16_MAX, max = 0;

	// Combine to gain edge
	Mat out(edgeX.size(), CV_8U);
	Mat sqrt_out(edgeX.size(), CV_16U);
	for (uint16_t y_index = 0; y_index < out.rows; y_index++)
	{
		for (uint16_t x_index = 0; x_index < out.cols; x_index++)
		{
			int tx = edgeX.at<I>(y_index * edgeX.cols + x_index);
			int ty = edgeY.at<I>(y_index * edgeY.cols + x_index);
			int sqx = tx * tx;
			int sqy = ty * ty;
			int value = static_cast<I>(std::sqrt(sqx + sqy));
			sqrt_out.at<I>(y_index * out.cols + x_index) = value;
			if (value > max) max = value;
			else if (value < min) min = value;
		}
	}
	cout << "min-max value of combine matrix = " << min << "--" << max << endl;
	//=====================================================
	//Filtering to output type
	for (uint16_t y_index = 0; y_index < out.rows; y_index++)
	{
		for (uint16_t x_index = 0; x_index < out.cols; x_index++)
		{
			float ratio = static_cast<float>(sqrt_out.at<I>(y_index * out.cols + x_index) - min) / static_cast<float>(max - min);
			
			if (threshold > 0 && ratio < threshold) ratio = 0;
			//out.at<O>(y_index * out.cols + x_index) = static_cast<O>((ratio > threshold) * static_cast<O>(numeric_limits<O>::max()));
			out.at<O>(y_index * out.cols + x_index) = static_cast<O>(ratio * static_cast<O>(numeric_limits<O>::max()));
		}
	}
	return out;
}


Mat detectEdge(Mat input, bool detail, float threshold) {
	Mat edgeX(input.size(), CV_16S);
	Mat edgeY(input.size(), CV_16S);
	// Using for low detail requirement
	Mat SobeliX = (Mat_<char>(3, 3) << -1, 0, 1, -2, 0, 2, -1, 0, 1);
	Mat SobeliY = (Mat_<char>(3, 3) << 1, 2, 1, 0, 0, 0, -1, -2, -1);
	// Using for high detail requirement
	Mat SobelfX = (Mat_<float>(3, 3) << 0.25, 0, -0.25, 0.5, 0, -0.5, 0.25, 0, -0.25);
	Mat SobelfY = (Mat_<float>(3, 3) << 0.25, 0.5, 0.25, 0, 0, 0, -0.25, -0.5, -0.25);

	if (detail == false) {
		conv2D<uchar, char, int16_t>(input, SobeliX, edgeX, 0);
		conv2D<uchar, char, int16_t>(input, SobeliY, edgeY, 0);
	}
	else {
		conv2D<uchar, float, int16_t>(input, SobelfX, edgeX, 0);
		conv2D<uchar, float, int16_t>(input, SobelfY, edgeY, 0);
	}

	Mat dest = combineEdge<int16_t, uchar>(edgeX, edgeY, threshold).clone();

	return dest;
}
// SEAM CARVING
