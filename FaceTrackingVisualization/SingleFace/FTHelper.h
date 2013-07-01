//------------------------------------------------------------------------------
// <copyright file="FTHelper.h" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#pragma once
#include <FaceTrackLib.h>
#include "KinectSensor.h"

#include "gazeTracking.h"

#include "Subdivision.h"
#include "mesh_model.h"
#include "transformer.h"



#define VERTEXCOUNT 121
#define TRIANGLECOUNT 206

#define LASTPOSITIONNUM 3

#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif

//#define READOBJFORDEBUG

//#define OUTPUTTOFILE

#ifdef OUTPUTTOFILE
#define FPNUM 8
enum FileIndex{LEFTX, LEFTY, NOSEX, NOSEY, FILTERLEFTX, FILTERLEFTY, INLEFTX, INLEFTY};
extern FILE* fp[FPNUM];
//#define NEEDFILTER
#define INFUNCTION
#endif

#define OPENGLMODE
#ifndef OPENGLMODE
#define IMAGEMODE
//#define DRAWFACEMODEL
#endif

//#define MYDEBUG

#ifdef MYDEBUG
#include <iostream>
#endif

typedef void (*FTHelperCallBack)(PVOID lpParam);

enum MeshType{KinectMesh, SrcRefMesh, TgtRefMesh, OutputMesh};

struct GazeState{
	inline void set(float ox, float oy, int t0, int t1, int t2)
	{
		x = ox;
		y = oy;
		triangle[0] = t0;
		triangle[1] = t1;
		triangle[2] = t2;
	}
	float x;
	float y;
	int triangle[3];
};

class FTHelper
{
public:
    FTHelper();
    ~FTHelper();

    HRESULT Init(HWND hWnd, FTHelperCallBack callBack, PVOID callBackParam, 
        NUI_IMAGE_TYPE depthType, NUI_IMAGE_RESOLUTION depthRes, BOOL bNearMode, BOOL bFallbackToDefault, NUI_IMAGE_TYPE colorType, NUI_IMAGE_RESOLUTION colorRes, BOOL bSeatedSkeletonMode);
    HRESULT Stop();
    IFTResult* GetResult()      { return(m_pFTResult);}
    BOOL IsKinectPresent()      { return(m_KinectSensorPresent);}
    IFTImage* GetColorImage()   { return(m_colorImage);}
    float GetXCenterFace()      { return(m_XCenterFace);}
    float GetYCenterFace()      { return(m_YCenterFace);}
    void SetDrawMask(BOOL drawMask) { m_DrawMask = drawMask;}
    BOOL GetDrawMask()          { return(m_DrawMask);}
    IFTFaceTracker* GetTracker() { return(m_pFaceTracker);}
    HRESULT GetCameraConfig(FT_CAMERA_CONFIG* cameraConfig);

	//gazetracking
	RECT& GetFaceRect()			{return m_faceRect;}
	FT_VECTOR3D& GetLeftPupil()	{return m_leftPupil;}
	FT_VECTOR3D& GetRightPupil(){return m_rightPupil;}

	//opengl
	FT_VECTOR3D* GetVertices()	{return m_pPts3D;}
	FT_TRIANGLE* GetTriangles() {return m_pTriangles;}
	int GetVertexNum()			{return VERTEXCOUNT;}
	int GetTriangleNum()		{return TRIANGLECOUNT;}

	//opengl tracking
	bool isLastTrackSucceed()	{return m_LastTrackSucceeded;}
	float GetPupilR()			{return m_pupilR;}
	void DrawSrcMeshModel(bool wired = false)		{srcMesh.DrawMeshModel(wired);}
	void CalculateSrcMeshPosCorrection(double *cx, double *cy, double *cz)	{srcMesh.CalculateModelPosCorrection(cx, cy, cz);}	
	void DrawTgtMeshModel(bool wired = false)		{tgtMesh.DrawMeshModel(wired);}
	void CalculateTgtMeshPosCorrection(double *cx, double *cy, double *cz)	{tgtMesh.CalculateModelPosCorrection(cx, cy, cz);}
	void DrawResMeshModel(bool wired = false)		{resultMesh.DrawMeshModel(wired);}
	void CalculateResMeshPosCorrection(double *cx, double *cy, double *cz)	{resultMesh.CalculateModelPosCorrection(cx, cy, cz);}


	void DrawMeshModel(MeshType meshType, bool wired = false);
	void CalculateMeshPosCorrection(MeshType meshType, double *cx, double *cy, double *cz);

private:
    KinectSensor                m_KinectSensor;
    BOOL                        m_KinectSensorPresent;
    IFTFaceTracker*             m_pFaceTracker;
    HWND                        m_hWnd;
    IFTResult*                  m_pFTResult;
    IFTImage*                   m_colorImage;
    IFTImage*                   m_depthImage;
    FT_VECTOR3D                 m_hint3D[2];
    bool                        m_LastTrackSucceeded;
    bool                        m_ApplicationIsRunning;
    FTHelperCallBack            m_CallBack;
    LPVOID                      m_CallBackParam;
    float                       m_XCenterFace;
    float                       m_YCenterFace;
    HANDLE                      m_hFaceTrackingThread;
    BOOL                        m_DrawMask;
    NUI_IMAGE_TYPE              m_depthType;
    NUI_IMAGE_RESOLUTION        m_depthRes;
    BOOL                        m_bNearMode;
    BOOL                        m_bFallbackToDefault;
    BOOL                        m_bSeatedSkeletonMode;
    NUI_IMAGE_TYPE              m_colorType;
    NUI_IMAGE_RESOLUTION        m_colorRes;

	//gazetracking
	GazeTracking*				m_gazeTrack;
	RECT						m_faceRect;
	FT_VECTOR3D					m_leftPupil;
	FT_VECTOR3D					m_rightPupil;

	//opengl
	FT_VECTOR3D					m_pPts3D[VERTEXCOUNT];
	FT_VECTOR2D					m_pPts2D[VERTEXCOUNT];
	FT_TRIANGLE*				m_pTriangles;
	UINT						m_TriangleCount;

	//opengl gaze tracking
	float						m_pupilR;
	GazeState					m_gazeLastState[2];

	dtMeshModel					refMesh;//reference mesh
	dtMeshModel					srcMesh;//input mesh, produced by Kinect
	dtMeshModel					tgtMesh;//input target-reference mesh
	dtMeshModel					resultMesh;//result mesh

	LS_Surface					lsSurface;

	DTTransformer				trans;

#ifdef NEEDFILTER
	POINT						m_lastPosition[LASTPOSITIONNUM][2];
#endif
    BOOL SubmitFraceTrackingResult(IFTResult* pResult);
    void SetCenterOfImage(IFTResult* pResult);
    void CheckCameraInput();
    DWORD WINAPI FaceTrackingThread();
    static DWORD WINAPI FaceTrackingStaticThread(PVOID lpParam);

	HRESULT VisualizeFacetracker(UINT32 color);
	HRESULT VisualizeFaceModel(IFTModel* pModel, FT_CAMERA_CONFIG const* pCameraConfig, FLOAT const* pSUCoef, FLOAT zoomFactor, POINT viewOffset, UINT32 color);

	void DrawGazeInImage(POINT pos, int radius, UINT32 color);

	void SaveModel(IFTModel* model, const float* pSUs, UINT32 suCount, const float* pAUs, UINT32 auCount, float scale, const float* rotationXYZ, const float* translationXYZ, int count);
	void SaveModel(int count);

	void Map2Dto3D();
	float PointDis(int n, int m);
	void GetPupilFromLastState(FT_VECTOR3D& pupil, GazeState& gazeState);
#ifdef NEEDFILTER
	void LowFilter(POINT pupil[2]);
#endif
};
