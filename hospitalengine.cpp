#include "hospitalengine.h"
#include <QRandomGenerator>
#include <QThread>

//  HospitalEngine
HospitalEngine::HospitalEngine(QObject* parent)
    : QObject(parent)
{
    for (int d = 0; d < NUM_DEPTS; d++) {
        m_deptProcessed[d].store(0);
        // QSemaphore doesn't have a constructor that takes initial count like this,
        // we acquire/release in start()
    }
}

HospitalEngine::~HospitalEngine() {
    stop();
}

void HospitalEngine::start() {
    if (m_running) return;
    m_running  = true;
    m_startMs  = QDateTime::currentMSecsSinceEpoch();

    // Reset stats
    m_totalEnqueued.store(0);
    m_totalProcessed.store(0);
    m_totalWaitMs.store(0);
    m_waitCount.store(0);
    for (int d = 0; d < NUM_DEPTS; d++) {
        m_deptProcessed[d].store(0);
        m_in[d] = m_out[d] = 0;

        // Reset semaphores: empty=BUFFER_SIZE, full=0
        // QSemaphore: acquire to drain, then release to set
        // Drain any leftover
        m_empty[d].tryAcquire(m_empty[d].available());
        m_full[d].tryAcquire(m_full[d].available());
        m_empty[d].release(BUFFER_SIZE);
    }

    // Spawn producers
    for (int i = 0; i < NUM_PRODUCERS; i++) {
        auto* t = new ProducerThread(this, i + 1, this);
        connect(t, &ProducerThread::eventOccurred,
                this, &HospitalEngine::onThreadEvent,
                Qt::QueuedConnection);
        m_producers.append(t);
        t->start();
    }

    // Spawn consumers
    for (int d = 0; d < NUM_DEPTS; d++) {
        auto* t = new ConsumerThread(this, d, this);
        connect(t, &ConsumerThread::eventOccurred,
                this, &HospitalEngine::onThreadEvent,
                Qt::QueuedConnection);
        m_consumers.append(t);
        t->start();
    }

    // Stats timer every 2 s
    m_statsTimer = new QTimer(this);
    connect(m_statsTimer, &QTimer::timeout, this, &HospitalEngine::onStatsTimer);
    m_statsTimer->start(2000);
}

void HospitalEngine::stop() {
    if (!m_running) return;
    m_running = false;

    if (m_statsTimer) { m_statsTimer->stop(); m_statsTimer->deleteLater(); m_statsTimer = nullptr; }

    // Unblock all waiting threads
    for (int d = 0; d < NUM_DEPTS; d++) {
        m_full[d].release(NUM_PRODUCERS + 1);
        m_empty[d].release(NUM_DEPTS + 1);
    }

    for (auto* t : m_producers) { t->stop(); t->wait(3000); t->deleteLater(); }
    for (auto* t : m_consumers) { t->stop(); t->wait(3000); t->deleteLater(); }
    m_producers.clear();
    m_consumers.clear();

    emit engineStopped();
}

bool HospitalEngine::enqueue(int dept, Patient& p) {
    // Wait for a free slot (with timeout so we can check m_running)
    while (m_running) {
        if (m_empty[dept].tryAcquire(1, 300)) break;
    }
    if (!m_running) return false;

    {
        QMutexLocker lk(&m_bufLocks[dept]);
        m_buffers[dept][m_in[dept]] = p;
        m_in[dept] = (m_in[dept] + 1) % BUFFER_SIZE;
    }
    m_full[dept].release(1);
    m_totalEnqueued++;
    return true;
}

bool HospitalEngine::dequeue(int dept, Patient& out) {
    while (m_running) {
        if (m_full[dept].tryAcquire(1, 300)) break;
    }
    if (!m_running) return false;

    {
        QMutexLocker lk(&m_bufLocks[dept]);
        out = m_buffers[dept][m_out[dept]];
        m_out[dept] = (m_out[dept] + 1) % BUFFER_SIZE;
    }
    m_empty[dept].release(1);
    return true;
}

void HospitalEngine::onThreadEvent(PatientEvent ev) {
    emit patientEvent(ev);
}

void HospitalEngine::onStatsTimer() {
    emit statsUpdated(
        m_totalEnqueued.load(),
        m_totalProcessed.load(),
        avgWaitMs(),
        m_deptProcessed[0].load(),
        m_deptProcessed[1].load(),
        m_deptProcessed[2].load(),
        m_deptProcessed[3].load()
    );
}

//  ProducerThread
ProducerThread::ProducerThread(HospitalEngine* engine, int id, QObject* parent)
    : QThread(parent), m_engine(engine), m_id(id) {}

void ProducerThread::stop() { m_running = false; }

void ProducerThread::run() {
    static std::atomic<int> patientCounter{0};

    while (m_running && m_engine->isRunning()) {
        int delay = QRandomGenerator::global()->bounded(ARRIVAL_MIN_MS, ARRIVAL_MAX_MS);
        QThread::msleep(delay);
        if (!m_running || !m_engine->isRunning()) break;

        Patient p;
        p.id           = ++patientCounter;
        p.dept         = QRandomGenerator::global()->bounded(NUM_DEPTS);
        p.enqueue_ms   = QDateTime::currentMSecsSinceEpoch();

        int bufFill = 0;
        if (!m_engine->enqueue(p.dept, p)) break;

        PatientEvent ev;
        ev.type       = EventType::Enqueued;
        ev.patient    = p;
        ev.producerId = m_id;
        ev.waitMs     = 0;
        ev.procMs     = 0;
        ev.bufferFill = bufFill;
        emit eventOccurred(ev);
    }
}

//  ConsumerThread
ConsumerThread::ConsumerThread(HospitalEngine* engine, int deptId, QObject* parent)
    : QThread(parent), m_engine(engine), m_deptId(deptId) {}

void ConsumerThread::stop() { m_running = false; }

void ConsumerThread::run() {
    while (m_running && m_engine->isRunning()) {
        Patient p;
        if (!m_engine->dequeue(m_deptId, p)) break;
        if (!m_running || !m_engine->isRunning()) break;

        qint64 waitMs = QDateTime::currentMSecsSinceEpoch() - p.enqueue_ms;
        m_engine->recordWait(waitMs);

        // Emit dequeue event
        PatientEvent evDq;
        evDq.type       = EventType::Dequeued;
        evDq.patient    = p;
        evDq.producerId = 0;
        evDq.waitMs     = waitMs;
        evDq.procMs     = 0;
        evDq.bufferFill = 0;
        emit eventOccurred(evDq);

        // Simulate treatment
        int procMs = QRandomGenerator::global()->bounded(
            PROC_MIN_MS[m_deptId], PROC_MAX_MS[m_deptId]);
        QThread::msleep(procMs);

        if (!m_running || !m_engine->isRunning()) break;

        m_engine->incrementProcessed(m_deptId);

        PatientEvent evPr;
        evPr.type       = EventType::Processed;
        evPr.patient    = p;
        evPr.producerId = 0;
        evPr.waitMs     = waitMs;
        evPr.procMs     = procMs;
        evPr.bufferFill = 0;
        emit eventOccurred(evPr);
    }
}
