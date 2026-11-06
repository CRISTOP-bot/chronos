// mainwindow.cpp — CHRONOS GUI
// Improved version: fixes eventTable_ double-instantiation bug, populates nav tree,
// adds visual timeline ruler, richer inspector, splitter-based memory layout,
// keyboard navigation, and shortcut keys.

#include "chronos/gui/mainwindow.h"
#include "chronos/analysis/state_diff.h"
#include "chronos/memory/strings.h"
#include "chronos/instrumentation/ptrace_tracer.h"
#include "chronos/recording/recording_engine.h"

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsView>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QPen>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QShortcut>
#include <QSlider>
#include <QSplitter>
#include <QStackedWidget>
#include <QStatusBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QSizePolicy>
#include <algorithm>
#include <set>
#include <utility>
#include <vector>

using namespace chronos;
using namespace chronos::gui;

// ---------------------------------------------------------------------------
// TimelineRuler: custom widget that draws events as colored ticks on a ruler
// ---------------------------------------------------------------------------
class TimelineRuler : public QWidget {
    Q_OBJECT
public:
    explicit TimelineRuler(QWidget* parent = nullptr) : QWidget(parent) {
        setMinimumHeight(56);
        setMaximumHeight(56);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }

    void setEvents(const std::vector<Event>& events) {
        events_ = &events;
        update();
    }

    void setPosition(int pos) {
        pos_ = pos;
        update();
    }

    // pos ranges 0..total
    int positionAt(int x) const {
        if (!events_ || events_->empty()) return 0;
        int n = static_cast<int>(events_->size());
        int w = width() - 2 * kPad;
        if (w <= 0) return 0;
        int p = static_cast<int>(static_cast<double>(x - kPad) / w * n + 0.5);
        return std::clamp(p, 0, n);
    }

signals:
    void positionClicked(int pos);

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        // background
        p.fillRect(rect(), QColor(0x12, 0x14, 0x18));

        if (!events_ || events_->empty()) {
            p.setPen(QColor(0x40, 0x45, 0x50));
            p.drawText(rect(), Qt::AlignCenter, "No recording loaded");
            return;
        }

        const int n = static_cast<int>(events_->size());
        const int w = width() - 2 * kPad;
        const int h = height();

        // tick lane
        const int laneY = h / 2;

        // baseline
        p.setPen(QPen(QColor(0x2a, 0x2e, 0x36), 1));
        p.drawLine(kPad, laneY, kPad + w, laneY);

        // draw one tick per event, colored by type
        for (int i = 0; i < n; ++i) {
            const auto& e = (*events_)[i];
            int x = kPad + static_cast<int>(static_cast<double>(i) / n * w);
            QColor col = colorForType(e.type);
            p.setPen(QPen(col, 1));
            int tickH = (e.type == EventType::Checkpoint) ? 10
                      : (e.type == EventType::SyscallEnter || e.type == EventType::SyscallExit) ? 5
                      : 3;
            p.drawLine(x, laneY - tickH, x, laneY + tickH);
        }

        // current position needle
        if (pos_ > 0 && pos_ <= n) {
            int nx = kPad + static_cast<int>(static_cast<double>(pos_ - 1) / n * w);
            p.setPen(QPen(QColor(0xff, 0xff, 0xff, 200), 2));
            p.drawLine(nx, 4, nx, h - 4);
            // position label
            p.setPen(QColor(0xdc, 0xe0, 0xe8));
            p.setFont(QFont("Monospace", 8));
            QString lbl = QString::number(pos_) + "/" + QString::number(n);
            QRect lr(nx + 4, 4, 80, 14);
            if (nx + 84 > width()) lr = QRect(nx - 84, 4, 80, 14);
            p.drawText(lr, Qt::AlignLeft | Qt::AlignVCenter, lbl);
        }

        // legend at bottom
        p.setFont(QFont("sans-serif", 7));
        int lx = kPad;
        auto drawLegend = [&](const QString& label, QColor c) {
            p.setPen(c); p.drawLine(lx, h - 6, lx + 12, h - 6);
            p.setPen(QColor(0x9a, 0xa0, 0xa6));
            p.drawText(lx + 14, h - 2, label);
            lx += 14 + p.fontMetrics().horizontalAdvance(label) + 10;
        };
        drawLegend("Syscall",    QColor(0x4c, 0xaf, 0x50));
        drawLegend("Checkpoint", QColor(0xff, 0xb3, 0x00));
        drawLegend("Function",   QColor(0x64, 0xb5, 0xf6));
        drawLegend("Variable",   QColor(0xce, 0x93, 0xd8));
        drawLegend("Memory",     QColor(0xef, 0x9a, 0x9a));
        drawLegend("Other",      QColor(0x60, 0x65, 0x70));
    }

    void mousePressEvent(QMouseEvent* ev) override {
        emit positionClicked(positionAt(ev->pos().x()));
    }
    void mouseMoveEvent(QMouseEvent* ev) override {
        if (ev->buttons() & Qt::LeftButton)
            emit positionClicked(positionAt(ev->pos().x()));
    }

private:
    static QColor colorForType(EventType t) {
        switch (t) {
            case EventType::SyscallEnter:
            case EventType::SyscallExit:      return QColor(0x4c, 0xaf, 0x50);
            case EventType::Checkpoint:       return QColor(0xff, 0xb3, 0x00);
            case EventType::FunctionEnter:
            case EventType::FunctionExit:     return QColor(0x64, 0xb5, 0xf6);
            case EventType::VariableChange:   return QColor(0xce, 0x93, 0xd8);
            case EventType::MemoryWrite:
            case EventType::MemoryAllocate:
            case EventType::MemoryFree:       return QColor(0xef, 0x9a, 0x9a);
            default:                          return QColor(0x60, 0x65, 0x70);
        }
    }

    const std::vector<Event>* events_ = nullptr;
    int pos_ = 0;
    static constexpr int kPad = 8;
};

#include "mainwindow.moc"

// ---------------------------------------------------------------------------
// MainWindow
// ---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("CHRONOS — Temporal Execution & Reconstruction Engine");
    resize(1440, 900);
    buildUi();
    applyDarkTheme();
}

// ── helpers ─────────────────────────────────────────────────────────────────

static QColor eventRowColor(EventType t) {
    switch (t) {
        case EventType::SyscallEnter:
        case EventType::SyscallExit:      return QColor(0x1a, 0x28, 0x1a);
        case EventType::Checkpoint:       return QColor(0x2a, 0x24, 0x10);
        case EventType::FunctionEnter:
        case EventType::FunctionExit:     return QColor(0x12, 0x1e, 0x2e);
        case EventType::VariableChange:   return QColor(0x22, 0x18, 0x28);
        case EventType::MemoryWrite:
        case EventType::MemoryAllocate:
        case EventType::MemoryFree:       return QColor(0x28, 0x18, 0x18);
        default:                          return QColor(0, 0, 0, 0);
    }
}

static QString eventDetail(const Event& e) {
    if (e.type == EventType::SyscallEnter || e.type == EventType::SyscallExit) {
        const auto& s = std::get<SyscallEvent>(e.payload);
        QString d = QString::fromStdString(s.name);
        if (e.type == EventType::SyscallExit && !s.result.empty())
            d += " → " + QString::fromStdString(s.result);
        return d;
    }
    if (e.type == EventType::FunctionEnter || e.type == EventType::FunctionExit)
        return QString::fromStdString(std::get<FunctionEvent>(e.payload).name);
    if (e.type == EventType::VariableChange) {
        const auto& v = std::get<VariableEvent>(e.payload);
        return QString::fromStdString(v.name + " = " + v.new_value);
    }
    if (e.type == EventType::Checkpoint)
        return QString::fromStdString(std::get<CheckpointEvent>(e.payload).label);
    if (e.type == EventType::Marker) {
        const auto& m = std::get<MarkerEvent>(e.payload);
        return QString::fromStdString(m.label + (m.note.empty() ? "" : ": " + m.note));
    }
    if (e.type == EventType::ExceptionThrown || e.type == EventType::ExceptionCaught) {
        const auto& ex = std::get<ExceptionEvent>(e.payload);
        return QString::fromStdString(ex.type_name + ": " + ex.message);
    }
    if (e.type == EventType::ProcessStart || e.type == EventType::ProcessExit ||
        e.type == EventType::ThreadCreate  || e.type == EventType::ThreadExit) {
        return QString::fromStdString(std::get<ProcessEvent>(e.payload).detail);
    }
    return {};
}

// ── buildUi ─────────────────────────────────────────────────────────────────

void MainWindow::buildUi() {
    setDockNestingEnabled(true);

    // ── Menus ──────────────────────────────────────────────────────────────
    auto* mb = new QMenuBar(this);

    auto* fileMenu = mb->addMenu("&File");
    auto* openAct  = fileMenu->addAction("&Open Recording…");
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::openRecording);

    auto* traceAct = fileMenu->addAction("&Trace Binary…");
    traceAct->setShortcut(QKeySequence("Ctrl+T"));
    connect(traceAct, &QAction::triggered, this, [this]() {
        QString bin = QFileDialog::getOpenFileName(this, "Select Binary", {}, "All Files (*)");
        if (bin.isEmpty()) return;
        QString out = QFileDialog::getSaveFileName(this, "Save Recording", {}, "CHRONOS (*.chronos)");
        if (out.isEmpty()) return;
        recording::RecordingEngine eng;
        recording::SessionConfig cfg{"trace", bin.toStdString(), 1};
        auto sess = eng.create_session(cfg);
        instrumentation::TraceConfig tcfg{bin.toStdString(), {}, out.toStdString()};
        instrumentation::PtraceTracer tracer;
        auto res = tracer.trace(tcfg, *sess);
        if (!res.ok()) { QMessageBox::critical(this, "Trace failed",
            QString::fromStdString(res.error().message)); return; }
        auto save = sess->save(out.toStdString());
        if (!save.ok()) { QMessageBox::critical(this, "Save failed",
            QString::fromStdString(save.error().message)); return; }
        loadRecording(out);
        statusBar()->showMessage(QString("Traced %1 events → %2").arg(res.value()).arg(out));
    });

    fileMenu->addSeparator();
    auto* exportHtmlAct = fileMenu->addAction("Export HTML Report…");
    connect(exportHtmlAct, &QAction::triggered, this, [this]() {
        if (recording_.events.empty()) return;
        QString path = QFileDialog::getSaveFileName(this, "Export HTML", {}, "HTML (*.html)");
        if (path.isEmpty()) return;
        // delegate to CLI logic inline — just emit status
        statusBar()->showMessage("Use: chronos_cli export-html <recording> " + path);
    });
    auto* exportJsonAct = fileMenu->addAction("Export JSON…");
    connect(exportJsonAct, &QAction::triggered, this, [this]() {
        if (recording_.events.empty()) return;
        QString path = QFileDialog::getSaveFileName(this, "Export JSON", {}, "JSON (*.json)");
        if (path.isEmpty()) return;
        statusBar()->showMessage("Use: chronos_cli export-json <recording> " + path);
    });
    fileMenu->addSeparator();
    auto* quitAct = fileMenu->addAction("&Quit");
    quitAct->setShortcut(QKeySequence::Quit);
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);

    auto* navMenu = mb->addMenu("&Navigate");
    auto* prevEvAct = navMenu->addAction("← Previous Event");
    prevEvAct->setShortcut(Qt::Key_Left);
    connect(prevEvAct, &QAction::triggered, this, &MainWindow::prevEvent);
    auto* nextEvAct = navMenu->addAction("→ Next Event");
    nextEvAct->setShortcut(Qt::Key_Right);
    connect(nextEvAct, &QAction::triggered, this, &MainWindow::nextEvent);
    navMenu->addSeparator();
    auto* prevCpAct = navMenu->addAction("↑ Previous Checkpoint");
    prevCpAct->setShortcut(Qt::Key_Up);
    connect(prevCpAct, &QAction::triggered, this, &MainWindow::prevCheckpoint);
    auto* nextCpAct = navMenu->addAction("↓ Next Checkpoint");
    nextCpAct->setShortcut(Qt::Key_Down);
    connect(nextCpAct, &QAction::triggered, this, &MainWindow::nextCheckpoint);

    auto* analysisMenu = mb->addMenu("&Analysis");
    auto* sdAct  = analysisMenu->addAction("State Diff");
    sdAct->setShortcut(QKeySequence("Ctrl+D"));
    connect(sdAct,  &QAction::triggered, this, &MainWindow::runStateDiff);
    auto* srAct  = analysisMenu->addAction("Search…");
    srAct->setShortcut(QKeySequence::Find);
    connect(srAct,  &QAction::triggered, this, &MainWindow::doSearch);
    auto* grAct  = analysisMenu->addAction("Exec Graph…");
    grAct->setShortcut(QKeySequence("Ctrl+G"));
    connect(grAct,  &QAction::triggered, this, &MainWindow::showExecGraph);

    auto* viewMenu = mb->addMenu("&View");
    viewMenu->addAction("Execution Graph…", this, &MainWindow::showExecGraph);

    auto* helpMenu = mb->addMenu("&Help");
    helpMenu->addAction("About CHRONOS", this, [this]() {
        QMessageBox::about(this, "About CHRONOS",
            "<b>CHRONOS 0.1.0</b><br>"
            "Temporal Execution &amp; Reconstruction Engine<br><br>"
            "Record, reconstruct and explore program executions over time.<br>"
            "Platform: Linux x86_64 &nbsp;|&nbsp; C++20 &nbsp;|&nbsp; Qt6");
    });
    setMenuBar(mb);

    // ── Toolbar ────────────────────────────────────────────────────────────
    auto* toolbar = new QToolBar("Main", this);
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->setIconSize(QSize(16, 16));

    auto addTbAct = [&](const QString& text, const QString& tooltip,
                        const char* slot, const QKeySequence& key = {}) -> QAction* {
        auto* a = toolbar->addAction(text);
        a->setToolTip(tooltip + (key.isEmpty() ? "" : "  [" + key.toString() + "]"));
        if (!key.isEmpty()) a->setShortcut(key);
        connect(a, SIGNAL(triggered()), this, slot);
        return a;
    };

    addTbAct("Open",     "Open a .chronos recording", SLOT(openRecording()), QKeySequence::Open);
    toolbar->addSeparator();
    addTbAct("◀ Prev",  "Previous event",   SLOT(prevEvent()),       Qt::Key_Left);
    addTbAct("Next ▶",  "Next event",        SLOT(nextEvent()),       Qt::Key_Right);
    toolbar->addSeparator();
    addTbAct("⬆ CP",    "Previous checkpoint", SLOT(prevCheckpoint()), Qt::Key_Up);
    addTbAct("CP ⬇",   "Next checkpoint",    SLOT(nextCheckpoint()),  Qt::Key_Down);
    toolbar->addSeparator();
    addTbAct("Search",  "Search events",     SLOT(doSearch()),         QKeySequence::Find);
    addTbAct("Diff",    "State diff",        SLOT(runStateDiff()),     QKeySequence("Ctrl+D"));

    // stretch spacer
    auto* spacer = new QWidget(this);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    statusLabel_ = new QLabel("No recording loaded", this);
    statusLabel_->setObjectName("statusLabel");
    toolbar->addWidget(statusLabel_);
    addToolBar(toolbar);

    // ── Timeline ruler (full-width, above central widget) ─────────────────
    timelineRuler_ = new TimelineRuler(this);
    connect(timelineRuler_, &TimelineRuler::positionClicked, this, [this](int pos) {
        if (pos <= 0 || recording_.events.empty()) return;
        int row = std::min(pos - 1, static_cast<int>(recording_.events.size()) - 1);
        selectRow(row);
    });

    timelineSlider_ = new QSlider(Qt::Horizontal, this);
    timelineSlider_->setRange(0, 0);
    timelineSlider_->setTickPosition(QSlider::NoTicks);
    connect(timelineSlider_, &QSlider::valueChanged, this, &MainWindow::onTimelineChanged);

    auto* timelineWidget = new QWidget(this);
    auto* timelineLayout = new QVBoxLayout(timelineWidget);
    timelineLayout->setContentsMargins(4, 2, 4, 0);
    timelineLayout->setSpacing(2);
    timelineLayout->addWidget(timelineRuler_);
    timelineLayout->addWidget(timelineSlider_);

    // ── Central: event table ───────────────────────────────────────────────
    // Single instance — fixes the double-instantiation bug
    eventTable_ = new QTableWidget(this);
    eventTable_->setColumnCount(5);
    eventTable_->setHorizontalHeaderLabels({"#", "Type", "PID", "TID", "Detail"});
    eventTable_->horizontalHeader()->setStretchLastSection(true);
    eventTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    eventTable_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Interactive);
    eventTable_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    eventTable_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    eventTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    eventTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    eventTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    eventTable_->setAlternatingRowColors(false); // we color by event type instead
    eventTable_->verticalHeader()->hide();
    eventTable_->setShowGrid(false);
    eventTable_->setSortingEnabled(false);
    connect(eventTable_, &QTableWidget::cellClicked,    this, &MainWindow::onEventSelected);
    connect(eventTable_, &QTableWidget::currentCellChanged, this,
            [this](int row, int, int, int) { if (row >= 0) onEventSelected(row); });

    // Central layout: timeline on top, table below
    auto* centralWidget = new QWidget(this);
    auto* centralLayout = new QVBoxLayout(centralWidget);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(timelineWidget);
    centralLayout->addWidget(eventTable_, 1);
    setCentralWidget(centralWidget);

    // ── Left dock: Navigation tree ─────────────────────────────────────────
    auto* navDock = new QDockWidget("Navigation", this);
    navDock->setObjectName("navDock");
    navDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    navTree_ = new QTreeWidget(this);
    navTree_->setHeaderHidden(true);
    navTree_->setIndentation(14);
    navTree_->setAnimated(true);
    navTreeProcesses_   = new QTreeWidgetItem(navTree_,    {"Processes"});
    navTreeThreads_     = new QTreeWidgetItem(navTree_,    {"Threads"});
    navTreeCheckpoints_ = new QTreeWidgetItem(navTree_,    {"Checkpoints"});
    navTreeSyscalls_    = new QTreeWidgetItem(navTree_,    {"Syscalls"});
    navTreeFunctions_   = new QTreeWidgetItem(navTree_,    {"Functions"});
    navTreeMemRegions_  = new QTreeWidgetItem(navTree_,    {"Memory Regions"});
    for (auto* item : {navTreeProcesses_, navTreeThreads_, navTreeCheckpoints_,
                       navTreeSyscalls_,  navTreeFunctions_, navTreeMemRegions_})
        item->setExpanded(false);
    // Click on a leaf → jump to event
    connect(navTree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        bool ok = false;
        int row = item->data(0, Qt::UserRole).toInt(&ok);
        if (ok && row >= 0) selectRow(row);
    });
    navDock->setWidget(navTree_);
    navDock->setMinimumWidth(180);
    addDockWidget(Qt::LeftDockWidgetArea, navDock);

    // ── Right dock: Inspector ──────────────────────────────────────────────
    auto* inspDock = new QDockWidget("Inspector", this);
    inspDock->setObjectName("inspDock");
    inspDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* inspWidget = new QWidget(this);
    auto* inspLayout = new QVBoxLayout(inspWidget);
    inspLayout->setContentsMargins(4, 4, 4, 4);
    inspLayout->setSpacing(6);

    // Event type badge + index
    inspBadge_ = new QLabel(this);
    inspBadge_->setObjectName("inspBadge");
    inspBadge_->setAlignment(Qt::AlignCenter);
    inspBadge_->setFixedHeight(28);
    inspLayout->addWidget(inspBadge_);

    // Key-value property table
    inspTable_ = new QTableWidget(0, 2, this);
    inspTable_->setObjectName("inspTable");
    inspTable_->setHorizontalHeaderLabels({"Property", "Value"});
    inspTable_->horizontalHeader()->setStretchLastSection(true);
    inspTable_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    inspTable_->verticalHeader()->hide();
    inspTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    inspTable_->setSelectionMode(QAbstractItemView::SingleSelection);
    inspTable_->setShowGrid(false);
    inspTable_->setAlternatingRowColors(true);
    inspLayout->addWidget(inspTable_, 1);

    // Call stack
    auto* csLabel = new QLabel("Call Stack", this);
    csLabel->setObjectName("sectionLabel");
    inspLayout->addWidget(csLabel);
    callStackTree_ = new QTreeWidget(this);
    callStackTree_->setHeaderLabels({"Function", "Address"});
    callStackTree_->header()->setStretchLastSection(true);
    callStackTree_->setMaximumHeight(120);
    callStackTree_->setRootIsDecorated(false);
    inspLayout->addWidget(callStackTree_);

    inspDock->setWidget(inspWidget);
    inspDock->setMinimumWidth(220);
    addDockWidget(Qt::RightDockWidgetArea, inspDock);

    // ── Bottom dock: tabbed output area ───────────────────────────────────
    auto* outDock = new QDockWidget("Output", this);
    outDock->setObjectName("outDock");
    auto* outTabs = new QTabWidget(this);
    outTabs->setTabPosition(QTabWidget::South);

    // Search tab
    auto* searchTab = new QWidget(this);
    auto* searchLayout = new QVBoxLayout(searchTab);
    searchLayout->setContentsMargins(4, 4, 4, 4);
    auto* searchBar = new QHBoxLayout();
    searchEdit_ = new QLineEdit(this);
    searchEdit_->setPlaceholderText("Search events (text, type, syscall name…)");
    searchEdit_->setClearButtonEnabled(true);
    auto* searchBtn = new QPushButton("Find", this);
    searchBtn->setDefault(true);
    searchBar->addWidget(searchEdit_, 1);
    searchBar->addWidget(searchBtn);
    searchLayout->addLayout(searchBar);
    searchResults_ = new QListWidget(this);
    searchResults_->setAlternatingRowColors(true);
    searchLayout->addWidget(searchResults_, 1);
    connect(searchEdit_, &QLineEdit::returnPressed,   this, &MainWindow::doSearch);
    connect(searchBtn,   &QPushButton::clicked,       this, &MainWindow::doSearch);
    connect(searchResults_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        bool ok = false;
        int row = item->data(Qt::UserRole).toInt(&ok);
        if (ok && row >= 0) selectRow(row);
    });
    outTabs->addTab(searchTab, "Search");

    // Memory tab
    auto* memTab = new QWidget(this);
    auto* memLayout = new QVBoxLayout(memTab);
    memLayout->setContentsMargins(4, 4, 4, 4);
    auto* memTop = new QHBoxLayout();
    auto* cpLabel = new QLabel("Checkpoint:", this);
    checkpointCombo_ = new QComboBox(this);
    checkpointCombo_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    memTop->addWidget(cpLabel);
    memTop->addWidget(checkpointCombo_, 1);
    memLayout->addLayout(memTop);

    auto* memSplitter = new QSplitter(Qt::Vertical, this);
    memoryTree_ = new QTreeWidget(this);
    memoryTree_->setHeaderLabels({"Base", "Size (B)", "Perms", "Name"});
    memoryTree_->header()->setStretchLastSection(true);
    memoryTree_->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    memoryTree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    memoryTree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    memoryTree_->setRootIsDecorated(false);
    memSplitter->addWidget(memoryTree_);

    auto* hexStrSplitter = new QSplitter(Qt::Horizontal, this);
    hexView_ = new QPlainTextEdit(this);
    hexView_->setReadOnly(true);
    hexView_->setFont(QFont("Monospace", 9));
    hexView_->setLineWrapMode(QPlainTextEdit::NoWrap);
    hexStrSplitter->addWidget(hexView_);

    auto* stringsPanel = new QWidget(this);
    auto* stringsLayout = new QVBoxLayout(stringsPanel);
    stringsLayout->setContentsMargins(0, 0, 0, 0);
    stringsLayout->setSpacing(2);
    stringsFilter_ = new QLineEdit(this);
    stringsFilter_->setPlaceholderText("Filter strings…");
    stringsFilter_->setClearButtonEnabled(true);
    stringsLayout->addWidget(stringsFilter_);
    stringsView_ = new QPlainTextEdit(this);
    stringsView_->setReadOnly(true);
    stringsView_->setFont(QFont("Monospace", 9));
    stringsLayout->addWidget(stringsView_, 1);
    hexStrSplitter->addWidget(stringsPanel);
    hexStrSplitter->setSizes({300, 200});
    memSplitter->addWidget(hexStrSplitter);
    memSplitter->setSizes({200, 200});
    memLayout->addWidget(memSplitter, 1);
    outTabs->addTab(memTab, "Memory");

    // State Diff tab
    auto* diffTab = new QWidget(this);
    auto* diffLayout = new QVBoxLayout(diffTab);
    diffLayout->setContentsMargins(4, 4, 4, 4);
    auto* diffBar = new QHBoxLayout();
    diffAButton_ = new QPushButton("Set A", this);
    diffBButton_ = new QPushButton("Set B", this);
    diffButton_  = new QPushButton("Compare →", this);
    diffButton_->setDefault(true);
    diffBar->addWidget(diffAButton_);
    diffBar->addWidget(diffBButton_);
    diffBar->addStretch(1);
    diffBar->addWidget(diffButton_);
    diffLayout->addLayout(diffBar);
    diffEdit_ = new QPlainTextEdit(this);
    diffEdit_->setReadOnly(true);
    diffEdit_->setFont(QFont("Monospace", 10));
    diffLayout->addWidget(diffEdit_, 1);
    connect(diffAButton_, &QPushButton::clicked, this, &MainWindow::setDiffA);
    connect(diffBButton_, &QPushButton::clicked, this, &MainWindow::setDiffB);
    connect(diffButton_,  &QPushButton::clicked, this, &MainWindow::runStateDiff);
    outTabs->addTab(diffTab, "State Diff");

    // Exec Graph tab
    auto* graphTab = new QWidget(this);
    auto* graphLayout = new QVBoxLayout(graphTab);
    graphLayout->setContentsMargins(4, 4, 4, 4);
    execGraphTree_ = new QTreeWidget(this);
    execGraphTree_->setHeaderLabels({"Function", "Event #", "Depth"});
    execGraphTree_->header()->setStretchLastSection(false);
    execGraphTree_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    execGraphTree_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    execGraphTree_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    auto* graphBtn = new QPushButton("Open Full Graph Window…", this);
    connect(graphBtn, &QPushButton::clicked, this, &MainWindow::showExecGraph);
    graphLayout->addWidget(graphBtn);
    graphLayout->addWidget(execGraphTree_, 1);
    // Click on function node → jump to that event
    connect(execGraphTree_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item, int) {
        bool ok = false;
        int row = item->data(0, Qt::UserRole).toInt(&ok);
        if (ok && row >= 0) selectRow(row - 1);
    });
    outTabs->addTab(graphTab, "Exec Graph");

    outDock->setWidget(outTabs);
    addDockWidget(Qt::BottomDockWidgetArea, outDock);
    resizeDocks({outDock}, {260}, Qt::Vertical);

    // ── Checkpoint combo connections ───────────────────────────────────────
    connect(checkpointCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onCheckpointSelected);

    // ── Memory tree → hex view ─────────────────────────────────────────────
    connect(memoryTree_, &QTreeWidget::itemSelectionChanged, this, [this]() {
        auto items = memoryTree_->selectedItems();
        if (items.isEmpty()) return;
        QByteArray ba = items.first()->data(0, Qt::UserRole).toByteArray();
        hexView_->setPlainText(buildHexDump(ba));
    });

    // ── Strings filter ─────────────────────────────────────────────────────
    connect(stringsFilter_, &QLineEdit::textChanged, this, [this](const QString& txt) {
        QString text;
        for (const auto& s : currentStrings_) {
            if (!txt.isEmpty() &&
                QString::fromStdString(s).indexOf(txt, 0, Qt::CaseInsensitive) < 0) continue;
            text += QString::fromStdString(s) + "\n";
        }
        stringsView_->setPlainText(text);
    });

    // ── Status bar ─────────────────────────────────────────────────────────
    statusBar()->showMessage("Ready — open a .chronos recording to begin");
}

// ── applyDarkTheme ──────────────────────────────────────────────────────────

void MainWindow::applyDarkTheme() {
    qApp->setStyle("Fusion");

    QPalette p;
    p.setColor(QPalette::Window,          QColor(0x18, 0x1a, 0x1e));
    p.setColor(QPalette::WindowText,      QColor(0xdc, 0xe0, 0xe8));
    p.setColor(QPalette::Base,            QColor(0x12, 0x14, 0x18));
    p.setColor(QPalette::AlternateBase,   QColor(0x1e, 0x20, 0x26));
    p.setColor(QPalette::Text,            QColor(0xdc, 0xe0, 0xe8));
    p.setColor(QPalette::Button,          QColor(0x24, 0x28, 0x2e));
    p.setColor(QPalette::ButtonText,      QColor(0xdc, 0xe0, 0xe8));
    p.setColor(QPalette::Highlight,       QColor(0x46, 0x82, 0xb4));
    p.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
    p.setColor(QPalette::Link,            QColor(0x64, 0xb5, 0xf6));
    p.setColor(QPalette::Midlight,        QColor(0x2a, 0x2e, 0x36));
    p.setColor(QPalette::Mid,             QColor(0x20, 0x24, 0x2a));
    p.setColor(QPalette::Dark,            QColor(0x14, 0x16, 0x1a));
    qApp->setPalette(p);

    setStyleSheet(R"(
        QWidget {
            font-family: 'Inter', 'DejaVu Sans', 'Segoe UI', sans-serif;
            font-size: 13px;
        }
        /* ─ tables & trees ─ */
        QTableWidget, QTreeWidget {
            background: #12141800;
            border: none;
            gridline-color: transparent;
            outline: none;
        }
        QTableWidget::item, QTreeWidget::item {
            padding: 3px 6px;
            border: none;
        }
        QTableWidget::item:selected, QTreeWidget::item:selected {
            background: #2a3a4a;
            color: #ffffff;
        }
        QHeaderView {
            background: #1e2228;
            border: none;
        }
        QHeaderView::section {
            background: #1e2228;
            color: #9aa0a6;
            padding: 5px 8px;
            border: none;
            border-right: 1px solid #2a2e36;
            font-size: 11px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        QHeaderView::section:last { border-right: none; }
        /* ─ plain text ─ */
        QPlainTextEdit {
            background: #0e1014;
            color: #c8ccd4;
            border: 1px solid #2a2e36;
            border-radius: 4px;
            selection-background-color: #2a3a4a;
        }
        /* ─ line edit ─ */
        QLineEdit {
            background: #1a1e24;
            color: #dce0e8;
            border: 1px solid #3a3f47;
            border-radius: 4px;
            padding: 4px 8px;
            selection-background-color: #2a3a4a;
        }
        QLineEdit:focus { border-color: #4682b4; }
        /* ─ combo ─ */
        QComboBox {
            background: #1a1e24;
            color: #dce0e8;
            border: 1px solid #3a3f47;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QComboBox:focus  { border-color: #4682b4; }
        QComboBox::drop-down { border: none; }
        QComboBox QAbstractItemView {
            background: #1e2228;
            border: 1px solid #2a2e36;
            selection-background-color: #2a3a4a;
        }
        /* ─ buttons ─ */
        QPushButton {
            background: #24282e;
            color: #dce0e8;
            border: 1px solid #3a3f47;
            border-radius: 5px;
            padding: 6px 14px;
            min-width: 48px;
        }
        QPushButton:hover   { background: #2e333b; border-color: #4682b4; }
        QPushButton:pressed { background: #1a1e24; }
        QPushButton:default { border-color: #4682b4; }
        /* ─ slider ─ */
        QSlider::groove:horizontal {
            height: 4px; background: #2a2e36; border-radius: 2px;
        }
        QSlider::handle:horizontal {
            background: #4682b4; width: 14px; height: 14px;
            margin: -5px 0; border-radius: 7px;
        }
        QSlider::handle:horizontal:hover { background: #64b5f6; }
        QSlider::sub-page:horizontal { background: #4682b4; border-radius: 2px; }
        /* ─ tabs ─ */
        QTabWidget::pane {
            border: 1px solid #2a2e36;
            background: #12141800;
        }
        QTabBar::tab {
            background: #14161a;
            color: #9aa0a6;
            padding: 6px 14px;
            border: 1px solid #2a2e36;
            border-bottom: none;
            border-top-left-radius:  5px;
            border-top-right-radius: 5px;
            margin-right: 2px;
        }
        QTabBar::tab:selected { background: #1e2228; color: #dce0e8; border-bottom: 2px solid #4682b4; }
        QTabBar::tab:hover    { color: #dce0e8; }
        QTabBar[tabPosition="3"]::tab { /* South */
            border-bottom: 1px solid #2a2e36;
            border-top: none;
            border-bottom-left-radius:  5px;
            border-bottom-right-radius: 5px;
        }
        QTabBar[tabPosition="3"]::tab:selected { border-top: 2px solid #4682b4; border-bottom: 1px solid #2a2e36; }
        /* ─ docks ─ */
        QDockWidget {
            color: #9aa0a6;
            font-size: 11px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
        }
        QDockWidget::title {
            background: #14161a;
            padding: 5px 8px;
            border-bottom: 1px solid #2a2e36;
        }
        /* ─ menus ─ */
        QMenuBar {
            background: #14161a;
            color: #dce0e8;
            border-bottom: 1px solid #2a2e36;
            spacing: 2px;
        }
        QMenuBar::item         { padding: 5px 10px; border-radius: 3px; }
        QMenuBar::item:selected { background: #24282e; }
        QMenu {
            background: #1e2228;
            color: #dce0e8;
            border: 1px solid #2a2e36;
            padding: 4px 0;
        }
        QMenu::item          { padding: 6px 24px 6px 16px; }
        QMenu::item:selected  { background: #2a3a4a; }
        QMenu::separator     { height: 1px; background: #2a2e36; margin: 4px 0; }
        /* ─ toolbar ─ */
        QToolBar {
            background: #14161a;
            border-bottom: 1px solid #2a2e36;
            spacing: 2px;
            padding: 2px 4px;
        }
        QToolButton {
            background: transparent;
            color: #dce0e8;
            border: 1px solid transparent;
            border-radius: 4px;
            padding: 4px 8px;
        }
        QToolButton:hover   { background: #24282e; border-color: #3a3f47; }
        QToolButton:pressed { background: #1a1e24; }
        /* ─ status bar ─ */
        QStatusBar { background: #14161a; color: #9aa0a6; font-size: 11px; }
        /* ─ scrollbars ─ */
        QScrollBar:vertical {
            background: #14161a; width: 8px; margin: 0;
        }
        QScrollBar::handle:vertical {
            background: #3a3f47; border-radius: 4px; min-height: 24px;
        }
        QScrollBar::handle:vertical:hover { background: #4a5057; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QScrollBar:horizontal {
            background: #14161a; height: 8px; margin: 0;
        }
        QScrollBar::handle:horizontal {
            background: #3a3f47; border-radius: 4px; min-width: 24px;
        }
        QScrollBar::handle:horizontal:hover { background: #4a5057; }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
        /* ─ splitters ─ */
        QSplitter::handle { background: #2a2e36; }
        QSplitter::handle:horizontal { width: 4px; }
        QSplitter::handle:vertical   { height: 4px; }
        /* ─ list widget ─ */
        QListWidget {
            background: #12141800;
            border: none;
            outline: none;
        }
        QListWidget::item { padding: 3px 6px; }
        QListWidget::item:selected { background: #2a3a4a; color: #ffffff; }
        QListWidget::item:alternate { background: #1e2026; }
        /* ─ inspector badge ─ */
        QLabel#inspBadge {
            border-radius: 5px;
            padding: 4px 10px;
            font-size: 11px;
            font-weight: bold;
            letter-spacing: 0.5px;
            color: #ffffff;
        }
        QLabel#sectionLabel {
            color: #9aa0a6;
            font-size: 10px;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            padding: 2px 0;
            border-top: 1px solid #2a2e36;
        }
        QLabel#statusLabel {
            color: #9aa0a6;
            padding: 0 8px;
            font-size: 12px;
        }
    )");
}

// ── load / populate ──────────────────────────────────────────────────────────

void MainWindow::openRecording() {
    QString path = QFileDialog::getOpenFileName(
        this, "Open Recording", {}, "CHRONOS (*.chronos);;All Files (*)");
    if (!path.isEmpty()) loadRecording(path);
}

void MainWindow::loadRecording(const QString& path) {
    auto rr = storage::Reader::read_recording(path.toStdString());
    if (!rr.ok()) {
        QMessageBox::critical(this, "Error loading recording",
            QString::fromStdString(rr.error().message));
        return;
    }
    recording_ = std::move(rr.value());
    eventLog_.clear();
    for (auto& e : recording_.events) eventLog_.append(e);

    diffA_ = diffB_ = selectedEventRow_ = -1;
    diffAButton_->setText("Set A");
    diffBButton_->setText("Set B");
    timeline_.set_log(&eventLog_);

    populateEvents();
    populateNavTree();
    populateExecGraph();

    checkpointCombo_->blockSignals(true);
    checkpointCombo_->clear();
    for (const auto& cp : recording_.checkpoints)
        checkpointCombo_->addItem(
            QString("CP %1 — t=%2 — %3")
                .arg(cp.id)
                .arg(cp.time_sequence)
                .arg(QString::fromStdString(cp.label)));
    checkpointCombo_->blockSignals(false);
    if (!recording_.checkpoints.empty()) {
        checkpointCombo_->setCurrentIndex(0);
        onCheckpointSelected(0);
    }

    const int n = static_cast<int>(recording_.events.size());
    timelineSlider_->setRange(0, n);
    timelineSlider_->setValue(0);
    timelineRuler_->setEvents(recording_.events);
    timelineRuler_->setPosition(0);

    statusLabel_->setText(
        QString("%1 events · %2 checkpoints")
            .arg(n).arg(recording_.checkpoints.size()));
    statusBar()->showMessage("Loaded: " + path);
}

void MainWindow::populateEvents() {
    eventTable_->setUpdatesEnabled(false);
    eventTable_->setRowCount(static_cast<int>(recording_.events.size()));
    for (size_t i = 0; i < recording_.events.size(); ++i) {
        const auto& e = recording_.events[i];
        const int row = static_cast<int>(i);

        auto* idxItem    = new QTableWidgetItem(QString::number(i + 1));
        auto* typeItem   = new QTableWidgetItem(event_type_name(e.type));
        auto* pidItem    = new QTableWidgetItem(QString::number(e.pid));
        auto* tidItem    = new QTableWidgetItem(QString::number(e.tid));
        auto* detailItem = new QTableWidgetItem(eventDetail(e));

        idxItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        pidItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        tidItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        // Color rows by event type for instant scanability
        QColor bg = eventRowColor(e.type);
        if (bg.alpha() > 0) {
            for (auto* item : {idxItem, typeItem, pidItem, tidItem, detailItem})
                item->setBackground(bg);
        }

        eventTable_->setItem(row, 0, idxItem);
        eventTable_->setItem(row, 1, typeItem);
        eventTable_->setItem(row, 2, pidItem);
        eventTable_->setItem(row, 3, tidItem);
        eventTable_->setItem(row, 4, detailItem);
        eventTable_->setRowHeight(row, 22);
    }
    eventTable_->setUpdatesEnabled(true);
}

void MainWindow::populateNavTree() {
    // Clear children of each top-level item
    auto clearChildren = [](QTreeWidgetItem* item) {
        while (item->childCount() > 0)
            delete item->takeChild(0);
    };
    clearChildren(navTreeProcesses_);
    clearChildren(navTreeThreads_);
    clearChildren(navTreeCheckpoints_);
    clearChildren(navTreeSyscalls_);
    clearChildren(navTreeFunctions_);
    clearChildren(navTreeMemRegions_);

    std::set<u32> pids;
    std::set<u64> tids;
    std::set<std::string> syscalls;
    std::set<std::string> functions;

    for (size_t i = 0; i < recording_.events.size(); ++i) {
        const auto& e = recording_.events[i];
        const int row = static_cast<int>(i);
        pids.insert(e.pid);
        tids.insert(e.tid);
        if (e.type == EventType::SyscallEnter) {
            const auto& s = std::get<SyscallEvent>(e.payload);
            if (syscalls.insert(s.name).second) {
                auto* item = new QTreeWidgetItem(navTreeSyscalls_,
                    {QString::fromStdString(s.name)});
                item->setData(0, Qt::UserRole, row);
            }
        }
        if (e.type == EventType::FunctionEnter) {
            const auto& f = std::get<FunctionEvent>(e.payload);
            if (functions.insert(f.name).second) {
                auto* item = new QTreeWidgetItem(navTreeFunctions_,
                    {QString::fromStdString(f.name)});
                item->setData(0, Qt::UserRole, row);
            }
        }
    }
    for (u32 pid : pids) {
        auto* item = new QTreeWidgetItem(navTreeProcesses_,
            {QString("PID %1").arg(pid)});
        (void)item;
    }
    for (u64 tid : tids) {
        auto* item = new QTreeWidgetItem(navTreeThreads_,
            {QString("TID %1").arg(tid)});
        (void)item;
    }
    for (const auto& cp : recording_.checkpoints) {
        auto* item = new QTreeWidgetItem(navTreeCheckpoints_,
            {QString("CP %1: %2").arg(cp.id).arg(QString::fromStdString(cp.label))});
        // Find the event that matches this checkpoint's time_sequence
        for (size_t i = 0; i < recording_.events.size(); ++i) {
            if (recording_.events[i].time.sequence == cp.time_sequence) {
                item->setData(0, Qt::UserRole, static_cast<int>(i));
                break;
            }
        }
    }
    if (!recording_.checkpoints.empty() && !recording_.checkpoints[0].regions.empty()) {
        for (const auto& r : recording_.checkpoints[0].regions) {
            auto* item = new QTreeWidgetItem(navTreeMemRegions_,
                {QString("0x%1  %2").arg(r.base, 0, 16).arg(QString::fromStdString(r.name))});
            (void)item;
        }
    }

    // Update counts in labels
    navTreeProcesses_->setText(0,   QString("Processes (%1)").arg(pids.size()));
    navTreeThreads_->setText(0,     QString("Threads (%1)").arg(tids.size()));
    navTreeCheckpoints_->setText(0, QString("Checkpoints (%1)").arg(recording_.checkpoints.size()));
    navTreeSyscalls_->setText(0,    QString("Syscalls (%1)").arg(syscalls.size()));
    navTreeFunctions_->setText(0,   QString("Functions (%1)").arg(functions.size()));
    navTreeMemRegions_->setText(0,  QString("Memory Regions"));
    navTreeCheckpoints_->setExpanded(true);
}

void MainWindow::populateExecGraph() {
    execGraphTree_->clear();
    std::vector<QTreeWidgetItem*> stack;
    for (size_t i = 0; i < recording_.events.size(); ++i) {
        const auto& e = recording_.events[i];
        if (e.type == EventType::FunctionEnter) {
            const auto& f = std::get<FunctionEvent>(e.payload);
            QTreeWidgetItem* parent = stack.empty()
                ? execGraphTree_->invisibleRootItem()
                : stack.back();
            auto* item = new QTreeWidgetItem(parent);
            item->setText(0, QString::fromStdString(f.name));
            item->setText(1, QString::number(i + 1));
            item->setText(2, QString::number(stack.size()));
            item->setData(0, Qt::UserRole, static_cast<int>(i + 1));
            stack.push_back(item);
        } else if (e.type == EventType::FunctionExit) {
            if (!stack.empty()) stack.pop_back();
        }
    }
    execGraphTree_->expandToDepth(1);
}

// ── selectRow (central navigation primitive) ────────────────────────────────

void MainWindow::selectRow(int row) {
    if (row < 0 || row >= static_cast<int>(recording_.events.size())) return;
    eventTable_->selectRow(row);
    eventTable_->scrollTo(eventTable_->model()->index(row, 0),
                          QAbstractItemView::EnsureVisible);
    // Block slider to avoid re-entry
    timelineSlider_->blockSignals(true);
    timelineSlider_->setValue(row + 1);
    timelineSlider_->blockSignals(false);
    timelineRuler_->setPosition(row + 1);
    onEventSelected(row);
}

// ── onTimelineChanged ────────────────────────────────────────────────────────

void MainWindow::onTimelineChanged(int value) {
    if (recording_.events.empty()) return;
    int row = std::clamp(value - 1, 0, static_cast<int>(recording_.events.size()) - 1);
    timelineRuler_->setPosition(value);
    eventTable_->selectRow(row);
    eventTable_->scrollTo(eventTable_->model()->index(row, 0),
                          QAbstractItemView::EnsureVisible);
    onEventSelected(row);
    statusLabel_->setText(
        QString("%1 / %2").arg(value).arg(recording_.events.size()));
}

// ── onEventSelected ──────────────────────────────────────────────────────────

void MainWindow::onEventSelected(int row) {
    if (row < 0 || row >= static_cast<int>(recording_.events.size())) return;
    selectedEventRow_ = row;
    const auto& e = recording_.events[row];

    // ── Inspector badge ──
    static const auto badgeColor = [](EventType t) -> QString {
        switch (t) {
            case EventType::SyscallEnter:
            case EventType::SyscallExit:    return "#1a2e1a; color:#4caf50";
            case EventType::Checkpoint:     return "#2a2010; color:#ffb300";
            case EventType::FunctionEnter:
            case EventType::FunctionExit:   return "#122030; color:#64b5f6";
            case EventType::VariableChange: return "#221a2a; color:#ce93d8";
            case EventType::MemoryWrite:
            case EventType::MemoryAllocate:
            case EventType::MemoryFree:     return "#281818; color:#ef9a9a";
            default:                        return "#20222a; color:#9aa0a6";
        }
    };
    inspBadge_->setStyleSheet(
        QString("background:#%1; border-radius:5px; padding:4px 10px; "
                "font-size:11px; font-weight:bold;")
            .arg(badgeColor(e.type)));
    inspBadge_->setText(QString("%1   #%2")
        .arg(event_type_name(e.type)).arg(row + 1));

    // ── Inspector property table ──
    inspTable_->setUpdatesEnabled(false);
    inspTable_->setRowCount(0);

    auto addProp = [this](const QString& key, const QString& val) {
        const int r = inspTable_->rowCount();
        inspTable_->insertRow(r);
        auto* k = new QTableWidgetItem(key);
        k->setForeground(QColor(0x9a, 0xa0, 0xa6));
        inspTable_->setItem(r, 0, k);
        inspTable_->setItem(r, 1, new QTableWidgetItem(val));
        inspTable_->setRowHeight(r, 20);
    };

    addProp("Sequence",  QString::number(e.time.sequence));
    addProp("PID",       QString::number(e.pid));
    addProp("TID",       QString::number(e.tid));
    if (e.timestamp.count() != 0)
        addProp("Timestamp", QString("%1 ns").arg(e.timestamp.count()));
    if (e.location.has_value())
        addProp("Location",
            QString::fromStdString(e.location->file) + ":" +
            QString::number(e.location->line));

    // payload-specific fields
    if (e.type == EventType::SyscallEnter || e.type == EventType::SyscallExit) {
        const auto& s = std::get<SyscallEvent>(e.payload);
        addProp("Syscall", QString::fromStdString(s.name));
        addProp("Number",  QString::number(s.number));
        for (size_t i = 0; i < s.args.size(); ++i)
            addProp(QString("arg[%1]").arg(i), QString::fromStdString(s.args[i]));
        if (!s.result.empty())
            addProp("Return", QString::fromStdString(s.result));
    } else if (e.type == EventType::FunctionEnter || e.type == EventType::FunctionExit) {
        const auto& f = std::get<FunctionEvent>(e.payload);
        addProp("Function", QString::fromStdString(f.name));
        addProp("Address",  QString("0x%1").arg(f.entry_address, 0, 16));
        for (size_t i = 0; i < f.args.size(); ++i)
            addProp(QString("arg[%1]").arg(i), QString::fromStdString(f.args[i]));
    } else if (e.type == EventType::VariableChange) {
        const auto& v = std::get<VariableEvent>(e.payload);
        addProp("Variable", QString::fromStdString(v.name));
        addProp("Type",     QString::fromStdString(v.type_name));
        addProp("Old",      QString::fromStdString(v.old_value));
        addProp("New",      QString::fromStdString(v.new_value));
        addProp("Address",  QString("0x%1").arg(v.address, 0, 16));
    } else if (e.type == EventType::MemoryWrite || e.type == EventType::MemoryAllocate ||
               e.type == EventType::MemoryFree) {
        const auto& m = std::get<MemoryEvent>(e.payload);
        addProp("Address", QString("0x%1").arg(m.address, 0, 16));
        addProp("Size",    QString::number(m.size) + " bytes");
    } else if (e.type == EventType::ExceptionThrown || e.type == EventType::ExceptionCaught) {
        const auto& ex = std::get<ExceptionEvent>(e.payload);
        addProp("Exception", QString::fromStdString(ex.type_name));
        addProp("Message",   QString::fromStdString(ex.message));
        addProp("At",        QString("0x%1").arg(ex.throw_address, 0, 16));
    } else if (e.type == EventType::Checkpoint) {
        const auto& c = std::get<CheckpointEvent>(e.payload);
        addProp("Label",         QString::fromStdString(c.label));
        addProp("Checkpoint ID", QString::number(c.checkpoint_id));
        // Select the matching checkpoint in the combo
        for (int i = 0; i < static_cast<int>(recording_.checkpoints.size()); ++i) {
            if (recording_.checkpoints[i].id == c.checkpoint_id) {
                checkpointCombo_->setCurrentIndex(i);
                break;
            }
        }
    } else if (e.type == EventType::Marker) {
        const auto& m = std::get<MarkerEvent>(e.payload);
        addProp("Label", QString::fromStdString(m.label));
        addProp("Note",  QString::fromStdString(m.note));
    } else if (e.type == EventType::Custom) {
        const auto& c = std::get<CustomEvent>(e.payload);
        addProp("Kind",    QString::fromStdString(c.kind));
        addProp("Payload", QString::fromStdString(c.payload));
    } else if (e.type == EventType::ProcessStart || e.type == EventType::ProcessExit ||
               e.type == EventType::ThreadCreate  || e.type == EventType::ThreadExit) {
        const auto& p = std::get<ProcessEvent>(e.payload);
        addProp("Detail", QString::fromStdString(p.detail));
    }

    inspTable_->setUpdatesEnabled(true);

    // ── Call stack ──
    callStackTree_->clear();
    std::vector<std::pair<std::string, u64>> stack;
    for (int i = 0; i <= row; ++i) {
        const auto& ev = recording_.events[i];
        if (ev.type == EventType::FunctionEnter) {
            const auto& f = std::get<FunctionEvent>(ev.payload);
            stack.push_back({f.name, f.entry_address});
        } else if (ev.type == EventType::FunctionExit) {
            if (!stack.empty()) stack.pop_back();
        }
    }
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        auto* item = new QTreeWidgetItem(callStackTree_);
        item->setText(0, QString::fromStdString(it->first));
        item->setText(1, QString("0x%1").arg(it->second, 0, 16));
    }
}

// ── Checkpoint / memory ──────────────────────────────────────────────────────

void MainWindow::onCheckpointSelected(int idx) {
    if (idx < 0 || idx >= static_cast<int>(recording_.checkpoints.size())) return;
    const auto& cp = recording_.checkpoints[idx];

    memoryTree_->clear();
    for (const auto& r : cp.regions) {
        auto* item = new QTreeWidgetItem(memoryTree_);
        item->setText(0, QString("0x%1").arg(r.base, 0, 16));
        item->setText(1, QString::number(r.size));
        item->setText(2, QString("%1%2%3")
            .arg(r.readable   ? "r" : "-")
            .arg(r.writable   ? "w" : "-")
            .arg(r.executable ? "x" : "-"));
        item->setText(3, QString::fromStdString(r.name));
        QByteArray ba(reinterpret_cast<const char*>(r.bytes.data()),
                      static_cast<int>(r.bytes.size()));
        item->setData(0, Qt::UserRole, ba);
    }

    // Rebuild strings from all regions
    std::vector<u8> allbytes;
    allbytes.reserve(1024 * 1024);
    for (const auto& r : cp.regions)
        allbytes.insert(allbytes.end(), r.bytes.begin(), r.bytes.end());
    currentStrings_ = memory::extract_strings(allbytes);
    QString text;
    text.reserve(static_cast<int>(currentStrings_.size()) * 40);
    for (const auto& s : currentStrings_)
        text += QString::fromStdString(s) + "\n";
    stringsView_->setPlainText(text);
    hexView_->clear();
}

// ── diff ─────────────────────────────────────────────────────────────────────

void MainWindow::setDiffA() {
    if (selectedEventRow_ < 0) {
        diffEdit_->setPlainText("Select an event first."); return;
    }
    diffA_ = selectedEventRow_;
    diffAButton_->setText(QString("A = #%1").arg(diffA_ + 1));
}

void MainWindow::setDiffB() {
    if (selectedEventRow_ < 0) {
        diffEdit_->setPlainText("Select an event first."); return;
    }
    diffB_ = selectedEventRow_;
    diffBButton_->setText(QString("B = #%1").arg(diffB_ + 1));
}

void MainWindow::runStateDiff() {
    if (diffA_ < 0 || diffB_ < 0 ||
        diffA_ >= static_cast<int>(recording_.events.size()) ||
        diffB_ >= static_cast<int>(recording_.events.size())) {
        diffEdit_->setPlainText("Set both A and B first."); return;
    }
    const u64 seqA = recording_.events[diffA_].time.sequence;
    const u64 seqB = recording_.events[diffB_].time.sequence;
    const auto snapA = analysis::StateDiffer::build(eventLog_, TimePoint{seqA});
    const auto snapB = analysis::StateDiffer::build(eventLog_, TimePoint{seqB});
    const auto diff  = analysis::StateDiffer::diff(snapA, snapB, eventLog_);

    QString out;
    out += QString("─ State Diff: event #%1 → #%2 (%3 events between) ─\n\n")
        .arg(diffA_ + 1).arg(diffB_ + 1).arg(diff.events_between);

    if (!diff.added_variables.empty()) {
        out += "ADDED:\n";
        for (const auto& n : diff.added_variables) out += "  + " + QString::fromStdString(n) + "\n";
        out += "\n";
    }
    if (!diff.removed_variables.empty()) {
        out += "REMOVED:\n";
        for (const auto& n : diff.removed_variables) out += "  - " + QString::fromStdString(n) + "\n";
        out += "\n";
    }
    if (!diff.changed_variables.empty()) {
        out += "CHANGED:\n";
        for (const auto& c : diff.changed_variables)
            out += QString("  ~ %1:  %2  →  %3\n")
                .arg(QString::fromStdString(c.first))
                .arg(QString::fromStdString(c.second.first))
                .arg(QString::fromStdString(c.second.second));
        out += "\n";
    }
    if (diff.added_variables.empty() && diff.removed_variables.empty() &&
        diff.changed_variables.empty())
        out += "(no variable changes between these events)\n";

    diffEdit_->setPlainText(out);
}

// ── exec graph popup ─────────────────────────────────────────────────────────

void MainWindow::showExecGraph() {
    if (recording_.events.empty()) {
        QMessageBox::information(this, "Execution Graph", "No recording loaded."); return;
    }
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Execution Graph — " + windowTitle());
    dlg->resize(960, 700);
    auto* layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(8, 8, 8, 8);

    auto* scene = new QGraphicsScene(dlg);
    auto* view  = new QGraphicsView(scene, dlg);
    view->setRenderHint(QPainter::Antialiasing);
    view->setDragMode(QGraphicsView::ScrollHandDrag);
    view->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    layout->addWidget(view, 1);

    auto* info = new QLabel("Scroll to zoom · drag to pan", dlg);
    info->setStyleSheet("color: #606570; font-size: 11px;");
    layout->addWidget(info);

    // Zoom shortcut
    auto* zoomIn  = new QShortcut(QKeySequence::ZoomIn,  dlg);
    auto* zoomOut = new QShortcut(QKeySequence::ZoomOut, dlg);
    connect(zoomIn,  &QShortcut::activated, view, [view](){ view->scale(1.2, 1.2); });
    connect(zoomOut, &QShortcut::activated, view, [view](){ view->scale(1/1.2, 1/1.2); });

    // Build tree: x = depth, y = node arrival order
    struct Node { std::string name; int x, y, idx; };
    std::vector<Node> nodes;
    std::vector<std::pair<int,int>> edges; // parent→child indices
    std::vector<int> stk;
    int yCounter = 0;

    for (size_t i = 0; i < recording_.events.size(); ++i) {
        const auto& ev = recording_.events[i];
        if (ev.type == EventType::FunctionEnter) {
            const auto& f = std::get<FunctionEvent>(ev.payload);
            int nodeIdx = static_cast<int>(nodes.size());
            nodes.push_back({f.name, static_cast<int>(stk.size()), yCounter++,
                             static_cast<int>(i)});
            if (!stk.empty()) edges.push_back({stk.back(), nodeIdx});
            stk.push_back(nodeIdx);
        } else if (ev.type == EventType::FunctionExit) {
            if (!stk.empty()) stk.pop_back();
        }
    }

    const double cellW = 180, cellH = 36, padX = 24, padY = 8;
    for (const auto& n : nodes) {
        double x = n.x * (cellW + padX);
        double y = n.y * (cellH + padY);
        auto* rect = scene->addRect(x, y, cellW, cellH,
            QPen(QColor(0x46, 0x82, 0xb4, 180)),
            QBrush(QColor(0x12, 0x1e, 0x2e)));
        rect->setToolTip(QString::fromStdString(n.name));
        auto* text = scene->addText(
            QString::fromStdString(n.name).left(22) +
                (n.name.size() > 22 ? "…" : ""),
            QFont("Monospace", 9));
        text->setDefaultTextColor(QColor(0xdc, 0xe0, 0xe8));
        text->setPos(x + 6, y + 8);
        (void)rect;
    }
    for (const auto& [pi, ci] : edges) {
        const auto& pn = nodes[pi];
        const auto& cn = nodes[ci];
        double px = pn.x*(cellW+padX) + cellW;
        double py = pn.y*(cellH+padY) + cellH/2;
        double cx = cn.x*(cellW+padX);
        double cy = cn.y*(cellH+padY) + cellH/2;
        scene->addLine(px, py, cx, cy, QPen(QColor(0x9a, 0xa0, 0xa6, 120)));
    }

    view->fitInView(scene->itemsBoundingRect().adjusted(-20,-20,20,20),
                    Qt::KeepAspectRatio);
    dlg->exec();
}

// ── navigation ───────────────────────────────────────────────────────────────

void MainWindow::prevEvent() {
    if (recording_.events.empty()) return;
    int row = eventTable_->currentRow();
    selectRow(std::max(0, row - 1));
}

void MainWindow::nextEvent() {
    if (recording_.events.empty()) return;
    int row = eventTable_->currentRow();
    selectRow(std::min(row + 1, static_cast<int>(recording_.events.size()) - 1));
}

void MainWindow::prevCheckpoint() {
    if (recording_.events.empty()) return;
    const int cur = eventTable_->currentRow();
    for (int i = cur - 1; i >= 0; --i) {
        if (recording_.events[i].type == EventType::Checkpoint) { selectRow(i); return; }
    }
    // wrap-around
    for (int i = static_cast<int>(recording_.events.size()) - 1; i > cur; --i) {
        if (recording_.events[i].type == EventType::Checkpoint) { selectRow(i); return; }
    }
}

void MainWindow::nextCheckpoint() {
    if (recording_.events.empty()) return;
    const int cur = eventTable_->currentRow();
    for (int i = cur + 1; i < static_cast<int>(recording_.events.size()); ++i) {
        if (recording_.events[i].type == EventType::Checkpoint) { selectRow(i); return; }
    }
    // wrap-around
    for (int i = 0; i <= cur; ++i) {
        if (recording_.events[i].type == EventType::Checkpoint) { selectRow(i); return; }
    }
}

// ── search ───────────────────────────────────────────────────────────────────

void MainWindow::doSearch() {
    if (recording_.events.empty() || !searchEdit_ || !searchResults_) return;
    const QString text = searchEdit_->text().trimmed();
    if (text.isEmpty()) return;

    searchResults_->clear();
    timeline::Timeline tl;
    tl.set_log(&eventLog_);
    const auto ids = tl.find_events(text.toStdString());

    for (auto id : ids) {
        const auto ev = eventLog_.by_id(id);
        if (!ev) continue;
        // Find row index
        const size_t idx = eventLog_.index_of(id);
        auto* item = new QListWidgetItem(
            QString("#%1  %2  %3")
                .arg(idx + 1)
                .arg(event_type_name(ev->type))
                .arg(eventDetail(*ev)));
        item->setData(Qt::UserRole, static_cast<int>(idx));
        searchResults_->addItem(item);
    }
    if (searchResults_->count() == 0)
        searchResults_->addItem("No results for: " + text);
    else
        statusBar()->showMessage(
            QString("Found %1 match(es) for '%2'").arg(ids.size()).arg(text));
}

// ── hex dump helper ──────────────────────────────────────────────────────────

QString MainWindow::buildHexDump(const QByteArray& ba) {
    QString out;
    out.reserve(ba.size() * 4);
    for (int i = 0; i < ba.size(); i += 16) {
        QString line = QString("%1  ").arg(i, 6, 16, QChar('0'));
        QString ascii;
        for (int j = 0; j < 16; ++j) {
            if (i + j < ba.size()) {
                const unsigned char b = static_cast<unsigned char>(ba[i + j]);
                line  += QString("%1 ").arg(b, 2, 16, QChar('0'));
                ascii += (b >= 32 && b < 127) ? QChar(b) : QChar('.');
            } else {
                line  += "   ";
                ascii += ' ';
            }
            if (j == 7) { line += ' '; ascii += ' '; }
        }
        out += line + " |" + ascii + "|\n";
    }
    return out;
}
