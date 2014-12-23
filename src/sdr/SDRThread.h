#pragma once

#include <atomic>

#include "wx/wxprec.h"
#include "rtl-sdr.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "ThreadQueue.h"
#include "DemodulatorMgr.h"

class SDRThreadCommand {
public:
    enum SDRThreadCommandEnum {
        SDR_THREAD_CMD_NULL, SDR_THREAD_CMD_TUNE
    };

    SDRThreadCommand() :
            cmd(SDR_THREAD_CMD_NULL), int_value(0) {

    }

    SDRThreadCommand(SDRThreadCommandEnum cmd) :
            cmd(cmd), int_value(0) {

    }

    SDRThreadCommandEnum cmd;
    int int_value;
};

class SDRThreadIQData {
public:
    unsigned int frequency;
    unsigned int bandwidth;
    std::vector<signed char> *data;

    SDRThreadIQData() :
            frequency(0), bandwidth(0), data(NULL) {

    }

    SDRThreadIQData(unsigned int bandwidth, unsigned int frequency, std::vector<signed char> *data) :
            data(data), frequency(frequency), bandwidth(bandwidth) {

    }

    ~SDRThreadIQData() {

    }
};

typedef ThreadQueue<SDRThreadCommand> SDRThreadCommandQueue;
typedef ThreadQueue<SDRThreadIQData> SDRThreadIQDataQueue;

class SDRThread {
public:
    rtlsdr_dev_t *dev;

    SDRThread(SDRThreadCommandQueue* pQueue);
    ~SDRThread();

    int enumerate_rtl();

    void threadMain();

    void setIQDataOutQueue(SDRThreadIQDataQueue* iqDataQueue) {
        iqDataOutQueue = iqDataQueue;
    }

    void terminate();
protected:
    uint32_t sample_rate;
    std::atomic<SDRThreadCommandQueue*> m_pQueue;
    std::atomic<SDRThreadIQDataQueue*> iqDataOutQueue;

    std::atomic<bool> terminated;
};
