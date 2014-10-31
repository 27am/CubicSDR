#include "SDRThread.h"
#include "CubicSDRDefs.h"
#include <vector>


//wxDEFINE_EVENT(wxEVT_COMMAND_SDRThread_INPUT, wxThreadEvent);

SDRThread::SDRThread(AppFrame *frame) :
        wxThread(wxTHREAD_DETACHED) {
    dev = NULL;
    this->frame = frame;
}
SDRThread::~SDRThread() {

}

void SDRThread::enumerate_rtl() {

    char manufact[256], product[256], serial[256];

    unsigned int rtl_count = rtlsdr_get_device_count();

    std::cout << "RTL Devices: " << rtl_count << std::endl;

    for (int i = 0; i < rtl_count; i++) {
        std::cout << "Device #" << i << ": " << rtlsdr_get_device_name(i) << std::endl;
        if (rtlsdr_get_device_usb_strings(i, manufact, product, serial) == 0) {
            std::cout << "\tManufacturer: " << manufact << ", Product Name: " << product << ", Serial: " << serial << std::endl;

            rtlsdr_open(&dev, i);

            std::cout << "\t Tuner type: ";
            switch (rtlsdr_get_tuner_type(dev)) {
            case RTLSDR_TUNER_UNKNOWN:
                std::cout << "Unknown";
                break;
            case RTLSDR_TUNER_E4000:
                std::cout << "Elonics E4000";
                break;
            case RTLSDR_TUNER_FC0012:
                std::cout << "Fitipower FC0012";
                break;
            case RTLSDR_TUNER_FC0013:
                std::cout << "Fitipower FC0013";
                break;
            case RTLSDR_TUNER_FC2580:
                std::cout << "Fitipower FC2580";
                break;
            case RTLSDR_TUNER_R820T:
                std::cout << "Rafael Micro R820T";
                break;
            case RTLSDR_TUNER_R828D:
                break;
            }

            std::cout << std::endl;
            /*
             int num_gains = rtlsdr_get_tuner_gains(dev, NULL);

             int *gains = (int *)malloc(sizeof(int) * num_gains);
             rtlsdr_get_tuner_gains(dev, gains);

             std::cout << "\t Valid gains: ";
             for (int g = 0; g < num_gains; g++) {
             if (g > 0) {
             std::cout << ", ";
             }
             std::cout << ((float)gains[g]/10.0f);
             }
             std::cout << std::endl;

             free(gains);
             */

            rtlsdr_close(dev);

        } else {
            std::cout << "\tUnable to access device #" << i << " (in use?)" << std::endl;
        }

    }

}

wxThread::ExitCode SDRThread::Entry() {
    signed char *buf = (signed char *) malloc(BUF_SIZE);

    enumerate_rtl();

    rtlsdr_open(&dev, 0);
    rtlsdr_set_sample_rate(dev, SRATE);
    rtlsdr_set_center_freq(dev, 98000000);
    rtlsdr_set_agc_mode(dev, 1);
    rtlsdr_set_offset_tuning(dev, 1);
    rtlsdr_reset_buffer(dev);

    sample_rate = rtlsdr_get_sample_rate(dev);

    std::cout << "Sample Rate is: " << sample_rate << std::endl;

    int n_read;
    double seconds = 0.0;

    std::cout << "Sampling..";
    while (!TestDestroy()) {

        rtlsdr_read_sync(dev, buf, BUF_SIZE, &n_read);
        // move around
        long freq = 98000000+(20000000)*sin(seconds/50.0);
        rtlsdr_set_center_freq(dev, freq);
        
        std::cout << "Frequency: " << freq << std::endl;

        if (!TestDestroy()) {
            std::vector<signed char> *new_buffer = new std::vector<signed char>();

            for (int i = 0; i < n_read; i++) {
                new_buffer->push_back(buf[i] - 127);
            }

            double time_slice = (double)n_read/(double)sample_rate;
            seconds += time_slice;

            // std::cout << "Time Slice: " << time_slice << std::endl;
            if (!TestDestroy()) {
                wxThreadEvent event(wxEVT_THREAD, EVENT_SDR_INPUT);
                event.SetPayload(new_buffer);
                wxQueueEvent(frame, event.Clone());
            } else {
                delete new_buffer;
            }
        }
    }
    std::cout << std::endl << "Done." << std::endl << std::endl;

    rtlsdr_close(dev);
    free(buf);

    return (wxThread::ExitCode) 0;
}

