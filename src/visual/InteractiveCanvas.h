#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include "MouseTracker.h"
#include <string>

class InteractiveCanvas: public wxGLCanvas {
public:
    InteractiveCanvas(wxWindow *parent, int *attribList = NULL);
    ~InteractiveCanvas();

    int getFrequencyAt(float x);

    void setView(int center_freq_in, int bandwidth_in);
    void disableView();

    void setCenterFrequency(unsigned int center_freq_in);
    unsigned int getCenterFrequency();

    void setBandwidth(unsigned int bandwidth_in);
    unsigned int getBandwidth();

protected:
    void OnKeyDown(wxKeyEvent& event);
    void OnKeyUp(wxKeyEvent& event);

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseRightDown(wxMouseEvent& event);
    void OnMouseRightReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    void setStatusText(std::string statusText);
    void setStatusText(std::string statusText, int value);

    wxWindow *parent;
    MouseTracker mouseTracker;

    bool shiftDown;
    bool altDown;
    bool ctrlDown;

    unsigned int centerFreq;
    unsigned int bandwidth;
    unsigned int lastBandwidth;

    bool isView;
};

