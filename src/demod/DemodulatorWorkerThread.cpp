#include "DemodulatorWorkerThread.h"
#include "CubicSDRDefs.h"
#include <vector>

DemodulatorWorkerThread::DemodulatorWorkerThread() : IOThread(),
        commandQueue(NULL), resultQueue(NULL), cModem(nullptr), cModemKit(nullptr) {
}

DemodulatorWorkerThread::~DemodulatorWorkerThread() {
}

void DemodulatorWorkerThread::run() {

    std::cout << "Demodulator worker thread started.." << std::endl;
    
    commandQueue = (DemodulatorThreadWorkerCommandQueue *)getInputQueue("WorkerCommandQueue");
    resultQueue = (DemodulatorThreadWorkerResultQueue *)getOutputQueue("WorkerResultQueue");
    
    while (!terminated) {
        bool filterChanged = false;
        bool makeDemod = false;
        DemodulatorWorkerThreadCommand filterCommand, demodCommand;
        DemodulatorWorkerThreadCommand command;

        bool done = false;
        while (!done) {
            commandQueue->pop(command);
            switch (command.cmd) {
            case DemodulatorWorkerThreadCommand::DEMOD_WORKER_THREAD_CMD_BUILD_FILTERS:
                filterChanged = true;
                filterCommand = command;
                break;
            case DemodulatorWorkerThreadCommand::DEMOD_WORKER_THREAD_CMD_MAKE_DEMOD:
                makeDemod = true;
                demodCommand = command;
                break;
            default:
                break;
            }
            done = commandQueue->empty();
        }
        

        if ((makeDemod || filterChanged) && !terminated) {
            DemodulatorWorkerThreadResult result(DemodulatorWorkerThreadResult::DEMOD_WORKER_THREAD_RESULT_FILTERS);

            float As = 60.0f;         // stop-band attenuation [dB]

            if (filterCommand.sampleRate && filterCommand.bandwidth) {
                result.iqResampleRatio = (double) (filterCommand.bandwidth) / (double) filterCommand.sampleRate;
                result.iqResampler = msresamp_crcf_create(result.iqResampleRatio, As);
            }

            if (makeDemod) {
                cModem = Modem::makeModem(demodCommand.demodType);
                cModemType = demodCommand.demodType;
            }
            result.modem = cModem;

            if (makeDemod && demodCommand.bandwidth && demodCommand.audioSampleRate) {
                if (cModem != nullptr) {
                    cModemKit = cModem->buildKit(demodCommand.bandwidth, demodCommand.audioSampleRate);
                } else {
                    cModemKit = nullptr;
                }
            } else if (makeDemod) {
                cModemKit = nullptr;
            }
            result.modemKit = cModemKit;

            if (filterCommand.bandwidth) {
                result.bandwidth = filterCommand.bandwidth;
            }

            if (filterCommand.sampleRate) {
                result.sampleRate = filterCommand.sampleRate;
            }

            result.modemType = cModemType;
            
            resultQueue->push(result);
        }

    }

    std::cout << "Demodulator worker thread done." << std::endl;
}

void DemodulatorWorkerThread::terminate() {
    terminated = true;
    DemodulatorWorkerThreadCommand inp;    // push dummy to nudge queue
    commandQueue->push(inp);
}
