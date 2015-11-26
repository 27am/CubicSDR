#pragma once

#include <vector>
#include <map>
#include <thread>

#include "DemodulatorThread.h"
#include "DemodulatorPreThread.h"

#include "ModemDigital.h"
#include "ModemAnalog.h"

class DemodulatorInstance {
public:

#ifdef __APPLE__
    pthread_t t_PreDemod;
    pthread_t t_Demod;
#else
    std::thread *t_PreDemod;
    std::thread *t_Demod;
#endif

    AudioThread *audioThread;
    std::thread *t_Audio;

    DemodulatorInstance();
    ~DemodulatorInstance();

    void setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue);

    DemodulatorThreadCommandQueue *getCommandQueue();

    void run();
    void terminate();
    std::string getLabel();
    void setLabel(std::string labelStr);

    bool isTerminated();
    void updateLabel(long long freq);

    bool isActive();
    void setActive(bool state);

    void squelchAuto();
    bool isSquelchEnabled();
    void setSquelchEnabled(bool state);

    float getSignalLevel();
    void setSquelchLevel(float signal_level_in);
    float getSquelchLevel();

    void setOutputDevice(int device_id);
    int getOutputDevice();

    void setDemodulatorType(std::string demod_type_in);
    std::string getDemodulatorType();
    
    void setDemodulatorLock(bool demod_lock_in);
    int getDemodulatorLock();

    void setBandwidth(int bw);
    int getBandwidth();

    void setGain(float gain_in);
    float getGain();

    void setFrequency(long long freq);
    long long getFrequency();

    void setAudioSampleRate(int sampleRate);
    int getAudioSampleRate();
    
    bool isFollow();
    void setFollow(bool follow);

    bool isTracking();
    void setTracking(bool tracking);
    
    bool isMuted();
    void setMuted(bool muted);

    DemodulatorThreadInputQueue *getIQInputDataPipe();

    ModemArgInfoList getModemArgs();
    std::string readModemSetting(std::string setting);
    ModemSettings readModemSettings();
    void writeModemSetting(std::string setting, std::string value);
    void writeModemSettings(ModemSettings settings);
    
    bool isModemInitialized();

protected:
    DemodulatorThreadInputQueue* pipeIQInputData;
    DemodulatorThreadPostInputQueue* pipeIQDemodData;
    AudioThreadInputQueue *pipeAudioData;
    DemodulatorThreadCommandQueue* pipeDemodNotify;
    DemodulatorPreThread *demodulatorPreThread;
    DemodulatorThread *demodulatorThread;
    DemodulatorThreadControlCommandQueue *threadQueueControl;

private:

    std::atomic<std::string *> label; //
    std::atomic_bool terminated; //
    std::atomic_bool demodTerminated; //
    std::atomic_bool audioTerminated; //
    std::atomic_bool preDemodTerminated;
    std::atomic_bool active;
    std::atomic_bool squelch;
    std::atomic_bool muted;

    std::atomic_int currentOutputDevice;
    std::atomic<float> currentAudioGain;
    std::atomic_bool follow, tracking;
  };
