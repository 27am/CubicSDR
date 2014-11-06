#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include "CubicSDRDefs.h"
#include "fftw3.h"

#include "Demodulate.h"

#include <AL/al.h>
#include <AL/alc.h>


class PrimaryGLContext: public wxGLContext {
public:
    PrimaryGLContext(wxGLCanvas *canvas);

    void Plot(std::vector<float> &points,std::vector<float> &points2);

private:
    // textures for the cube faces
    GLuint m_textures[6];
};

class TestGLCanvas: public wxGLCanvas {
public:
    TestGLCanvas(wxWindow *parent, int *attribList = NULL);
    ~TestGLCanvas();

    void setData(std::vector<signed char> *data);

private:
    void OnPaint(wxPaintEvent& event);
    void OnKeyDown(wxKeyEvent& event);

    void OnIdle(wxIdleEvent &event);

    wxWindow *parent;
    std::vector<float> spectrum_points;
    std::vector<float> waveform_points;

    fftw_complex *in, *out[2];
    fftw_plan plan[2];

    float fft_ceil_ma, fft_ceil_maa;

    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;

    Demodulate demod;

    ALCdevice *dev;
    ALCcontext *ctx;

wxDECLARE_EVENT_TABLE();
};
