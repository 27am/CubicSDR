#pragma once
#include "ModemDigital.h"

class ModemPSK : public ModemDigital {
public:
    ModemPSK();
    ~ModemPSK();
    Modem *factory();
    void demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut);
    
private:
    modem demodPSK;
    modem demodPSK2;
    modem demodPSK4;
    modem demodPSK8;
    modem demodPSK16;
    modem demodPSK32;
    modem demodPSK64;
    modem demodPSK128;
    modem demodPSK256;
};

