#include "DemodulatorWorkerThread.h"
#include "CubicSDRDefs.h"
#include <vector>

DemodulatorWorkerThread::DemodulatorWorkerThread(DemodulatorThreadWorkerCommandQueue* in, DemodulatorThreadWorkerResultQueue* out) :
        terminated(false), commandQueue(in), resultQueue(out) {

}

DemodulatorWorkerThread::~DemodulatorWorkerThread() {
}

void DemodulatorWorkerThread::threadMain() {

    std::cout << "Demodulator worker thread started.." << std::endl;

    while (!terminated) {
        bool filterChanged = false;
        DemodulatorWorkerThreadCommand filterCommand;
        DemodulatorWorkerThreadCommand command;

        bool done = false;
        while (!done) {
            commandQueue->pop(command);
            switch (command.cmd) {
            case DemodulatorWorkerThreadCommand::DEMOD_WORKER_THREAD_CMD_BUILD_FILTERS:
                filterChanged = true;
                filterCommand = command;
                break;
            default:
                break;
            }
            done = commandQueue->empty();
        }
        

        if (filterChanged && !terminated) {
            DemodulatorWorkerThreadResult result(DemodulatorWorkerThreadResult::DEMOD_WORKER_THREAD_RESULT_FILTERS);

            float As = 60.0f;         // stop-band attenuation [dB]

            if (filterCommand.sampleRate && filterCommand.bandwidth) {
                result.iqResampleRatio = (double) (filterCommand.bandwidth) / (double) filterCommand.sampleRate;
                result.iqResampler = msresamp_crcf_create(result.iqResampleRatio, As);
            }

            if (filterCommand.bandwidth && filterCommand.audioSampleRate) {
                result.audioResamplerRatio = (double) (filterCommand.audioSampleRate) / (double) filterCommand.bandwidth;
                result.audioResampler = msresamp_rrrf_create(result.audioResamplerRatio, As);
                result.stereoResampler = msresamp_rrrf_create(result.audioResamplerRatio, As);
                result.audioSampleRate = filterCommand.audioSampleRate;

                // Stereo filters / shifters
                double firStereoCutoff = 0.5 * ((double) 36000 / (double) filterCommand.audioSampleRate);         // filter cutoff frequency
                float ft = 0.05f;         // filter transition
                float mu = 0.0f;         // fractional timing offset

                if (firStereoCutoff < 0) {
                    firStereoCutoff = 0;
                }

                if (firStereoCutoff > 0.5) {
                    firStereoCutoff = 0.5;
                }

                unsigned int h_len = estimate_req_filter_len(ft, As);
                float *h = new float[h_len];
                liquid_firdes_kaiser(h_len, firStereoCutoff, As, mu, h);

                result.firStereoLeft = firfilt_rrrf_create(h, h_len);
                result.firStereoRight = firfilt_rrrf_create(h, h_len);
            }

            if (filterCommand.bandwidth) {
                result.bandwidth = filterCommand.bandwidth;
            }

            if (filterCommand.sampleRate) {
                result.sampleRate = filterCommand.sampleRate;
            }

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
