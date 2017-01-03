// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "FFTVisualDataThread.h"
#include "CubicSDR.h"

FFTVisualDataThread::FFTVisualDataThread() {
	linesPerSecond.store(DEFAULT_WATERFALL_LPS);
    lpsChanged.store(true);
}

FFTVisualDataThread::~FFTVisualDataThread() {
    
}

void FFTVisualDataThread::setLinesPerSecond(int lps) {
    linesPerSecond.store(lps);
    lpsChanged.store(true);
}

int FFTVisualDataThread::getLinesPerSecond() {
    return linesPerSecond.load();
}

SpectrumVisualProcessor *FFTVisualDataThread::getProcessor() {
    return &wproc;
}

void FFTVisualDataThread::run() {
    DemodulatorThreadInputQueue *pipeIQDataIn = static_cast<DemodulatorThreadInputQueue *>(getInputQueue("IQDataInput"));
    SpectrumVisualDataQueue *pipeFFTDataOut = static_cast<SpectrumVisualDataQueue *>(getOutputQueue("FFTDataOutput"));
    
    fftQueue.set_max_num_items(100);
    pipeFFTDataOut->set_max_num_items(100);
    fftDistrib.setInput(pipeIQDataIn);
    fftDistrib.attachOutput(&fftQueue);
    wproc.setInput(&fftQueue);
    wproc.attachOutput(pipeFFTDataOut);
    wproc.setup(DEFAULT_FFT_SIZE);

//    std::cout << "FFT visual data thread started." << std::endl;
    
    while(!stopping) {
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
//        std::this_thread::yield();
        
        int fftSize = wproc.getDesiredInputSize();
        
        if (fftSize) {
            fftDistrib.setFFTSize(fftSize);
        } else {
            fftDistrib.setFFTSize(DEFAULT_FFT_SIZE * SPECTRUM_VZM);
        }
    
        if (lpsChanged.load()) {
            fftDistrib.setLinesPerSecond(linesPerSecond.load());
//            pipeIQDataIn->set_max_num_items(linesPerSecond.load());
            lpsChanged.store(false);
        }
        
        fftDistrib.run();
        
        while (!wproc.isInputEmpty()) {
            wproc.run();
        }
    }
    
//    std::cout << "FFT visual data thread done." << std::endl;
}

