#pragma once

typedef struct RGBColor {
	int r;
	int g;
	int b;
} RGBColor;

RGBColor hsv2rgb(float H, float S, float V);
