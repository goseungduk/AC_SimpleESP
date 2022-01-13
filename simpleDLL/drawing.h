#pragma once
#include <iostream>
#include "pch.h"

void DrawStr(HDC hdcObj, int x, int y, const char* text, int thickness) {
	HFONT Font = CreateFont(thickness, 0, 0, 0, 0, 0, 0, 0, HANGEUL_CHARSET, 0, 0, 0,
		VARIABLE_PITCH | FF_ROMAN, TEXT("±Ã¼­"));
	SetTextColor(hdcObj, RGB(50, 200, 255));
	SetTextAlign(hdcObj, TA_CENTER | TA_NOUPDATECP);
	SetBkColor(hdcObj, RGB(0, 0, 0));
	SetBkMode(hdcObj, TRANSPARENT);
	SelectObject(hdcObj, Font);
	TextOutA(hdcObj, x, y, text, strlen(text));
	DeleteObject(Font);
}

void DrawRect(HDC hdcObj, int x, int y, int w, int h, int thick, HBRUSH Brush) {
	RECT rect_up = { x, y, x + w, y + thick };
	RECT rect_left = { x, y, x + thick, y - h };
	RECT rect_right = { x + w, y, x + w + thick, y - h };
	RECT rect_down = { x, y - h, x + w, y - h + thick };
	FillRect(hdcObj, &rect_up, Brush);
	FillRect(hdcObj, &rect_left, Brush);
	FillRect(hdcObj, &rect_right, Brush);
	FillRect(hdcObj, &rect_down, Brush);
}