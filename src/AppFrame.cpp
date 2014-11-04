#include "AppFrame.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include <vector>
#include "SDRThread.h"

wxBEGIN_EVENT_TABLE(AppFrame, wxFrame)
//EVT_MENU(wxID_NEW, AppFrame::OnNewWindow)
EVT_MENU(wxID_CLOSE, AppFrame::OnClose)
EVT_THREAD(EVENT_SDR_INPUT, AppFrame::OnEventInput)
EVT_IDLE(AppFrame::OnIdle)
wxEND_EVENT_TABLE()

AppFrame::AppFrame() :
        wxFrame(NULL, wxID_ANY, wxT("CubicSDR")) {

    canvas = new TestGLCanvas(this, NULL);

//    SetIcon(wxICON(sample));

// Make a menubar
    wxMenu *menu = new wxMenu;
//    menu->Append(wxID_NEW);
//    menu->AppendSeparator();
    menu->Append(wxID_CLOSE);
    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(menu, wxT("&File"));

    SetMenuBar(menuBar);

    CreateStatusBar();
    SetClientSize(1280, 400);
    Centre();
    Show();

    m_pQueue = new SDRThreadQueue(this);

    t_SDR = new SDRThread(m_pQueue);
    if (t_SDR->Run() != wxTHREAD_NO_ERROR) {
        wxLogError
        ("Can't create the thread!");
        delete t_SDR;
        t_SDR = NULL;
    }

//    t_IQBuffer = new IQBufferThread(this);
//    if (t_IQBuffer->Run() != wxTHREAD_NO_ERROR) {
//        wxLogError
//        ("Can't create the thread!");
//        delete t_IQBuffer;
        t_IQBuffer = NULL;
//    }

//    static const int attribs[] = { WX_GL_RGBA, WX_GL_DOUBLEBUFFER, 0 };
//    wxLogStatus("Double-buffered display %s supported", wxGLCanvas::IsDisplaySupported(attribs) ? "is" : "not");
//    ShowFullScreen(true);
}

AppFrame::~AppFrame() {
    delete t_SDR;
//    delete t_IQBuffer;
    delete m_pQueue;
}

void AppFrame::OnClose(wxCommandEvent& WXUNUSED(event)) {

    {
        wxCriticalSectionLocker enter(m_pThreadCS);
        if (t_SDR) {
            wxMessageOutputDebug().Printf("CubicSDR: deleting thread");
            if (t_SDR->Delete() != wxTHREAD_NO_ERROR) {
                wxLogError
                ("Can't delete the thread!");
            }
        }
    }

    {
        wxCriticalSectionLocker enter(m_pThreadCS);
        if (t_IQBuffer) {
            wxMessageOutputDebug().Printf("CubicSDR: deleting thread");
            if (t_IQBuffer->Delete() != wxTHREAD_NO_ERROR) {
                wxLogError
                ("Can't delete the thread!");
            }
        }
    }
    // true is to force the frame to close
    Close(true);
}

void AppFrame::OnNewWindow(wxCommandEvent& WXUNUSED(event)) {
    new AppFrame();
}

void AppFrame::OnEventInput(wxThreadEvent& event) {
    std::vector<signed char> *new_buffer = event.GetPayload<std::vector<signed char> *>();

//    std::cout << "Got IQ buffer, length: " << new_buffer->size() << std::endl;

    canvas->setData(new_buffer);

    delete new_buffer;
}

void AppFrame::OnIdle(wxIdleEvent& event) {

    event.Skip();
}
