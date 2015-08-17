#pragma once

#include "wx/glcanvas.h"
#include "wx/timer.h"

#include <vector>
#include <queue>

#include "InteractiveCanvas.h"
#include "ModeSelectorContext.h"
#include "MouseTracker.h"

#include "fftw3.h"
#include "Timer.h"

class ModeSelectorMode {
public:
    int value;
    std::string label;

    ModeSelectorMode(int value, std::string label) : value(value), label(label) {

    }
};

class ModeSelectorCanvas: public InteractiveCanvas {
public:
    ModeSelectorCanvas(wxWindow *parent, int *attribList = NULL);
    ~ModeSelectorCanvas();

    int getHoveredSelection();
    void setHelpTip(std::string tip);

    void addChoice(int value, std::string label);
    void setSelection(int value);
    int getSelection();

    void setToggleMode(bool toggleMode);

    bool modeChanged();
    void clearModeChanged();
    
private:
    void setNumChoices(int numChoices_in);

    void OnPaint(wxPaintEvent& event);
    void OnIdle(wxIdleEvent &event);

    void OnMouseMoved(wxMouseEvent& event);
    void OnMouseDown(wxMouseEvent& event);
    void OnMouseWheelMoved(wxMouseEvent& event);
    void OnMouseReleased(wxMouseEvent& event);
    void OnMouseEnterWindow(wxMouseEvent& event);
    void OnMouseLeftWindow(wxMouseEvent& event);

    ModeSelectorContext *glContext;

    std::string helpTip;
    int numChoices;
    int currentSelection;
    bool toggleMode;
    bool inputChanged;
    std::vector<ModeSelectorMode> selections;
    //
wxDECLARE_EVENT_TABLE();
};

