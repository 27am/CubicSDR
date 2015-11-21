#pragma once

#include <vector>
#include <map>
#include <thread>

#include "DemodulatorInstance.h"

class DemodulatorMgr {
public:
    DemodulatorMgr();
    ~DemodulatorMgr();

    DemodulatorInstance *newThread();
    std::vector<DemodulatorInstance *> &getDemodulators();
    std::vector<DemodulatorInstance *> *getDemodulatorsAt(long long freq, int bandwidth);
    void deleteThread(DemodulatorInstance *);

    void terminateAll();

    void setActiveDemodulator(DemodulatorInstance *demod, bool temporary = true);
    DemodulatorInstance *getActiveDemodulator();
    DemodulatorInstance *getLastActiveDemodulator();

    int getLastBandwidth() const;
    void setLastBandwidth(int lastBandwidth);

    std::string getLastDemodulatorType() const;
    void setLastDemodulatorType(std::string lastDemodType);
    
    int getLastDemodulatorCons() const;
    void setLastDemodulatorCons(int lastDemodCons);

    float getLastGain() const;
    void setLastGain(float lastGain);

    float getLastSquelchLevel() const;
    void setLastSquelchLevel(float lastSquelch);

    bool isLastSquelchEnabled() const;
    void setLastSquelchEnabled(bool lastSquelchEnabled);
    
    bool isLastMuted() const;
    void setLastMuted(bool lastMuted);

private:
    void garbageCollect();
    void updateLastState();

    std::vector<DemodulatorInstance *> demods;
    std::vector<DemodulatorInstance *> demods_deleted;
    DemodulatorInstance *activeDemodulator;
    DemodulatorInstance *lastActiveDemodulator;
    DemodulatorInstance *activeVisualDemodulator;

    int lastBandwidth;
    std::string lastDemodType;
    bool lastDemodLock;
    int lastDemodCons;
    bool lastSquelchEnabled;
    float lastSquelch;
    float lastGain;
    bool lastMuted;
};
