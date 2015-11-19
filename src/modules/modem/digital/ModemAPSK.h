#pragma once
#include "ModemDigital.h"

class ModemAPSK : public ModemDigital {
public:
    ModemAPSK();
    ~ModemAPSK();
    Modem *factory();
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodAPSK;
    modem demodAPSK4;
    modem demodAPSK8;
    modem demodAPSK16;
    modem demodAPSK32;
    modem demodAPSK64;
    modem demodAPSK128;
    modem demodAPSK256;
};
