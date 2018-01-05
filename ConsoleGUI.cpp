#include <SFML/Graphics.hpp>

#if _WIN32_WINNT < 0x0501
#define _WIN32_WINNT 0x0501
#endif
#include <windows.h>

// Set part of the window that can be clicked by removing fully transparent pixels from the region
void setShape(HWND hWnd, const sf::Image& image)
{
	const sf::Uint8* pixelData = image.getPixelsPtr();
	HRGN hRegion = CreateRectRgn(0, 0, image.getSize().x, image.getSize().y);

	for (unsigned int y = 0; y < image.getSize().y; y++)
	{
		for (unsigned int x = 0; x < image.getSize().x; x++)
		{
			if (pixelData[y * image.getSize().x * 4 + x * 4 + 3] == 0)
			{
				HRGN hRegionPixel = CreateRectRgn(x, y, x + 1, y + 1);
				CombineRgn(hRegion, hRegion, hRegionPixel, RGN_XOR);
				DeleteObject(hRegionPixel);
			}
		}
	}

	SetWindowRgn(hWnd, hRegion, true);
	DeleteObject(hRegion);
}

int main()
{

	// Change this if you want to apply transparency on top of the one already in the image
	const sf::Uint8 alpha = 0xff;

	// Load an image with transparent parts that will be drawn to the screen
	sf::Image image;
	image.loadFromFile("image.png");

	// Create the window and center it on the screen
	sf::RenderWindow window(sf::VideoMode(image.getSize().x, image.getSize().y, 32), "Transparent Window", sf::Style::None);
	window.setPosition(sf::Vector2i((sf::VideoMode::getDesktopMode().width - image.getSize().x) / 2,
		(sf::VideoMode::getDesktopMode().height - image.getSize().y) / 2));

	HWND hWnd = window.getSystemHandle();

	// Full transparent pixels should not be part of the window
	setShape(hWnd, image);

	// Create a DC for our bitmap
	HDC hdcWnd = GetDC(hWnd);
	HDC hdc = CreateCompatibleDC(hdcWnd);

	// The window has to be layered if you want transparency
	SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST);
	SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

	// Create our DIB section and select the bitmap into the DC
	void* pvBits;
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = image.getSize().x;
	bmi.bmiHeader.biHeight = image.getSize().y;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;         // four 8-bit components
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = image.getSize().x * image.getSize().y * 4;
	HBITMAP hbitmap = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0x0);
	SelectObject(hdc, hbitmap);

	// Copy the pixels from the image to the bitmap (but pre-multiply the alpha value)
	const sf::Uint8* pixelData = image.getPixelsPtr();
	for (unsigned int y = 0; y < image.getSize().y; ++y)
	{
		for (unsigned int x = 0; x < image.getSize().x; ++x)
		{
			float fAlphaFactor = (float)pixelData[y * image.getSize().x * 4 + x * 4 + 3] / (float)0xff;
			((UINT32 *)pvBits)[x + (image.getSize().y - y - 1) * image.getSize().x]
				= (pixelData[y * image.getSize().x * 4 + x * 4 + 3] << 24) |                        //0xaa000000
				((UCHAR)(pixelData[y * image.getSize().x * 4 + x * 4] * fAlphaFactor) << 16) |     //0x00rr0000
				((UCHAR)(pixelData[y * image.getSize().x * 4 + x * 4 + 1] * fAlphaFactor) << 8) |  //0x0000gg00
				((UCHAR)(pixelData[y * image.getSize().x * 4 + x * 4 + 2] * fAlphaFactor));      //0x000000bb
		}
	}

	// Put the image on the screen
	POINT ptSrc = { 0, 0 };
	SIZE sizeWnd = { (long)image.getSize().x, (long)image.getSize().y };
	BLENDFUNCTION bf;
	bf.BlendOp = AC_SRC_OVER;
	bf.BlendFlags = 0;
	bf.AlphaFormat = AC_SRC_ALPHA;
	bf.SourceConstantAlpha = alpha;
	UpdateLayeredWindow(hWnd, NULL, NULL, &sizeWnd, hdc, &ptSrc, 0, &bf, ULW_ALPHA);

	// Cleanup
	DeleteObject(hbitmap);
	DeleteDC(hdc);
	DeleteDC(hdcWnd);

	// Main loop that handles the events but does not draw anything
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape))
				window.close();
		}

		// Don't waste CPU time
		sf::sleep(sf::milliseconds(10));
	}
}
