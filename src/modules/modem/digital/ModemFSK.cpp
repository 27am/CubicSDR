#include "ModemFSK.h"
#include <iomanip>

ModemFSK::ModemFSK() : ModemDigital()  {
    // DMR defaults?
    bps = 1;
    sps = 9600;
    outStream << std::hex;
}

Modem *ModemFSK::factory() {
    return new ModemFSK;
}

int ModemFSK::checkSampleRate(long long sampleRate, int audioSampleRate) {
    float minSps = pow(2.0,bps);
    float nextSps = (float(sampleRate) / float(sps));
    if (nextSps < minSps) {
        return 2 * bps * sps;
    } else {
        return sampleRate;
    }
}

ModemArgInfoList ModemFSK::getSettings() {
    ModemArgInfoList args;
    
    ModemArgInfo bpsArg;
    bpsArg.key = "bps";
    bpsArg.name = "Bits/symbol";
    bpsArg.value = std::to_string(bps);
    bpsArg.description = "Modem bits-per-symbol";
    bpsArg.type = ModemArgInfo::STRING;
    
    std::vector<std::string> bpsOpts;
    bpsOpts.push_back("1");
    bpsOpts.push_back("2");
    bpsOpts.push_back("4");
    bpsOpts.push_back("8");
    bpsOpts.push_back("16");
    bpsArg.options = bpsOpts;
    
    args.push_back(bpsArg);
    
    ModemArgInfo spsArg;
    spsArg.key = "sps";
    spsArg.name = "Symbols/second";
    spsArg.value = std::to_string(sps);
    spsArg.description = "Modem symbols-per-second";
    spsArg.type = ModemArgInfo::STRING;
    
    std::vector<std::string> spsOpts;
    // some common modem rates ..?
    spsOpts.push_back("300");
    spsOpts.push_back("600");
    spsOpts.push_back("1200");
    spsOpts.push_back("2400");
    spsOpts.push_back("4800");
    spsOpts.push_back("9600");
    spsOpts.push_back("19200");
    spsOpts.push_back("38400");
    spsArg.options = spsOpts;
    
    args.push_back(spsArg);

    return args;
}

void ModemFSK::writeSetting(std::string setting, std::string value) {
    if (setting == "bps") {
        bps = std::stoi(value);
        rebuildKit();
    } else if (setting == "sps") {
        sps = std::stoi(value);
        rebuildKit();
    }
}

std::string ModemFSK::readSetting(std::string setting) {
    if (setting == "bps") {
        return std::to_string(bps);
    } else if (setting == "sps") {
        return std::to_string(sps);
    }
    return "";
}

ModemKit *ModemFSK::buildKit(long long sampleRate, int audioSampleRate) {
    ModemKitFSK *dkit = new ModemKitFSK;
    dkit->m           = bps;
    dkit->k           = sampleRate / sps;

    dkit->demodFSK = fskdem_create(dkit->m, dkit->k, 0.45);

    dkit->sampleRate = sampleRate;
    dkit->audioSampleRate = audioSampleRate;
 
    return dkit;
}

void ModemFSK::disposeKit(ModemKit *kit) {
    ModemKitFSK *dkit = (ModemKitFSK *)kit;
    
    fskdem_destroy(dkit->demodFSK);
    
    delete dkit;
}

std::string ModemFSK::getName() {
    return "FSK";
}

ModemFSK::~ModemFSK() {

}

void ModemFSK::demodulate(ModemKit *kit, ModemIQData *input, AudioThreadInput *audioOut) {
    ModemKitFSK *dkit = (ModemKitFSK *)kit;
    
    digitalStart(dkit, nullptr, input);

    dkit->inputBuffer.insert(dkit->inputBuffer.end(),input->data.begin(),input->data.end());

    while (dkit->inputBuffer.size() >= dkit->k) {
        outStream << fskdem_demodulate(dkit->demodFSK, &dkit->inputBuffer[0]);
        
//        float err = fskdem_get_frequency_error(dkit->demodFSK);
        dkit->inputBuffer.erase(dkit->inputBuffer.begin(),dkit->inputBuffer.begin()+dkit->k);
    }
    
    digitalFinish(dkit, nullptr);
}