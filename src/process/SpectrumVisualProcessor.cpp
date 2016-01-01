#include "SpectrumVisualProcessor.h"
#include "CubicSDR.h"


SpectrumVisualProcessor::SpectrumVisualProcessor() : lastInputBandwidth(0), lastBandwidth(0), fftwInput(NULL), fftwOutput(NULL), fftInData(NULL), fftLastData(NULL), lastDataSize(0), fftw_plan(NULL), resampler(NULL), resamplerRatio(0), outputBuffers("SpectrumVisualProcessorBuffers") {
    
    is_view.store(false);
    fftSize.store(0);
    centerFreq.store(0);
    bandwidth.store(0);
    hideDC.store(false);
    
    freqShifter = nco_crcf_create(LIQUID_NCO);
    shiftFrequency = 0;
    
    fft_ceil_ma = fft_ceil_maa = 100.0;
    fft_floor_ma = fft_floor_maa = 0.0;
    desiredInputSize.store(0);
    fft_average_rate = 0.65;
    scaleFactor.store(1.0);
    fftSizeChanged.store(false);
    newFFTSize.store(0);
    lastView = false;
    peakHold.store(false);
}

SpectrumVisualProcessor::~SpectrumVisualProcessor() {
    nco_crcf_destroy(freqShifter);
}

bool SpectrumVisualProcessor::isView() {
    return is_view.load();
}

void SpectrumVisualProcessor::setView(bool bView) {
    busy_run.lock();
    is_view.store(bView);
    busy_run.unlock();
}

void SpectrumVisualProcessor::setView(bool bView, long long centerFreq_in, long bandwidth_in) {
    busy_run.lock();
    is_view.store(bView);
    bandwidth.store(bandwidth_in);
    centerFreq.store(centerFreq_in);
    busy_run.unlock();
}


void SpectrumVisualProcessor::setFFTAverageRate(float fftAverageRate) {
    busy_run.lock();
    this->fft_average_rate.store(fftAverageRate);
    busy_run.unlock();
}

float SpectrumVisualProcessor::getFFTAverageRate() {
    return this->fft_average_rate.load();
}

void SpectrumVisualProcessor::setCenterFrequency(long long centerFreq_in) {
    busy_run.lock();
    centerFreq.store(centerFreq_in);
    busy_run.unlock();
}

long long SpectrumVisualProcessor::getCenterFrequency() {
    return centerFreq.load();
}

void SpectrumVisualProcessor::setBandwidth(long bandwidth_in) {
    busy_run.lock();
    bandwidth.store(bandwidth_in);
    busy_run.unlock();
}

long SpectrumVisualProcessor::getBandwidth() {
    return bandwidth.load();
}

void SpectrumVisualProcessor::setPeakHold(bool peakHold_in) {
    fft_ceil_peak = fft_floor_maa;
    fft_floor_peak = fft_ceil_maa;
    
    for (int i = 0, iMax = fft_result_peak.size(); i < iMax; i++) {
        fft_result_peak[i] = fft_floor_maa;
    }
    peakHold.store(peakHold_in);
}

bool SpectrumVisualProcessor::getPeakHold() {
    return peakHold.load();
}

int SpectrumVisualProcessor::getDesiredInputSize() {
    return desiredInputSize.load();
}

void SpectrumVisualProcessor::setup(int fftSize_in) {
    busy_run.lock();

    fftSize = fftSize_in;
    fftSizeInternal = fftSize_in * SPECTRUM_VZM;
    lastDataSize = 0;
    
    int memSize = sizeof(fftwf_complex) * fftSizeInternal;
    
    if (fftwInput) {
        free(fftwInput);
    }
    //fftwInput = (fftwf_complex*) fftwf_malloc(memSize);
	fftwInput = (fftwf_complex*)malloc(memSize);
    memset(fftwInput,0,memSize);

    if (fftInData) {
        free(fftInData);
    }
    //fftInData = (fftwf_complex*) fftwf_malloc(memSize);
	fftInData = (fftwf_complex*)malloc(memSize);
    memset(fftwInput,0,memSize);
    
    if (fftLastData) {
        free(fftLastData);
    }
    //fftLastData = (fftwf_complex*) fftwf_malloc(memSize);
	fftLastData = (fftwf_complex*)malloc(memSize);
    memset(fftwInput,0,memSize);
    
    if (fftwOutput) {
        free(fftwOutput);
    }
    //fftwOutput = (fftwf_complex*) fftwf_malloc(memSize);
	fftwOutput = (fftwf_complex*)malloc(memSize);
	memset(fftwInput,0,memSize);
    
    if (fftw_plan) {
        fftwf_destroy_plan(fftw_plan);
    }
    fftw_plan = fftwf_plan_dft_1d(fftSizeInternal, fftwInput, fftwOutput, FFTW_FORWARD, FFTW_ESTIMATE);
    busy_run.unlock();
}

void SpectrumVisualProcessor::setFFTSize(int fftSize_in) {
    if (fftSize_in == fftSize) {
        return;
    }
    newFFTSize = fftSize_in;
    fftSizeChanged.store(true);
}

void SpectrumVisualProcessor::setHideDC(bool hideDC) {
    this->hideDC.store(hideDC);
}


void SpectrumVisualProcessor::process() {
    if (!isOutputEmpty()) {
        return;
    }
    if (!input || input->empty()) {
        return;
    }
    
    if (fftSizeChanged.load()) {
        setup(newFFTSize);
        fftSizeChanged.store(false);
    }

    DemodulatorThreadIQData *iqData;
    
    input->pop(iqData);
    
    if (!iqData) {
        return;
    }
    
    iqData->busy_rw.lock();
    busy_run.lock();
    bool doPeak = peakHold.load();
    
    std::vector<liquid_float_complex> *data = &iqData->data;
    
    if (data && data->size()) {
        unsigned int num_written;
        long resampleBw = iqData->sampleRate;
        bool newResampler = false;
        int bwDiff;
        
        if (is_view.load()) {
            if (!iqData->frequency || !iqData->sampleRate) {
                iqData->decRefCount();
                iqData->busy_rw.unlock();
                busy_run.unlock();
                return;
            }
            
            while (resampleBw / SPECTRUM_VZM >= bandwidth) {
                resampleBw /= SPECTRUM_VZM;
            }
            
            resamplerRatio = (double) (resampleBw) / (double) iqData->sampleRate;
            
            int desired_input_size = fftSizeInternal / resamplerRatio;
            
            this->desiredInputSize.store(desired_input_size);
            
            if (iqData->data.size() < desired_input_size) {
                //                std::cout << "fft underflow, desired: " << desired_input_size << " actual:" << input->data.size() << std::endl;
                desired_input_size = iqData->data.size();
            }
            
            if (centerFreq != iqData->frequency) {
                if ((centerFreq - iqData->frequency) != shiftFrequency || lastInputBandwidth != iqData->sampleRate) {
                    if (abs(iqData->frequency - centerFreq) < (wxGetApp().getSampleRate() / 2)) {
                        long lastShiftFrequency = shiftFrequency;
                        shiftFrequency = centerFreq - iqData->frequency;
                        nco_crcf_set_frequency(freqShifter, (2.0 * M_PI) * (((double) abs(shiftFrequency)) / ((double) iqData->sampleRate)));
                        
                        if (is_view.load()) {
                            long freqDiff = shiftFrequency - lastShiftFrequency;
                            
                            if (lastBandwidth!=0) {
                                double binPerHz = double(lastBandwidth) / double(fftSizeInternal);
                                
                                int numShift = floor(double(abs(freqDiff)) / binPerHz);
                                
                                if (numShift < fftSizeInternal/2 && numShift) {
                                    if (freqDiff > 0) {
                                        memmove(&fft_result_ma[0], &fft_result_ma[numShift], (fftSizeInternal-numShift) * sizeof(double));
                                        memmove(&fft_result_maa[0], &fft_result_maa[numShift], (fftSizeInternal-numShift) * sizeof(double));
                                        memmove(&fft_result_peak[0], &fft_result_peak[numShift], (fftSizeInternal-numShift) * sizeof(double));
                                        memset(&fft_result_peak[fftSizeInternal-numShift], 0, numShift * sizeof(double));
                                    } else {
                                        memmove(&fft_result_ma[numShift], &fft_result_ma[0], (fftSizeInternal-numShift) * sizeof(double));
                                        memmove(&fft_result_maa[numShift], &fft_result_maa[0], (fftSizeInternal-numShift) * sizeof(double));
                                        memmove(&fft_result_peak[numShift], &fft_result_peak[0], (fftSizeInternal-numShift) * sizeof(double));
                                        memset(&fft_result_peak[0], 0, numShift * sizeof(double));
                                    }
                                }
                            }
                        }
                    }
                }
                
                if (shiftBuffer.size() != desired_input_size) {
                    if (shiftBuffer.capacity() < desired_input_size) {
                        shiftBuffer.reserve(desired_input_size);
                    }
                    shiftBuffer.resize(desired_input_size);
                }
                
                if (shiftFrequency < 0) {
                    nco_crcf_mix_block_up(freqShifter, &iqData->data[0], &shiftBuffer[0], desired_input_size);
                } else {
                    nco_crcf_mix_block_down(freqShifter, &iqData->data[0], &shiftBuffer[0], desired_input_size);
                }
            } else {
                shiftBuffer.assign(iqData->data.begin(), iqData->data.begin()+desired_input_size);
            }
            
            if (!resampler || resampleBw != lastBandwidth || lastInputBandwidth != iqData->sampleRate) {
                float As = 60.0f;
                
                if (resampler) {
                    msresamp_crcf_destroy(resampler);
                }
                
                resampler = msresamp_crcf_create(resamplerRatio, As);
                
                bwDiff = resampleBw-lastBandwidth;
                lastBandwidth = resampleBw;
                lastInputBandwidth = iqData->sampleRate;
                newResampler = true;
            }
            
            
            int out_size = ceil((double) (desired_input_size) * resamplerRatio) + 512;
            
            if (resampleBuffer.size() != out_size) {
                if (resampleBuffer.capacity() < out_size) {
                    resampleBuffer.reserve(out_size);
                }
                resampleBuffer.resize(out_size);
            }
            
            msresamp_crcf_execute(resampler, &shiftBuffer[0], desired_input_size, &resampleBuffer[0], &num_written);
            
            if (num_written < fftSizeInternal) {
                for (int i = 0; i < num_written; i++) {
                    fftInData[i][0] = resampleBuffer[i].real;
                    fftInData[i][1] = resampleBuffer[i].imag;
                }
                for (int i = num_written; i < fftSizeInternal; i++) {
                    fftInData[i][0] = 0;
                    fftInData[i][1] = 0;
                }
            } else {
                for (int i = 0; i < fftSizeInternal; i++) {
                    fftInData[i][0] = resampleBuffer[i].real;
                    fftInData[i][1] = resampleBuffer[i].imag;
                }
            }
        } else {
            this->desiredInputSize.store(fftSizeInternal);

            num_written = data->size();
            if (data->size() < fftSizeInternal) {
                for (int i = 0, iMax = data->size(); i < iMax; i++) {
                    fftInData[i][0] = (*data)[i].real;
                    fftInData[i][1] = (*data)[i].imag;
                }
                for (int i = data->size(); i < fftSizeInternal; i++) {
                    fftInData[i][0] = 0;
                    fftInData[i][1] = 0;
                }
            } else {
                for (int i = 0; i < fftSizeInternal; i++) {
                    fftInData[i][0] = (*data)[i].real;
                    fftInData[i][1] = (*data)[i].imag;
                }
            }
        }
        
        bool execute = false;
        
        if (num_written >= fftSizeInternal) {
            execute = true;
            memcpy(fftwInput, fftInData, fftSizeInternal * sizeof(fftwf_complex));
            memcpy(fftLastData, fftwInput, fftSizeInternal * sizeof(fftwf_complex));
            
        } else {
            if (lastDataSize + num_written < fftSizeInternal) { // priming
                unsigned int num_copy = fftSizeInternal - lastDataSize;
                if (num_written > num_copy) {
                    num_copy = num_written;
                }
                memcpy(fftLastData, fftInData, num_copy * sizeof(fftwf_complex));
                lastDataSize += num_copy;
            } else {
                unsigned int num_last = (fftSizeInternal - num_written);
                memcpy(fftwInput, fftLastData + (lastDataSize - num_last), num_last * sizeof(fftwf_complex));
                memcpy(fftwInput + num_last, fftInData, num_written * sizeof(fftwf_complex));
                memcpy(fftLastData, fftwInput, fftSizeInternal * sizeof(fftwf_complex));
                execute = true;
            }
        }
        
        if (execute) {
            SpectrumVisualData *output = outputBuffers.getBuffer();
            
            if (output->spectrum_points.size() != fftSize * 2) {
                output->spectrum_points.resize(fftSize * 2);
            }
            if (doPeak) {
                if (output->spectrum_hold_points.size() != fftSize * 2) {
                    output->spectrum_hold_points.resize(fftSize * 2);
                }
            } else {
                output->spectrum_hold_points.resize(0);
            }
            
            fftwf_execute(fftw_plan);
            
            float fft_ceil = 0, fft_floor = 1;
            
            if (fft_result.size() != fftSizeInternal) {
                if (fft_result.capacity() < fftSizeInternal) {
                    fft_result.reserve(fftSizeInternal);
                    fft_result_ma.reserve(fftSizeInternal);
                    fft_result_maa.reserve(fftSizeInternal);
                    fft_result_peak.reserve(fftSizeInternal);
                }
                fft_result.resize(fftSizeInternal);
                fft_result_ma.resize(fftSizeInternal);
                fft_result_maa.resize(fftSizeInternal);
                fft_result_temp.resize(fftSizeInternal);
                fft_result_peak.resize(fftSizeInternal);
            }
            
            for (int i = 0, iMax = fftSizeInternal / 2; i < iMax; i++) {
                float a = fftwOutput[i][0];
                float b = fftwOutput[i][1];
                float c = sqrt(a * a + b * b);
                
                float x = fftwOutput[fftSizeInternal / 2 + i][0];
                float y = fftwOutput[fftSizeInternal / 2 + i][1];
                float z = sqrt(x * x + y * y);
                
                fft_result[i] = (z);
                fft_result[fftSizeInternal / 2 + i] = (c);
            }
            
            if (newResampler && lastView) {
                if (bwDiff < 0) {
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_temp[i] = fft_result_ma[(fftSizeInternal/4) + (i/2)];
                    }
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_ma[i] = fft_result_temp[i];
                        
                        fft_result_temp[i] = fft_result_maa[(fftSizeInternal/4) + (i/2)];
                    }
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_maa[i] = fft_result_temp[i];
                    }
                } else {
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        if (i < fftSizeInternal/4) {
                            fft_result_temp[i] = 0; // fft_result_ma[fftSizeInternal/4];
                        } else if (i >= fftSizeInternal - fftSizeInternal/4) {
                            fft_result_temp[i] = 0; // fft_result_ma[fftSizeInternal - fftSizeInternal/4-1];
                        } else {
                            fft_result_temp[i] = fft_result_ma[(i-fftSizeInternal/4)*2];
                        }
                    }
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_ma[i] = fft_result_temp[i];
                        
                        if (i < fftSizeInternal/4) {
                            fft_result_temp[i] = 0; //fft_result_maa[fftSizeInternal/4];
                        } else if (i >= fftSizeInternal - fftSizeInternal/4) {
                            fft_result_temp[i] = 0; // fft_result_maa[fftSizeInternal - fftSizeInternal/4-1];
                        } else {
                            fft_result_temp[i] = fft_result_maa[(i-fftSizeInternal/4)*2];
                        }
                    }
                    for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                        fft_result_maa[i] = fft_result_temp[i];
                    }
                }
            }
            if (newResampler) {
                for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                    fft_result_peak[i] = fft_floor_maa;
                }
                fft_ceil_peak = fft_floor_maa;
                fft_floor_peak = fft_ceil_maa;
            }
            
            for (int i = 0, iMax = fftSizeInternal; i < iMax; i++) {
                if (fft_result_maa[i] != fft_result_maa[i]) fft_result_maa[i] = fft_result[i];
                fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * fft_average_rate;
                if (fft_result_ma[i] != fft_result_ma[i]) fft_result_ma[i] = fft_result[i];
                fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * fft_average_rate;
                
                if (fft_result_maa[i] > fft_ceil || fft_ceil != fft_ceil) {
                    fft_ceil = fft_result_maa[i];
                }
                if (fft_result_maa[i] < fft_floor || fft_floor != fft_floor) {
                    fft_floor = fft_result_maa[i];
                }
                if (doPeak) {
                    if (fft_result_maa[i] > fft_result_peak[i]) {
                        fft_result_peak[i] = fft_result_maa[i];
                    }
                }
            }
            
            if (fft_ceil_ma != fft_ceil_ma) fft_ceil_ma = fft_ceil;
            fft_ceil_ma = fft_ceil_ma + (fft_ceil - fft_ceil_ma) * 0.05;
            if (fft_ceil_maa != fft_ceil_maa) fft_ceil_maa = fft_ceil;
            fft_ceil_maa = fft_ceil_maa + (fft_ceil_ma - fft_ceil_maa) * 0.05;
            
            if (fft_floor_ma != fft_floor_ma) fft_floor_ma = fft_floor;
            fft_floor_ma = fft_floor_ma + (fft_floor - fft_floor_ma) * 0.05;
            if (fft_floor_maa != fft_floor_maa) fft_floor_maa = fft_floor;
            fft_floor_maa = fft_floor_maa + (fft_floor_ma - fft_floor_maa) * 0.05;

            if (doPeak) {
                if (fft_ceil_maa > fft_ceil_peak) {
                    fft_ceil_peak = fft_ceil_maa;
                }
                if (fft_floor_maa < fft_floor_peak) {
                    fft_floor_peak = fft_floor_maa;
                }
            }
            
            float sf = scaleFactor.load();
 
            double visualRatio = (double(bandwidth) / double(resampleBw));
            double visualStart = (double(fftSizeInternal) / 2.0) - (double(fftSizeInternal) * (visualRatio / 2.0));
            double visualAccum = 0;
            double peak_acc = 0, acc = 0, accCount = 0, i = 0;
   
            double point_ceil = doPeak?fft_ceil_peak:fft_ceil_maa;
            double point_floor = doPeak?fft_floor_peak:fft_floor_maa;
            
            for (int x = 0, xMax = output->spectrum_points.size() / 2; x < xMax; x++) {
                visualAccum += visualRatio * double(SPECTRUM_VZM);

                while (visualAccum >= 1.0) {
                    int idx = round(visualStart+i);
                    if (idx > 0 && idx < fftSizeInternal) {
                        acc += fft_result_maa[idx];
                        if (doPeak) {
                            peak_acc += fft_result_peak[idx];
                        }
                    } else {
                        acc += fft_floor_maa;
                        if (doPeak) {
                            peak_acc += fft_floor_maa;
                        }
                    }
                    accCount += 1.0;
                    visualAccum -= 1.0;
                    i++;
                }

                output->spectrum_points[x * 2] = ((float) x / (float) xMax);
                if (doPeak) {
                    output->spectrum_hold_points[x * 2] = ((float) x / (float) xMax);
                }
                if (accCount) {
                    output->spectrum_points[x * 2 + 1] = ((log10((acc/accCount)+0.25 - (point_floor-0.75)) / log10((point_ceil+0.25) - (point_floor-0.75))))*sf;
                    acc = 0.0;
                    if (doPeak) {
                        output->spectrum_hold_points[x * 2 + 1] = ((log10((peak_acc/accCount)+0.25 - (point_floor-0.75)) / log10((point_ceil+0.25) - (point_floor-0.75))))*sf;
                        peak_acc = 0.0;
                    }
                    accCount = 0.0;
                }
            }
            
            if (hideDC.load()) { // DC-spike removal
                long long freqMin = centerFreq-(bandwidth/2);
                long long freqMax = centerFreq+(bandwidth/2);
                long long zeroPt = (iqData->frequency-freqMin);
                
                if (freqMin < iqData->frequency && freqMax > iqData->frequency) {
                    int freqRange = int(freqMax-freqMin);
                    int freqStep = freqRange/fftSize;
                    int fftStart = (zeroPt/freqStep)-(2000/freqStep);
                    int fftEnd = (zeroPt/freqStep)+(2000/freqStep);
                    
//                    std::cout << "range:" << freqRange << ", step: " << freqStep << ", start: " << fftStart << ", end: " << fftEnd << std::endl;
                    
                    if (fftEnd-fftStart < 2) {
                        fftEnd++;
                        fftStart--;
                    }
                    
                    int numSteps = (fftEnd-fftStart);
                    int halfWay = fftStart+(numSteps/2);

                    if ((fftEnd+numSteps/2+1 < fftSize) && (fftStart-numSteps/2-1 >= 0) && (fftEnd > fftStart)) {
                        int n = 1;
                        for (int i = fftStart; i < halfWay; i++) {
                            output->spectrum_points[i * 2 + 1] = output->spectrum_points[(fftStart - n) * 2 + 1];
                            n++;
                        }
                        n = 1;
                        for (int i = halfWay; i < fftEnd; i++) {
                            output->spectrum_points[i * 2 + 1] = output->spectrum_points[(fftEnd + n) * 2 + 1];
                            n++;
                        }
                        if (doPeak) {
                            int n = 1;
                            for (int i = fftStart; i < halfWay; i++) {
                                output->spectrum_hold_points[i * 2 + 1] = output->spectrum_hold_points[(fftStart - n) * 2 + 1];
                                n++;
                            }
                            n = 1;
                            for (int i = halfWay; i < fftEnd; i++) {
                                output->spectrum_hold_points[i * 2 + 1] = output->spectrum_hold_points[(fftEnd + n) * 2 + 1];
                                n++;
                            }
                        }
                    }
                }
            }
            
            output->fft_ceiling = point_ceil/sf;
            output->fft_floor = point_floor;

            output->centerFreq = centerFreq;
            output->bandwidth = bandwidth;

            distribute(output);
        }
    }
 
    iqData->decRefCount();
    iqData->busy_rw.unlock();
    busy_run.unlock();
    
    lastView = is_view.load();
}


void SpectrumVisualProcessor::setScaleFactor(float sf) {
    scaleFactor.store(sf);
}


float SpectrumVisualProcessor::getScaleFactor() {
    return scaleFactor.load();
}

