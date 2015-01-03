#include "WaterfallCanvas.h"

#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#if !wxUSE_GLCANVAS
#error "OpenGL required: set wxUSE_GLCANVAS to 1 and rebuild the library"
#endif

#include "CubicSDR.h"
#include "CubicSDRDefs.h"
#include "AppFrame.h"
#include <algorithm>

#include <wx/numformatter.h>

#define MIN_BANDWIDTH 1500

wxBEGIN_EVENT_TABLE(WaterfallCanvas, wxGLCanvas) EVT_PAINT(WaterfallCanvas::OnPaint)
EVT_KEY_DOWN(WaterfallCanvas::OnKeyDown)
EVT_KEY_UP(WaterfallCanvas::OnKeyUp)
EVT_IDLE(WaterfallCanvas::OnIdle)
EVT_MOTION(WaterfallCanvas::mouseMoved)
EVT_LEFT_DOWN(WaterfallCanvas::mouseDown)
EVT_LEFT_UP(WaterfallCanvas::mouseReleased)
EVT_LEAVE_WINDOW(WaterfallCanvas::mouseLeftWindow)
EVT_ENTER_WINDOW(WaterfallCanvas::mouseEnterWindow)
wxEND_EVENT_TABLE()

WaterfallCanvas::WaterfallCanvas(wxWindow *parent, int *attribList) :
        InteractiveCanvas(parent, attribList), spectrumCanvas(NULL), activeDemodulatorBandwidth(0), activeDemodulatorFrequency(0), dragState(
                WF_DRAG_NONE), nextDragState(WF_DRAG_NONE), fft_size(0), waterfall_lines(0), plan(
        NULL), in(NULL), out(NULL), resampler(NULL), resample_ratio(0), last_input_bandwidth(0), zoom(0) {

    glContext = new WaterfallContext(this, &wxGetApp().GetContext(this));

    nco_shift = nco_crcf_create(LIQUID_NCO);
    shift_freq = 0;

    fft_ceil_ma = fft_ceil_maa = 100.0;
    fft_floor_ma = fft_floor_maa = 0.0;

    SetCursor(wxCURSOR_CROSS);
}

WaterfallCanvas::~WaterfallCanvas() {
    nco_crcf_destroy(nco_shift);
}

void WaterfallCanvas::Setup(int fft_size_in, int waterfall_lines_in) {
    if (fft_size == fft_size_in && waterfall_lines_in == waterfall_lines) {
        return;
    }
    fft_size = fft_size_in;
    waterfall_lines = waterfall_lines_in;

    if (in) {
        free(in);
    }
    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fft_size);
    if (out) {
        free(out);
    }
    out = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * fft_size);
    if (plan) {
        fftw_destroy_plan(plan);
    }
    plan = fftw_plan_dft_1d(fft_size, in, out, FFTW_FORWARD, FFTW_ESTIMATE);

    glContext->Setup(fft_size, waterfall_lines);
}

WaterfallCanvas::DragState WaterfallCanvas::getDragState() {
    return dragState;
}

WaterfallCanvas::DragState WaterfallCanvas::getNextDragState() {
    return nextDragState;
}

void WaterfallCanvas::attachSpectrumCanvas(SpectrumCanvas *canvas_in) {
    spectrumCanvas = canvas_in;
}

void WaterfallCanvas::OnPaint(wxPaintEvent& WXUNUSED(event)) {
    wxPaintDC dc(this);
    const wxSize ClientSize = GetClientSize();

    glContext->SetCurrent(*this);
    glViewport(0, 0, ClientSize.x, ClientSize.y);

    glContext->BeginDraw();
    glContext->Draw(spectrum_points);

    std::vector<DemodulatorInstance *> &demods = wxGetApp().getDemodMgr().getDemodulators();

    DemodulatorInstance *activeDemodulator = wxGetApp().getDemodMgr().getActiveDemodulator();
    DemodulatorInstance *lastActiveDemodulator = wxGetApp().getDemodMgr().getLastActiveDemodulator();

    bool isNew = shiftDown
            || (wxGetApp().getDemodMgr().getLastActiveDemodulator() && !wxGetApp().getDemodMgr().getLastActiveDemodulator()->isActive());

    int currentBandwidth = GetBandwidth();
    int currentCenterFreq = GetCenterFrequency();

    if (mTracker.mouseInView()) {
        if (nextDragState == WF_DRAG_RANGE) {
            if (mTracker.mouseDown()) {
                float width = mTracker.getOriginDeltaMouseX();
                float centerPos = mTracker.getOriginMouseX() + width / 2.0;

                if (isNew) {
                    glContext->DrawDemod(lastActiveDemodulator, 1, 1, 1, currentCenterFreq, currentBandwidth);
                    glContext->DrawFreqSelector(centerPos, 0, 1, 0, width ? width : (1.0 / (float) ClientSize.x), currentCenterFreq,
                            currentBandwidth);
                } else {
                    glContext->DrawDemod(lastActiveDemodulator, 1, 0, 0, currentCenterFreq, currentBandwidth);
                    glContext->DrawFreqSelector(centerPos, 1, 1, 0, width ? width : (1.0 / (float) ClientSize.x), currentCenterFreq,
                            currentBandwidth);
                }
            } else {
                if (isNew) {
                    glContext->DrawDemod(lastActiveDemodulator, 1, 1, 1, currentCenterFreq, currentBandwidth);
                    glContext->DrawFreqSelector(mTracker.getMouseX(), 0, 1, 0, 1.0 / (float) ClientSize.x, currentCenterFreq, currentBandwidth);
                } else {
                    glContext->DrawDemod(lastActiveDemodulator, 1, 0, 0, currentCenterFreq, currentBandwidth);
                    glContext->DrawFreqSelector(mTracker.getMouseX(), 1, 1, 0, 1.0 / (float) ClientSize.x, currentCenterFreq, currentBandwidth);
                }
            }
        } else {
            if (activeDemodulator == NULL) {
                if (lastActiveDemodulator) {
                    if (isNew) {
                        glContext->DrawDemod(lastActiveDemodulator, 1, 1, 1, currentCenterFreq, currentBandwidth);
                        glContext->DrawFreqSelector(mTracker.getMouseX(), 0, 1, 0, 0, currentCenterFreq, currentBandwidth);
                    } else {
                        glContext->DrawDemod(lastActiveDemodulator, 1, 0, 0, currentCenterFreq, currentBandwidth);
                        glContext->DrawFreqSelector(mTracker.getMouseX(), 1, 1, 0, 0, currentCenterFreq, currentBandwidth);
                    }
                } else {
                    glContext->DrawFreqSelector(mTracker.getMouseX(), 1, 1, 0, 0, currentCenterFreq, currentBandwidth);
                }
            } else {
                if (lastActiveDemodulator) {
                    glContext->DrawDemod(lastActiveDemodulator, 1, 1, 1, currentCenterFreq, currentBandwidth);
                }
                glContext->DrawDemod(activeDemodulator, 1, 1, 0, currentCenterFreq, currentBandwidth);
            }
        }
    } else {
        if (activeDemodulator) {
            glContext->DrawDemod(activeDemodulator, 1, 1, 1, currentCenterFreq, currentBandwidth);
        }
        if (lastActiveDemodulator) {
            glContext->DrawDemod(lastActiveDemodulator, 1, 1, 1, currentCenterFreq, currentBandwidth);
        }
    }

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        if (activeDemodulator == demods[i] || lastActiveDemodulator == demods[i]) {
            continue;
        }
        glContext->DrawDemod(demods[i], 1, 1, 1, currentCenterFreq, currentBandwidth);
    }

    glContext->EndDraw();

    SwapBuffers();
}

void WaterfallCanvas::OnKeyUp(wxKeyEvent& event) {
    InteractiveCanvas::OnKeyUp(event);
    shiftDown = event.ShiftDown();
    altDown = event.AltDown();
    ctrlDown = event.ControlDown();
    switch (event.GetKeyCode()) {
    case 'A':
        zoom = 0;
        break;
    case 'Z':
        zoom = 0;
        break;
    }
}

void WaterfallCanvas::OnKeyDown(wxKeyEvent& event) {
    InteractiveCanvas::OnKeyDown(event);
    float angle = 5.0;

    DemodulatorInstance *activeDemod = wxGetApp().getDemodMgr().getActiveDemodulator();

    unsigned int freq;
    unsigned int bw;
    switch (event.GetKeyCode()) {
    case 'A':
        zoom = 1;
        break;
    case 'Z':
        zoom = -1;
        break;
    case WXK_RIGHT:
        freq = wxGetApp().getFrequency();
        if (shiftDown) {
            freq += SRATE * 10;
            if (isView) {
                SetView(center_freq + SRATE * 10, GetBandwidth());
                if (spectrumCanvas) {
                    spectrumCanvas->SetView(GetCenterFrequency(), GetBandwidth());
                }
            }
        } else {
            freq += SRATE / 2;
            if (isView) {
                SetView(center_freq + SRATE / 2, GetBandwidth());
                if (spectrumCanvas) {
                    spectrumCanvas->SetView(GetCenterFrequency(), GetBandwidth());
                }
            }
        }
        wxGetApp().setFrequency(freq);
        setStatusText("Set center frequency: %s", freq);
        break;
    case WXK_LEFT:
        freq = wxGetApp().getFrequency();
        if (shiftDown) {
            freq -= SRATE * 10;
            if (isView) {
                SetView(center_freq - SRATE * 10, GetBandwidth());
                if (spectrumCanvas) {
                    spectrumCanvas->SetView(GetCenterFrequency(), GetBandwidth());
                }
            }
        } else {
            freq -= SRATE / 2;
            if (isView) {
                SetView(center_freq - SRATE / 2, GetBandwidth());
                if (spectrumCanvas) {
                    spectrumCanvas->SetView(GetCenterFrequency(), GetBandwidth());
                }
            }
        }
        wxGetApp().setFrequency(freq);
        setStatusText("Set center frequency: %s", freq);
        break;
    case 'D':
    case WXK_DELETE:
        if (!activeDemod) {
            break;
        }
        wxGetApp().removeDemodulator(activeDemod);
        wxGetApp().getDemodMgr().deleteThread(activeDemod);
        break;
    case 'S':
        if (!activeDemod) {
            break;
        }
        if (activeDemod->isSquelchEnabled()) {
            activeDemod->setSquelchEnabled(false);
        } else {
            activeDemod->squelchAuto();
        }
        break;
    case WXK_SPACE:
        if (!activeDemod) {
            break;
        }
        if (activeDemod->isStereo()) {
            activeDemod->setStereo(false);
        } else {
            activeDemod->setStereo(true);
        }
        break;
    default:
        event.Skip();
        return;
    }
}

void WaterfallCanvas::setData(DemodulatorThreadIQData *input) {
    if (!input) {
        return;
    }

    unsigned int bw;
    if (zoom) {
        int freq = wxGetApp().getFrequency();

        if (zoom > 0) {
            center_freq = GetCenterFrequency();
            bw = GetBandwidth();
            bw = (unsigned int) ceil((float) bw * 0.95);
            if (bw < 80000) {
                bw = 80000;
            }
            if (mTracker.mouseInView()) {
                int mfreqA = GetFrequencyAt(mTracker.getMouseX());
                SetBandwidth(bw);
                int mfreqB = GetFrequencyAt(mTracker.getMouseX());
                center_freq += mfreqA - mfreqB;
            }

            SetView(center_freq, bw);
            if (spectrumCanvas) {
                spectrumCanvas->SetView(center_freq, bw);
            }
        } else {
            if (isView) {
                bw = GetBandwidth();
                bw = (unsigned int) ceil((float) bw * 1.05);
                if ((int) bw >= SRATE) {
                    bw = (unsigned int) SRATE;
                    DisableView();
                    if (spectrumCanvas) {
                        spectrumCanvas->DisableView();
                    }
                } else {
                    if (mTracker.mouseInView()) {
                        int freq = wxGetApp().getFrequency();
                        int mfreqA = GetFrequencyAt(mTracker.getMouseX());
                        SetBandwidth(bw);
                        int mfreqB = GetFrequencyAt(mTracker.getMouseX());
                        center_freq += mfreqA - mfreqB;
                    }

                    SetView(GetCenterFrequency(), bw);
                    if (spectrumCanvas) {
                        spectrumCanvas->SetView(center_freq, bw);
                    }
                }
            }
        }
        if (center_freq < freq && (center_freq - bandwidth / 2) < (freq - SRATE / 2)) {
            center_freq = (freq - SRATE / 2) + bandwidth / 2;
        }
        if (center_freq > freq && (center_freq + bandwidth / 2) > (freq + SRATE / 2)) {
            center_freq = (freq + SRATE / 2) - bandwidth / 2;
        }
    }

    std::vector<liquid_float_complex> *data = &input->data;

    if (data && data->size()) {
//        if (fft_size != data->size() && !isView) {
//            Setup(data->size(), waterfall_lines);
//        }

//        if (last_bandwidth != bandwidth && !isView) {
//            Setup(bandwidth, waterfall_lines);
//        }

        if (spectrum_points.size() < fft_size * 2) {
            spectrum_points.resize(fft_size * 2);
        }

        if (isView) {
            if (!input->frequency || !input->bandwidth) {
                return;
            }

            if (center_freq != input->frequency) {
                if (((int) center_freq - (int) input->frequency) != shift_freq || last_input_bandwidth != input->bandwidth) {
                    if ((int) input->frequency - abs((int) center_freq) < (int) ((float) ((float) SRATE / 2.0))) {
                        shift_freq = (int) center_freq - (int) input->frequency;
                        nco_crcf_reset(nco_shift);
                        nco_crcf_set_frequency(nco_shift, (2.0 * M_PI) * (((float) abs(shift_freq)) / ((float) input->bandwidth)));
                    }
                }

                if (shift_buffer.size() != input->data.size()) {
                    if (shift_buffer.capacity() < input->data.size()) {
                        shift_buffer.reserve(input->data.size());
                    }
                    shift_buffer.resize(input->data.size());
                }

                if (shift_freq < 0) {
                    nco_crcf_mix_block_up(nco_shift, &input->data[0], &shift_buffer[0], input->data.size());
                } else {
                    nco_crcf_mix_block_down(nco_shift, &input->data[0], &shift_buffer[0], input->data.size());
                }
            } else {
                shift_buffer.assign(input->data.begin(), input->data.end());
            }

            if (!resampler || bandwidth != last_bandwidth || last_input_bandwidth != input->bandwidth) {
                resample_ratio = (double) (bandwidth) / (double) input->bandwidth;

                float As = 60.0f;

                if (resampler) {
                    msresamp_crcf_destroy(resampler);
                }
                resampler = msresamp_crcf_create(resample_ratio, As);

                last_bandwidth = bandwidth;
                last_input_bandwidth = input->bandwidth;
            }

            int out_size = ceil((double) (input->data.size()) * resample_ratio) + 512;

            if (resampler_buffer.size() != out_size) {
                if (resampler_buffer.capacity() < out_size) {
                    resampler_buffer.reserve(out_size);
                }
                resampler_buffer.resize(out_size);
            }

            unsigned int num_written;
            msresamp_crcf_execute(resampler, &shift_buffer[0], input->data.size(), &resampler_buffer[0], &num_written);

            resampler_buffer.resize(fft_size);

            if (num_written < fft_size) {
                for (int i = 0; i < num_written; i++) {
                    in[i][0] = resampler_buffer[i].real;
                    in[i][1] = resampler_buffer[i].imag;
                }
                for (int i = num_written; i < fft_size; i++) {
                    in[i][0] = 0;
                    in[i][1] = 0;
                }
            } else {
                for (int i = 0; i < fft_size; i++) {
                    in[i][0] = resampler_buffer[i].real;
                    in[i][1] = resampler_buffer[i].imag;
                }
            }
        } else {

            if (data->size() < fft_size) {
                for (int i = 0, iMax = data->size(); i < iMax; i++) {
                    in[i][0] = (*data)[i].real;
                    in[i][1] = (*data)[i].imag;
                }
                for (int i = data->size(); i < fft_size; i++) {
                    in[i][0] = 0;
                    in[i][1] = 0;
                }
            } else {
                for (int i = 0; i < fft_size; i++) {
                    in[i][0] = (*data)[i].real;
                    in[i][1] = (*data)[i].imag;
                }
            }
        }

        fftw_execute(plan);

        double fft_ceil = 0, fft_floor = 1;

        if (fft_result.size() < fft_size) {
            fft_result.resize(fft_size);
            fft_result_ma.resize(fft_size);
            fft_result_maa.resize(fft_size);
        }

        int n;
        for (int i = 0, iMax = fft_size / 2; i < iMax; i++) {
            n = (i == 0) ? 1 : i;
            double a = out[n][0];
            double b = out[n][1];
            double c = sqrt(a * a + b * b);

            double x = out[fft_size / 2 + n][0];
            double y = out[fft_size / 2 + n][1];
            double z = sqrt(x * x + y * y);

            fft_result[i] = (z);
            fft_result[fft_size / 2 + i] = (c);
        }

        for (int i = 0, iMax = fft_size; i < iMax; i++) {
            if (isView) {
                fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * 0.85;
                fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * 0.55;
            } else {
                fft_result_maa[i] += (fft_result_ma[i] - fft_result_maa[i]) * 0.65;
                fft_result_ma[i] += (fft_result[i] - fft_result_ma[i]) * 0.65;
            }

            if (fft_result_maa[i] > fft_ceil) {
                fft_ceil = fft_result_maa[i];
            }
            if (fft_result_maa[i] < fft_floor) {
                fft_floor = fft_result_maa[i];
            }
        }

        fft_ceil += 1;
        fft_floor -= 1;

        fft_ceil_ma = fft_ceil_ma + (fft_ceil - fft_ceil_ma) * 0.05;
        fft_ceil_maa = fft_ceil_maa + (fft_ceil_ma - fft_ceil_maa) * 0.05;

        fft_floor_ma = fft_floor_ma + (fft_floor - fft_floor_ma) * 0.05;
        fft_floor_maa = fft_floor_maa + (fft_floor_ma - fft_floor_maa) * 0.05;

        for (int i = 0, iMax = fft_size; i < iMax; i++) {
            double v = (log10(fft_result_maa[i] - fft_floor_maa) / log10(fft_ceil_maa - fft_floor_maa));
            spectrum_points[i * 2] = ((float) i / (float) iMax);
            spectrum_points[i * 2 + 1] = v;
        }

        if (spectrumCanvas) {
            spectrumCanvas->spectrum_points.assign(spectrum_points.begin(), spectrum_points.end());
        }
    }
}

void WaterfallCanvas::OnIdle(wxIdleEvent &event) {
    Refresh(false);
}

void WaterfallCanvas::mouseMoved(wxMouseEvent& event) {
    InteractiveCanvas::mouseMoved(event);
    DemodulatorInstance *demod = wxGetApp().getDemodMgr().getActiveDemodulator();

    if (mTracker.mouseDown()) {
        if (demod == NULL) {
            return;
        }
        if (dragState == WF_DRAG_BANDWIDTH_LEFT || dragState == WF_DRAG_BANDWIDTH_RIGHT) {

            int bwDiff = (int) (mTracker.getDeltaMouseX() * (float) GetBandwidth()) * 2;

            if (dragState == WF_DRAG_BANDWIDTH_LEFT) {
                bwDiff = -bwDiff;
            }

            if (!activeDemodulatorBandwidth) {
                activeDemodulatorBandwidth = demod->getParams().bandwidth;
            }

            DemodulatorThreadCommand command;
            command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_BANDWIDTH;
            activeDemodulatorBandwidth = activeDemodulatorBandwidth + bwDiff;
            if (activeDemodulatorBandwidth > SRATE) {
                activeDemodulatorBandwidth = SRATE;
            }
            if (activeDemodulatorBandwidth < MIN_BANDWIDTH) {
                activeDemodulatorBandwidth = MIN_BANDWIDTH;
            }

            command.int_value = activeDemodulatorBandwidth;
            demod->getCommandQueue()->push(command);
            setStatusText("Set demodulator bandwidth: %s", activeDemodulatorBandwidth);
        }

        if (dragState == WF_DRAG_FREQUENCY) {
            int bwDiff = (int) (mTracker.getDeltaMouseX() * (float) GetBandwidth());

            if (!activeDemodulatorFrequency) {
                activeDemodulatorFrequency = demod->getParams().frequency;
            }

            DemodulatorThreadCommand command;
            command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_FREQUENCY;
            activeDemodulatorFrequency = activeDemodulatorFrequency + bwDiff;

            command.int_value = activeDemodulatorFrequency;
            demod->getCommandQueue()->push(command);

            demod->updateLabel(activeDemodulatorFrequency);

            setStatusText("Set demodulator frequency: %s", activeDemodulatorFrequency);
        }
    } else {
        int freqPos = GetFrequencyAt(mTracker.getMouseX());

        std::vector<DemodulatorInstance *> *demodsHover = wxGetApp().getDemodMgr().getDemodulatorsAt(freqPos, 15000);

        wxGetApp().getDemodMgr().setActiveDemodulator(NULL);

        if (altDown) {
            nextDragState = WF_DRAG_RANGE;
            mTracker.setVertDragLock(true);
            mTracker.setHorizDragLock(false);
            if (shiftDown) {
                setStatusText("Click and drag to create a new demodulator by range.");
            } else {
                setStatusText("Click and drag to set the current demodulator range.");
            }
        } else if (demodsHover->size()) {
            int hovered = -1;
            int near_dist = GetBandwidth();

            DemodulatorInstance *activeDemodulator = NULL;

            for (int i = 0, iMax = demodsHover->size(); i < iMax; i++) {
                DemodulatorInstance *demod = (*demodsHover)[i];
                int freqDiff = (int) demod->getParams().frequency - freqPos;
                int halfBw = (demod->getParams().bandwidth / 2);

                int dist = abs(freqDiff);

                if (dist < near_dist) {
                    activeDemodulator = demod;
                    near_dist = dist;
                }

                if (dist <= halfBw && dist >= (int) ((float) halfBw / (float) 1.5)) {
                    int edge_dist = abs(halfBw - dist);
                    if (edge_dist < near_dist) {
                        activeDemodulator = demod;
                        near_dist = edge_dist;
                    }
                }
            }

            if (activeDemodulator == NULL) {
                return;
            }

            wxGetApp().getDemodMgr().setActiveDemodulator(activeDemodulator);

            int freqDiff = ((int) activeDemodulator->getParams().frequency - freqPos);

            if (abs(freqDiff) > (activeDemodulator->getParams().bandwidth / 3)) {
                SetCursor(wxCURSOR_SIZEWE);

                if (freqDiff > 0) {
                    nextDragState = WF_DRAG_BANDWIDTH_LEFT;
                } else {
                    nextDragState = WF_DRAG_BANDWIDTH_RIGHT;
                }

                mTracker.setVertDragLock(true);
                mTracker.setHorizDragLock(false);
                setStatusText("Click and drag to change demodulator bandwidth. D to delete, SPACE for stereo.");
            } else {
                SetCursor(wxCURSOR_SIZING);
                nextDragState = WF_DRAG_FREQUENCY;

                mTracker.setVertDragLock(true);
                mTracker.setHorizDragLock(false);
                setStatusText("Click and drag to change demodulator frequency. D to delete, SPACE for stereo.");
            }
        } else {
            SetCursor(wxCURSOR_CROSS);
            nextDragState = WF_DRAG_NONE;
            if (shiftDown) {
                setStatusText("Click to create a new demodulator or hold ALT to drag range.");
            } else {
                setStatusText("Click to move active demodulator frequency or hold ALT to drag range; hold SHIFT to create new.  A / Z to Zoom.  Arrow keys (+SHIFT) to move center frequency.");
            }
        }

        delete demodsHover;
    }
}

void WaterfallCanvas::mouseDown(wxMouseEvent& event) {
    InteractiveCanvas::mouseDown(event);

    dragState = nextDragState;

    if (dragState && dragState != WF_DRAG_RANGE) {
        wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getActiveDemodulator(), false);
    }

    activeDemodulatorBandwidth = 0;
    activeDemodulatorFrequency = 0;
}

void WaterfallCanvas::mouseWheelMoved(wxMouseEvent& event) {
    InteractiveCanvas::mouseWheelMoved(event);
}

void WaterfallCanvas::mouseReleased(wxMouseEvent& event) {
    InteractiveCanvas::mouseReleased(event);

    bool isNew = shiftDown
            || (wxGetApp().getDemodMgr().getLastActiveDemodulator() && !wxGetApp().getDemodMgr().getLastActiveDemodulator()->isActive());

    mTracker.setVertDragLock(false);
    mTracker.setHorizDragLock(false);

    DemodulatorInstance *demod;

    if (mTracker.getOriginDeltaMouseX() == 0 && mTracker.getOriginDeltaMouseY() == 0) {
        float pos = mTracker.getMouseX();
        int input_center_freq = GetCenterFrequency();
        int freq = input_center_freq - (int) (0.5 * (float) GetBandwidth()) + (int) ((float) pos * (float) GetBandwidth());

        if (dragState == WF_DRAG_NONE) {
            if (!isNew && wxGetApp().getDemodMgr().getDemodulators().size()) {
                demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();
            } else {
                demod = wxGetApp().getDemodMgr().newThread();
                demod->getParams().frequency = freq;

                if (DemodulatorInstance *last = wxGetApp().getDemodMgr().getLastActiveDemodulator()) {
                    demod->getParams().bandwidth = last->getParams().bandwidth;
                    demod->setDemodulatorType(last->getDemodulatorType());
                    demod->setSquelchLevel(last->getSquelchLevel());
                    demod->setSquelchEnabled(last->isSquelchEnabled());
                    demod->setStereo(last->isStereo());
                }

                demod->run();

                wxGetApp().bindDemodulator(demod);
                wxGetApp().getDemodMgr().setActiveDemodulator(demod, false);
            }

            if (demod == NULL) {
                dragState = WF_DRAG_NONE;
                return;
            }

            demod->updateLabel(freq);

            DemodulatorThreadCommand command;
            command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_FREQUENCY;
            command.int_value = freq;
            demod->getCommandQueue()->push(command);

            setStatusText("New demodulator at frequency: %s", freq);

            wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getLastActiveDemodulator(), false);
            SetCursor(wxCURSOR_SIZING);
            nextDragState = WF_DRAG_FREQUENCY;
            mTracker.setVertDragLock(true);
            mTracker.setHorizDragLock(false);
        } else {
            wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getActiveDemodulator(), false);
            nextDragState = WF_DRAG_FREQUENCY;
        }
    } else if (dragState == WF_DRAG_RANGE) {
        float width = mTracker.getOriginDeltaMouseX();
        float pos = mTracker.getOriginMouseX() + width / 2.0;

        int input_center_freq = GetCenterFrequency();
        unsigned int freq = input_center_freq - (int) (0.5 * (float) GetBandwidth()) + (int) ((float) pos * (float) GetBandwidth());
        unsigned int bw = (unsigned int) (fabs(width) * (float) GetBandwidth());

        if (bw < MIN_BANDWIDTH) {
            bw = MIN_BANDWIDTH;
        }

        if (!bw) {
            dragState = WF_DRAG_NONE;
            return;
        }

        if (!isNew && wxGetApp().getDemodMgr().getDemodulators().size()) {
            demod = wxGetApp().getDemodMgr().getLastActiveDemodulator();
        } else {
            demod = wxGetApp().getDemodMgr().newThread();
            demod->getParams().frequency = freq;
            demod->getParams().bandwidth = bw;
            if (DemodulatorInstance *last = wxGetApp().getDemodMgr().getLastActiveDemodulator()) {
                demod->setDemodulatorType(last->getDemodulatorType());
                demod->setSquelchLevel(last->getSquelchLevel());
                demod->setSquelchEnabled(last->isSquelchEnabled());
                demod->setStereo(last->isStereo());
            }
            demod->run();

            wxGetApp().bindDemodulator(demod);
            wxGetApp().getDemodMgr().setActiveDemodulator(demod, false);
        }

        if (demod == NULL) {
            dragState = WF_DRAG_NONE;
            return;
        }

        setStatusText("New demodulator at frequency: %s", freq);

        wxGetApp().getDemodMgr().setActiveDemodulator(wxGetApp().getDemodMgr().getLastActiveDemodulator(), false);
        demod->updateLabel(freq);

        DemodulatorThreadCommand command;
        command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_FREQUENCY;
        command.int_value = freq;
        demod->getCommandQueue()->push(command);
        command.cmd = DemodulatorThreadCommand::DEMOD_THREAD_CMD_SET_BANDWIDTH;
        command.int_value = bw;
        demod->getCommandQueue()->push(command);
    }

    dragState = WF_DRAG_NONE;
}

void WaterfallCanvas::mouseLeftWindow(wxMouseEvent& event) {
    InteractiveCanvas::mouseLeftWindow(event);
    SetCursor(wxCURSOR_CROSS);
    wxGetApp().getDemodMgr().setActiveDemodulator(NULL);
}

void WaterfallCanvas::mouseEnterWindow(wxMouseEvent& event) {
    InteractiveCanvas::mouseEnterWindow(event);
    SetCursor(wxCURSOR_CROSS);
}

