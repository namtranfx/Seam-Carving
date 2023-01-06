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

template <class MyMat> int* findCostArr(MyMat* matrix, const int& nrow, const int& ncol) {
	int* cost = new int[static_cast<int>(nrow * ncol)]{ 0 };
	for (int i = 0; i < ncol; i++)
	{
		cost[(nrow - 1) * ncol + i] = matrix[(nrow - 1) * ncol + i];
	}
	for (int row = nrow - 2; row >= 0; row--)
	{
		for (int col = 0; col < ncol; col++)
		{
			int col1 = max(col - 1, 0), col2 = min(ncol - 1, col + 1);
			int vmin = cost[(row + 1) * ncol + col];
			if (vmin < cost[(row + 1) * ncol + col1]) vmin = cost[(row + 1) * ncol + col1];
			if (vmin < cost[(row + 1) * ncol + col2]) vmin = cost[(row + 1) * ncol + col2];
			cost[row * ncol + col] = static_cast<int>(matrix[row * ncol + col]) + vmin;
		}
	}
	return cost;
}

int* findSeam(int* cost, const int& nrow, const int& ncol) {
	int* path = new int[nrow] {0};
	int path_idx = 0;
	int vmin_idx = 0;
	for (int i = 1; i < ncol; i++)
	{
		if (cost[0 * ncol + vmin_idx] < cost[0 * ncol + i]) vmin_idx = i;
	}
	path[path_idx++] = vmin_idx;

	for (int row = 0; row < nrow - 1; row++)
	{
		int col1 = max(vmin_idx - 1, 0), col2 = min(ncol - 1, vmin_idx + 1);
		if (cost[(row + 1) * ncol + col1] < cost[(row + 1) * ncol + vmin_idx]) vmin_idx = col1;
		if (cost[(row + 1) * ncol + col2] < cost[(row + 1) * ncol + vmin_idx]) vmin_idx = col2;
		path[path_idx++] = vmin_idx;
	}
	return path;
}
template <class MyMat> void removeSeam(MyMat*& pixels,const int& nrow, int& ncol, int* path, const int& nchannel = 1) {
	MyMat* newPixels = new MyMat[nchannel * nrow * (ncol - 1)]{ 0 };
	int colidx = 0;
	for (int row = 0; row < nrow; row++)
	{
		colidx = 0;
		for (int col = 0; col < ncol; col++)
		{
			if (col == path[row]) continue;
			for (int c = 0; c < nchannel; c++)
			{
				newPixels[nchannel * (row * (ncol - 1) + colidx) + c] = pixels[nchannel * (row * ncol + col) + c];
			}
			colidx++;
		}
	}
	delete[] pixels;
	ncol--;
	pixels = newPixels;
}
void SeamCarving(Mat input, int npixels = 1) {
	int nrow = input.rows, ncol = input.cols, nchannel = input.channels();
	int edge_ncol = input.cols;
	if (npixels > ncol) {
		cout << "Cannot resize image too much" << endl;
		return;
	}
	// Initialize variable
	uchar* pixel = new uchar[nchannel * nrow * ncol]{ 0 };
	uchar* edgepixel = new uchar[nchannel * nrow * ncol]{ 0 };
	memcpy(pixel, input.ptr(), nchannel * nrow * ncol);

	// GRAYSCALE Edge Image
	Mat edgeImg = input.clone();
	if (nchannel == 3) {
		cvtColor(input, edgeImg, cv::COLOR_RGB2GRAY);
	}
	
	// Edge Detection using as an Energy Map
	GaussianBlur(edgeImg, edgeImg, cv::Size(3, 3), 0);
	edgeImg = detectEdge(edgeImg);
	memcpy(edgepixel, edgeImg.ptr(), nchannel * nrow * ncol);
	imshow("Edge Detection Result", edgeImg);
	waitKey(0);
	for (int i = 0; i < npixels; i++) {
		// Calculate cost for pixel
		int* cost = findCostArr<uchar>(pixel, nrow, ncol);
		// Find Seam Path which can remove from image
		int* seam = findSeam(cost, nrow, ncol);
		// Remove that seams 
		removeSeam<uchar>(pixel, nrow, ncol, seam, nchannel);
		removeSeam<uchar>(edgepixel, nrow, edge_ncol, seam);
		// End the manipulation progress
		delete[] cost;
		delete[] seam;
	}
	// Get the final image
	Mat final(nrow, ncol, input.type(), pixel);
	// Show the image
	imshow("Seam Carving Image", final);
	waitKey(0);
}