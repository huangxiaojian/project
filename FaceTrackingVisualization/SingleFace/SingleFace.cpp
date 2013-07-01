//------------------------------------------------------------------------------
// <copyright file="SingleFace.cpp" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

// Defines the entry point for the application.
//

#include "stdafx.h"
#include <windowsx.h>
#include "SingleFace.h"
#include "EggAvatar.h"
#include <FaceTrackLib.h>
#include "FTHelper.h"

#include "GLContext.h"

#define _DEBUG

#ifdef _DEBUG
#include <io.h>
#include <fcntl.h>
#endif

#ifdef OUTPUTTOFILE
#include <string>
#include <stdio.h>
FILE* fp[FPNUM];
#endif

//#include <GL/freeglut_ext.h>

#define WINDOW_WIDTH		1210
#define WINDOW_HEIGHT		600
#define BORDER_WIDTH		10
#define BORDER_COLOR		0.2f,0.2f,0.2f
#define MODEL_COLOR			0.5f,0.5f,0.5f

#define LEFT_CENTER			0.103273f,-0.002881f,1.05936f
#define RIGHT_CENTER		-0.27774f,-2.788298f,-4.67569f

const GLdouble cam_fovy = 60, cam_zNear = 0.01, cam_zFar = 50;

enum Panel{Border, LeftPanel, RightPanel};

struct PanelLayout 
{
	PanelLayout():cameraAngleX(0.0), cameraAngleY(0.0), mouseX(0), 
		mouseY(0), cameraLeftDistance(0.22), cameraRightDistance(2.0)
	{}
	GLfloat cameraAngleX, cameraAngleY;
	GLint mouseX, mouseY;
	GLfloat cameraLeftDistance;
	GLfloat cameraRightDistance;
};


class SingleFace
{
public:
    SingleFace() 
        : m_hInst(NULL)
        , m_hWnd(NULL)
        , m_hAccelTable(NULL)
        , m_pImageBuffer(NULL)
        , m_pVideoBuffer(NULL)
        , m_depthType(NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX)
        , m_colorType(NUI_IMAGE_TYPE_COLOR)
		, m_depthRes(NUI_IMAGE_RESOLUTION_640x480)
		, m_colorRes(NUI_IMAGE_RESOLUTION_1280x960)
        /*, m_depthRes(NUI_IMAGE_RESOLUTION_320x240)
        , m_colorRes(NUI_IMAGE_RESOLUTION_640x480)*/
        , m_bNearMode(TRUE)
        , m_bSeatedSkeletonMode(FALSE)
		, m_windowWidth(WINDOW_WIDTH)
		, m_windowHeight(WINDOW_HEIGHT)
		, m_wired(false)
    {}

    int Run(HINSTANCE hInst, PWSTR lpCmdLine, int nCmdShow);

protected:
    BOOL                        InitInstance(HINSTANCE hInst, PWSTR lpCmdLine, int nCmdShow);
    void                        ParseCmdString(PWSTR lpCmdLine);
    void                        UninitInstance();
    ATOM                        RegisterClass(PCWSTR szWindowClass);
    static LRESULT CALLBACK     WndProcStatic(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT CALLBACK            WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK     About(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    BOOL                        PaintWindow(HDC hdc, HWND hWnd);
    BOOL                        ShowVideo(HDC hdc, int width, int height, int originX, int originY);
    BOOL                        ShowEggAvatar(HDC hdc, int width, int height, int originX, int originY);

	//gazetracking
	BOOL						ShowEye(HDC hdc, int width, int height, int originX, int originY);

	//opengl
	void						ReSizeGLScene(GLsizei width, GLsizei height);
	void						InitGL();
	void						DrawGLScene();
	void						DrawPanelBorder();
	void						DrawLeftPanel();
	void						DrawRightPanel();
	void						SetCurrentView(Panel panel);
	void						SetCamera(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble atX, GLdouble atY, GLdouble atZ);
	void						SetMaterial();
	void						SetTransform(GLfloat cam_pitch, GLfloat cam_heading, GLfloat cam_dist, GLdouble cx, GLdouble cy, GLdouble cz);
	void						MouseDown(int x, int y);
	void						MouseMove(int state, int x, int y);
	void						MouseWheel(int dir, int x, int y);
	Panel						SelectPanel(int x, int y);
	GLContext					m_GLContext;
	bool						m_wired;
	//dtMeshModel					srcMeshModel;
	//dtMeshModel					tgtMeshModel;

    static void                 FTHelperCallingBack(LPVOID lpParam);
    static int const            MaxLoadStringChars = 100;

    HINSTANCE                   m_hInst;
    HWND                        m_hWnd;
    HACCEL                      m_hAccelTable;
    EggAvatar                   m_eggavatar;
    FTHelper                    m_FTHelper;
    IFTImage*                   m_pImageBuffer;
    IFTImage*                   m_pVideoBuffer;

    NUI_IMAGE_TYPE              m_depthType;
    NUI_IMAGE_TYPE              m_colorType;
    NUI_IMAGE_RESOLUTION        m_depthRes;
    NUI_IMAGE_RESOLUTION        m_colorRes;
    BOOL                        m_bNearMode;
    BOOL                        m_bSeatedSkeletonMode;

	int							m_windowWidth;
	int							m_windowHeight;
	PanelLayout					m_panelLayout;
};

void SingleFace::SetTransform(GLfloat cam_pitch, GLfloat cam_heading, 
			GLfloat cam_dist, GLdouble cx, GLdouble cy, GLdouble cz)
{
#ifdef _DEBUG
	std::cout << "angleX:" << cam_pitch << " angleY:" << cam_heading << " distance:" << cam_dist << std::endl;
	std::cout << "(cx, cy, cz):(" << cx << ", " << cy << ", " << cz << ")" << std::endl;
#endif
	glTranslatef(0, 0, -cam_dist);
	glRotatef(cam_pitch, 1, 0, 0);
	glRotatef(cam_heading, 0, 1, 0);
	glTranslated(cx, cy, cz);
}
void SingleFace::SetMaterial()
{
	const static GLfloat ambient[4] = {0.5f, 0.5f, 0.6f, 1.0f};
	const static GLfloat diffuse[4] = {0.5f, 0.5f, 0.5f, 1.0f};
	const static GLfloat specular[4] = {0.5f, 0.5f, 0.5f, 1.0f};
	const static GLfloat shininess[4] = {20.f, 0, 0, 0};

	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT,   ambient);
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE,   diffuse);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR,  specular);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
}

void SingleFace::SetCamera(GLdouble eyeX, GLdouble eyeY, GLdouble eyeZ, GLdouble atX, GLdouble atY, GLdouble atZ)
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(eyeX, eyeY, eyeZ,	atX, atY, atZ, 0.0, 1.0, 0.0);
}

void SingleFace::SetCurrentView(Panel panel)
{
	GLint x = 0, y = 0;
	GLsizei width = 0, height = m_windowHeight;
	switch (panel)
	{
	case Border:
		width = m_windowWidth;
		break;
	case LeftPanel:
		width = (m_windowWidth - BORDER_WIDTH) / 2;
		break;
	case RightPanel:
		width = (m_windowWidth - BORDER_WIDTH) / 2;
		x = width + BORDER_WIDTH;
		break;
	default:
		break;
	}

#ifdef _DEBUG
	std::cout << "x:" << x << " y:" << y << " width:" << width << " height:" << height << std::endl;
#endif
	glViewport(x, y, width, height);
	glEnable(GL_SCISSOR_TEST);
	glScissor(x, y, width, height);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(cam_fovy, (GLdouble)width / (GLdouble)height, cam_zNear, cam_zFar);

	glMatrixMode(GL_MODELVIEW);
}

void SingleFace::MouseWheel(int dir, int x, int y)
{
#ifdef _DEBUG
	std::cout << "MouseWheel************" << std::endl;
	std::cout << "dir:" << dir << std::endl;
	std::cout << "x:" << x << ", y:" << y << std::endl; 
#endif
	Panel panel = SelectPanel(x, y);
	if(panel == LeftPanel)
		m_panelLayout.cameraLeftDistance += (GLfloat)(dir * 0.02 / WHEEL_DELTA);
	else if(panel == RightPanel)
		m_panelLayout.cameraRightDistance += (GLfloat)(dir * 0.2 / WHEEL_DELTA);
#ifdef _DEBUG
	std::cout << "cameraLeftDistance:" << m_panelLayout.cameraLeftDistance << std::endl;
	std::cout << "cameraRightDistance:" << m_panelLayout.cameraRightDistance << std::endl;
#endif
	return;
}

void SingleFace::MouseMove(int state, int x, int y)
{
	if(state == MK_LBUTTON)
	{
#ifdef _DEBUG
		std::cout << "MouseMove************" << std::endl;
		std::cout << "x:" << x << ", y:" << y << std::endl; 
#endif
		m_panelLayout.cameraAngleY += (GLfloat)(x - m_panelLayout.mouseX);
		m_panelLayout.cameraAngleX += (GLfloat)(y - m_panelLayout.mouseY);
		m_panelLayout.mouseX = x;
		m_panelLayout.mouseY = y;
#ifdef _DEBUG
		std::cout << "angleX:" << m_panelLayout.cameraAngleX << std::endl;
		std::cout << "angleY:" << m_panelLayout.cameraAngleY << std::endl;
#endif
	}
}

void SingleFace::MouseDown(int x, int y)
{
#ifdef _DEBUG
	std::cout << "MouseDown************" << std::endl;
	std::cout << "x:" << x << ", y:" << y << std::endl; 
#endif
	m_panelLayout.mouseX = x;
	m_panelLayout.mouseY = y;
}

Panel SingleFace::SelectPanel(int x, int y)
{
	if(x < (m_windowWidth-BORDER_WIDTH)/2)
		return LeftPanel;
	if(x > (m_windowWidth+BORDER_WIDTH)/2)
		return RightPanel;
	return Border;
}

void SingleFace::DrawPanelBorder()
{
	SetCurrentView(Border);

	glClearColor(BORDER_COLOR, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glClearColor(0, 0, 0, 1.0);
}

void SingleFace::DrawLeftPanel()
{
#ifdef _DEBUG
	std::cout << "Left Panel:" << std::endl;
#endif
	SetCurrentView(LeftPanel);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SetCamera(0, 0, -1, 0, 0, 0);

	glColor3f(MODEL_COLOR);
	SetMaterial();

	GLdouble cx, cy, cz;
	m_FTHelper.CalculateSrcMeshPosCorrection(&cx, &cy, &cz);
	SetTransform(-m_panelLayout.cameraAngleX, m_panelLayout.cameraAngleY, m_panelLayout.cameraLeftDistance, cx, cy, cz);
	m_FTHelper.DrawSrcMeshModel(m_wired);
#ifdef _DEBUG
	std::cout << "*******************" << std::endl;
#endif
}

void SingleFace::DrawRightPanel()
{
	SetCurrentView(RightPanel);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	SetCamera(0, 0, 5, 0, 0, 0);

	//glColor3f(MODEL_COLOR);
	//SetMaterial();

	glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_TRIANGLES);
	glVertex3f( 0.0f, 1.0f, 0.0f);
	glVertex3f(-1.0f,-1.0f, 0.0f);
	glVertex3f( 1.0f,-1.0f, 0.0f); 
	glEnd();  
	/*
	glBegin(GL_TRIANGLES);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(-0.5, -0.4, 0.0);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(0.5, -0.4, 0.0);
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(0.0, 0.3, 0.0);
	glEnd();*/
	glFlush();
}

void SingleFace::DrawGLScene()
{
#ifndef READOBJFORDEBUG
	if(m_FTHelper.GetTriangles() == NULL)
		return;
#endif

#ifdef _DEBUG
	std::cout << "DrawScene:" << std::endl;
#endif
	DrawPanelBorder();

	//glDisable(GL_LIGHTING);
	DrawLeftPanel();
	//glEnable(GL_LIGHTING);

	//glDisable(GL_LIGHTING);
	DrawRightPanel();
	//glEnable(GL_LIGHTING);

#ifdef _DEBUG
	std::cout << std::endl;
#endif

	return;
}

void SingleFace::InitGL()
{
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	/* glEnable(GL_MULTISAMPLE); */

	/* track material ambient and diffuse from surface color */
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
}

void SingleFace::ReSizeGLScene(GLsizei width, GLsizei height)
{
#ifdef _DEBUG
	std::cout << "ReSizeGL:" << std::endl;
#endif
	m_windowWidth = width;
	m_windowHeight = height;
#ifdef _DEBUG
	std::cout << std::endl;
#endif
}

//
//void SingleFace::DrawGLScene()
//{
//	GLfloat pos[3];
//	pos[0] = (m_FTHelper.GetVertices()[73].x+m_FTHelper.GetVertices()[70].x)*0.5;
//	pos[1] = (m_FTHelper.GetVertices()[73].y+m_FTHelper.GetVertices()[70].y)*0.5;
//	pos[2] = (m_FTHelper.GetVertices()[73].z+m_FTHelper.GetVertices()[70].z)*0.5;
//	
//	Vector3 a(m_FTHelper.GetVertices()[70].x, m_FTHelper.GetVertices()[70].y, m_FTHelper.GetVertices()[70].z);
//	Vector3 b(m_FTHelper.GetVertices()[73].x, m_FTHelper.GetVertices()[73].y, m_FTHelper.GetVertices()[73].z);
//	Vector3 c(m_FTHelper.GetVertices()[69].x, m_FTHelper.GetVertices()[69].y, m_FTHelper.GetVertices()[69].z);
//	Vector3 base((m_FTHelper.GetVertices()[73].x+m_FTHelper.GetVertices()[70].x)*0.5, (m_FTHelper.GetVertices()[73].y+m_FTHelper.GetVertices()[70].y)*0.5, pos[2] = (m_FTHelper.GetVertices()[73].z+m_FTHelper.GetVertices()[70].z)*0.5);
//	Vector3 n = ((c-a)%(b-a)).normalize();
//	Vector3 eye = base + n*0.03;
//
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);         // Clear The Screen And The Depth Buffer
//	//glLoadIdentity();                           // Reset The Current Modelview Matrix
//
//	glMatrixMode(GL_PROJECTION);                        // Select The Projection Matrix
//	glLoadIdentity();                           // Reset The Projection Matrix
//
//	// Calculate The Aspect Ratio Of The Window
//	gluPerspective(75.0f,1,0.001f,5.0f);
//
//	glMatrixMode(GL_MODELVIEW);                     // Select The Modelview Matrix
//	glLoadIdentity();                           // Reset The Modelview Matrix
//	
//#ifdef NEAREYEMODE
//	//gluLookAt(pos[0], pos[1], pos[2]+0.03, pos[0], pos[1], pos[2], 0, 1, 0);
//	gluLookAt(eye[0], eye[1], eye[2], pos[0], pos[1], pos[2], 0, 1, 0);
//#else
//	gluLookAt(0, 0, -3, 0.059938, -0.240880, 1.064333, 0, 1, 0);
//#endif	
//
//
//	GLint* triangles;
//	if(sizeof(GLint)*3 == sizeof(FT_TRIANGLE))
//		triangles = (GLint*)m_FTHelper.GetTriangles();
//	else
//	{
//		triangles = new GLint[m_FTHelper.GetTriangleNum()*3];
//		for(int i = 0 ; i < m_FTHelper.GetTriangleNum(); i++)
//		{
//			triangles[3*i] = m_FTHelper.GetTriangles()[i].i;
//			triangles[3*i+1] = m_FTHelper.GetTriangles()[i].j;
//			triangles[3*i+2] = m_FTHelper.GetTriangles()[i].k;
//		}
//	}
//
//	GLfloat* vertices;
//	if(sizeof(GLfloat)*3 == sizeof(FT_VECTOR3D))
//		vertices = (GLfloat*)m_FTHelper.GetVertices();
//	else
//	{
//		vertices = new GLfloat[m_FTHelper.GetTriangleNum()*3];
//		for(int i = 0; i < m_FTHelper.GetVertexNum(); i++)
//		{
//			vertices[3*i] = m_FTHelper.GetVertices()[i].x;
//			vertices[3*i+1] = m_FTHelper.GetVertices()[i].y;
//			vertices[3*i+2] = m_FTHelper.GetVertices()[i].z;
//		}
//	}
//
//	glPushMatrix();
//	glColor3f(0.0, 1.0, 0.0);
//#ifndef NEAREYEMODE
//	glScalef(5.0, 5.0, 5.0);
//	glTranslatef(0.0, 0, -1.3);
//#endif
//
//	glBegin(GL_LINES);
//	for(int i = 0; i < 206; i++)
//	{
//		glVertex3f(vertices[triangles[i*3]*3], vertices[triangles[i*3]*3+1], vertices[triangles[i*3]*3+2]);
//		glVertex3f(vertices[triangles[i*3+1]*3], vertices[triangles[i*3+1]*3+1], vertices[triangles[i*3+1]*3+2]);
//
//		glVertex3f(vertices[triangles[i*3]*3], vertices[triangles[i*3]*3+1], vertices[triangles[i*3]*3+2]);
//		glVertex3f(vertices[triangles[i*3+2]*3], vertices[triangles[i*3+2]*3+1], vertices[triangles[i*3+2]*3+2]);
//
//		glVertex3f(vertices[triangles[i*3+1]*3], vertices[triangles[i*3+1]*3+1], vertices[triangles[i*3+1]*3+2]);
//		glVertex3f(vertices[triangles[i*3+2]*3], vertices[triangles[i*3+2]*3+1], vertices[triangles[i*3+2]*3+2]);
//	}
//	glEnd();
//
//	glColor3f(0.0, 0.0, 1.0);
//	glPushMatrix();
//	//glTranslatef(vertices[210], vertices[211], vertices[212]);
//	glTranslatef(m_FTHelper.GetRightPupil().x, m_FTHelper.GetRightPupil().y, m_FTHelper.GetRightPupil().z);
//#ifdef _DEBUG
//	//std::cout << "DrawGLScene(): vert::" << vertices[210] << ' ' << vertices[211] << ' ' << vertices[212] << std::endl;
//	//std::cout << "DrawGLScene(): Draw:" << m_FTHelper.GetRightPupil().x << ' ' << m_FTHelper.GetRightPupil().y << ' ' << m_FTHelper.GetRightPupil().z << std::endl;
//#endif
//	//glutSolidSphere(m_FTHelper.GetPupilR(), 10, 20);
//	glutWireSphere(m_FTHelper.GetPupilR(), 10, 10);
//	glPopMatrix();
//
//	glColor3f(1.0, 0.0, 0.0);
//	glPushMatrix();
//	//glTranslatef(vertices[15], vertices[16], vertices[17]);
//	glTranslatef(m_FTHelper.GetLeftPupil().x, m_FTHelper.GetLeftPupil().y, m_FTHelper.GetLeftPupil().z);
//#ifdef _DEBUG
//	//std::cout << "DrawGLScene(): Draw:" << m_FTHelper.GetLeftPupil().x << ' ' << m_FTHelper.GetLeftPupil().y << ' ' << m_FTHelper.GetLeftPupil().z << std::endl;
//#endif
//	//glutSolidSphere(m_FTHelper.GetPupilR(), 10, 20);
//	glutWireSphere(m_FTHelper.GetPupilR(), 10, 10);
//	glPopMatrix();
//
//	glPopMatrix();
//}
//
//void SingleFace::InitGL()
//{
//	glShadeModel(GL_SMOOTH);                        // Enables Smooth Shading
//	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);                   // Black Background
//	glClearDepth(1.0f);                         // Depth Buffer Setup
//	glEnable(GL_DEPTH_TEST);                        // Enables Depth Testing
//	glDepthFunc(GL_LEQUAL);                         // The Type Of Depth Test To Do
//	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);          // Really Nice Perspective Calculations
//}
//
//void SingleFace::ReSizeGLScene(GLsizei width, GLsizei height)
//{
//	if (height==0)                              // Prevent A Divide By Zero By
//	{
//		height=1;                           // Making Height Equal One
//	}
//
//	GLsizei dis = width > height ? height : width;
//	glViewport(0, 0, dis, dis);                    // Reset The Current Viewport
//	//glViewport(0, 0, dis/2, dis/2);
//	//glViewport(dis/2, dis/2, dis, dis);
//	//glViewport(dis, dis, -dis, -dis);                    // Reset The Current Viewport
//}

// Run the SingleFace application.
int SingleFace::Run(HINSTANCE hInst, PWSTR lpCmdLine, int nCmdShow)
{
#ifdef OUTPUTTOFILE
	//std::string file = "stablemove";
	//std::string file = "headaround";
	std::string file = "gazearound";
	//std::string file = "headsin";
	//std::string file = "wink";
	fp[LEFTX] = fopen((file+"x.xls").c_str(), "w");
	fp[LEFTY] = fopen((file+"y.xls").c_str(), "w");
	fp[NOSEX] = fopen((file+"nosex.xls").c_str(), "w");
	fp[NOSEY] = fopen((file+"nosey.xls").c_str(), "w");
#ifdef NEEDFILTER
	fp[FILTERLEFTX] = fopen((file+"xfilter.xls").c_str(), "w");
	fp[FILTERLEFTY] = fopen((file+"yfilter.xls").c_str(), "w");
#endif
#ifdef INFUNCTION
	fp[INLEFTX] = fopen((file+"inx.xls").c_str(), "w");
	fp[INLEFTY] = fopen((file+"iny.xls").c_str(), "w");
#endif
#endif 

    MSG msg = {static_cast<HWND>(0), static_cast<UINT>(0), static_cast<WPARAM>(-1)};
    if (InitInstance(hInst, lpCmdLine, nCmdShow))
    {
        // Main message loop:
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (!TranslateAccelerator(msg.hwnd, m_hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
    }
    UninitInstance();

#ifdef OUTPUTTOFILE
	fclose(fp[LEFTX]);
	fclose(fp[LEFTY]);
	fclose(fp[NOSEX]);
	fclose(fp[NOSEY]);
#ifdef NEEDFILTER
	fclose(fp[FILTERLEFTX]);
	fclose(fp[FILTERLEFTY]);
#endif
#ifdef INFUNCTION
	fclose(fp[INLEFTX]);
	fclose(fp[INLEFTY]);
#endif
#endif
    return (int)msg.wParam;
}

// In this function, we save the instance handle, then create and display the main program window.
BOOL SingleFace::InitInstance(HINSTANCE hInstance, PWSTR lpCmdLine, int nCmdShow)
{
    m_hInst = hInstance; // Store instance handle in our global variable

    ParseCmdString(lpCmdLine);

    WCHAR szTitle[MaxLoadStringChars];                  // The title bar text
    LoadString(m_hInst, IDS_APP_TITLE, szTitle, ARRAYSIZE(szTitle));

    static const PCWSTR RES_MAP[] = { L"80x60", L"320x240", L"640x480", L"1280x960" };
    static const PCWSTR IMG_MAP[] = { L"PLAYERID", L"RGB", L"YUV", L"YUV_RAW", L"DEPTH" };

    // Add mode params in title
    WCHAR szTitleComplete[MAX_PATH];
    swprintf_s(szTitleComplete, L"%s -- Depth:%s:%s Color:%s:%s NearMode:%s, SeatedSkeleton:%s", szTitle,
        IMG_MAP[m_depthType], (m_depthRes < 0)? L"ERROR": RES_MAP[m_depthRes], IMG_MAP[m_colorType], (m_colorRes < 0)? L"ERROR": RES_MAP[m_colorRes], m_bNearMode? L"ON": L"OFF",
        m_bSeatedSkeletonMode?L"ON": L"OFF");

    WCHAR szWindowClass[MaxLoadStringChars];            // the main window class name
    LoadString(m_hInst, IDC_SINGLEFACE, szWindowClass, ARRAYSIZE(szWindowClass));

    RegisterClass(szWindowClass);

    m_hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SINGLEFACE));

    m_pImageBuffer = FTCreateImage();
    m_pVideoBuffer = FTCreateImage();

    //m_hWnd = CreateWindow(szWindowClass, szTitleComplete, WS_OVERLAPPEDWINDOW,
        //CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, m_hInst, this);
	m_hWnd = CreateWindow(szWindowClass, szTitleComplete, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, m_windowWidth, m_windowHeight, NULL, NULL, m_hInst, this);
	if (!m_hWnd)
    {
        return FALSE;
    }

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

#ifdef OPENGLMODE
	m_GLContext.init(m_hWnd);
	InitGL();
#endif

    return SUCCEEDED(m_FTHelper.Init(m_hWnd,
        FTHelperCallingBack,
        this,
        m_depthType,
        m_depthRes,
        m_bNearMode,
        TRUE, // if near mode doesn't work, fall back to default mode
        m_colorType,
        m_colorRes,
        m_bSeatedSkeletonMode));
}

void SingleFace::UninitInstance()
{
    // Clean up the memory allocated for Face Tracking and rendering.
    m_FTHelper.Stop();

    if (m_hAccelTable)
    {
        DestroyAcceleratorTable(m_hAccelTable);
        m_hAccelTable = NULL;
    }

    DestroyWindow(m_hWnd);
    m_hWnd = NULL;

    if (m_pImageBuffer)
    {
        m_pImageBuffer->Release();
        m_pImageBuffer = NULL;
    }

    if (m_pVideoBuffer)
    {
        m_pVideoBuffer->Release();
        m_pVideoBuffer = NULL;
    }
}


// Register the window class.
ATOM SingleFace::RegisterClass(PCWSTR szWindowClass)
{
    WNDCLASSEX wcex = {0};

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = &SingleFace::WndProcStatic;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = m_hInst;
    wcex.hIcon          = LoadIcon(m_hInst, MAKEINTRESOURCE(IDI_SINGLEFACE));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_SINGLEFACE);
    wcex.lpszClassName  = szWindowClass;

    return RegisterClassEx(&wcex);
}

LRESULT CALLBACK SingleFace::WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static SingleFace* pThis = NULL; // cheating, but since there is just one window now, it will suffice.
    if (WM_CREATE == message)
    {
        pThis = reinterpret_cast<SingleFace*>(reinterpret_cast<CREATESTRUCT*>(lParam)->lpCreateParams);
    }
    return pThis ? pThis->WndProc(hWnd, message, wParam, lParam) : DefWindowProc(hWnd, message, wParam, lParam);
}

//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_KEYUP    - Exit in response to ESC key
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
LRESULT CALLBACK SingleFace::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UINT wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message)
    {
    case WM_COMMAND:
        wmId    = LOWORD(wParam);
        wmEvent = HIWORD(wParam);
        // Parse the menu selections:
        switch (wmId)
        {
        case IDM_ABOUT:
            DialogBox(m_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
            break;
        case IDM_EXIT:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        break;
    case WM_KEYUP:
        if (wParam == VK_ESCAPE)
        {
            PostQuitMessage(0);
        }
        break;
    case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        // Draw the avatar window and the video window
        PaintWindow(hdc, hWnd);
        EndPaint(hWnd, &ps);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
#ifdef OPENGLMODE
	case WM_SIZE:
		ReSizeGLScene(LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONDOWN:
		MouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_RBUTTONDOWN:
		m_wired = !m_wired;
		break;
	case WM_MOUSEMOVE:
		MouseMove(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		break;
	case WM_MOUSEWHEEL:
		MouseWheel(GET_WHEEL_DELTA_WPARAM(wParam), m_panelLayout.mouseX, m_panelLayout.mouseY);
		break;
#endif
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK SingleFace::About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// Drawing the video window
BOOL SingleFace::ShowVideo(HDC hdc, int width, int height, int originX, int originY)
{
    BOOL ret = TRUE;

    // Now, copy a fraction of the camera image into the screen.
    IFTImage* colorImage = m_FTHelper.GetColorImage();
    if (colorImage)
    {
        int iWidth = colorImage->GetWidth();
        int iHeight = colorImage->GetHeight();
        if (iWidth > 0 && iHeight > 0)
        {
            int iTop = 0;
            int iBottom = iHeight;
            int iLeft = 0;
            int iRight = iWidth;

            // Keep a separate buffer.
            if (m_pVideoBuffer && SUCCEEDED(m_pVideoBuffer->Allocate(iWidth, iHeight, FTIMAGEFORMAT_UINT8_B8G8R8A8)))
            {
                // Copy do the video buffer while converting bytes
                colorImage->CopyTo(m_pVideoBuffer, NULL, 0, 0);

                // Compute the best approximate copy ratio.
                float w1 = (float)iHeight * (float)width;
                float w2 = (float)iWidth * (float)height;
                if (w2 > w1 && height > 0)
                {
                    // video image too wide
                    float wx = w1/height;
                    iLeft = (int)max(0, m_FTHelper.GetXCenterFace() - wx / 2);
                    iRight = iLeft + (int)wx;
                    if (iRight > iWidth)
                    {
                        iRight = iWidth;
                        iLeft = iRight - (int)wx;
                    }
                }
                else if (w1 > w2 && width > 0)
                {
                    // video image too narrow
                    float hy = w2/width;
                    iTop = (int)max(0, m_FTHelper.GetYCenterFace() - hy / 2);
                    iBottom = iTop + (int)hy;
                    if (iBottom > iHeight)
                    {
                        iBottom = iHeight;
                        iTop = iBottom - (int)hy;
                    }
                }

                int const bmpPixSize = m_pVideoBuffer->GetBytesPerPixel();
                SetStretchBltMode(hdc, HALFTONE);
                BITMAPINFO bmi = {sizeof(BITMAPINFO), iWidth, iHeight, 1, static_cast<WORD>(bmpPixSize * CHAR_BIT), BI_RGB, m_pVideoBuffer->GetStride() * iHeight, 5000, 5000, 0, 0};
                if (0 == StretchDIBits(hdc, originX, originY, width, height,
                    iLeft, iBottom, iRight-iLeft, iTop-iBottom, m_pVideoBuffer->GetBuffer(), &bmi, DIB_RGB_COLORS, SRCCOPY))
                {
                    ret = FALSE;
                }
				RECT const& faceRect =  m_FTHelper.GetFaceRect();
				if (0 == StretchDIBits(hdc, 0, 0, width, height,
					faceRect.left, faceRect.bottom, faceRect.right-faceRect.left, faceRect.top-faceRect.bottom, m_pVideoBuffer->GetBuffer(), &bmi, DIB_RGB_COLORS, SRCCOPY))
				{
					ret = FALSE;
				}
            }
        }
    }
    return ret;
}

// Drawing code
BOOL SingleFace::ShowEggAvatar(HDC hdc, int width, int height, int originX, int originY)
{
    static int errCount = 0;
    BOOL ret = FALSE;

    if (m_pImageBuffer && SUCCEEDED(m_pImageBuffer->Allocate(width, height, FTIMAGEFORMAT_UINT8_B8G8R8A8)))
    {
        memset(m_pImageBuffer->GetBuffer(), 0, m_pImageBuffer->GetStride() * height); // clear to black

        m_eggavatar.SetScaleAndTranslationToWindow(height, width);
        m_eggavatar.DrawImage(m_pImageBuffer);

        BITMAPINFO bmi = {sizeof(BITMAPINFO), width, height, 1, static_cast<WORD>(m_pImageBuffer->GetBytesPerPixel() * CHAR_BIT), BI_RGB, m_pImageBuffer->GetStride() * height, 5000, 5000, 0, 0};
        errCount += (0 == StretchDIBits(hdc, 0, 0, width, height, 0, 0, width, height, m_pImageBuffer->GetBuffer(), &bmi, DIB_RGB_COLORS, SRCCOPY));

        ret = TRUE;
    }

    return ret;
}

// Draw the egg head and the camera video with the mask superimposed.
BOOL SingleFace::PaintWindow(HDC hdc, HWND hWnd)
{
    static int errCount = 0;
    BOOL ret = FALSE;
    RECT rect;
    GetClientRect(hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    int halfWidth = width/2;

#ifdef OPENGLMODE
	DrawGLScene();
	SwapBuffers(hdc);
#endif

    // Show the video on the right of the window
#ifdef IMAGEMODE    
	errCount += !ShowVideo(hdc, width - halfWidth, height, halfWidth, 0);
#endif

	//errCount += !ShowEye(hdc, halfWidth, height, 0, 0);

    // Draw the egg avatar on the left of the window
    //errCount += !ShowEggAvatar(hdc, halfWidth, height, 0, 0);

	

    return ret;
}

/*
* The "Face Tracker" helper class is generic. It will call back this function
* after a face has been successfully tracked. The code in the call back passes the parameters
* to the Egg Avatar, so it can be animated.
*/
void SingleFace::FTHelperCallingBack(PVOID pVoid)
{
#ifdef MYDEBUG
	std::cout << "FTHelperCallingBack() begin:" << std::endl;
#endif
    SingleFace* pApp = reinterpret_cast<SingleFace*>(pVoid);
    if (pApp)
    {
        IFTResult* pResult = pApp->m_FTHelper.GetResult();
        if (pResult && SUCCEEDED(pResult->GetStatus()))
        {
            FLOAT* pAU = NULL;
            UINT numAU;
            pResult->GetAUCoefficients(&pAU, &numAU);
            pApp->m_eggavatar.SetCandideAU(pAU, numAU);
            FLOAT scale;
            FLOAT rotationXYZ[3];
            FLOAT translationXYZ[3];
            pResult->Get3DPose(&scale, rotationXYZ, translationXYZ);
            pApp->m_eggavatar.SetTranslations(translationXYZ[0], translationXYZ[1], translationXYZ[2]);
            pApp->m_eggavatar.SetRotations(rotationXYZ[0], rotationXYZ[1], rotationXYZ[2]);
        }
    }
#ifdef MYDEBUG
	std::cout << "FTHelperCallingBack() end" << std::endl;
#endif
}

void SingleFace::ParseCmdString(PWSTR lpCmdLine)
{
    const WCHAR KEY_DEPTH[]                                 = L"-Depth";
    const WCHAR KEY_COLOR[]                                 = L"-Color";
    const WCHAR KEY_NEAR_MODE[]                             = L"-NearMode";
    const WCHAR KEY_DEFAULT_DISTANCE_MODE[]                 = L"-DefaultDistanceMode";
    const WCHAR KEY_SEATED_SKELETON_MODE[]                  = L"-SeatedSkeleton";

    const WCHAR STR_NUI_IMAGE_TYPE_DEPTH[]                  = L"DEPTH";
    const WCHAR STR_NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX[] = L"PLAYERID";
    const WCHAR STR_NUI_IMAGE_TYPE_COLOR[]                  = L"RGB";
    const WCHAR STR_NUI_IMAGE_TYPE_COLOR_YUV[]              = L"YUV";

    const WCHAR STR_NUI_IMAGE_RESOLUTION_80x60[]            = L"80x60";
    const WCHAR STR_NUI_IMAGE_RESOLUTION_320x240[]          = L"320x240";
    const WCHAR STR_NUI_IMAGE_RESOLUTION_640x480[]          = L"640x480";
    const WCHAR STR_NUI_IMAGE_RESOLUTION_1280x960[]         = L"1280x960";

    enum TOKEN_ENUM
    {
        TOKEN_ERROR,
        TOKEN_DEPTH,
        TOKEN_COLOR,
        TOKEN_NEARMODE,
        TOKEN_DEFAULTDISTANCEMODE,
        TOKEN_SEATEDSKELETON
    }; 

    int argc = 0;
    LPWSTR *argv = CommandLineToArgvW(lpCmdLine, &argc);

    for(int i = 0; i < argc; i++)
    {
        NUI_IMAGE_TYPE* pType = NULL;
        NUI_IMAGE_RESOLUTION* pRes = NULL;

        TOKEN_ENUM tokenType = TOKEN_ERROR; 
        PWCHAR context = NULL;
        PWCHAR token = wcstok_s(argv[i], L":", &context);
        if(0 == wcsncmp(token, KEY_DEPTH, ARRAYSIZE(KEY_DEPTH)))
        {
            tokenType = TOKEN_DEPTH;
            pType = &m_depthType;
            pRes = &m_depthRes;
        }
        else if(0 == wcsncmp(token, KEY_COLOR, ARRAYSIZE(KEY_COLOR)))
        {
            tokenType = TOKEN_COLOR;
            pType = &m_colorType;
            pRes = &m_colorRes;
        }
        else if(0 == wcsncmp(token, KEY_NEAR_MODE, ARRAYSIZE(KEY_NEAR_MODE)))
        {
            tokenType = TOKEN_NEARMODE;
            m_bNearMode = TRUE;
        }
        else if(0 == wcsncmp(token, KEY_DEFAULT_DISTANCE_MODE, ARRAYSIZE(KEY_DEFAULT_DISTANCE_MODE)))
        {
            tokenType = TOKEN_DEFAULTDISTANCEMODE;
            m_bNearMode = FALSE;
        }
        else if(0 == wcsncmp(token, KEY_SEATED_SKELETON_MODE, ARRAYSIZE(KEY_SEATED_SKELETON_MODE)))
        {
            tokenType = TOKEN_SEATEDSKELETON;
            m_bSeatedSkeletonMode = TRUE;
        }

        if(tokenType == TOKEN_DEPTH || tokenType == TOKEN_COLOR)
        {
            _ASSERT(pType != NULL && pRes != NULL);

            while((token = wcstok_s(NULL, L":", &context)) != NULL)
            {
                if(0 == wcsncmp(token, STR_NUI_IMAGE_TYPE_DEPTH, ARRAYSIZE(STR_NUI_IMAGE_TYPE_DEPTH)))
                {
                    *pType = NUI_IMAGE_TYPE_DEPTH;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX, ARRAYSIZE(STR_NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX)))
                {
                    *pType = NUI_IMAGE_TYPE_DEPTH_AND_PLAYER_INDEX;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_TYPE_COLOR, ARRAYSIZE(STR_NUI_IMAGE_TYPE_COLOR)))
                {
                    *pType = NUI_IMAGE_TYPE_COLOR;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_TYPE_COLOR_YUV, ARRAYSIZE(STR_NUI_IMAGE_TYPE_COLOR_YUV)))
                {
                    *pType = NUI_IMAGE_TYPE_COLOR_YUV;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_RESOLUTION_80x60, ARRAYSIZE(STR_NUI_IMAGE_RESOLUTION_80x60)))
                {
                    *pRes = NUI_IMAGE_RESOLUTION_80x60;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_RESOLUTION_320x240, ARRAYSIZE(STR_NUI_IMAGE_RESOLUTION_320x240)))
                {
                    *pRes = NUI_IMAGE_RESOLUTION_320x240;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_RESOLUTION_640x480, ARRAYSIZE(STR_NUI_IMAGE_RESOLUTION_640x480)))
                {
                    *pRes = NUI_IMAGE_RESOLUTION_640x480;
                }
                else if(0 == wcsncmp(token, STR_NUI_IMAGE_RESOLUTION_1280x960, ARRAYSIZE(STR_NUI_IMAGE_RESOLUTION_1280x960)))
                {
                    *pRes = NUI_IMAGE_RESOLUTION_1280x960;
                }
            }
        }
    }

    if(argv) LocalFree(argv);
}

BOOL SingleFace::ShowEye(HDC hdc, int width, int height, int originX, int originY)
{
	BOOL ret = TRUE;

	// Now, copy a fraction of the camera image into the screen.
	IFTImage* colorImage = m_FTHelper.GetColorImage();
	if (colorImage)
	{
		int iWidth = colorImage->GetWidth();
		int iHeight = colorImage->GetHeight();
		if (iWidth > 0 && iHeight > 0)
		{
			int iTop = 0;
			int iBottom = iHeight;
			int iLeft = 0;
			int iRight = iWidth;

			// Keep a separate buffer.
			if (m_pVideoBuffer && SUCCEEDED(m_pVideoBuffer->Allocate(iWidth, iHeight, FTIMAGEFORMAT_UINT8_B8G8R8A8)))
			{
				// Copy do the video buffer while converting bytes
				colorImage->CopyTo(m_pVideoBuffer, NULL, 0, 0);

				// Compute the best approximate copy ratio.
				float w1 = (float)iHeight * (float)width;
				float w2 = (float)iWidth * (float)height;
				if (w2 > w1 && height > 0)
				{
					// video image too wide
					float wx = w1/height;
					iLeft = (int)max(0, m_FTHelper.GetXCenterFace() - wx / 2);
					iRight = iLeft + (int)wx;
					if (iRight > iWidth)
					{
						iRight = iWidth;
						iLeft = iRight - (int)wx;
					}
				}
				else if (w1 > w2 && width > 0)
				{
					// video image too narrow
					float hy = w2/width;
					iTop = (int)max(0, m_FTHelper.GetYCenterFace() - hy / 2);
					iBottom = iTop + (int)hy;
					if (iBottom > iHeight)
					{
						iBottom = iHeight;
						iTop = iBottom - (int)hy;
					}
				}

				RECT const& faceRect =  m_FTHelper.GetFaceRect();
				int const bmpPixSize = m_pVideoBuffer->GetBytesPerPixel();
				SetStretchBltMode(hdc, HALFTONE);
				BITMAPINFO bmi = {sizeof(BITMAPINFO), iWidth, iHeight, 1, static_cast<WORD>(bmpPixSize * CHAR_BIT), BI_RGB, m_pVideoBuffer->GetStride() * iHeight, 5000, 5000, 0, 0};
				if (0 == StretchDIBits(hdc, originX, originY, width, height,
					faceRect.left, faceRect.bottom, faceRect.right-faceRect.left, faceRect.top-faceRect.bottom, m_pVideoBuffer->GetBuffer(), &bmi, DIB_RGB_COLORS, SRCCOPY))
				{
					ret = FALSE;
				}
			}
		}
	}
	return ret;
}

// Program's main entry point
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
#ifdef _DEBUG
	AllocConsole();

	HANDLE handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long) handle_out, _O_TEXT);
	FILE* hf_out = _fdopen(hCrt, "w");
	setvbuf(hf_out, NULL, _IONBF, 1);
	*stdout = *hf_out;

	HANDLE handle_in = GetStdHandle(STD_INPUT_HANDLE);
	hCrt = _open_osfhandle((long) handle_in, _O_TEXT);
	FILE* hf_in = _fdopen(hCrt, "r");
	setvbuf(hf_in, NULL, _IONBF, 128);
	*stdin = *hf_in;
#endif

    UNREFERENCED_PARAMETER(hPrevInstance);
    SingleFace app;

    HeapSetInformation(NULL, HeapEnableTerminationOnCorruption, NULL, 0);

    return app.Run(hInstance, lpCmdLine, nCmdShow);
}
