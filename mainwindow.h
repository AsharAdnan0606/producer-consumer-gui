#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QProgressBar>
#include <QFrame>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTimer>
#include <QScrollArea>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include "hospitalengine.h"

// Buffer slot widget — one coloured square per buffer slot
class BufferSlot : public QFrame {
    Q_OBJECT
public:
    explicit BufferSlot(QWidget* parent = nullptr);
    void setOccupied(bool occupied, int patientId = 0);
    void flash();

private:
    QLabel* m_label;
    bool    m_occupied = false;
};

// Department card
class DeptCard : public QFrame {
    Q_OBJECT
public:
    explicit DeptCard(int deptId, QWidget* parent = nullptr);

    void setStatus(bool busy, int patientId = 0);
    void setProcessed(int count);
    void setBufferFill(int fill);
    void flashEnqueue();
    void flashProcess();

private:
    int           m_deptId;
    QLabel*       m_nameLabel;
    QLabel*       m_statusLabel;
    QLabel*       m_patientLabel;
    QLabel*       m_countLabel;
    QHBoxLayout*  m_slotLayout;
    QVector<BufferSlot*> m_slots;
    int           m_fill = 0;
};

// Stat card
class StatCard : public QFrame {
    Q_OBJECT
public:
    StatCard(const QString& label, const QString& unit, QWidget* parent = nullptr);
    void setValue(const QString& val);

private:
    QLabel* m_value;
    QLabel* m_unit;
};

// MainWindow
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onStartStop();
    void onPatientEvent(PatientEvent ev);
    void onStatsUpdated(int enqueued, int processed, double avgWait,
                        int e, int o, int r, int p);
    void onEngineStopped();
    void onUptimeTick();

private:
    void buildUi();
    void applyStyles();
    void appendLog(const QString& html);
    void updateUptimeLabel();

    // Engine
    HospitalEngine* m_engine;

    // Widgets
    QPushButton*    m_startBtn;
    QLabel*         m_uptimeLabel;
    QLabel*         m_statusDot;

    StatCard*       m_cardEnqueued;
    StatCard*       m_cardProcessed;
    StatCard*       m_cardThroughput;
    StatCard*       m_cardAvgWait;

    DeptCard*       m_deptCards[NUM_DEPTS];

    QTextEdit*      m_logView;

    // Uptime
    QTimer*         m_uptimeTimer;
    qint64          m_startMs = 0;
    bool            m_running = false;

    // For throughput calc
    int             m_lastProcessed = 0;
    qint64          m_lastStatMs    = 0;
};
