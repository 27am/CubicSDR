#pragma once

#include "ThreadQueue.h"
#include "CubicSDRDefs.h"
#include "liquid/liquid.h"

enum DemodulatorType {
	DEMOD_TYPE_NULL,
	DEMOD_TYPE_AM,
	DEMOD_TYPE_FM,
	DEMOD_TYPE_LSB,
	DEMOD_TYPE_USB
};

class DemodulatorThread;
class DemodulatorThreadCommand {
public:
	enum DemodulatorThreadCommandEnum {
		DEMOD_THREAD_CMD_NULL,
		DEMOD_THREAD_CMD_SET_BANDWIDTH,
		DEMOD_THREAD_CMD_SET_FREQUENCY,
		DEMOD_THREAD_CMD_DEMOD_PREPROCESS_TERMINATED,
		DEMOD_THREAD_CMD_DEMOD_TERMINATED,
		DEMOD_THREAD_CMD_AUDIO_TERMINATED
	};

	DemodulatorThreadCommand() :
			cmd(DEMOD_THREAD_CMD_NULL), int_value(0), context(NULL) {

	}

	DemodulatorThreadCommand(DemodulatorThreadCommandEnum cmd) :
			cmd(cmd), int_value(0), context(NULL) {

	}

	DemodulatorThreadCommandEnum cmd;
	void *context;
	int int_value;
};

class DemodulatorThreadIQData {
public:
	unsigned int frequency;
	unsigned int bandwidth;
	std::vector<signed char> data;

	DemodulatorThreadIQData() :
			frequency(0), bandwidth(0) {

	}

	DemodulatorThreadIQData(unsigned int bandwidth, unsigned int frequency,
			std::vector<signed char> data) :
			data(data), frequency(frequency), bandwidth(bandwidth) {

	}

	~DemodulatorThreadIQData() {

	}
};

class DemodulatorThreadPostIQData {
public:
	std::vector<liquid_float_complex> data;
	float audio_resample_ratio;
	msresamp_crcf audio_resampler;
    float resample_ratio;
    msresamp_crcf resampler;

	DemodulatorThreadPostIQData(): audio_resample_ratio(0), audio_resampler(NULL) {

	}

	~DemodulatorThreadPostIQData() {

	}
};


class DemodulatorThreadAudioData {
public:
	unsigned int frequency;
	unsigned int sampleRate;
	unsigned char channels;

	std::vector<float> data;

	DemodulatorThreadAudioData() :
			sampleRate(0), frequency(0), channels(0) {

	}

	DemodulatorThreadAudioData(unsigned int frequency, unsigned int sampleRate,
			std::vector<float> data) :
			data(data), sampleRate(sampleRate), frequency(frequency), channels(
					1) {

	}

	~DemodulatorThreadAudioData() {

	}
};

typedef ThreadQueue<DemodulatorThreadIQData> DemodulatorThreadInputQueue;
typedef ThreadQueue<DemodulatorThreadPostIQData> DemodulatorThreadPostInputQueue;
typedef ThreadQueue<DemodulatorThreadCommand> DemodulatorThreadCommandQueue;


class DemodulatorThreadParameters {
public:
    unsigned int frequency;
    unsigned int inputRate;
    unsigned int bandwidth; // set equal to disable second stage re-sampling?
    unsigned int audioSampleRate;

    DemodulatorType demodType;

    DemodulatorThreadParameters() :
            frequency(0), inputRate(SRATE), bandwidth(200000), audioSampleRate(
                    AUDIO_FREQUENCY), demodType(DEMOD_TYPE_FM) {

    }

    ~DemodulatorThreadParameters() {

    }
};
