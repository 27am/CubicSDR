#pragma once

#include "PrimaryGLContext.h"
#include "Gradient.h"

#define NUM_WATERFALL_LINES 512

class ScopeCanvas;

class ScopeContext: public PrimaryGLContext {
public:
    ScopeContext(ScopeCanvas *canvas, wxGLContext *sharedContext);

    void Plot(std::vector<float> &points, std::vector<float> &points2);

private:
    Gradient grad;
    GLuint waterfall;
    unsigned char waterfall_tex[FFT_SIZE * NUM_WATERFALL_LINES];
};
