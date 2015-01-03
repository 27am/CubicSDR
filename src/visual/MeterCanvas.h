#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "MeterContext.h"
#include "MouseTracker.h"

#include "fftw3.h"
#include "Timer.h"

class MeterCanvas: public wxGLCanvas {
public:
    std::vector<float> waveform_points;

    MeterCanvas(wxWindow *parent, int *attribList = NULL);
    ~MeterCanvas();

    void setLevel(float level_in);
    float getLevel();

    void setMax(float max_in);

    bool setInputValue(float slider_in);
    bool inputChanged();
    float getInputValue();

private:
    void OnPaint(wxPaintEvent& event);
    void OnIdle(wxIdleEvent &event);

    void mouseMoved(wxMouseEvent& event);
    void mouseDown(wxMouseEvent& event);
    void mouseWheelMoved(wxMouseEvent& event);
    void mouseReleased(wxMouseEvent& event);
    void mouseEnterWindow(wxMouseEvent& event);
    void mouseLeftWindow(wxMouseEvent& event);

    MouseTracker mTracker;
    wxWindow *parent;
    MeterContext *glContext;

    float level;
    float level_max;

    float inputValue;
    float userInputValue;

    bool shiftDown;
    bool altDown;
    bool ctrlDown;
wxDECLARE_EVENT_TABLE();
};

