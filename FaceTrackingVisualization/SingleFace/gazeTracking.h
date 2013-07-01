#ifndef GACETRACKING_H
#define GACETRACKING_H

#include <opencv2/objdetect/objdetect.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class GazeTracking{
public:
	GazeTracking();
	~GazeTracking();
	bool initialize(cv::String fileName);
	void process(cv::Mat& frame);
	void process(IplImage* image);

	cv::Point getLeftPupil(){return cv::Point(leftPupil.x+detectedFace.x, leftPupil.y+detectedFace.y);}
	cv::Point getRightPupil(){return cv::Point(rightPupil.x+detectedFace.x, rightPupil.y+detectedFace.y);}
	cv::Point getLeftPupilInFaceRect(){return leftPupil;}
	cv::Point getRightPupilInFaceRect(){return rightPupil;}

	cv::Rect GetFaceRect(){return detectedFace;}
	cv::Mat getFace(){return faceROI;}
	bool isFindFace(){return findFace;}

	void getLeftPupilXY(int& x, int& y){x=leftPupil.x+detectedFace.x;y=leftPupil.y+detectedFace.y;}
	void getRightPupilXY(int& x, int& y){x=rightPupil.x+detectedFace.x;y=rightPupil.y+detectedFace.y;}
	void getLeftPupilXYInFaceRect(int& x, int& y){x=leftPupil.x;y=leftPupil.y;}
	void getRightPupilXYInFaceRect(int& x, int& y){x=rightPupil.x;y=rightPupil.y;}

	int getLeftPupilXInFaceRect(){return leftPupil.x;}
	int getLeftPupilYInFaceRect(){return leftPupil.y;}
	int getRightPupilXInFaceRect(){return rightPupil.x;}
	int getRightPupilYInFaceRect(){return rightPupil.y;}

	int getLeftPupilX(){return leftPupil.x+detectedFace.x;}
	int getLeftPupilY(){return leftPupil.y+detectedFace.y;}
	int getRightPupilX(){return rightPupil.x+detectedFace.x;}
	int getRightPupilY(){return rightPupil.y+detectedFace.y;}
	
	void setLeftPupilXY(int x, int y){leftPupil.x=x-detectedFace.x;leftPupil.y=y-detectedFace.y;}
	void setRightPupilXY(int x, int y){rightPupil.x=x-detectedFace.x;rightPupil.y=y-detectedFace.y;}

private:
	void findEyes(cv::Mat& frame_gray, cv::Rect& face);
	cv::Point findEyeCenter(cv::Mat& face, cv::Rect& eye);
	void scaleToFastSize(const cv::Mat &src,cv::Mat &dst);
	cv::Point unscalePoint(cv::Point& p, cv::Rect& origSize);
	cv::Mat computeMatXGradient(const cv::Mat &mat);
	void testPossibleCentersFormula(int x, int y, unsigned char weight,double gx, double gy, cv::Mat &out);
	bool floodShouldPushPoint(const cv::Point &np, const cv::Mat &mat);
	cv::Mat floodKillEdges(cv::Mat &mat);
	bool rectInImage(cv::Rect& rect, cv::Mat& image);
	bool inMat(const cv::Point& p,int rows,int cols);
	cv::Mat matrixMagnitude(const cv::Mat &matX, const cv::Mat &matY);
	double computeDynamicThreshold(const cv::Mat &mat, double stdDevFactor);
	cv::Mat eyeCornerMap(const cv::Mat &region, bool left, bool left2);
	cv::Point2f findEyeCorner(cv::Mat& region, bool left, bool left2);
	cv::Point2f findSubpixelEyeCorner(cv::Mat& region, cv::Point& maxP);

	void createCornerKernels();
	void releaseCornerKernels();

	int round(float a){return (int)(a+0.5);}

	cv::Rect detectedFace;
	cv::Mat faceROI;
	bool findFace;
	cv::Rect leftEyeRegion;
	cv::Rect rightEyeRegion;

	cv::Point leftPupil;
	cv::Point rightPupil;

	cv::CascadeClassifier faceCascade;

	cv::Mat *leftCornerKernel;
	cv::Mat *rightCornerKernel;

	// Debugging
	const bool kPlotVectorField;

	// Size constants
	const int kEyePercentTop;
	const int kEyePercentSide;
	const int kEyePercentHeight;
	const int kEyePercentWidth;

	// Preprocessing
	const bool kSmoothFaceImage;
	const float kSmoothFaceFactor;

	// Algorithm Parameters
	const int kFastEyeWidth;
	const int kWeightBlurSize;
	const bool kEnableWeight;
	const float kWeightDivisor;
	const double kGradientThreshold;

	// Postprocessing
	const bool kEnablePostProcess;
	const float kPostProcessThreshold;

	// Eye Corner
	const bool kEnableEyeCorner;
};

#endif