#pragma once

#include <string>
#include <vector>

#include <SoapySDR/Types.hpp>

typedef struct _SDRManualDef {
    std::string factory;
    std::string params;
} SDRManualDef;

class SDRDeviceRange {
public:
    SDRDeviceRange();
    SDRDeviceRange(double low, double high);
    SDRDeviceRange(std::string name, double low, double high);
    
    double getLow();
    void setLow(double low);
    double getHigh();
    void setHigh(double high);
    std::string getName();
    void setName(std::string name);
    
private:
    std::string name;
    double low, high;
};

class SDRDeviceChannel {
public:
    SDRDeviceChannel();
    ~SDRDeviceChannel();
    
    int getChannel();
    void setChannel(int channel);
    
    bool isFullDuplex();
    void setFullDuplex(bool fullDuplex);
    
    bool isTx();
    void setTx(bool tx);
    
    bool isRx();
    void setRx(bool rx);
    
    void addGain(SDRDeviceRange range);
    void addGain(std::string name, SoapySDR::Range range);
    std::vector<SDRDeviceRange> &getGains();
    
    SDRDeviceRange &getGain();
    SDRDeviceRange &getLNAGain();
    SDRDeviceRange &getFreqRange();
    SDRDeviceRange &getRFRange();

    std::vector<long> &getSampleRates();
    long getSampleRateNear(long sampleRate_in);
    std::vector<long long> &getFilterBandwidths();
    
    const bool& hasHardwareDC() const;
    void setHardwareDC(const bool& hardware);

    const bool& hasCORR() const;
    void setCORR(const bool& corr);
    
    void setStreamArgsInfo(SoapySDR::ArgInfoList streamArgs);
    SoapySDR::ArgInfoList getStreamArgsInfo();
    std::vector<std::string> getStreamArgNames();

private:
    int channel;
    bool fullDuplex, tx, rx, hardwareDC, hasCorr;
    SDRDeviceRange rangeGain, rangeLNA, rangeFull, rangeRF;
    std::vector<long> sampleRates;
    std::vector<long long> filterBandwidths;
    SoapySDR::ArgInfoList streamArgInfo;
    std::vector<SDRDeviceRange> gainInfo;
};


class SDRDeviceInfo {
public:
    SDRDeviceInfo();
    
    std::string getDeviceId();
    
    const int getIndex() const;
    void setIndex(const int index);
    
    bool isAvailable() const;
    void setAvailable(bool available);
    
    const std::string& getName() const;
    void setName(const std::string& name);
    
    const std::string& getSerial() const;
    void setSerial(const std::string& serial);
    
    const std::string& getTuner() const;
    void setTuner(const std::string& tuner);
    
    const std::string& getManufacturer() const;
    void setManufacturer(const std::string& manufacturer);
    
    const std::string& getProduct() const;
    void setProduct(const std::string& product);

    const std::string& getDriver() const;
    void setDriver(const std::string& driver);
    
    const std::string& getHardware() const;
    void setHardware(const std::string& hardware);
    
    bool hasTimestamps() const;
    void setTimestamps(bool timestamps);

    bool isRemote() const;
    void setRemote(bool remote);

    bool isManual() const;
    void setManual(bool manual);
    
    void setManualParams(std::string manualParams);
    std::string getManualParams();

    void addChannel(SDRDeviceChannel *chan);
    std::vector<SDRDeviceChannel *> &getChannels();
    SDRDeviceChannel * getRxChannel();
    SDRDeviceChannel * getTxChannel();
    
    void setDeviceArgs(SoapySDR::Kwargs deviceArgs);
    SoapySDR::Kwargs getDeviceArgs();

    void setStreamArgs(SoapySDR::Kwargs deviceArgs);
    SoapySDR::Kwargs getStreamArgs();

    void setSettingsInfo(SoapySDR::ArgInfoList settingsArgs);
    SoapySDR::ArgInfoList getSettingsArgInfo();

    std::vector<std::string> getSettingNames();
    
private:
    int index;
    std::string name, serial, product, manufacturer, tuner;
    std::string driver, hardware, manual_params;
    bool timestamps, available, remote, manual;
    
    SoapySDR::Kwargs deviceArgs, streamArgs;
    SoapySDR::ArgInfoList settingInfo;
    std::vector<SDRDeviceChannel *> channels;
};
