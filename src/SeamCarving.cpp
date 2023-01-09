#define _CRT_SECURE_NO_WARNINGS
//#define EXIT_FAILURE -5

#include <iostream>
#include <math.h>
#include <stdio.h>
#include <limits>
#include <string.h>
#include <stdint.h>

using namespace std;

//=======================================================================================
// READ AND WRITE IMAGE FILE
//=======================================================================================

void readPnm(char* fileName, int& numChannels, int& width, int& height, uint8_t*& pixels)
{
	FILE* f = fopen(fileName, "r");
	if (f == NULL)
	{
		printf("Cannot read %s\n", fileName);
		exit(EXIT_FAILURE);
	}

	char type[3];
	fscanf(f, "%s", type);
	if (strcmp(type, "P2") == 0)
		numChannels = 1;
	else if (strcmp(type, "P3") == 0)
		numChannels = 3;
	else // In this exercise, we don't touch other types
	{
		fclose(f);
		printf("Cannot read %s\n", fileName);
		exit(EXIT_FAILURE);
	}

	fscanf(f, "%i", &width);
	fscanf(f, "%i", &height);

	int max_val;
	fscanf(f, "%i", &max_val);
	if (max_val > 255) // In this exercise, we assume 1 byte per value
	{
		fclose(f);
		printf("Cannot read %s\n", fileName);
		exit(EXIT_FAILURE);
	}

	pixels = (uint8_t*)malloc(static_cast<int64_t>(width) * height * numChannels);
	for (int i = 0; i < width * height * numChannels; i++)
		fscanf(f, "%hhu", &pixels[i]);

	fclose(f);
}

void writePnm(uint8_t* pixels, int numChannels, int width, int height,
	char* fileName)
{
	FILE* f = fopen(fileName, "w");
	if (f == NULL)
	{
		printf("Cannot write %s\n", fileName);
		exit(EXIT_FAILURE);
	}

	if (numChannels == 1)
		fprintf(f, "P2\n");
	else if (numChannels == 3)
		fprintf(f, "P3\n");
	else
	{
		fclose(f);
		printf("Cannot write %s\n", fileName);
		exit(EXIT_FAILURE);
	}

	fprintf(f, "%i\n%i\n255\n", width, height);

	for (int i = 0; i < width * height * numChannels; i++)
		fprintf(f, "%hhu\n", pixels[i]);

	fclose(f);
}


//=======================================================================================
// IMAGE PROCESSING ON HOST
//=======================================================================================

template <class ElementType> class Matrix_t
{
public:
	unsigned int rows;
	unsigned int cols;
	Matrix_t() = delete;
	Matrix_t(int nrow, int ncol, int nchannel) {
		this->dataPtr = new ElementType[static_cast<int64_t>(nchannel) * nrow * ncol];
		this->rows = nrow;
		this->cols = ncol;
		this->channels_t = nchannel;
	}
	Matrix_t(int nrow, int ncol, int nchannel, ElementType* arr) {
		this->dataPtr = new ElementType[static_cast<int64_t>(nchannel) * nrow * ncol];
		this->rows = nrow;
		this->cols = ncol;
		this->channels_t = nchannel;
		for (int i = 0; i < nchannel * nrow * ncol; i++)
		{
			this->dataPtr[i] = static_cast<ElementType>(arr[i]);
		}
	}
	Matrix_t(const Matrix_t& obj) {
		this->dataPtr = new ElementType[static_cast<int64_t>(obj.channels_t) * obj.rows * obj.cols];
		this->rows = obj.rows;
		this->cols = obj.cols;
		this->channels_t = obj.channels_t;
		/*for (int64_t i = 0; i < static_cast<int64_t>(obj.channels_t) * obj.rows * obj.cols; i++)
		{
			this->dataPtr[i] = obj.dataPtr[i];
		}*/
		memcpy(this->dataPtr, obj.dataPtr, sizeof(ElementType) * static_cast<int64_t>(obj.channels_t) * obj.rows * obj.cols);
	}
	int channels() {
		return this->channels_t;
	}
	void copyTo(ElementType* ptr) {
		//memcpy(this->dataPtr, ptr, sizeof(ElementType) * static_cast<int64_t>(this->channels_t) * this->rows * this->cols);
		for (int row = 0; row < this->rows; row++)
		{
			for (int col = 0; col < this->cols; col++)
			{
				for (int c = 0; c < this->channels(); c++)
				{
					ptr[this->channels() * (row * this->cols + col) + c] = this->dataPtr[this->channels() * (row * this->cols + col) + c];
				}
			}
		}
	}
	/*
	* @brief	Access element of the matrix which data was spread out into 1D array
	* @param	index: index correspond to the 1D array data
	* @retval	Reference to the element
	*/
	ElementType* at(int index) {
		return &(this->dataPtr[index]);
	}
	/*
	* @brief	Access element of matrix with format: pixel[row, col, channel]
	* @param	row: the row index in matrix that element lie
	* @param	col: the col index in matrix that element lie
	* @param	channel: index of channel wanna access
	* @retval	Reference to the element
	*/
	ElementType& at(int row, int col, int channel = 0) {
		return (this->dataPtr[this->channels_t * (row * this->cols + col) + channel]);
	}
	Matrix_t<ElementType> operator=(const Matrix_t<ElementType>& obj) {
		if (this->dataPtr != nullptr) {
			delete[] this->dataPtr;
		}
		this->dataPtr = new ElementType[static_cast<int64_t>(obj.channels_t) * obj.rows * obj.cols]{ 0 };
		memcpy(this->dataPtr, obj.dataPtr, sizeof(ElementType) * static_cast<int64_t>(obj.channels_t) * obj.rows * obj.cols);
		this->rows = obj.rows;
		this->cols = obj.cols;
		this->channels_t = obj.channels_t;
		return *this;
	}

	~Matrix_t() {
		delete[] this->dataPtr;
		this->rows = this->cols = this->channels_t = 0;
	}

private:
	ElementType* dataPtr;
	unsigned int channels_t;

};

// CONVOLUTION OPERATION =======================================================================

/*
* @brief	Produce convolution operation between a matrix and a kernel with padding = 1 by HOST
* @param	input: input matrix
* @param	kernel: convolution kernel
* @param	output: output matrix
* @param	channel: point out which channel to do convolution( for multidimension input matrix)
* @retval	None
*/
template <class In, class Kern, class Out> void conv2D(Matrix_t<In> input, Matrix_t<Kern> kernel, Matrix_t<Out>& output, int channel = 0) {
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
						value += *input.at(input.channels() * (in_PixelY * input.cols + in_PixelX) + channel) * (*kernel.at(kRow * kernel.cols + kCol));
					}
				}
			}
			if (static_cast<int>(value) < minValue) minValue = static_cast<int>(value); // check value range of convolution result
			if (static_cast<int>(value) > maxValue) maxValue = static_cast<int>(value);
			*output.at(output.channels() * (y * output.cols + x) + channel) = static_cast<Out>(value); // update result
		}
	}
}

//==================================================================================================

template <class In> void printMat(Matrix_t<In> input) {


	for (uint16_t row = 0; row < input.rows; row++)
	{
		for (uint16_t col = 0; col < input.cols; col++)
		{
			for (uint8_t channel_idx = 0; channel_idx < input.channels(); channel_idx++)
			{

				cout << (int)static_cast<In>(*input.at(input.channels() * (row * input.cols + col) + channel_idx));
				if (channel_idx < input.channels() - 1) cout << "-";
			}
			if (col < input.cols - 1) cout << "  ";
		}
		cout << endl;
	}
}
// CONVERT FROM RGB TO GRAYSCALE IMAGE
template <class MyType> void convert2Gray(Matrix_t<MyType>& src) {
	// Reminder: gray = 0.299*red + 0.587*green + 0.114*blue  
	if (src.channels() == 1) return;
	Matrix_t<MyType> result(src.rows, src.cols, 1);
	for (int row = 0; row < src.rows; row++)
	{
		for (int col = 0; col < src.cols; col++)
		{
			int idx = row * src.cols + col;
			MyType red = *src.at(3 * idx);
			MyType green = *src.at(3 * idx + 1);
			MyType blue = *src.at(3 * idx + 2);
			*result.at(idx) = static_cast<MyType>(0.299f * red + 0.587f * green + 0.114f * blue);
		}
	}
	src = result;
}

// EDGE DETECTION =================================================================================
/*
* @brief	Combine edgeX and edgeY to an image
* @param	edgeX: Gradient X-axis of the image using Kernel X
* @param	edgeY: Gradient Y-axis of the image using Kernel Y
* @retval	The Matrix that show Edge Detection Result
*/
template <class I, class O> Matrix_t<O> combineEdge(Matrix_t<I> edgeX, Matrix_t<I> edgeY, float threshold) {

	if (edgeX.cols != edgeY.cols || edgeX.rows != edgeY.rows) {
		cout << "Cannot combine two Matrix with different size" << endl;
		exit(-5);
	}
	int min = INT16_MAX, max = 0;

	// Combine to gain edge
	Matrix_t<O> out(edgeX.rows, edgeX.cols, 1);
	Matrix_t<unsigned int> sqrt_out(edgeX.rows, edgeX.cols, 1);
	for (uint16_t y_index = 0; y_index < out.rows; y_index++)
	{
		for (uint16_t x_index = 0; x_index < out.cols; x_index++)
		{
			int tx = *edgeX.at(y_index * edgeX.cols + x_index);
			int ty = *edgeY.at(y_index * edgeY.cols + x_index);
			int sqx = tx * tx;
			int sqy = ty * ty;
			int value = static_cast<I>(std::sqrt(sqx + sqy));
			*sqrt_out.at(y_index * out.cols + x_index) = value;
			if (value > max) max = value;
			else if (value < min) min = value;
		}
	}
	//=====================================================
	//Filtering to output type
	for (uint16_t y_index = 0; y_index < out.rows; y_index++)
	{
		for (uint16_t x_index = 0; x_index < out.cols; x_index++)
		{
			float ratio = static_cast<float>(*sqrt_out.at(y_index * out.cols + x_index) - min) / static_cast<float>(max - min);

			if (threshold > 0 && ratio < threshold) ratio = 0;
			//out.at<O>(y_index * out.cols + x_index) = static_cast<O>((ratio > threshold) * static_cast<O>(numeric_limits<O>::max()));
			*out.at(y_index * out.cols + x_index) = static_cast<O>(ratio * static_cast<O>(numeric_limits<O>::max()));
		}
	}
	return out;
}

template <class DType_t> Matrix_t<DType_t> detectEdge(Matrix_t<DType_t> input, bool detail, float threshold = -1) {
	Matrix_t<int> edgeX(input.rows, input.cols, 1);
	Matrix_t<int> edgeY(input.rows, input.cols, 1);
	// Using for low detail requirement
	char sobeliarrx[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
	char sobeliarry[9] = { 1, 2, 1, 0, 0, 0, -1, -2, -1 };
	Matrix_t<char> SobeliX(3, 3, 1, sobeliarrx);
	Matrix_t<char> SobeliY(3, 3, 1, sobeliarry);
	// Using for high detail requirement
	float sobelfarrx[9] = { 0.25, 0, -0.25, 0.5, 0, -0.5, 0.25, 0, -0.25 };
	float sobelfarry[9] = { 0.25, 0.5, 0.25, 0, 0, 0, -0.25, -0.5, -0.25 };
	Matrix_t<float> SobelfX(3, 3, 1, sobelfarrx);
	Matrix_t<float> SobelfY(3, 3, 1, sobelfarry);

	if (detail == false) {
		conv2D<unsigned char, char, int>(input, SobeliX, edgeX, 0);
		conv2D<unsigned char, char, int>(input, SobeliY, edgeY, 0);
	}
	else {
		conv2D<unsigned char, float, int>(input, SobelfX, edgeX, 0);
		conv2D<unsigned char, float, int>(input, SobelfY, edgeY, 0);
	}

	Matrix_t<unsigned char> dest(combineEdge<int, unsigned char>(edgeX, edgeY, threshold));

	return dest;
}
// SEAM CARVING ===============================================================================

template <class MyType> int* findCostArr(MyType* matrix, const int& nrow, const int& ncol) {
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
		if (cost[0 * ncol + vmin_idx] > cost[0 * ncol + i]) vmin_idx = i;
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
template <class MyType> void removeSeam(MyType*& pixels, const int& nrow, int& ncol, int* path, int nchannel = 1) {
	MyType* newPixels = new MyType[static_cast<int64_t>(nchannel) * nrow * (ncol - 1)]{ 0 };
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
/*
* @brief	Produce Seam Carving to change image size
* @param	input: array pixels of image
* @param	nrow: height of image
* @param	ncol: width of image
* @param	nchannel: number of channel of image
* @param	output: a null pointer to store result
* @param	npixels: number pixel in the width that we want to remove
* @retval	None
*/
template <class MyType> int SeamCarving(MyType* input, int nrow, int& ncol, int nchannel, MyType*& output, int npixels = 1) {
	int edge_ncol = ncol;
	if (npixels > ncol) {
		cout << "Cannot resize image too much" << endl;
		return 0;
	}
  // Initialize timer
  GpuTimer timer;
	timer.Start();
	// Initialize variable
	output = new MyType[static_cast<int64_t>(nchannel) * nrow * ncol];
	MyType* edgepixel = new MyType[static_cast<int64_t>(nrow) * ncol];
	memcpy(output, input, sizeof(MyType) * static_cast<MyType>(nchannel) * nrow * ncol);

	// GRAYSCALE Edge Image
	Matrix_t<MyType> edgeImg(nrow, ncol, nchannel, input);
	if (nchannel == 3) {
		convert2Gray<MyType>(edgeImg);
	}

	// Edge Detection using as an Energy Map
	// blurImage(edgeImg); 
	edgeImg = detectEdge<MyType>(edgeImg, false);
	edgeImg.copyTo(edgepixel);
	// Save Edge Detection result for reporting
	writePnm(edgepixel, 1, ncol, nrow, (char*)"edgeDetectionImg.pnm");
	// Find and remove seam
	for (int i = 0; i < npixels; i++) {
		// Calculate cost for pixel
		int* cost = findCostArr<MyType>(edgepixel, nrow, edge_ncol);
		// Find Seam Path which can remove from image
		int* seam = findSeam(cost, nrow, edge_ncol);
		// Remove that seams 
		removeSeam<MyType>(output, nrow, ncol, seam, nchannel);
		removeSeam<MyType>(edgepixel, nrow, edge_ncol, seam);
		// End the manipulation progress
		delete[] cost;
		delete[] seam;
	}
  timer.Stop();
	float time = timer.Elapsed();
	cout << "Processing time: " << time << " ms " << endl;
	// Collecting garbage
	delete[] edgepixel;
	// Return result
	return 1;

}


int main(int argc, char** argv)
{
	// Check command line argument of the program
	if (argc != 3 && argc != 4)
	{
		printf("The number of arguments is invalid\n");
		return EXIT_FAILURE;
	}
  
	// Read input RGB image file
	int numChannels, width, height;
	uint8_t* inPixels;
	readPnm(argv[1], numChannels, width, height, inPixels);
	//readPnm((char*)"in.pnm", numChannels, width, height, inPixels);

	if (numChannels != 3)
		return EXIT_FAILURE; // Input image must be RGB
	printf("Image size (width x height): %i x %i\n\n", width, height);

  //Calculate horizontal pixel number to carve
  char* carvingPercent_str = argv[3];
  int percent = atoi(carvingPercent_str);
  if (percent > 70) {
    cout << "Carve the image more than 70% is not recommended and maybe loss more information" << endl;
  }
  int carvePixel = static_cast<int>(width * static_cast<float>(percent)/100);

	Matrix_t<uint8_t> img(height, width, numChannels, inPixels);

	// Produce Processing
	// Convert RGB to grayscale not using device
	uint8_t* correctOutPixels = NULL;
	int state = SeamCarving<uint8_t>(inPixels, height, width, numChannels, correctOutPixels, carvePixel);
	if (state == 0) {
		cout << "Cannot perform Seam Carving" << endl;
	}
	else {
		// Write results to files
		char* outFileNameBase = strtok(argv[2], "."); // Get rid of extension
		strcat(outFileNameBase, "_host.pnm");
		writePnm(correctOutPixels, numChannels, width, height, outFileNameBase);
		//writePnm(correctOutPixels, numChannels, width, height, (char*)"SeamCarvingResult.pnm");
	}


	cout << "Image size after Resize(width x height): " << width << " x " << height << endl;

	// Free memories
	free(inPixels);
	free(correctOutPixels);

	return 0;
}