#pragma once
#include <windows.h>

#define MAX_COLORS  16

#define CL_AQUA     0xFFFF00
#define CL_GRAY     0x080808
#define CL_NAVY     0x080000
#define CL_SILVER   0x0C0C0C

#define CL_BLACK    0x000000
#define CL_GREEN    0x000800
#define CL_OLIVE    0x000808
#define CL_TEAL     0x080800

#define CL_BLUE     0xFF0000
#define CL_LIME     0x00FF00
#define CL_PURPLE   0x080008 
#define CL_FUCHSIA  0xFF00FF

#define CL_MAROON   0x000008
#define CL_RED      0x0000FF
#define CL_YELLOW   0x00FFFF
#define CL_BROWN    0xA2A25A

#define CL_WHITE    0xFFFFFF
#define CL_GRID		0x484848
#define CL_AXES		0x222222


int getColorByIndex(int index) {
	switch (index % MAX_COLORS) {
	case 0: return CL_AQUA;
	case 1: return CL_GRAY;
	case 2: return CL_NAVY;
	case 3: return CL_SILVER;
	
	case 4: return CL_BLACK;
	case 5: return CL_GREEN;
	case 6: return CL_OLIVE;
	case 7: return CL_TEAL;
	
	case 8: return CL_BLUE;
	case 9: return CL_LIME;
	case 10: return CL_PURPLE;
	case 11: return CL_FUCHSIA;

	case 12: return CL_MAROON;
	case 13: return CL_RED;
	case 14: return CL_YELLOW;
	case 15: return CL_BROWN;

	default: return CL_WHITE;
	}
}

LPCWSTR getColorName(int rgb) {
	switch (rgb) {
	case CL_AQUA: return L"морская волна";
	case CL_GRAY: return L"серый";
	case CL_NAVY: return L"тёмно-синий";
	case CL_SILVER: return L"серебряный";

	case CL_BLACK: return L"чёрный";
	case CL_GREEN: return L"зелёный";
	case CL_OLIVE: return L"оливковый";
	case CL_TEAL: return L"сине-зелёный";

	case CL_BLUE: return L"синий";
	case CL_LIME: return L"лаймовый";
	case CL_PURPLE: return L"фиолетовый";
	case CL_FUCHSIA: return L"фуксия";

	case CL_MAROON: return L"бордовый";
	case CL_RED: return L"красный";
	case CL_YELLOW: return L"жёлтый";
	case CL_BROWN: return L"коричневый";

	case CL_WHITE: return L"белый";
	default: return NULL;
	}
}



template <typename T>
struct Point {
	T x, y;
	Point(T x, T y) : x(x), y(y) {};
};

struct ViewPort {
	double xReal = 0, yReal = 0, wReal = 1, hReal = 1;
	int xScreen = 0, yScreen = 0, wScreen, hScreen;

	inline Point<double> toReal(int x, int y) const {
		return Point<double>((x - xScreen) / wScreen * wReal, (y - yScreen) / hScreen * hReal);
	}

	inline Point<int> toScreen(double x, double y) const  {
		return Point<int>((x - xReal) / wReal * wScreen, (y - yReal) / hReal * hScreen);
	}

	inline Point<double> toReal(Point<int>& p) const  {
		return Point<double>((p.x - xScreen) / wScreen * wReal, (p.y - yScreen) / hScreen * hReal);
	}

	inline Point<int> toScreen(Point<double>& p) const {
		return Point<int>((p.x - xReal) / wReal * wScreen, (p.y - yReal) / hReal * hScreen);
	}

	inline RECT& toRect() {
		RECT rect;
		rect.left = xScreen; rect.right = xScreen + wScreen;
		rect.top = yScreen; rect.bottom = yScreen + hScreen;
		return rect;
	}
};
