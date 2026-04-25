#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QSemaphore>
#include <QTimer>
#include <QDateTime>
#include <atomic>
#include <vector>

// ── Configuration ─────────────────────────────────────────────────────────────
#define NUM_DEPTS       4
#define BUFFER_SIZE     8
#define NUM_PRODUCERS   2

static const char* DEPT_NAMES[NUM_DEPTS] = {
    "Emergency", "OPD", "Radiology", "Pharmacy"
};

// Processing delay range per department (ms)
static const int PROC_MIN_MS[NUM_DEPTS] = {  500, 1000, 1500,  600 };
static const int PROC_MAX_MS[NUM_DEPTS] = { 1200, 2500, 4000, 1500 };

// Arrival delay range per receptionist (ms)
static const int ARRIVAL_MIN_MS = 600;
static const int ARRIVAL_MAX_MS = 1800;

// ── Patient record ─────────────────────────────────────────────────────────────
struct Patient {
    int     id;
    int     dept;
    qint64  enqueue_ms;   // QDateTime::currentMSecsSinceEpoch()
};

// ── Event types emitted to the GUI ────────────────────────────────────────────
enum class EventType { Enqueued, Dequeued, Processed };

struct PatientEvent {
    EventType type;
    Patient   patient;
    int       producerId;   // only for Enqueued
    qint64    waitMs;       // only for Dequeued / Processed
    int       procMs;       // only for Processed
    int       bufferFill;   // current occupancy of that dept buffer
};

// ══════════════════════════════════════════════════════════════════════════════
//  ProducerThread
// ══════════════════════════════════════════════════════════════════════════════
class HospitalEngine;

class ProducerThread : public QThread {
    Q_OBJECT
public:
    ProducerThread(HospitalEngine* engine, int id, QObject* parent = nullptr);
    void stop();

protected:
    void run() override;

signals:
    void eventOccurred(PatientEvent ev);

private:
    HospitalEngine* m_engine;
    int             m_id;
    std::atomic<bool> m_running{true};
};

// ══════════════════════════════════════════════════════════════════════════════
//  ConsumerThread
// ══════════════════════════════════════════════════════════════════════════════
class ConsumerThread : public QThread {
    Q_OBJECT
public:
    ConsumerThread(HospitalEngine* engine, int deptId, QObject* parent = nullptr);
    void stop();

protected:
    void run() override;

signals:
    void eventOccurred(PatientEvent ev);

private:
    HospitalEngine* m_engine;
    int             m_deptId;
    std::atomic<bool> m_running{true};
};

// ══════════════════════════════════════════════════════════════════════════════
//  HospitalEngine  — owns the shared buffers and synchronisation primitives
// ══════════════════════════════════════════════════════════════════════════════
class HospitalEngine : public QObject {
    Q_OBJECT
public:
    explicit HospitalEngine(QObject* parent = nullptr);
    ~HospitalEngine();

    void start();
    void stop();
    bool isRunning() const { return m_running; }

    // Called by ProducerThread — returns false if engine stopped
    bool enqueue(int dept, Patient& p);

    // Called by ConsumerThread — returns false if engine stopped
    bool dequeue(int dept, Patient& out);

    // Stats
    int  totalEnqueued()  const { return m_totalEnqueued.load(); }
    int  totalProcessed() const { return m_totalProcessed.load(); }
    int  deptProcessed(int d) const { return m_deptProcessed[d].load(); }
    void recordWait(qint64 ms)   { m_totalWaitMs += ms; m_waitCount++; }
    double avgWaitMs() const {
        int c = m_waitCount.load();
        return c > 0 ? (double)m_totalWaitMs.load() / c : 0.0;
    }
    void incrementProcessed(int dept) {
        m_deptProcessed[dept]++;
        m_totalProcessed++;
    }

    std::atomic<int>   m_totalEnqueued{0};
    std::atomic<int>   m_totalProcessed{0};
    std::atomic<int>   m_deptProcessed[NUM_DEPTS];
    std::atomic<qint64> m_totalWaitMs{0};
    std::atomic<int>   m_waitCount{0};

signals:
    void patientEvent(PatientEvent ev);
    void statsUpdated(int enqueued, int processed, double avgWait,
                      int e, int o, int r, int p);
    void engineStopped();

private slots:
    void onStatsTimer();
    void onThreadEvent(PatientEvent ev);

private:
    bool            m_running = false;

    // Per-department circular buffer
    Patient         m_buffers[NUM_DEPTS][BUFFER_SIZE];
    int             m_in[NUM_DEPTS]  = {};
    int             m_out[NUM_DEPTS] = {};

    QMutex          m_bufLocks[NUM_DEPTS];
    QSemaphore      m_empty[NUM_DEPTS];
    QSemaphore      m_full[NUM_DEPTS];

    QVector<ProducerThread*> m_producers;
    QVector<ConsumerThread*> m_consumers;
    QTimer*                  m_statsTimer = nullptr;

    qint64          m_startMs = 0;
};
