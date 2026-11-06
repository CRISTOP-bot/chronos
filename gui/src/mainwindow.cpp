#include "chronos/gui/mainwindow.h"
#include "chronos/analysis/state_diff.h"
#include <vector>
#include <utility>
#include <QFileDialog>
#include <QTableWidget>
#include <QSlider>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QMenuBar>
#include <QStatusBar>
#include <QMessageBox>
#include <QApplication>
#include <QFont>

using namespace chronos;
using namespace chronos::gui;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("CHRONOS — Temporal Execution & Reconstruction Engine");
    resize(1280, 800);
    buildUi();
    applyDarkTheme();
}

void MainWindow::buildUi() {
    auto* menuBar = new QMenuBar(this);
    auto* fileMenu = menuBar->addMenu("File");
    auto* openAct = fileMenu->addAction("Open Recording...");
    connect(openAct, &QAction::triggered, this, &MainWindow::openRecording);
    fileMenu->addSeparator();
    auto* quitAct = fileMenu->addAction("Quit");
    connect(quitAct, &QAction::triggered, qApp, &QApplication::quit);
    setMenuBar(menuBar);

    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(8,8,8,8);

    // Top: timeline slider
    auto* topBar = new QHBoxLayout();
    auto* tlLabel = new QLabel("Timeline:", this);
    timelineSlider_ = new QSlider(Qt::Horizontal, this);
    timelineSlider_->setRange(0,0);
    connect(timelineSlider_, &QSlider::valueChanged, this, &MainWindow::onTimelineChanged);
    statusLabel_ = new QLabel("No recording loaded", this);
    topBar->addWidget(tlLabel);
    topBar->addWidget(timelineSlider_, 1);
    topBar->addWidget(statusLabel_);
    mainLayout->addLayout(topBar);

    // Splitter: left events, right panels
    auto* splitter = new QSplitter(Qt::Horizontal, this);

    // Event log table
    eventTable_ = new QTableWidget(this);
    eventTable_->setColumnCount(4);
    eventTable_->setHorizontalHeaderLabels({"#", "Type", "PID/TID", "Detail"});
    eventTable_->horizontalHeader()->setStretchLastSection(true);
    eventTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(eventTable_, &QTableWidget::cellClicked, this, &MainWindow::onEventSelected);
    splitter->addWidget(eventTable_);

    // Right: tab-like vertical stack
    auto* right = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0,0,0,0);

    eventDetail_ = new QLabel("Select an event", this);
    eventDetail_->setWordWrap(true);
    eventDetail_->setMinimumHeight(60);
    rightLayout->addWidget(eventDetail_);

    auto* memLabel = new QLabel("Memory Explorer (checkpoint regions)", this);
    rightLayout->addWidget(memLabel);
    memoryTree_ = new QTreeWidget(this);
    memoryTree_->setHeaderLabels({"Base", "Size", "Perms", "Name"});
    memoryTree_->header()->setStretchLastSection(true);
    connect(memoryTree_, &QTreeWidget::itemSelectionChanged, this, [this](){
        auto items = memoryTree_->selectedItems();
        if (items.isEmpty()) return;
        // find matching region bytes in selected checkpoint
        // We'll store pointer in item data
        auto* it = items.first();
        QByteArray ba = it->data(0, Qt::UserRole).toByteArray();
        QString hex;
        for (int i = 0; i < ba.size(); i += 16) {
            QString line = QString("%1  ").arg(i, 4, 16, QChar('0'));
            QString ascii;
            for (int j = 0; j < 16; ++j) {
                if (i+j < ba.size()) {
                    unsigned char b = (unsigned char)ba[i+j];
                    line += QString("%1 ").arg(b, 2, 16, QChar('0'));
                    ascii += (b >= 32 && b < 127) ? QChar(b) : QChar('.');
                } else {
                    line += "   ";
                    ascii += " ";
                }
            }
            hex += line + " " + ascii + "\n";
        }
        hexView_->setPlainText(hex);
    });
    rightLayout->addWidget(memoryTree_, 1);

    auto* hexLabel = new QLabel("Memory Hex Viewer", this);
    rightLayout->addWidget(hexLabel);
    hexView_ = new QPlainTextEdit(this);
    hexView_->setReadOnly(true);
    hexView_->setFont(QFont("Monospace", 9));
    hexView_->setMaximumHeight(180);
    rightLayout->addWidget(hexView_);

    auto* csLabel = new QLabel("Call Stack (up to selected event)", this);
    rightLayout->addWidget(csLabel);
    callStackTree_ = new QTreeWidget(this);
    callStackTree_->setHeaderLabels({"Function", "Address"});
    callStackTree_->header()->setStretchLastSection(true);
    callStackTree_->setMaximumHeight(160);
    rightLayout->addWidget(callStackTree_);

    auto* egLabel = new QLabel("Execution Graph (function calls)", this);
    rightLayout->addWidget(egLabel);
    execGraphTree_ = new QTreeWidget(this);
    execGraphTree_->setHeaderLabels({"Function", "Event #"});
    execGraphTree_->header()->setStretchLastSection(true);
    execGraphTree_->setMaximumHeight(200);
    rightLayout->addWidget(execGraphTree_);

    auto* diffLabel = new QLabel("State Diff", this);
    rightLayout->addWidget(diffLabel);
    auto* diffBar = new QHBoxLayout();
    diffButton_ = new QPushButton("Diff A vs B (set A/B by selecting events)", this);
    connect(diffButton_, &QPushButton::clicked, this, &MainWindow::runStateDiff);
    diffBar->addWidget(diffButton_);
    rightLayout->addLayout(diffBar);
    diffEdit_ = new QPlainTextEdit(this);
    diffEdit_->setReadOnly(true);
    diffEdit_->setMaximumBlockCount(10000);
    rightLayout->addWidget(diffEdit_, 1);

    splitter->addWidget(right);
    splitter->setSizes({600, 600});
    mainLayout->addWidget(splitter, 1);

    setCentralWidget(central);
    statusBar()->showMessage("Ready");
}

void MainWindow::applyDarkTheme() {
    qApp->setStyle("Fusion");
    QPalette p;
    p.setColor(QPalette::Window, QColor(24,26,30));
    p.setColor(QPalette::Base, QColor(18,20,24));
    p.setColor(QPalette::AlternateBase, QColor(30,32,36));
    p.setColor(QPalette::Text, QColor(220,224,232));
    p.setColor(QPalette::WindowText, QColor(220,224,232));
    p.setColor(QPalette::Button, QColor(36,40,46));
    p.setColor(QPalette::ButtonText, QColor(220,224,232));
    p.setColor(QPalette::Highlight, QColor(70,130,180));
    p.setColor(QPalette::HighlightedText, QColor(255,255,255));
    qApp->setPalette(p);
    setStyleSheet(
        "QTableWidget, QTreeWidget, QPlainTextEdit { border: 1px solid #2a2e36; }"
        "QHeaderView::section { background: #1e2228; color: #dce0e8; padding: 4px; border: 0; }"
        "QSlider::groove:horizontal { height: 6px; background: #2a2e36; }"
        "QSlider::handle:horizontal { background: #4682b4; width: 14px; margin: -4px 0; border-radius: 7px; }"
        "QPushButton { background: #24282e; border: 1px solid #3a3f47; padding: 6px 12px; }"
        "QPushButton:hover { background: #2e333b; }"
    );
}

void MainWindow::openRecording() {
    QString path = QFileDialog::getOpenFileName(this, "Open Recording", QString(), "CHRONOS (*.chronos);;All Files (*)");
    if (path.isEmpty()) return;
    loadRecording(path);
}

void MainWindow::loadRecording(const QString& path) {
    auto rr = storage::Reader::read_recording(path.toStdString());
    if (!rr.ok()) {
        QMessageBox::critical(this, "Error", QString::fromStdString(rr.error().message));
        return;
    }
    recording_ = rr.value();
    eventLog_.clear();
    for (auto& e : recording_.events) eventLog_.append(e);
    timeline_.set_log(&eventLog_);
    populateEvents();
    timelineSlider_->setRange(0, (int)recording_.events.size());
    timelineSlider_->setValue(0);
    statusLabel_->setText(QString("Loaded: %1 events, %2 checkpoints").arg(recording_.events.size()).arg(recording_.checkpoints.size()));
    statusBar()->showMessage("Loaded " + path);
}

void MainWindow::populateEvents() {
    eventTable_->setRowCount((int)recording_.events.size());
    for (size_t i = 0; i < recording_.events.size(); ++i) {
        const auto& e = recording_.events[i];
        eventTable_->setItem((int)i, 0, new QTableWidgetItem(QString::number(i+1)));
        eventTable_->setItem((int)i, 1, new QTableWidgetItem(event_type_name(e.type)));
        eventTable_->setItem((int)i, 2, new QTableWidgetItem(QString("%1/%2").arg(e.pid).arg(e.tid)));
        QString detail;
        if (e.type == EventType::SyscallEnter || e.type == EventType::SyscallExit) {
            auto s = std::get<SyscallEvent>(e.payload);
            detail = QString::fromStdString(s.name) + " " + (e.type==EventType::SyscallExit ? QString::fromStdString(s.result) : QString());
        } else if (e.type == EventType::FunctionEnter || e.type == EventType::FunctionExit) {
            detail = QString::fromStdString(std::get<FunctionEvent>(e.payload).name);
        } else if (e.type == EventType::VariableChange) {
            auto v = std::get<VariableEvent>(e.payload);
            detail = QString::fromStdString(v.name + " = " + v.new_value);
        } else if (e.type == EventType::Checkpoint) {
            detail = QString::fromStdString(std::get<CheckpointEvent>(e.payload).label);
        } else if (e.type == EventType::ProcessExit) {
            detail = "exited";
        }
        eventTable_->setItem((int)i, 3, new QTableWidgetItem(detail));
    }
}

void MainWindow::onTimelineChanged(int value) {
    if (recording_.events.empty()) return;
    size_t idx = (value <= 0) ? 0 : (size_t)value - 1;
    if (idx >= recording_.events.size()) idx = recording_.events.size() - 1;
    // show event at idx
    eventTable_->selectRow((int)idx);
    onEventSelected((int)idx);
    statusLabel_->setText(QString("Position %1 / %2").arg(idx+1).arg(recording_.events.size()));
}

void MainWindow::onEventSelected(int row) {
    if (row < 0 || row >= (int)recording_.events.size()) return;
    const auto& e = recording_.events[row];
    QString txt = QString("Type: %1\nPID/TID: %2/%3\nTime seq: %4\n")
        .arg(event_type_name(e.type)).arg(e.pid).arg(e.tid).arg(e.time.sequence);
    if (e.type == EventType::SyscallEnter || e.type == EventType::SyscallExit) {
        auto s = std::get<SyscallEvent>(e.payload);
        txt += QString("Syscall: %1 (#%2)\nArgs: %3\nResult: %4")
            .arg(QString::fromStdString(s.name)).arg(s.number)
            .arg(QString::fromStdString([&]{ std::string a; for(auto&x:s.args){a+=x+" ";} return a;}()))
            .arg(QString::fromStdString(s.result));
    } else if (e.type == EventType::VariableChange) {
        auto v = std::get<VariableEvent>(e.payload);
        txt += QString("Var: %1 (%2)\nOld: %3\nNew: %4\nAddr: 0x%5")
            .arg(QString::fromStdString(v.name)).arg(QString::fromStdString(v.type_name))
            .arg(QString::fromStdString(v.old_value)).arg(QString::fromStdString(v.new_value))
            .arg(QString::number(v.address,16));
    } else if (e.type == EventType::Checkpoint) {
        auto c = std::get<CheckpointEvent>(e.payload);
        txt += QString("Checkpoint: %1 (id %2)").arg(QString::fromStdString(c.label)).arg(c.checkpoint_id);
        // show memory regions from matching checkpoint record
        memoryTree_->clear();
        for (auto& cp : recording_.checkpoints) {
            if (cp.id == c.checkpoint_id) {
                for (auto& r : cp.regions) {
                    auto* item = new QTreeWidgetItem(memoryTree_);
                    item->setText(0, QString("0x%1").arg(r.base, 0, 16));
                    item->setText(1, QString::number(r.size));
                    item->setText(2, QString("%1%2%3").arg(r.readable?"r":"-").arg(r.writable?"w":"-").arg(r.executable?"x":"-"));
                    item->setText(3, QString::fromStdString(r.name));
                    QByteArray ba(reinterpret_cast<const char*>(r.bytes.data()), (int)r.bytes.size());
                    item->setData(0, Qt::UserRole, ba);
                }
            }
        }
    }
    // Update call stack
    callStackTree_->clear();
    std::vector<std::pair<std::string,u64>> stack;
    for (int i = 0; i <= row && i < (int)recording_.events.size(); ++i) {
        const auto& ev = recording_.events[i];
        if (ev.type == EventType::FunctionEnter) {
            auto f = std::get<FunctionEvent>(ev.payload);
            stack.push_back({f.name, f.entry_address});
        } else if (ev.type == EventType::FunctionExit) {
            if (!stack.empty()) stack.pop_back();
        }
    }
    for (auto& f : stack) {
        auto* item = new QTreeWidgetItem(callStackTree_);
        item->setText(0, QString::fromStdString(f.first));
        item->setText(1, QString("0x%1").arg(f.second, 0, 16));
    }
    // Update execution graph up to selected event
    execGraphTree_->clear();
    std::vector<QTreeWidgetItem*> egStack;
    for (int i = 0; i <= row && i < (int)recording_.events.size(); ++i) {
        const auto& ev = recording_.events[i];
        if (ev.type == EventType::FunctionEnter) {
            auto f = std::get<FunctionEvent>(ev.payload);
            QTreeWidgetItem* parent = egStack.empty() ? execGraphTree_->invisibleRootItem() : egStack.back();
            auto* item = new QTreeWidgetItem(parent);
            item->setText(0, QString::fromStdString(f.name));
            item->setText(1, QString::number(i+1));
            egStack.push_back(item);
        } else if (ev.type == EventType::FunctionExit) {
            if (!egStack.empty()) egStack.pop_back();
        }
    }
    eventDetail_->setText(txt);
    diffA_ = row; // last selected becomes A candidate; user can set B by selecting another then diff
}

void MainWindow::runStateDiff() {
    // Use first and last selected? We'll diff state at event index 0..current and current..end for demo
    if (recording_.events.size() < 2) { diffEdit_->setPlainText("Need at least 2 events"); return; }
    int a = 0;
    int b = (int)recording_.events.size() - 1;
    // Build EventLog views
    EventLog la, lb;
    for (int i = 0; i <= a && i < (int)recording_.events.size(); ++i) la.append(recording_.events[i]);
    for (int i = 0; i <= b && i < (int)recording_.events.size(); ++i) lb.append(recording_.events[i]);
    auto sa = analysis::StateDiffer::build(la, TimePoint{(u64)(a+1)});
    auto sb = analysis::StateDiffer::build(lb, TimePoint{(u64)(b+1)});
    auto d = analysis::StateDiffer::diff(sa, sb, lb);
    QString out;
    out += QString("Diff between event %1 and %2\n").arg(a+1).arg(b+1);
    out += QString("Events between: %1\n").arg(d.events_between);
    out += "Added variables:\n";
    for (auto& v : d.added_variables) out += "  + " + QString::fromStdString(v) + "\n";
    out += "Removed variables:\n";
    for (auto& v : d.removed_variables) out += "  - " + QString::fromStdString(v) + "\n";
    out += "Changed variables:\n";
    for (auto& c : d.changed_variables) out += QString("  ~ %1: %2 -> %3\n")
        .arg(QString::fromStdString(c.first))
        .arg(QString::fromStdString(c.second.first))
        .arg(QString::fromStdString(c.second.second));
    diffEdit_->setPlainText(out);
}
