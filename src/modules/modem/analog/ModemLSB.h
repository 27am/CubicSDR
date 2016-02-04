#pragma once
#include "ModemAnalog.h"

class ModemLSB : public ModemAnalog {
public:
    ModemLSB();
    ~ModemLSB();
    
    std::string getName();
    
    Modem *factory();
    
    int checkSampleRate(long long sampleRate, int audioSampleRate);
    int getDefaultSampleRate();

    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    iirfilt_crcf ssbFilt;
    firhilbf c2rFilt;
    nco_crcf ssbShift;
    //    firfilt_crcf ssbFilt;
    //    ampmodem demodAM_LSB;
};