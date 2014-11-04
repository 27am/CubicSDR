#pragma once

#include "wx/frame.h"
#include "PrimaryGLContext.h"
#include "SDRThread.h"

// Define a new frame type
class AppFrame: public wxFrame {
public:
    AppFrame();
    ~AppFrame();
    void OnEventInput(wxThreadEvent& event);

private:
    void OnClose(wxCommandEvent& event);
    void OnNewWindow(wxCommandEvent& event);
    void OnIdle(wxIdleEvent& event);

    TestGLCanvas *canvas;
    SDRThread *t_SDR;
    IQBufferThread *t_IQBuffer;
    wxCriticalSection m_pThreadCS;
    SDRThreadQueue* m_pQueue;

wxDECLARE_EVENT_TABLE();
};
