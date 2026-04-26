#include "mainwindow.h"
#include <QApplication>
#include <QDateTime>
#include <QScrollBar>
#include <QPainter>
#include <cmath>

// Colour palette (dark medical theme)
static const char* C_BG        = "#0D1117";   // near-black background
static const char* C_SURFACE   = "#161B22";   // card surface
static const char* C_BORDER    = "#30363D";   // subtle borders
static const char* C_ACCENT    = "#00B4D8";   // vivid teal — primary accent
static const char* C_GREEN     = "#3FB950";   // success / processed
static const char* C_ORANGE    = "#F78166";   // busy / warning
static const char* C_MUTED     = "#8B949E";   // secondary text
static const char* C_TEXT      = "#E6EDF3";   // primary text
static const char* C_SLOT_FULL = "#00B4D8";   // occupied buffer slot
static const char* C_SLOT_EMPTY= "#21262D";   // empty buffer slot

static const char* DEPT_COLOURS[NUM_DEPTS] = {
    "#F78166",   // Emergency — red-orange
    "#79C0FF",   // OPD — soft blue
    "#D2A8FF",   // Radiology — purple
    "#56D364",   // Pharmacy — green
};

//  BufferSlot
BufferSlot::BufferSlot(QWidget* parent) : QFrame(parent) {
    setFixedSize(28, 28);
    setObjectName("BufferSlot");

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setGeometry(0, 0, 28, 28);
    m_label->setStyleSheet("font-size: 8px; font-weight: bold; color: transparent;");

    setOccupied(false);
}

void BufferSlot::setOccupied(bool occupied, int patientId) {
    m_occupied = occupied;
    if (occupied) {
        setStyleSheet(QString(
            "QFrame#BufferSlot {"
            "  background: %1;"
            "  border-radius: 5px;"
            "  border: 1px solid rgba(255,255,255,0.15);"
            "}"
        ).arg(C_SLOT_FULL));
        m_label->setText(patientId > 0 ? QString::number(patientId % 100) : "");
        m_label->setStyleSheet("font-size: 7px; font-weight: bold; color: #0D1117;");
    } else {
        setStyleSheet(QString(
            "QFrame#BufferSlot {"
            "  background: %1;"
            "  border-radius: 5px;"
            "  border: 1px solid #21262D;"
            "}"
        ).arg(C_SLOT_EMPTY));
        m_label->setText("");
    }
}

void BufferSlot::flash() {
    // Quick bright flash animation
    setStyleSheet(QString(
        "QFrame#BufferSlot {"
        "  background: #FFFFFF;"
        "  border-radius: 5px;"
        "  border: 1px solid #FFFFFF;"
        "}"
    ));
    QTimer::singleShot(150, this, [this]() {
        setOccupied(m_occupied);
    });
}

//  DeptCard
DeptCard::DeptCard(int deptId, QWidget* parent)
    : QFrame(parent), m_deptId(deptId)
{
    setObjectName("DeptCard");
    setMinimumWidth(200);

    const char* dc = DEPT_COLOURS[deptId];

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(10);

    // Header row
    auto* hdr = new QHBoxLayout();
    hdr->setSpacing(8);

    // Coloured dept indicator dot
    auto* dot = new QLabel();
    dot->setFixedSize(10, 10);
    dot->setStyleSheet(QString(
        "background: %1; border-radius: 5px;"
    ).arg(dc));

    m_nameLabel = new QLabel(DEPT_NAMES[deptId]);
    m_nameLabel->setStyleSheet(QString(
        "color: %1; font-size: 13px; font-weight: 700; letter-spacing: 0.5px;"
    ).arg(dc));

    m_statusLabel = new QLabel("IDLE");
    m_statusLabel->setStyleSheet(QString(
        "color: %1; font-size: 10px; font-weight: 600;"
        "background: rgba(255,255,255,0.06); border-radius: 4px;"
        "padding: 2px 6px;"
    ).arg(C_MUTED));

    hdr->addWidget(dot);
    hdr->addWidget(m_nameLabel);
    hdr->addStretch();
    hdr->addWidget(m_statusLabel);
    root->addLayout(hdr);

    // Current patient
    m_patientLabel = new QLabel("Waiting for patient...");
    m_patientLabel->setStyleSheet(QString(
        "color: %1; font-size: 11px;"
    ).arg(C_MUTED));
    root->addWidget(m_patientLabel);

    // Buffer slots
    auto* slotSection = new QVBoxLayout();
    slotSection->setSpacing(4);
    auto* slotHdr = new QLabel("QUEUE BUFFER");
    slotHdr->setStyleSheet(QString(
        "color: %1; font-size: 9px; font-weight: 600; letter-spacing: 1px;"
    ).arg(C_MUTED));
    slotSection->addWidget(slotHdr);

    auto* slotRow = new QHBoxLayout();
    slotRow->setSpacing(4);
    for (int i = 0; i < BUFFER_SIZE; i++) {
        auto* slot = new BufferSlot(this);
        m_slots.append(slot);
        slotRow->addWidget(slot);
    }
    slotRow->addStretch();
    slotSection->addLayout(slotRow);
    root->addLayout(slotSection);

    // Processed count
    auto* footer = new QHBoxLayout();
    auto* countHdr = new QLabel("Processed:");
    countHdr->setStyleSheet(QString("color: %1; font-size: 10px;").arg(C_MUTED));
    m_countLabel = new QLabel("0");
    m_countLabel->setStyleSheet(QString(
        "color: %1; font-size: 14px; font-weight: 700;"
    ).arg(C_GREEN));
    footer->addWidget(countHdr);
    footer->addWidget(m_countLabel);
    footer->addStretch();
    root->addLayout(footer);

    setStatus(false);
}

void DeptCard::setStatus(bool busy, int patientId) {
    if (busy) {
        m_statusLabel->setText("BUSY");
        m_statusLabel->setStyleSheet(QString(
            "color: %1; font-size: 10px; font-weight: 600;"
            "background: rgba(247,129,102,0.15); border-radius: 4px;"
            "padding: 2px 6px;"
        ).arg(C_ORANGE));
        m_patientLabel->setText(
            QString("Treating Patient #%1").arg(patientId));
        m_patientLabel->setStyleSheet(QString(
            "color: %1; font-size: 11px; font-weight: 500;"
        ).arg(C_TEXT));
    } else {
        m_statusLabel->setText("IDLE");
        m_statusLabel->setStyleSheet(QString(
            "color: %1; font-size: 10px; font-weight: 600;"
            "background: rgba(255,255,255,0.06); border-radius: 4px;"
            "padding: 2px 6px;"
        ).arg(C_MUTED));
        m_patientLabel->setText("Waiting for patient...");
        m_patientLabel->setStyleSheet(QString(
            "color: %1; font-size: 11px;"
        ).arg(C_MUTED));
    }
}

void DeptCard::setProcessed(int count) {
    m_countLabel->setText(QString::number(count));
}

void DeptCard::setBufferFill(int fill) {
    m_fill = qMin(fill, BUFFER_SIZE);
    for (int i = 0; i < m_slots.size(); i++) {
        m_slots[i]->setOccupied(i < m_fill);
    }
}

void DeptCard::flashEnqueue() {
    // Flash the next free slot
    if (m_fill < m_slots.size())
        m_slots[m_fill]->flash();
}

void DeptCard::flashProcess() {
    if (m_fill > 0 && m_fill <= m_slots.size())
        m_slots[m_fill - 1]->flash();
}

//  StatCard
StatCard::StatCard(const QString& label, const QString& unit, QWidget* parent)
    : QFrame(parent)
{
    setObjectName("StatCard");
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(16, 14, 16, 14);
    lay->setSpacing(4);

    auto* lbl = new QLabel(label.toUpper());
    lbl->setStyleSheet(QString(
        "color: %1; font-size: 9px; font-weight: 700; letter-spacing: 1.5px;"
    ).arg(C_MUTED));

    m_value = new QLabel("—");
    m_value->setStyleSheet(QString(
        "color: %1; font-size: 24px; font-weight: 800;"
    ).arg(C_ACCENT));

    m_unit = new QLabel(unit);
    m_unit->setStyleSheet(QString(
        "color: %1; font-size: 10px;"
    ).arg(C_MUTED));

    lay->addWidget(lbl);
    lay->addWidget(m_value);
    lay->addWidget(m_unit);
}

void StatCard::setValue(const QString& val) {
    m_value->setText(val);
}

//  MainWindow
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    m_engine = new HospitalEngine(this);
    connect(m_engine, &HospitalEngine::patientEvent,
            this, &MainWindow::onPatientEvent);
    connect(m_engine, &HospitalEngine::statsUpdated,
            this, &MainWindow::onStatsUpdated);
    connect(m_engine, &HospitalEngine::engineStopped,
            this, &MainWindow::onEngineStopped);

    m_uptimeTimer = new QTimer(this);
    connect(m_uptimeTimer, &QTimer::timeout, this, &MainWindow::onUptimeTick);

    buildUi();
    applyStyles();
    setWindowTitle("Hospital Patient Queue — OS Simulation");
    setMinimumSize(1100, 700);
    resize(1280, 780);
}

MainWindow::~MainWindow() {
    m_engine->stop();
}

//  Build UI
void MainWindow::buildUi() {
    auto* central = new QWidget(this);
    setCentralWidget(central);

    auto* root = new QVBoxLayout(central);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Top bar
    auto* topBar = new QFrame();
    topBar->setObjectName("TopBar");
    topBar->setFixedHeight(64);
    auto* topLay = new QHBoxLayout(topBar);
    topLay->setContentsMargins(24, 0, 24, 0);

    // Status dot
    m_statusDot = new QLabel();
    m_statusDot->setFixedSize(10, 10);
    m_statusDot->setStyleSheet(
        "background: #30363D; border-radius: 5px;");

    auto* titleLbl = new QLabel("Hospital Patient Queue");
    titleLbl->setStyleSheet(
        "color: #E6EDF3; font-size: 18px; font-weight: 700; letter-spacing: 0.3px;");

    auto* subLbl = new QLabel("Producer–Consumer · Pthreads Simulation");
    subLbl->setStyleSheet(
        "color: #8B949E; font-size: 11px;");

    m_uptimeLabel = new QLabel("STOPPED");
    m_uptimeLabel->setStyleSheet(
        "color: #8B949E; font-size: 11px; font-family: monospace;");

    m_startBtn = new QPushButton("▶  Start Simulation");
    m_startBtn->setObjectName("StartBtn");
    m_startBtn->setFixedHeight(36);
    m_startBtn->setCursor(Qt::PointingHandCursor);

    auto* titleCol = new QVBoxLayout();
    titleCol->setSpacing(2);
    titleCol->addWidget(titleLbl);
    titleCol->addWidget(subLbl);

    topLay->addWidget(m_statusDot);
    topLay->addSpacing(10);
    topLay->addLayout(titleCol);
    topLay->addStretch();
    topLay->addWidget(m_uptimeLabel);
    topLay->addSpacing(20);
    topLay->addWidget(m_startBtn);
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartStop);
    root->addWidget(topBar);

    // Divider
    auto* divider = new QFrame();
    divider->setFixedHeight(1);
    divider->setStyleSheet(QString("background: %1;").arg(C_BORDER));
    root->addWidget(divider);

    // Body
    auto* body = new QHBoxLayout();
    body->setContentsMargins(20, 20, 20, 20);
    body->setSpacing(16);

    // Left column: stat cards + department cards
    auto* leftCol = new QVBoxLayout();
    leftCol->setSpacing(16);

    // Stat cards row
    auto* statsRow = new QHBoxLayout();
    statsRow->setSpacing(12);
    m_cardEnqueued    = new StatCard("Enqueued",    "patients",     this);
    m_cardProcessed   = new StatCard("Processed",   "patients",     this);
    m_cardThroughput  = new StatCard("Throughput",  "patients/min", this);
    m_cardAvgWait     = new StatCard("Avg Wait",    "ms",           this);
    statsRow->addWidget(m_cardEnqueued);
    statsRow->addWidget(m_cardProcessed);
    statsRow->addWidget(m_cardThroughput);
    statsRow->addWidget(m_cardAvgWait);
    leftCol->addLayout(statsRow);

    // Department cards in a 2x2 grid
    auto* deptGrid = new QGridLayout();
    deptGrid->setSpacing(12);
    for (int d = 0; d < NUM_DEPTS; d++) {
        m_deptCards[d] = new DeptCard(d, this);
        deptGrid->addWidget(m_deptCards[d], d / 2, d % 2);
    }
    leftCol->addLayout(deptGrid);
    leftCol->addStretch();

    body->addLayout(leftCol, 3);

    // Right column: event log
    auto* rightCol = new QVBoxLayout();
    rightCol->setSpacing(8);

    auto* logHeader = new QHBoxLayout();
    auto* logTitle = new QLabel("LIVE EVENT LOG");
    logTitle->setStyleSheet(QString(
        "color: %1; font-size: 10px; font-weight: 700; letter-spacing: 1.5px;"
    ).arg(C_MUTED));

    auto* clearBtn = new QPushButton("Clear");
    clearBtn->setObjectName("ClearBtn");
    clearBtn->setFixedHeight(24);
    clearBtn->setCursor(Qt::PointingHandCursor);
    connect(clearBtn, &QPushButton::clicked, this, [this]() { m_logView->clear(); });

    logHeader->addWidget(logTitle);
    logHeader->addStretch();
    logHeader->addWidget(clearBtn);
    rightCol->addLayout(logHeader);

    m_logView = new QTextEdit();
    m_logView->setObjectName("LogView");
    m_logView->setReadOnly(true);
    m_logView->setLineWrapMode(QTextEdit::NoWrap);
    rightCol->addWidget(m_logView, 1);

    body->addLayout(rightCol, 2);
    root->addLayout(body, 1);

    // Bottom info bar
    auto* bottomBar = new QFrame();
    bottomBar->setObjectName("BottomBar");
    bottomBar->setFixedHeight(32);
    auto* btmLay = new QHBoxLayout(bottomBar);
    btmLay->setContentsMargins(24, 0, 24, 0);

    auto* configLbl = new QLabel(
        QString("Buffer: %1 slots/dept  ·  Producers: %2 receptionists  ·  "
                "Consumers: %3 departments  ·  Sync: pthread_mutex + QSemaphore")
        .arg(BUFFER_SIZE).arg(NUM_PRODUCERS).arg(NUM_DEPTS));
    configLbl->setStyleSheet(
        "color: #484F58; font-size: 10px; font-family: monospace;");
    btmLay->addWidget(configLbl);
    btmLay->addStretch();

    auto* osLbl = new QLabel("OS Course Project  ·  FAST NUCES");
    osLbl->setStyleSheet("color: #484F58; font-size: 10px;");
    btmLay->addWidget(osLbl);

    root->addWidget(bottomBar);
}

// Stylesheet
void MainWindow::applyStyles() {
    qApp->setStyleSheet(QString(R"(
        QMainWindow, QWidget {
            background: %1;
            color: %2;
            font-family: 'Segoe UI', 'SF Pro Display', sans-serif;
        }
        QFrame#TopBar {
            background: %3;
            border-bottom: 1px solid %4;
        }
        QFrame#BottomBar {
            background: %3;
            border-top: 1px solid %4;
        }
        QFrame#DeptCard {
            background: %3;
            border: 1px solid %4;
            border-radius: 10px;
        }
        QFrame#DeptCard:hover {
            border: 1px solid #58A6FF;
        }
        QFrame#StatCard {
            background: %3;
            border: 1px solid %4;
            border-radius: 10px;
        }
        QPushButton#StartBtn {
            background: %5;
            color: #0D1117;
            border: none;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 700;
            padding: 0 20px;
        }
        QPushButton#StartBtn:hover {
            background: #48CAE4;
        }
        QPushButton#StartBtn:pressed {
            background: #0096C7;
        }
        QPushButton#ClearBtn {
            background: transparent;
            color: %6;
            border: 1px solid %4;
            border-radius: 5px;
            font-size: 10px;
            padding: 0 10px;
        }
        QPushButton#ClearBtn:hover {
            background: rgba(255,255,255,0.05);
        }
        QTextEdit#LogView {
            background: #0D1117;
            border: 1px solid %4;
            border-radius: 8px;
            color: %2;
            font-family: 'Cascadia Code', 'Consolas', 'Courier New', monospace;
            font-size: 11px;
            selection-background-color: #264F78;
            padding: 8px;
        }
        QScrollBar:vertical {
            background: %3;
            width: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: %4;
            border-radius: 4px;
            min-height: 20px;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background: %3;
            height: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:horizontal {
            background: %4;
            border-radius: 4px;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }
    )").arg(C_BG, C_TEXT, C_SURFACE, C_BORDER, C_ACCENT, C_MUTED));
}

// Slots
void MainWindow::onStartStop() {
    if (!m_running) {
        // START
        m_running    = true;
        m_startMs    = QDateTime::currentMSecsSinceEpoch();
        m_lastStatMs = m_startMs;
        m_lastProcessed = 0;

        m_startBtn->setText("■  Stop Simulation");
        m_startBtn->setStyleSheet(
            "background: #F78166; color: #0D1117; border: none; border-radius: 8px;"
            "font-size: 13px; font-weight: 700; padding: 0 20px;");
        m_statusDot->setStyleSheet(
            QString("background: %1; border-radius: 5px;").arg(C_GREEN));
        m_uptimeLabel->setStyleSheet(
            QString("color: %1; font-size: 11px; font-family: monospace;").arg(C_GREEN));

        // Reset dept cards
        for (int d = 0; d < NUM_DEPTS; d++) {
            m_deptCards[d]->setStatus(false);
            m_deptCards[d]->setProcessed(0);
            m_deptCards[d]->setBufferFill(0);
        }
        m_cardEnqueued->setValue("0");
        m_cardProcessed->setValue("0");
        m_cardThroughput->setValue("0");
        m_cardAvgWait->setValue("0");

        appendLog("<span style='color:#8B949E;'>─────────── Simulation Started ───────────</span>");

        m_uptimeTimer->start(1000);
        m_engine->start();
    } else {
        // STOP
        m_engine->stop();
        // UI reset handled in onEngineStopped
    }
}

void MainWindow::onPatientEvent(PatientEvent ev) {
    QString ts = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
    int dept   = ev.patient.dept;
    const char* dc = DEPT_COLOURS[dept];
    const char* dn = DEPT_NAMES[dept];

    if (ev.type == EventType::Enqueued) {
        // Update buffer fill
        int fill = 0;
        // approximate fill from engine stats — we track it via events
        static int fills[NUM_DEPTS] = {};
        fills[dept]++;
        fills[dept] = qMin(fills[dept], BUFFER_SIZE);
        m_deptCards[dept]->setBufferFill(fills[dept]);
        m_deptCards[dept]->flashEnqueue();

        appendLog(
            QString("<span style='color:#8B949E;'>[%1]</span>"
                    " <span style='color:#79C0FF;'>Receptionist-%2</span>"
                    " <span style='color:#8B949E;'>→ ENQUEUED</span>"
                    " <b style='color:#E6EDF3;'>Patient #%3</b>"
                    " <span style='color:%4;'>[%5]</span>")
            .arg(ts)
            .arg(ev.producerId)
            .arg(ev.patient.id)
            .arg(dc).arg(dn)
        );

    } else if (ev.type == EventType::Dequeued) {
        static int fills[NUM_DEPTS] = {};
        fills[dept] = qMax(0, fills[dept] - 1);
        m_deptCards[dept]->setBufferFill(fills[dept]);
        m_deptCards[dept]->setStatus(true, ev.patient.id);

        appendLog(
            QString("<span style='color:#8B949E;'>[%1]</span>"
                    " <span style='color:%2;'>%3</span>"
                    " <span style='color:#8B949E;'>→ TREATING</span>"
                    " <b style='color:#E6EDF3;'>Patient #%4</b>"
                    " <span style='color:#8B949E;'>wait=%5ms</span>")
            .arg(ts).arg(dc).arg(dn)
            .arg(ev.patient.id).arg(ev.waitMs)
        );

    } else if (ev.type == EventType::Processed) {
        m_deptCards[dept]->setStatus(false);
        m_deptCards[dept]->setProcessed(m_engine->deptProcessed(dept));
        m_deptCards[dept]->flashProcess();

        appendLog(
            QString("<span style='color:#8B949E;'>[%1]</span>"
                    " <span style='color:%2;'>%3</span>"
                    " <span style='color:#3FB950;'>✓ DONE</span>"
                    " <b style='color:#E6EDF3;'>Patient #%4</b>"
                    " <span style='color:#8B949E;'>proc=%5ms</span>")
            .arg(ts).arg(dc).arg(dn)
            .arg(ev.patient.id).arg(ev.procMs)
        );
    }
}

void MainWindow::onStatsUpdated(int enqueued, int processed, double avgWait,
                                int e, int o, int r, int p)
{
    m_cardEnqueued->setValue(QString::number(enqueued));
    m_cardProcessed->setValue(QString::number(processed));
    m_cardAvgWait->setValue(QString::number((int)avgWait));

    // Throughput: processed since last update / time since last update
    qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    qint64 dtMs  = nowMs - m_lastStatMs;
    int    dProc = processed - m_lastProcessed;
    double tp = (dtMs > 0) ? (dProc * 60000.0 / dtMs) : 0.0;
    m_cardThroughput->setValue(QString::number((int)tp));
    m_lastStatMs    = nowMs;
    m_lastProcessed = processed;

    m_deptCards[0]->setProcessed(e);
    m_deptCards[1]->setProcessed(o);
    m_deptCards[2]->setProcessed(r);
    m_deptCards[3]->setProcessed(p);
}

void MainWindow::onEngineStopped() {
    m_running = false;
    m_uptimeTimer->stop();

    m_startBtn->setText("▶  Start Simulation");
    m_startBtn->setStyleSheet("");  // revert to stylesheet default
    m_statusDot->setStyleSheet(
        "background: #30363D; border-radius: 5px;");
    m_uptimeLabel->setText("STOPPED");
    m_uptimeLabel->setStyleSheet(
        "color: #8B949E; font-size: 11px; font-family: monospace;");

    appendLog("<span style='color:#8B949E;'>─────────── Simulation Stopped ───────────</span>");
}

void MainWindow::onUptimeTick() {
    qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_startMs;
    int s = (elapsed / 1000) % 60;
    int m = (elapsed / 60000) % 60;
    int h = elapsed / 3600000;
    m_uptimeLabel->setText(
        QString("RUNNING  %1:%2:%3")
        .arg(h, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(s, 2, 10, QChar('0'))
    );
}

void MainWindow::appendLog(const QString& html) {
    m_logView->append(html);
    // Auto-scroll to bottom
    auto* sb = m_logView->verticalScrollBar();
    sb->setValue(sb->maximum());
}
