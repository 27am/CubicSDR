#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"

#define NUM_WATERFALL_LINES 512

class WaterfallCanvas;

class WaterfallContext: public PrimaryGLContext {
public:
    WaterfallContext(WaterfallCanvas *canvas, wxGLContext *sharedContext);

    void BeginDraw();
    void Draw(std::vector<float> &points);
    void DrawFreqSelector(float uxPos);
    void EndDraw();

private:
    Gradient grad;
    GLuint waterfall;
    unsigned char waterfall_tex[FFT_SIZE * NUM_WATERFALL_LINES];
};
