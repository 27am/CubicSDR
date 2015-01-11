#pragma once

#include "wx/frame.h"
#include "PrimaryGLContext.h"

#include "ScopeCanvas.h"
#include "SpectrumCanvas.h"
#include "WaterfallCanvas.h"
#include "MeterCanvas.h"
#include "TuningCanvas.h"
#include "ModeSelectorCanvas.h"

#include <map>

#define wxID_RT_AUDIO_DEVICE 1000
#define wxID_SET_FREQ_OFFSET 2001
#define wxID_RESET 2002

#define wxID_THEME_DEFAULT 2100
#define wxID_THEME_SHARP 2101
#define wxID_THEME_BW 2102
#define wxID_THEME_RAD 2103
#define wxID_THEME_TOUCH 2104
#define wxID_THEME_HD 2105
#define wxID_THEME_RADAR 2106

// Define a new frame type
class AppFrame: public wxFrame {
public:
    AppFrame();
    ~AppFrame();
    void OnThread(wxCommandEvent& event);
    void OnEventInput(wxThreadEvent& event);

    void saveSession(std::string fileName);
    bool loadSession(std::string fileName);

private:
    void OnMenu(wxCommandEvent& event);
    void OnClose(wxCommandEvent& event);
    void OnNewWindow(wxCommandEvent& event);
    void OnIdle(wxIdleEvent& event);

    ScopeCanvas *scopeCanvas;
    SpectrumCanvas *spectrumCanvas;
    WaterfallCanvas *waterfallCanvas;
    ModeSelectorCanvas *demodModeSelector;
    SpectrumCanvas *demodSpectrumCanvas;
    WaterfallCanvas *demodWaterfallCanvas;
    MeterCanvas *demodSignalMeter;
    MeterCanvas *demodGainMeter;
    TuningCanvas *demodTuner;
// event table

    DemodulatorInstance *activeDemodulator;

    std::vector<RtAudio::DeviceInfo> devices;
    std::map<int,RtAudio::DeviceInfo> inputDevices;
    std::map<int,RtAudio::DeviceInfo> outputDevices;
    std::map<int,wxMenuItem *> outputDeviceMenuItems;

    std::string currentSessionFile;

    wxDECLARE_EVENT_TABLE();
};
