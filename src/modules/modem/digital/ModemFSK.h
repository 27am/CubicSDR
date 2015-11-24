#pragma once
#include "ModemDigital.h"

class ModemKitFSK : public ModemKitDigital {
public:
    unsigned int m, k;
    
    fskdem demodFSK;
    std::vector<liquid_float_complex> inputBuffer;
};


class ModemFSK : public ModemDigital {
public:
    ModemFSK();
    ~ModemFSK();

    std::string getName();
    
    Modem *factory();
    
    int checkSampleRate(long long sampleRate, int audioSampleRate);

    ModemArgInfoList getSettings();
    void writeSetting(std::string setting, std::string value);
    std::string readSetting(std::string setting);
    
    ModemKit *buildKit(long long sampleRate, int audioSampleRate);
    void disposeKit(ModemKit *kit);
    
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);

private:
    int sps;
    int bps;
};

