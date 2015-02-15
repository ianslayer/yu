#include "renderer.h"

#if defined YU_GL

#include "../core/system.h"
#include "gl_utility.h"

#pragma comment(lib,"opengl32.lib")
PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;

void open_libgl(void);
void close_libgl(void);
void *get_proc(const char *proc);

void GetWGLExt()
{
	open_libgl();
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)get_proc("wglChoosePixelFormatARB");
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)get_proc("wglCreateContextAttribsARB");
	close_libgl();
}

YU_INTERNAL LRESULT CALLBACK FakeWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
		//case WM_DESTROY:
		// PostQuitMessage(0);
		// break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}


void InitGLContext(const yu::Window& win, const yu::RendererDesc& desc)
{
	//create fake window
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);

	DWORD windowStyle = WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = FakeWndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = NULL;
	wcex.hIcon = NULL;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = TEXT("fakeMainWindow");
	wcex.hIconSm = NULL;

	RegisterClassEx(&wcex);

	HWND fakeWinHwnd = CreateWindowEx(0, TEXT("fakeMainWindow"), TEXT("fakeWindow"),
		windowStyle, 0, 0, 512, 512, NULL, NULL, NULL, NULL);

	HDC fakeDc;
	fakeDc = GetDC(fakeWinHwnd);
	int result;
	PIXELFORMATDESCRIPTOR  pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		32,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // useless parameters
		32,
		0, 0, 0, 0, 0, 0, 0
	};

	int indexPixelFormat = ChoosePixelFormat(fakeDc, &pfd);
	result = SetPixelFormat(fakeDc, indexPixelFormat, &pfd);

	HGLRC fakeHglrc = wglCreateContext(fakeDc);

	wglMakeCurrent(fakeDc, fakeHglrc);
	GetWGLExt();

	int enableMSAA = GL_FALSE;
	if (desc.sampleCount > 1)
		enableMSAA = GL_TRUE;

	//create real context
	int attributeListInt[] = {
		WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
		WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
		WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
		WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
		WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
		WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
		WGL_COLOR_BITS_ARB, 24,
		WGL_DEPTH_BITS_ARB, 24,
		WGL_STENCIL_BITS_ARB, 8,
		WGL_ACCUM_BITS_ARB, 0,
		WGL_SAMPLE_BUFFERS_ARB, enableMSAA, // MSAA on
		//WGL_SAMPLES_ARB, desc.sampleCount,
		0, 0
	};

	float fAttributes[] = { 0, 0 };

	int pixelFormat[1];
	unsigned int formatCount;

	PIXELFORMATDESCRIPTOR pixelFormatDescriptor;
	// int attributeList[5];

	HDC hdc = GetDC(win.hwnd);
	// Query for a pixel format that fits the attributes we want.
	result = wglChoosePixelFormatARB(hdc, attributeListInt, fAttributes, 1, pixelFormat, &formatCount);


	if (result != 1)
	{
		
	}
	result = DescribePixelFormat(hdc, pixelFormat[0], sizeof(PIXELFORMATDESCRIPTOR), &pixelFormatDescriptor);
	// If the video card/display can handle our desired pixel format then we set it as the current one.
	result = SetPixelFormat(hdc, pixelFormat[0], &pixelFormatDescriptor);
	DWORD error = 0;
	if (result != 1)
	{
		error = GetLastError();
		
	}
	// int attriblist[] = {WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
	//                    WGL_CONTEXT_MINOR_VERSION_ARB, 0,
	//                    0, 0};

	HGLRC hglrc = wglCreateContextAttribsARB(hdc, 0, 0);

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(fakeHglrc);
	ReleaseDC(fakeWinHwnd, fakeDc);
	DestroyWindow(fakeWinHwnd);
	UnregisterClass(TEXT("fakeMainWindow"), 0);

	BOOL success = wglMakeCurrent(hdc, hglrc);

	if (!success)
	{
	}

	int initGL = gl3wInit();
	if (initGL != 0)
	{
	}

	GetWGLExt();

	ReleaseDC(win.hwnd, hdc);

}

void SwapBuffer(yu::Window& win)
{
	HDC hdc = GetDC(win.hwnd);
	SwapBuffers(hdc);
	ReleaseDC(win.hwnd, hdc);
}

#endif