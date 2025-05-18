#ifndef CLP_THREAD_H
#define CLP_THREAD_H

#include <pthread.h>
#include <iostream>
#include <windows.h>

class CLPThread {
public:
    CLPThread();
    ~CLPThread();

    bool start();
    void stop();
    bool isRunning() const;

private:
    static void* threadFunction(void* arg);
    void run();

    pthread_t m_thread;
    bool m_running;
    mutable pthread_mutex_t m_mutex;
};

#endif // CLP_THREAD_H
