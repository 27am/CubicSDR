#pragma once

#include "wx/frame.h"
#include "PrimaryGLContext.h"

#include "ScopeCanvas.h"
#include "SpectrumCanvas.h"
#include "WaterfallCanvas.h"
#include "MeterCanvas.h"
#include "TuningCanvas.h"
#include "ModeSelectorCanvas.h"
#include "FFTVisualDataThread.h"
#include "SDRDeviceInfo.h"
//#include "UITestCanvas.h"

#include <map>

#define wxID_RT_AUDIO_DEVICE 1000
#define wxID_SET_FREQ_OFFSET 2001
#define wxID_RESET 2002
#define wxID_SET_PPM 2003
#define wxID_SET_DS_OFF 2004
#define wxID_SET_DS_I 2005
#define wxID_SET_DS_Q 2006
#define wxID_SET_SWAP_IQ 2007
#define wxID_SDR_DEVICES 2008

#define wxID_THEME_DEFAULT 2100
#define wxID_THEME_SHARP 2101
#define wxID_THEME_BW 2102
#define wxID_THEME_RAD 2103
#define wxID_THEME_TOUCH 2104
#define wxID_THEME_HD 2105
#define wxID_THEME_RADAR 2106

#define wxID_BANDWIDTH_BASE 2150
#define wxID_BANDWIDTH_MANUAL 2200

#define wxID_DEVICE_ID 3500

#define wxID_AUDIO_BANDWIDTH_BASE 9000
#define wxID_AUDIO_DEVICE_MULTIPLIER 50


// Define a new frame type
class AppFrame: public wxFrame {
public:
    AppFrame();
    ~AppFrame();
    void OnThread(wxCommandEvent& event);
    void OnEventInput(wxThreadEvent& event);
    void initDeviceParams(SDRDeviceInfo *devInfo);

    void saveSession(std::string fileName);
    bool loadSession(std::string fileName);

private:
    void OnMenu(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
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
//    UITestCanvas *testCanvas;
    MeterCanvas *spectrumAvgMeter;
    MeterCanvas *waterfallSpeedMeter;
    ModeSelectorCanvas *demodMuteButton;

    DemodulatorInstance *activeDemodulator;

    std::vector<RtAudio::DeviceInfo> devices;
    std::map<int,RtAudio::DeviceInfo> inputDevices;
    std::map<int,RtAudio::DeviceInfo> outputDevices;
    std::map<int, wxMenuItem *> outputDeviceMenuItems;
    std::map<int, wxMenuItem *> sampleRateMenuItems;
    std::map<int, wxMenuItem *> audioSampleRateMenuItems;
    std::map<int, wxMenuItem *> directSamplingMenuItems;
    wxMenuItem *iqSwapMenuItem;
    wxMenu *sampleRateMenu;
    std::vector<long> sampleRates;
    
    std::string currentSessionFile;
    
    FFTVisualDataThread *waterfallDataThread;
    
    std::thread *t_FFTData;

    wxDECLARE_EVENT_TABLE();
};
