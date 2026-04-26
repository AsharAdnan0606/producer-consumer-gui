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