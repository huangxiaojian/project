#include "stdafx.h"
#include "gazeTracking.h"

#include <vector>
#include <queue>

GazeTracking::GazeTracking():kPlotVectorField(false),
	kEyePercentTop(25), kEyePercentSide(13), kEyePercentHeight(30), kEyePercentWidth(35),
	kSmoothFaceImage(false), kSmoothFaceFactor(0.005),
	kFastEyeWidth(50), kWeightBlurSize(5), kEnableWeight(true), kWeightDivisor(150.0),kGradientThreshold(50.0),
	kEnablePostProcess(true), kPostProcessThreshold(0.97),
	kEnableEyeCorner(false)
{
	createCornerKernels();
}

GazeTracking::~GazeTracking()
{
	releaseCornerKernels();
}

bool GazeTracking::initialize(cv::String fileName)
{
	return faceCascade.load(fileName);
}

void GazeTracking::process(IplImage* image)
{
	cv::Mat frame(image, true);
	process(frame);
}

void GazeTracking::process(cv::Mat& frame)
{
	std::vector<cv::Rect> faces;
	std::vector<cv::Mat> rgbChannels(frame.channels());
	cv::split(frame, rgbChannels);
	cv::Mat frame_gray = rgbChannels[2];

	faceCascade.detectMultiScale(frame_gray, faces, 1.1, 2, 0|CV_HAAR_SCALE_IMAGE|CV_HAAR_FIND_BIGGEST_OBJECT, cv::Size(150, 150));
	findFace = false;
	if (faces.size() > 0) 
	{
		findFace = true;
		findEyes(frame_gray, faces[0]);
	}
}

void GazeTracking::findEyes(cv::Mat& frame_gray, cv::Rect& face) 
{
	detectedFace = face;
	faceROI = frame_gray(face);

	//-- Find eye regions and draw them
	int eye_region_width = face.width * (kEyePercentWidth/100.0);
	int eye_region_height = face.width * (kEyePercentHeight/100.0);
	int eye_region_top = face.height * (kEyePercentTop/100.0);
	cv::Rect left_eye_region(face.width*(kEyePercentSide/100.0),
		eye_region_top,eye_region_width,eye_region_height);
	leftEyeRegion = left_eye_region;
	cv::Rect right_eye_region(face.width - eye_region_width - face.width*(kEyePercentSide/100.0),
		eye_region_top,eye_region_width,eye_region_height);
	rightEyeRegion = right_eye_region;
	//-- Find Eye Centers
	leftPupil = findEyeCenter(faceROI,leftEyeRegion);
	rightPupil = findEyeCenter(faceROI,rightEyeRegion);

	rightPupil.x += rightEyeRegion.x;
	rightPupil.y += rightEyeRegion.y;
	leftPupil.x += leftEyeRegion.x;
	leftPupil.y += leftEyeRegion.y;
}

cv::Point GazeTracking::findEyeCenter(cv::Mat& face, cv::Rect& eye)//used
{
	cv::Mat eyeROIUnscaled = face(eye);
	cv::Mat eyeROI;
	scaleToFastSize(eyeROIUnscaled, eyeROI);

	//-- Find the gradient
	cv::Mat gradientX = computeMatXGradient(eyeROI);
	cv::Mat gradientY = computeMatXGradient(eyeROI.t()).t();
	//-- Normalize and threshold the gradient
	// compute all the magnitudes
	cv::Mat mags = matrixMagnitude(gradientX, gradientY);
	//compute the threshold
	double gradientThresh = computeDynamicThreshold(mags, kGradientThreshold);
	//double gradientThresh = kGradientThreshold;
	//double gradientThresh = 0;
	//normalize
	for (int y = 0; y < eyeROI.rows; ++y) {
		double *Xr = gradientX.ptr<double>(y), *Yr = gradientY.ptr<double>(y);
		const double *Mr = mags.ptr<double>(y);
		for (int x = 0; x < eyeROI.cols; ++x) {
			double gX = Xr[x], gY = Yr[x];
			double magnitude = Mr[x];
			if (magnitude > gradientThresh) {
				Xr[x] = gX/magnitude;
				Yr[x] = gY/magnitude;
			} else {
				Xr[x] = 0.0;
				Yr[x] = 0.0;
			}
		}
	}
	//imshow(debugWindow,gradientX);
	//-- Create a blurred and inverted image for weighting
	cv::Mat weight;
	GaussianBlur( eyeROI, weight, cv::Size( kWeightBlurSize, kWeightBlurSize ), 0, 0 );
	for (int y = 0; y < weight.rows; ++y) {
		unsigned char *row = weight.ptr<unsigned char>(y);
		for (int x = 0; x < weight.cols; ++x) {
			row[x] = (255 - row[x]);
		}
	}
	//imshow(debugWindow,weight);
	//-- Run the algorithm!
	cv::Mat outSum = cv::Mat::zeros(eyeROI.rows,eyeROI.cols,CV_64F);
	// for each possible center
	//printf("Eye Size: %ix%i\n",outSum.cols,outSum.rows);
	for (int y = 0; y < weight.rows; ++y) {
		const unsigned char *Wr = weight.ptr<unsigned char>(y);
		const double *Xr = gradientX.ptr<double>(y), *Yr = gradientY.ptr<double>(y);
		for (int x = 0; x < weight.cols; ++x) {
			double gX = Xr[x], gY = Yr[x];
			if (gX == 0.0 && gY == 0.0) {
				continue;
			}
			testPossibleCentersFormula(x, y, Wr[x], gX, gY, outSum);
		}
	}
	// scale all the values down, basically averaging them
	double numGradients = (weight.rows*weight.cols);
	cv::Mat out;
	outSum.convertTo(out, CV_32F,1.0/numGradients);
	//imshow(debugWindow,out);
	//-- Find the maximum point
	cv::Point maxP;
	double maxVal;
	cv::minMaxLoc(out, NULL,&maxVal,NULL,&maxP);
	//-- Flood fill the edges
	if(kEnablePostProcess) 
	{
		cv::Mat floodClone;
		double floodThresh = maxVal * kPostProcessThreshold;
		cv::threshold(out, floodClone, floodThresh, 0.0f, cv::THRESH_TOZERO);

		cv::Mat mask = floodKillEdges(floodClone);

		cv::minMaxLoc(out, NULL,&maxVal,NULL,&maxP,mask);
	}
	return unscalePoint(maxP,eye);
}

bool GazeTracking::floodShouldPushPoint(const cv::Point &np, const cv::Mat &mat) 
{
	return inMat(np, mat.rows, mat.cols);
}

cv::Mat GazeTracking::floodKillEdges(cv::Mat &mat) 
{
	rectangle(mat,cv::Rect(0,0,mat.cols,mat.rows),255);

	cv::Mat mask(mat.rows, mat.cols, CV_8U, 255);
	std::queue<cv::Point> toDo;
	toDo.push(cv::Point(0,0));
	while (!toDo.empty()) {
		cv::Point p = toDo.front();
		toDo.pop();
		if (mat.at<float>(p) == 0.0f) {
			continue;
		}
		// add in every direction
		cv::Point np(p.x + 1, p.y); // right
		if (floodShouldPushPoint(np, mat)) toDo.push(np);
		np.x = p.x - 1; np.y = p.y; // left
		if (floodShouldPushPoint(np, mat)) toDo.push(np);
		np.x = p.x; np.y = p.y + 1; // down
		if (floodShouldPushPoint(np, mat)) toDo.push(np);
		np.x = p.x; np.y = p.y - 1; // up
		if (floodShouldPushPoint(np, mat)) toDo.push(np);
		// kill it
		mat.at<float>(p) = 0.0f;
		mask.at<uchar>(p) = 0;
	}
	return mask;
}

void GazeTracking::scaleToFastSize(const cv::Mat &src,cv::Mat &dst) //used
{
	cv::resize(src, dst, cv::Size(kFastEyeWidth,(((float)kFastEyeWidth)/src.cols) * src.rows));
}

cv::Point GazeTracking::unscalePoint(cv::Point& p, cv::Rect& origSize) //used 
{
	float ratio = (((float)kFastEyeWidth)/origSize.width);
	int x = round(p.x / ratio);
	int y = round(p.y / ratio);
	return cv::Point(x,y);
}

cv::Mat GazeTracking::computeMatXGradient(const cv::Mat &mat) //used
{
	cv::Mat out(mat.rows,mat.cols,CV_64F);

	for (int y = 0; y < mat.rows; ++y) {
		const uchar *Mr = mat.ptr<uchar>(y);
		double *Or = out.ptr<double>(y);

		Or[0] = Mr[1] - Mr[0];
		for (int x = 1; x < mat.cols - 1; ++x) {
			Or[x] = (Mr[x+1] - Mr[x-1])/2.0;
		}
		Or[mat.cols-1] = Mr[mat.cols-1] - Mr[mat.cols-2];
	}

	return out;
}

void GazeTracking::testPossibleCentersFormula(int x, int y, unsigned char weight,double gx, double gy, cv::Mat &out) //used
{
	// for all possible centers
	for (int cy = 0; cy < out.rows; ++cy) {
		double *Or = out.ptr<double>(cy);
		for (int cx = 0; cx < out.cols; ++cx) {
			if (x == cx && y == cy) {
				continue;
			}
			// create a vector from the possible center to the gradient origin
			double dx = x - cx;
			double dy = y - cy;
			// normalize d
			double magnitude = sqrt((dx * dx) + (dy * dy));
			dx = dx / magnitude;
			dy = dy / magnitude;
			double dotProduct = dx*gx + dy*gy;
			dotProduct = std::max(0.0,dotProduct);
			// square and multiply by the weight
			if (kEnableWeight) {
				Or[cx] += dotProduct * dotProduct * (weight/kWeightDivisor);
			} else {
				Or[cx] += dotProduct * dotProduct;
			}
		}
	}
}

bool GazeTracking::rectInImage(cv::Rect& rect, cv::Mat& image) 
{
	return rect.x > 0 && rect.y > 0 && rect.x+rect.width < image.cols &&
		rect.y+rect.height < image.rows;
}

bool GazeTracking::inMat(const cv::Point& p,int rows,int cols) 
{
	return p.x >= 0 && p.x < cols && p.y >= 0 && p.y < rows;
}

cv::Mat GazeTracking::matrixMagnitude(const cv::Mat &matX, const cv::Mat &matY)//used 
{
	cv::Mat mags(matX.rows,matX.cols,CV_64F);
	for (int y = 0; y < matX.rows; ++y) {
		const double *Xr = matX.ptr<double>(y), *Yr = matY.ptr<double>(y);
		double *Mr = mags.ptr<double>(y);
		for (int x = 0; x < matX.cols; ++x) {
			double gX = Xr[x], gY = Yr[x];
			double magnitude = sqrt((gX * gX) + (gY * gY));
			Mr[x] = magnitude;
		}
	}
	return mags;
}

double GazeTracking::computeDynamicThreshold(const cv::Mat &mat, double stdDevFactor)//used 
{
	cv::Scalar stdMagnGrad, meanMagnGrad;
	cv::meanStdDev(mat, meanMagnGrad, stdMagnGrad);
	double stdDev = stdMagnGrad[0] / sqrt((double)mat.rows*mat.cols);
	return stdDevFactor * stdDev + meanMagnGrad[0];
}

cv::Mat GazeTracking::eyeCornerMap(const cv::Mat &region, bool left, bool left2) 
{
  cv::Mat cornerMap;

  cv::Size sizeRegion = region.size();
  cv::Range colRange(sizeRegion.width / 4, sizeRegion.width * 3 / 4);
  cv::Range rowRange(sizeRegion.height / 4, sizeRegion.height * 3 / 4);

  cv::Mat miRegion(region, rowRange, colRange);

  cv::filter2D(miRegion, cornerMap, CV_32F,
               (left && !left2) || (!left && !left2) ? *leftCornerKernel : *rightCornerKernel);

  return cornerMap;
}

cv::Point2f GazeTracking::findEyeCorner(cv::Mat& region, bool left, bool left2) 
{
  cv::Mat cornerMap = eyeCornerMap(region, left, left2);

  cv::Point maxP;
  cv::minMaxLoc(cornerMap,NULL,NULL,NULL,&maxP);

  cv::Point2f maxP2;
  maxP2 = findSubpixelEyeCorner(cornerMap, maxP);

  return maxP2;
}

cv::Point2f GazeTracking::findSubpixelEyeCorner(cv::Mat& region, cv::Point& maxP) 
{
  cv::Size sizeRegion = region.size();

  cv::Mat cornerMap(sizeRegion.height * 10, sizeRegion.width * 10, CV_32F);

  cv::resize(region, cornerMap, cornerMap.size(), 0, 0, cv::INTER_CUBIC);

  cv::Point maxP2;
  cv::minMaxLoc(cornerMap, NULL,NULL,NULL,&maxP2);

  return cv::Point2f(sizeRegion.width / 2 + maxP2.x / 10,
                     sizeRegion.height / 2 + maxP2.y / 10);
}

void GazeTracking::createCornerKernels() 
{
	float kEyeCornerKernel[4][6] = {
		{-1,-1,-1, 1, 1, 1},
		{-1,-1,-1,-1, 1, 1},
		{-1,-1,-1,-1, 0, 3},
		{ 1, 1, 1, 1, 1, 1},
	};
	rightCornerKernel = new cv::Mat(4,6,CV_32F,kEyeCornerKernel);
	leftCornerKernel = new cv::Mat(4,6,CV_32F);
	// flip horizontally
	cv::flip(*rightCornerKernel, *leftCornerKernel, 1);
}

void GazeTracking::releaseCornerKernels() 
{
	delete leftCornerKernel;
	delete rightCornerKernel;
}