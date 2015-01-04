#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "MeterContext.h"
#include "MouseTracker.h"

#include "fftw3.h"
#include "Timer.h"

class MeterCanvas: public InteractiveCanvas {
public:
    MeterCanvas(wxWindow *parent, int *attribList = NULL);
    ~MeterCanvas();

    void setLevel(float level_in);
    float getLevel();

    void setMax(float max_in);

    bool setInputValue(float slider_in);
    bool inputChanged();
    float getInputValue();

    void setHelpTip(std::string tip);

private:
    void OnPaint(wxPaintEvent& event);
    void OnIdle(wxIdleEvent &event);

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    MeterContext *glContext;

    float level;
    float level_max;

    float inputValue;
    float userInputValue;

    std::string helpTip;
    //
wxDECLARE_EVENT_TABLE();
};

