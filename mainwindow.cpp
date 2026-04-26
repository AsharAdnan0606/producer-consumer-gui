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