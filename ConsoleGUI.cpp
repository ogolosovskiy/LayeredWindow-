
#if _WIN32_WINNT < 0x0501
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>

#include <stdio.h>

#define CURRENT_WND_CLASS L"WndClass_AAA"
#define IDT_TIMER 0x100

unsigned int width = 400;
unsigned int height = 400;
HDC hdcBmp;
int globalDeb = 0;
void* pvBits;


void clear()
{
	for (unsigned int y = 0; y < height; ++y)
	{
		for (unsigned int x = 0; x < width; ++x)
		{
			float fAlphaFactor = 0;
			int alpha = 0;

			((UINT32 *)pvBits)[x + (height - y - 1) * width]
				= (alpha << 24) |                        //0xaa000000
				((UCHAR)(150 * fAlphaFactor) << 16) |     //0x00rr0000
				((UCHAR)(150 * fAlphaFactor) << 8) |  //0x0000gg00
				((UCHAR)(150 * fAlphaFactor));      //0x000000bb

		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_TIMER:
		switch (wParam)
		{
			case IDT_TIMER:
			{
				// Put the image on the screen
				POINT ptSrc = { 0, 0 };
				SIZE sizeWnd = { (long)width, (long)height };
				BLENDFUNCTION bf;
				bf.BlendOp = AC_SRC_OVER;
				bf.BlendFlags = 0;
				bf.AlphaFormat = AC_SRC_ALPHA;
				bf.SourceConstantAlpha = 255;
				UpdateLayeredWindow(hWnd, NULL, NULL, &sizeWnd, hdcBmp, &ptSrc, 0, &bf, ULW_ALPHA);


				// clear area
				clear();

				// draw line
				HPEN hBluePen = CreatePen(PS_SOLID, 5, RGB(0, 255, 255));
				SelectObject(hdcBmp, hBluePen); //делаем кисть активной
				MoveToEx(hdcBmp, 10, 10, NULL);
				LineTo(hdcBmp, 300, 300 + globalDeb);
				globalDeb += 50;

				return 0;
			}
			default:
				return DefWindowProc(hWnd, uMsg, wParam, lParam);
		}
	case WM_QUIT:
	case WM_DESTROY:
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}

int main()
{
	WNDCLASSEX wcex; /* Structure needed for creating Window */
	MSG msg;

	ZeroMemory(&wcex, sizeof(WNDCLASSEX));
	ZeroMemory(&msg, sizeof(MSG));
	HINSTANCE hInstance = GetModuleHandle(NULL);

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wcex.hCursor = LoadCursor(hInstance, IDC_ARROW);
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hInstance = hInstance;
	wcex.lpfnWndProc = WndProc;
	wcex.lpszClassName = CURRENT_WND_CLASS;
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.cbWndExtra = 0;
	wcex.cbClsExtra = 0;
	wcex.lpszMenuName = NULL;

	if (!RegisterClassEx(&wcex)) {
		return -1;
	}


	// Create the window and center it on the screen
	HWND hWnd = CreateWindowEx(	WS_EX_LAYERED | // Layered Windows
								WS_EX_TRANSPARENT | // Don't hittest this window
								WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
								CURRENT_WND_CLASS, 
								L"Title",
								WS_POPUP | WS_VISIBLE, 
								200, 200, width, height, 
								NULL, // hWndParent
								NULL, // hMenu
								hInstance,
								NULL);
	
	// Full transparent pixels should not be part of the window
	//setShape(hWnd, image);

	// Create a DC for our bitmap
	HDC hdcWnd = GetDC(hWnd);
	hdcBmp = CreateCompatibleDC(hdcWnd);

	// The window has to be layered if you want transparency
	SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// Create our DIB section and select the bitmap into the DC
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;         // four 8-bit components
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = width * height * 4;
	HBITMAP hbitmap = CreateDIBSection(hdcBmp, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
	SelectObject(hdcBmp, hbitmap);

	// Copy the pixels from the image to the bitmap (but pre-multiply the alpha value)
/*	for (unsigned int y = 0; y < height; ++y)
	{
		for (unsigned int x = 0; x < width; ++x)
		{
			float fAlphaFactor = 0;

			if (y>100)
				fAlphaFactor = 1;

			int alpha = 0;
			if (x>100)
				alpha = 255;

			((UINT32 *)pvBits)[x + (height - y - 1) * width]
				= (alpha << 24) |                        //0xaa000000
				((UCHAR)(150 * fAlphaFactor) << 16) |     //0x00rr0000
				((UCHAR)(150 * fAlphaFactor) << 8) |  //0x0000gg00
				((UCHAR)(150 * fAlphaFactor));      //0x000000bb

//			float fAlphaFactor = (float)pixelData[y * image.getSize().x * 4 + x * 4 + 3] / (float)0xff;
//			((UINT32 *)pvBits)[x + (image.getSize().y - y - 1) * image.getSize().x]
//				= (pixelData[y * image.getSize().x * 4 + x * 4 + 3] << 24) |                        //0xaa000000
//				((UCHAR)(pixelData[y * image.getSize().x * 4 + x * 4] * fAlphaFactor) << 16) |     //0x00rr0000
//				((UCHAR)(pixelData[y * image.getSize().x * 4 + x * 4 + 1] * fAlphaFactor) << 8) |  //0x0000gg00
//				((UCHAR)(pixelData[y * image.getSize().x * 4 + x * 4 + 2] * fAlphaFactor));      //0x000000bb
		}
	} 
	*/

	/*
	// Put the image on the screen
	POINT ptSrc = { 0, 0 };
	SIZE sizeWnd = { (long)width, (long)height };
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.SourceConstantAlpha = 255;
	UpdateLayeredWindow(hWnd, NULL, NULL, &sizeWnd, hdc, &ptSrc, 0, &bf, ULW_ALPHA);
	*/


	SetTimer(hWnd, IDT_TIMER,
				1000,                 // 10-second interval 
				(TIMERPROC)NULL);

	BOOL bDone = FALSE;
	while (FALSE == bDone) {
		if (PeekMessage(&msg, hWnd, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT) {
				bDone = TRUE;
			}
		}
		else {
			/* do rendering here */
		}
	}

	// Cleanup
	DeleteObject(hbitmap);
	DeleteDC(hdcBmp);
	DeleteDC(hdcWnd);

	DestroyWindow(hWnd);
	UnregisterClass(CURRENT_WND_CLASS, hInstance);
	return 0;
}
