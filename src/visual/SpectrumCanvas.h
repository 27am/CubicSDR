#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "SpectrumContext.h"

#include "fftw3.h"
#include "Timer.h"
#include "MouseTracker.h"

class SpectrumCanvas: public wxGLCanvas {
public:
    std::vector<float> spectrum_points;

    SpectrumCanvas(wxWindow *parent, int *attribList = NULL);
    void Setup(int fft_size_in);
    ~SpectrumCanvas();

    void setData(DemodulatorThreadIQData *input);

    void SetView(int center_freq_in, int bandwidth_in);
    void DisableView();

    void SetCenterFrequency(unsigned int center_freq_in);
    unsigned int GetCenterFrequency();

    void SetBandwidth(unsigned int bandwidth_in);
    unsigned int GetBandwidth();

private:
    void OnPaint(wxPaintEvent& event);

    void OnIdle(wxIdleEvent &event);

    void mouseMoved(wxMouseEvent& event);
    void mouseDown(wxMouseEvent& event);
    void mouseWheelMoved(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);

//    void rightClick(wxMouseEvent& event);
    void mouseLeftWindow(wxMouseEvent& event);

    wxWindow *parent;

    fftw_complex *in, *out;
    fftw_plan plan;

    float fft_ceil_ma, fft_ceil_maa;
    float fft_floor_ma, fft_floor_maa;

    std::vector<float> fft_result;
    std::vector<float> fft_result_ma;
    std::vector<float> fft_result_maa;

    SpectrumContext *glContext;
    int fft_size;

    unsigned int center_freq;
    unsigned int bandwidth;

    bool isView;

    MouseTracker mTracker;
// event table
wxDECLARE_EVENT_TABLE();
};

