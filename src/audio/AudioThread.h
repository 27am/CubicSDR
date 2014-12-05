#pragma once

#include <queue>
#include <vector>
#include <string>
#include <atomic>
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

#include "wx/thread.h"

#include "AudioThread.h"
#include "ThreadQueue.h"
#include "RtAudio.h"

class AudioThreadInput {
public:
    int frequency;
    int sampleRate;

    std::vector<float> data;
};

typedef ThreadQueue<AudioThreadInput> AudioThreadInputQueue;

class AudioThread {
public:
    std::queue<std::vector<float> > audio_queue;
    unsigned int audio_queue_ptr;

    AudioThread(AudioThreadInputQueue *inputQueue);
    ~AudioThread();

    void threadMain();
    void terminate();

private:
    AudioThreadInputQueue *inputQueue;
    RtAudio dac;
    std::atomic<bool> terminated;
};

