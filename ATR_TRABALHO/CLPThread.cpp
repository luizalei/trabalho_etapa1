#include "CLPThread.h"

CLPThread::CLPThread() : m_running(false) {
    pthread_mutex_init(&m_mutex, nullptr);
}

CLPThread::~CLPThread() {
    stop();
    pthread_mutex_destroy(&m_mutex);
}

bool CLPThread::start() {
    pthread_mutex_lock(&m_mutex);

    if (m_running) {
        pthread_mutex_unlock(&m_mutex);
        return false;
    }

    m_running = true;

    if (pthread_create(&m_thread, nullptr, &CLPThread::threadFunction, this) != 0) {
        m_running = false;
        pthread_mutex_unlock(&m_mutex);
        return false;
    }

    pthread_mutex_unlock(&m_mutex);
    return true;
}

void CLPThread::stop() {
    pthread_mutex_lock(&m_mutex);

    if (m_running) {
        m_running = false;
        pthread_join(m_thread, nullptr);
    }

    pthread_mutex_unlock(&m_mutex);
}

bool CLPThread::isRunning() const {
    pthread_mutex_lock(&m_mutex);
    bool running = m_running;
    pthread_mutex_unlock(&m_mutex);
    return running;
}

void* CLPThread::threadFunction(void* arg) {
    CLPThread* self = static_cast<CLPThread*>(arg);
    self->run();
    return nullptr;
}

void CLPThread::run() {
    while (isRunning()) {
        // Lógica específica da CLP: soma 1+1 e exibe
        int resultado = 1 + 1;
        std::cout << "Thread CLP: 1 + 1 = " << resultado << std::endl;

        // Intervalo entre as somas
        Sleep(1000); // 1 segundo
    }

    std::cout << "Thread CLP: Finalizando execução..." << std::endl;
}