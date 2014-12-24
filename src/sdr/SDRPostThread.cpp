#include "SDRPostThread.h"
#include "CubicSDRDefs.h"
#include "CubicSDR.h"

#include <vector>
#include <deque>

SDRPostThread::SDRPostThread() :
        sample_rate(SRATE), iqDataOutQueue(NULL), iqDataInQueue(NULL), iqVisualQueue(NULL), terminated(false), dcFilter(NULL) {
}

SDRPostThread::~SDRPostThread() {
}

void SDRPostThread::bindDemodulator(DemodulatorInstance *demod) {
    demodulators_add.push_back(demod);
}

void SDRPostThread::removeDemodulator(DemodulatorInstance *demod) {
    if (!demod) {
        return;
    }

    demodulators_remove.push_back(demod);
}

void SDRPostThread::setIQDataInQueue(SDRThreadIQDataQueue* iqDataQueue) {
    iqDataInQueue = iqDataQueue;
}
void SDRPostThread::setIQDataOutQueue(SDRThreadIQDataQueue* iqDataQueue) {
    iqDataOutQueue = iqDataQueue;
}
void SDRPostThread::setIQVisualQueue(SDRThreadIQDataQueue *iqVisQueue) {
    iqVisualQueue = iqVisQueue;
}

void SDRPostThread::threadMain() {
    int n_read;
    double seconds = 0.0;

#ifdef __APPLE__
    pthread_t tID = pthread_self();  // ID of this thread
    int priority = sched_get_priority_max( SCHED_FIFO) - 1;
    sched_param prio = {priority}; // scheduling priority of thread
    pthread_setschedparam(tID, SCHED_FIFO, &prio);
#endif

    dcFilter = iirfilt_crcf_create_dc_blocker(0.0005);

    liquid_float_complex x, y;

    std::cout << "SDR post-processing thread started.." << std::endl;

    std::deque<DemodulatorThreadIQData *> buffers;
    std::deque<DemodulatorThreadIQData *>::iterator buffers_i;

    while (!terminated) {
        SDRThreadIQData *data_in;

        iqDataInQueue.load()->pop(data_in);
//        std::lock_guard < std::mutex > lock(data_in->m_mutex);

        if (data_in && data_in->data.size()) {
            SDRThreadIQData *dataOut = new SDRThreadIQData;

            dataOut->frequency = data_in->frequency;
            dataOut->bandwidth = data_in->bandwidth;
            dataOut->data.assign(data_in->data.begin(), data_in->data.end());

            for (int i = 0, iMax = dataOut->data.size() / 2; i < iMax; i++) {
                x.real = (float) dataOut->data[i * 2] / 127.0;
                x.imag = (float) dataOut->data[i * 2 + 1] / 127.0;

                iirfilt_crcf_execute(dcFilter, x, &y);

                dataOut->data[i * 2] = (signed char) floor(y.real * 127.0);
                dataOut->data[i * 2 + 1] = (signed char) floor(y.imag * 127.0);
            }

            if (iqDataOutQueue != NULL) {
                iqDataOutQueue.load()->push(dataOut);
            }

            if (iqVisualQueue != NULL && iqVisualQueue.load()->empty()) {
                SDRThreadIQData *visualDataOut = new SDRThreadIQData;
                visualDataOut->data.assign(dataOut->data.begin(), dataOut->data.begin() + (FFT_SIZE * 2));
                iqVisualQueue.load()->push(visualDataOut);
            }

            if (demodulators_add.size()) {
                while (!demodulators_add.empty()) {
                    demodulators.push_back(demodulators_add.back());
                    demodulators_add.pop_back();
                }
            }
            if (demodulators_remove.size()) {
                while (!demodulators_remove.empty()) {
                    DemodulatorInstance *demod = demodulators_remove.back();
                    demodulators_remove.pop_back();

                    std::vector<DemodulatorInstance *>::iterator i = std::find(demodulators.begin(), demodulators.end(), demod);

                    if (i != demodulators.end()) {
                        demodulators.erase(i);
                    }
                }
            }

            int activeDemods = 0;
            bool pushedData = false;

            if (demodulators.size()) {

                std::vector<DemodulatorInstance *>::iterator i;
                for (i = demodulators.begin(); i != demodulators.end(); i++) {
                    DemodulatorInstance *demod = *i;

                    if (demod->getParams().frequency != data_in->frequency
                            && abs(data_in->frequency - demod->getParams().frequency) > (int) ((float) ((float) SRATE / 2.0))) {
                        continue;
                    }
                    activeDemods++;
                }

                if (demodulators.size()) {

                    DemodulatorThreadIQData *demodDataOut = NULL;

                    for (buffers_i = buffers.begin(); buffers_i != buffers.end(); buffers_i++) {
                        if ((*buffers_i)->getRefCount() <= 0) {
                            demodDataOut = (*buffers_i);
                            break;
                        }
                    }

                    if (demodDataOut == NULL) {
                        demodDataOut = new DemodulatorThreadIQData;
                        buffers.push_back(demodDataOut);
                    }

//                    std::lock_guard < std::mutex > lock(demodDataOut->m_mutex);
                    demodDataOut->frequency = data_in->frequency;
                    demodDataOut->bandwidth = data_in->bandwidth;
                    demodDataOut->setRefCount(activeDemods);
                    demodDataOut->data.assign(dataOut->data.begin(), dataOut->data.begin() + dataOut->data.size());

                    std::vector<DemodulatorInstance *>::iterator i;
                    for (i = demodulators.begin(); i != demodulators.end(); i++) {
                        DemodulatorInstance *demod = *i;
                        DemodulatorThreadInputQueue *demodQueue = demod->threadQueueDemod;

                        if (demod->getParams().frequency != data_in->frequency
                                && abs(data_in->frequency - demod->getParams().frequency) > (int) ((float) ((float) SRATE / 2.0))) {
                            if (demod->isActive()) {
                                demod->setActive(false);
                                DemodulatorThreadIQData *dummyDataOut = new DemodulatorThreadIQData;
                                dummyDataOut->frequency = data_in->frequency;
                                dummyDataOut->bandwidth = data_in->bandwidth;
                                demodQueue->push(dummyDataOut);
                            }
                        } else if (!demod->isActive()) {
                            demod->setActive(true);
                        }

                        if (!demod->isActive()) {
                            continue;
                        }

                        demodQueue->push(demodDataOut);
                        pushedData = true;
                    }

                    if (!pushedData) {
                        demodDataOut->setRefCount(0);
                    }
                }
            }
            delete dataOut;
        }
        data_in->decRefCount();
    }

    while (!buffers.empty()) {
        DemodulatorThreadIQData *demodDataDel = buffers.front();
        buffers.pop_front();
//        std::lock_guard < std::mutex > lock(demodDataDel->m_mutex);
//        delete demodDataDel;
    }

    std::cout << "SDR post-processing thread done." << std::endl;
}

void SDRPostThread::terminate() {
    terminated = true;
    SDRThreadIQData *dummy = new SDRThreadIQData;
    iqDataInQueue.load()->push(dummy);
}
