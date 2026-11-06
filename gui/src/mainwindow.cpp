#include "chronos/gui/mainwindow.h"
#include "chronos/analysis/state_diff.h"
#include "chronos/memory/strings.h"
#include <vector>
#include <utility>
#include <algorithm>
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
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsTextItem>
#include <QGraphicsLineItem>
#include <QDialog>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QTabWidget>

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
    auto* viewMenu = menuBar->addMenu("View");
    auto* graphAct = viewMenu->addAction("Execution Graph...");
    connect(graphAct, &QAction::triggered, this, &MainWindow::showExecGraph);
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

    // Welcome page (simple)
    welcomeLabel_ = new QLabel("Welcome to CHRONOS\\n\\nOpen a recording or trace a program to begin.", this);
    welcomeLabel_->setAlignment(Qt::AlignCenter);
    welcomeLabel_->setStyleSheet("font-size: 24px; color: #9aa0a6;");
    mainLayout->addWidget(welcomeLabel_, 1);

    // Splitter: left events, right panels
    mainSplitter_ = new QSplitter(Qt::Horizontal, this);
    mainSplitter_->setVisible(false);

    // Event log table
    eventTable_ = new QTableWidget(this);
    eventTable_->setColumnCount(4);
    eventTable_->setHorizontalHeaderLabels({"#", "Type", "PID/TID", "Detail"});
    eventTable_->horizontalHeader()->setStretchLastSection(true);
    eventTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(eventTable_, &QTableWidget::cellClicked, this, &MainWindow::onEventSelected);
    mainSplitter_->addWidget(eventTable_);

    // Right: tab-like vertical stack
    auto* rightTabs = new QTabWidget(this);
    // Tab: Memory
    auto* memTab = new QWidget(this);
    auto* memLayout = new QVBoxLayout(memTab);
    memLayout->setContentsMargins(8,8,8,8);
    checkpointCombo_ = new QComboBox(this);
    checkpointCombo_->setPlaceholderText("Select checkpoint");
    connect(checkpointCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx){
        if (idx < 0 || idx >= (int)recording_.checkpoints.size()) return;
        const auto& cp = recording_.checkpoints[idx];
        memoryTree_->clear();
        for (auto& r : cp.regions) {
            auto* item = new QTreeWidgetItem(memoryTree_);
            item->setText(0, QString("0x%1").arg(r.base, 0, 16));
            item->setText(1, QString::number(r.size));
            item->setText(2, QString("%1%2%3").arg(r.readable?"r":"-").arg(r.writable?"w":"-").arg(r.executable?"x":"-"));
            item->setText(3, QString::fromStdString(r.name));
            QByteArray ba(reinterpret_cast<const char*>(r.bytes.data()), (int)r.bytes.size());
            item->setData(0, Qt::UserRole, ba);
        }
        // Extract strings from all regions
        std::vector<u8> allbytes;
        for (auto& r : cp.regions) {
            allbytes.insert(allbytes.end(), r.bytes.begin(), r.bytes.end());
        }
        currentStrings_ = memory::extract_strings(allbytes);
        // apply filter
        QString filter = stringsFilter_ ? stringsFilter_->text() : QString();
        QString text;
        for (auto& s : currentStrings_) {
            if (!filter.isEmpty() && QString::fromStdString(s).indexOf(filter, 0, Qt::CaseInsensitive) < 0) continue;
            text += QString::fromStdString(s) + "\\n";
        }
        if (stringsView_) stringsView_->setPlainText(text);
    });
    memLayout->addWidget(checkpointCombo_);
    memoryTree_ = new QTreeWidget(this);
    memoryTree_->setHeaderLabels({"Base", "Size", "Perms", "Name"});
    memoryTree_->header()->setStretchLastSection(true);
    connect(memoryTree_, &QTreeWidget::itemSelectionChanged, this, [this](){
        auto items = memoryTree_->selectedItems();
        if (items.isEmpty()) return;
        QByteArray ba = items.first()->data(0, Qt::UserRole).toByteArray();
        QString hex;
        for (int i = 0; i < ba.size(); i += 16) {
            QString line = QString("%1  ").arg(i, 4, 16, QChar('0'));
            QString ascii;
            for (int j = 0; j < 16; ++j) {
                if (i+j < ba.size()) {
                    unsigned char b = (unsigned char)ba[i+j];
                    line += QString("%1 ").arg(b, 2, 16, QChar('0'));
                    ascii += (b >= 32 && b < 127) ? QChar(b) : QChar('.');
                } else { line += "   "; ascii += " "; }
            }
            hex += line + " " + ascii + "\n";
        }
        hexView_->setPlainText(hex);
    });
    memLayout->addWidget(memoryTree_, 1);
    hexView_ = new QPlainTextEdit(this);
    hexView_->setReadOnly(true);
    hexView_->setFont(QFont("Monospace", 9));
    hexView_->setMaximumHeight(200);
    memLayout->addWidget(hexView_);
    stringsFilter_ = new QLineEdit(this);
    stringsFilter_->setPlaceholderText("Filter strings...");
    connect(stringsFilter_, &QLineEdit::textChanged, this, [this](const QString& txt){
        if (!stringsView_) return;
        QString text;
        for (auto& s : currentStrings_) {
            if (!txt.isEmpty() && QString::fromStdString(s).indexOf(txt, 0, Qt::CaseInsensitive) < 0) continue;
            text += QString::fromStdString(s) + "\\n";
        }
        stringsView_->setPlainText(text);
    });
    memLayout->addWidget(stringsFilter_);
    stringsView_ = new QPlainTextEdit(this);
    stringsView_->setReadOnly(true);
    stringsView_->setFont(QFont("Monospace", 9));
    stringsView_->setMaximumHeight(160);
    memLayout->addWidget(stringsView_);
    rightTabs->addTab(memTab, "Memory");

    // Tab: Calls & Graph
    auto* callTab = new QWidget(this);
    auto* callLayout = new QVBoxLayout(callTab);
    callLayout->setContentsMargins(8,8,8,8);
    callStackTree_ = new QTreeWidget(this);
    callStackTree_->setHeaderLabels({"Function", "Address"});
    callStackTree_->header()->setStretchLastSection(true);
    callLayout->addWidget(callStackTree_, 1);
    execGraphTree_ = new QTreeWidget(this);
    execGraphTree_->setHeaderLabels({"Function", "Event #"});
    execGraphTree_->header()->setStretchLastSection(true);
    callLayout->addWidget(execGraphTree_, 1);
    rightTabs->addTab(callTab, "Calls");

    // Tab: State Diff
    auto* diffTab = new QWidget(this);
    auto* diffLayout = new QVBoxLayout(diffTab);
    diffLayout->setContentsMargins(8,8,8,8);
    auto* diffBar = new QHBoxLayout();
    diffAButton_ = new QPushButton("Set A", this);
    diffBButton_ = new QPushButton("Set B", this);
    diffButton_ = new QPushButton("Compare", this);
    connect(diffAButton_, &QPushButton::clicked, this, &MainWindow::setDiffA);
    connect(diffBButton_, &QPushButton::clicked, this, &MainWindow::setDiffB);
    connect(diffButton_, &QPushButton::clicked, this, &MainWindow::runStateDiff);
    diffBar->addWidget(diffAButton_);
    diffBar->addWidget(diffBButton_);
    diffBar->addWidget(diffButton_);
    diffLayout->addLayout(diffBar);
    diffEdit_ = new QPlainTextEdit(this);
    diffEdit_->setReadOnly(true);
    diffLayout->addWidget(diffEdit_, 1);
    rightTabs->addTab(diffTab, "Diff");

    eventDetail_ = new QLabel("Select an event", this);
    eventDetail_->setWordWrap(true);
    eventDetail_->setStyleSheet("background: #181a1e; padding: 8px; border: 1px solid #2a2e36; border-radius: 6px;");
    eventDetail_->setMinimumHeight(60);

    auto* right = new QWidget(this);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0,0,0,0);
    rightLayout->addWidget(eventDetail_);
    rightLayout->addWidget(rightTabs, 1);
    mainSplitter_->addWidget(right);
    mainSplitter_->setSizes({600, 600});
    mainLayout->addWidget(mainSplitter_, 1);

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
        "QWidget { font-family: 'DejaVu Sans', 'Segoe UI', sans-serif; font-size: 13px; }"
        "QTableWidget, QTreeWidget, QPlainTextEdit { border: 1px solid #2a2e36; background: #181a1e; border-radius: 6px; }"
        "QHeaderView::section { background: #1e2228; color: #dce0e8; padding: 6px; border: 0; font-weight: bold; }"
        "QTableWidget::item:selected, QTreeWidget::item:selected { background: #2a3a4a; color: #ffffff; }"
        "QSlider::groove:horizontal { height: 6px; background: #2a2e36; border-radius: 3px; }"
        "QSlider::handle:horizontal { background: #4682b4; width: 16px; margin: -5px 0; border-radius: 8px; }"
        "QPushButton { background: #24282e; border: 1px solid #3a3f47; padding: 8px 16px; border-radius: 6px; }"
        "QPushButton:hover { background: #2e333b; border-color: #4682b4; }"
        "QPushButton:pressed { background: #1a1e24; }"
        "QTabWidget::pane { border: 1px solid #2a2e36; border-radius: 6px; background: #181a1e; }"
        "QTabBar::tab { background: #14161a; color: #9aa0a6; padding: 8px 16px; border: 1px solid #2a2e36; border-bottom: none; border-top-left-radius: 6px; border-top-right-radius: 6px; margin-right: 2px; }"
        "QTabBar::tab:selected { background: #1e2228; color: #4682b4; }"
        "QTabBar::tab:hover { color: #dce0e8; }"
        "QLabel { color: #dce0e8; }"
        "QMenuBar { background: #14161a; color: #dce0e8; }"
        "QMenuBar::item:selected { background: #24282e; }"
        "QMenu { background: #1e2228; color: #dce0e8; border: 1px solid #2a2e36; }"
        "QMenu::item:selected { background: #2a3a4a; }"
        "QStatusBar { background: #14161a; color: #9aa0a6; }"
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
    diffA_ = diffB_ = selectedEventRow_ = -1;
    diffAButton_->setText("Set A");
    diffBButton_->setText("Set B");
    timeline_.set_log(&eventLog_);
    populateEvents();
    if (checkpointCombo_) {
        checkpointCombo_->clear();
        for (auto& cp : recording_.checkpoints) checkpointCombo_->addItem(QString("Checkpoint %1 (t=%2)").arg(cp.id).arg(cp.time_sequence));
        if (!recording_.checkpoints.empty()) checkpointCombo_->setCurrentIndex(0);
    }
    if (welcomeLabel_) welcomeLabel_->hide();
    if (mainSplitter_) mainSplitter_->setVisible(true);
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
    selectedEventRow_ = row;
}

void MainWindow::setDiffA() {
    if (selectedEventRow_ < 0) { diffEdit_->setPlainText("Select an event before setting A."); return; }
    diffA_ = selectedEventRow_;
    diffAButton_->setText(QString("A: event %1").arg(diffA_ + 1));
}

void MainWindow::setDiffB() {
    if (selectedEventRow_ < 0) { diffEdit_->setPlainText("Select an event before setting B."); return; }
    diffB_ = selectedEventRow_;
    diffBButton_->setText(QString("B: event %1").arg(diffB_ + 1));
}

void MainWindow::runStateDiff() {
    if (diffA_ < 0 || diffB_ < 0 || diffA_ >= (int)recording_.events.size() || diffB_ >= (int)recording_.events.size()) {
        diffEdit_->setPlainText("Select an event and set both A and B first."); return;
    }
    const auto seqA = recording_.events[diffA_].time.sequence;
    const auto seqB = recording_.events[diffB_].time.sequence;
    const auto stateA = analysis::StateDiffer::build(eventLog_, TimePoint{seqA});
    const auto stateB = analysis::StateDiffer::build(eventLog_, TimePoint{seqB});
    const auto diff = analysis::StateDiffer::diff(stateA, stateB, eventLog_);
    QString out = QString("Diff between event %1 and %2\nEvents between: %3\n")
        .arg(diffA_ + 1).arg(diffB_ + 1).arg(diff.events_between);
    out += "Added variables:\n";
    for (const auto& name : diff.added_variables) out += "  + " + QString::fromStdString(name) + "\n";
    out += "Removed variables:\n";
    for (const auto& name : diff.removed_variables) out += "  - " + QString::fromStdString(name) + "\n";
    out += "Changed variables:\n";
    for (const auto& change : diff.changed_variables) out += QString("  ~ %1: %2 -> %3\n")
        .arg(QString::fromStdString(change.first))
        .arg(QString::fromStdString(change.second.first))
        .arg(QString::fromStdString(change.second.second));
    diffEdit_->setPlainText(out);
}

void MainWindow::showExecGraph() {
    if (recording_.events.empty()) { QMessageBox::information(this, "Execution Graph", "No recording loaded."); return; }
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("Execution Graph");
    dlg->resize(900, 700);
    auto* layout = new QVBoxLayout(dlg);
    auto* view = new QGraphicsView(dlg);
    auto* scene = new QGraphicsScene(dlg);
    view->setScene(scene);
    view->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(view);

    // Build call tree positions
    struct Node { QGraphicsEllipseItem* item; double x,y; };
    std::vector<Node> nodes;
    std::vector<QGraphicsLineItem*> edges;
    std::vector<int> stack;
    double x = 50, y = 50;
    int depth = 0;
    double stepX = 160, stepY = 80;
    for (size_t i = 0; i < recording_.events.size(); ++i) {
        const auto& ev = recording_.events[i];
        if (ev.type == EventType::FunctionEnter) {
            auto f = std::get<FunctionEvent>(ev.payload);
            double px = 50 + depth * stepX;
            double py = 50 + nodes.size() * 30; // vertical by order
            auto* ellipse = scene->addEllipse(px, py, 120, 40, QPen(QColor(0x46,0x82,0xb4)), QBrush(QColor(0x1e,0x22,0x28)));
            auto* text = scene->addText(QString::fromStdString(f.name), QFont("Monospace", 9));
            text->setDefaultTextColor(QColor(0xdc,0xe0,0xe8));
            text->setPos(px+4, py+10);
            nodes.push_back({ellipse, px, py});
            if (!stack.empty()) {
                int parentIdx = stack.back();
                auto* line = scene->addLine(nodes[parentIdx].x+60, nodes[parentIdx].y+40, px+60, py, QPen(QColor(0x9a,0xa0,0xa6)));
                edges.push_back(line);
            }
            stack.push_back((int)nodes.size()-1);
            depth++;
        } else if (ev.type == EventType::FunctionExit) {
            if (!stack.empty()) { stack.pop_back(); depth = std::max(0, depth-1); }
        }
    }
    view->fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    dlg->exec();
    delete dlg;
}
