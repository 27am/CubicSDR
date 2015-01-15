#pragma once

#include "Gradient.h"

#include <map>
#include <vector>
#include <string>

#define COLOR_THEME_DEFAULT 0
#define COLOR_THEME_BW 1
#define COLOR_THEME_SHARP 2
#define COLOR_THEME_RAD 3
#define COLOR_THEME_TOUCH 4
#define COLOR_THEME_HD 5
#define COLOR_THEME_RADAR 6
#define COLOR_THEME_MAX 7

class RGBColor {
public:
    float r, g, b;
    RGBColor(float r, float g, float b) :
            r(r), g(g), b(b) {
    }

    RGBColor() :
            RGBColor(0, 0, 0) {
    }

    ~RGBColor() {
    }

    RGBColor & operator=(const RGBColor &other) {
        r = other.r;
        g = other.g;
        b = other.b;
        return *this;
    }
};

class ColorTheme {
public:
    RGBColor waterfallHighlight;
    RGBColor waterfallNew;
    RGBColor wfHighlight;
    RGBColor waterfallHover;
    RGBColor waterfallDestroy;
    RGBColor fftLine;
    RGBColor fftHighlight;
    RGBColor scopeLine;
    RGBColor tuningBar;
    RGBColor meterLevel;
    RGBColor meterValue;
    RGBColor text;
    RGBColor freqLine;
    RGBColor button;
    RGBColor buttonHighlight;

    Gradient waterfallGradient;

    std::string name;
};

class ThemeMgr {
public:
    ThemeMgr();
    ~ThemeMgr();
    ColorTheme *currentTheme;
    std::map<int, ColorTheme *> themes;
    void setTheme(int themeId);
    int getTheme();
    int themeId;

    static ThemeMgr mgr;
};

class DefaultColorTheme: public ColorTheme {
public:
    DefaultColorTheme();
};

class BlackAndWhiteColorTheme: public ColorTheme {
public:
    BlackAndWhiteColorTheme();
};

class SharpColorTheme: public ColorTheme {
public:
    SharpColorTheme();
};

class RadColorTheme: public ColorTheme {
public:
    RadColorTheme();
};

class TouchColorTheme: public ColorTheme {
public:
    TouchColorTheme();
};

class HDColorTheme: public ColorTheme {
public:
    HDColorTheme();
};

class RadarColorTheme: public ColorTheme {
public:
    RadarColorTheme();
};

