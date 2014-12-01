#include <DemodulatorMgr.h>

DemodulatorInstance::DemodulatorInstance() :
        t_Demod(NULL), t_Audio(NULL), threadQueueDemod(NULL), demodulatorThread(NULL) {

    threadQueueDemod = new DemodulatorThreadInputQueue;
    threadQueueCommand = new DemodulatorThreadCommandQueue;
    demodulatorThread = new DemodulatorThread(threadQueueDemod);
    demodulatorThread->setCommandQueue(threadQueueCommand);
    audioInputQueue = new AudioThreadInputQueue;
    audioThread = new AudioThread(audioInputQueue);
    demodulatorThread->setAudioInputQueue(audioInputQueue);
}

DemodulatorInstance::~DemodulatorInstance() {

    delete audioThread;
    delete t_Audio;

    delete audioInputQueue;
    delete threadQueueDemod;
    delete demodulatorThread;
    delete t_Demod;
}

void DemodulatorInstance::setVisualOutputQueue(DemodulatorThreadOutputQueue *tQueue) {
    demodulatorThread->setVisualOutputQueue(tQueue);
}

void DemodulatorInstance::run() {
    if (t_Demod) {
        terminate();

        delete threadQueueDemod;
        delete demodulatorThread;
        delete t_Demod;
        delete audioThread;
        delete audioInputQueue;
        delete t_Audio;

        threadQueueDemod = new DemodulatorThreadInputQueue;
        threadQueueCommand = new DemodulatorThreadCommandQueue;
        demodulatorThread = new DemodulatorThread(threadQueueDemod);
        demodulatorThread->setCommandQueue(threadQueueCommand);

        audioInputQueue = new AudioThreadInputQueue;
        audioThread = new AudioThread(audioInputQueue);

        demodulatorThread->setAudioInputQueue(audioInputQueue);
    }

    t_Audio = new std::thread(&AudioThread::threadMain, audioThread);

#ifdef __APPLE__	// Already using pthreads, might as well do some custom init..
	pthread_attr_t attr;
	size_t size;

	pthread_attr_init(&attr);
	pthread_attr_setstacksize(&attr, 2048000);
	pthread_attr_getstacksize(&attr, &size);
    pthread_create(&t_Demod, &attr, &DemodulatorThread::pthread_helper, demodulatorThread);
	pthread_attr_destroy(&attr);

	std::cout << "Initialized demodulator stack size of " << size << std::endl;

#else
    t_Demod = new std::thread(&DemodulatorThread::threadMain, demodulatorThread);
#endif
}

DemodulatorThreadCommandQueue *DemodulatorInstance::getCommandQueue() {
    return threadQueueCommand;
}

DemodulatorThreadParameters &DemodulatorInstance::getParams() {
    return demodulatorThread->getParams();
}

void DemodulatorInstance::terminate() {
    std::cout << "Terminating demodulator thread.." << std::endl;
    demodulatorThread->terminate();
#ifdef __APPLE__
    pthread_join(t_Demod,NULL);
#else
    t_Demod->join();
#endif
    std::cout << "Terminating demodulator audio thread.." << std::endl;
    audioThread->terminate();
    t_Audio->join();
}

DemodulatorMgr::DemodulatorMgr() {

}

DemodulatorMgr::~DemodulatorMgr() {
    terminateAll();
}

DemodulatorInstance *DemodulatorMgr::newThread() {
    DemodulatorInstance *newDemod = new DemodulatorInstance;
    demods.push_back(newDemod);
    return newDemod;
}

void DemodulatorMgr::terminateAll() {
    while (demods.size()) {
        DemodulatorInstance *d = demods.back();
        demods.pop_back();
        d->terminate();
        delete d;
    }
}

std::vector<DemodulatorInstance *> &DemodulatorMgr::getDemodulators() {
    return demods;
}

std::vector<DemodulatorInstance *> *DemodulatorMgr::getDemodulatorsAt(int freq, int bandwidth) {
    std::vector<DemodulatorInstance *> *foundDemods = new std::vector<DemodulatorInstance *>();

    for (int i = 0, iMax = demods.size(); i < iMax; i++) {
        DemodulatorInstance *testDemod = demods[i];

        int freqTest = testDemod->getParams().frequency;
        int bandwidthTest = testDemod->getParams().bandwidth;
        int halfBandwidthTest = bandwidthTest / 2;

        int halfBuffer = bandwidth / 2;

        if ((freq <= (freqTest + halfBandwidthTest + halfBuffer)) && (freq >= (freqTest - halfBandwidthTest - halfBuffer))) {
            foundDemods->push_back(testDemod);
        }
    }

    return foundDemods;
}
