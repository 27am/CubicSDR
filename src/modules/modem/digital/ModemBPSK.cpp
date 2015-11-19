#include "ModemBPSK.h"

ModemBPSK::ModemBPSK() {
    demodBPSK = modem_create(LIQUID_MODEM_BPSK);
    
}

Modem *ModemBPSK::factory() {
    return new ModemBPSK;
}

ModemBPSK::~ModemBPSK() {
    modem_destroy(demodBPSK);
}

void ModemBPSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    for (int i = 0, bufSize=input->data.size(); i < bufSize; i++) {
        modem_demodulate(demodBPSK, input->data[i], &demodOutputDataDigital[i]);
    }
    updateDemodulatorLock(demodBPSK, 0.005f);
}