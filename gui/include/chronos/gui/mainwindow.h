#pragma once
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMainWindow>
#include <QSlider>
#include <QSplitter>
#include <QString>
#include <QTableWidget>
#include <QTreeWidget>
#include <vector>
#include "chronos/storage/writer.h"
#include "chronos/events/event_log.h"
#include "chronos/timeline/timeline.h"
#include "chronos/analysis/state_diff.h"

class QPlainTextEdit;
class QPushButton;
class QTreeWidgetItem;

// Forward-declared so the .moc include works inside the .cpp
class TimelineRuler;

namespace chronos { namespace gui {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    void loadRecording(const QString& path);

private slots:
    void openRecording();
    void onTimelineChanged(int value);
    void onEventSelected(int row);
    void onCheckpointSelected(int idx);
    void runStateDiff();
    void setDiffA();
    void setDiffB();
    void showExecGraph();
    void prevEvent();
    void nextEvent();
    void prevCheckpoint();
    void nextCheckpoint();
    void doSearch();

private:
    void buildUi();
    void applyDarkTheme();
    void populateEvents();
    void populateNavTree();
    void populateExecGraph();
    void selectRow(int row);
    static QString buildHexDump(const QByteArray& ba);

    // ── data ──────────────────────────────────────────────────────────────
    chronos::storage::Reader::Recording  recording_{};
    chronos::EventLog                    eventLog_{};
    chronos::timeline::Timeline          timeline_{};
    std::vector<std::string>             currentStrings_;
    int selectedEventRow_ = -1;
    int diffA_            = -1;
    int diffB_            = -1;

    // ── central ───────────────────────────────────────────────────────────
    QTableWidget*   eventTable_      = nullptr;
    QSlider*        timelineSlider_  = nullptr;
    TimelineRuler*  timelineRuler_   = nullptr;

    // ── left dock: nav tree ───────────────────────────────────────────────
    QTreeWidget*     navTree_              = nullptr;
    QTreeWidgetItem* navTreeProcesses_     = nullptr;
    QTreeWidgetItem* navTreeThreads_       = nullptr;
    QTreeWidgetItem* navTreeCheckpoints_   = nullptr;
    QTreeWidgetItem* navTreeSyscalls_      = nullptr;
    QTreeWidgetItem* navTreeFunctions_     = nullptr;
    QTreeWidgetItem* navTreeMemRegions_    = nullptr;

    // ── right dock: inspector ─────────────────────────────────────────────
    QLabel*         inspBadge_       = nullptr;
    QTableWidget*   inspTable_       = nullptr;
    QTreeWidget*    callStackTree_   = nullptr;

    // ── bottom dock: output tabs ──────────────────────────────────────────
    // Search
    QLineEdit*      searchEdit_      = nullptr;
    QListWidget*    searchResults_   = nullptr;
    // Memory
    QComboBox*      checkpointCombo_ = nullptr;
    QTreeWidget*    memoryTree_      = nullptr;
    QPlainTextEdit* hexView_         = nullptr;
    QLineEdit*      stringsFilter_   = nullptr;
    QPlainTextEdit* stringsView_     = nullptr;
    // State Diff
    QPlainTextEdit* diffEdit_        = nullptr;
    QPushButton*    diffButton_      = nullptr;
    QPushButton*    diffAButton_     = nullptr;
    QPushButton*    diffBButton_     = nullptr;
    // Exec graph (embedded in tab)
    QTreeWidget*    execGraphTree_   = nullptr;

    // ── toolbar ───────────────────────────────────────────────────────────
    QLabel*         statusLabel_     = nullptr;
    // legacy compat (kept for welcomeLabel_ / mainSplitter_ refs, set to nullptr)
    QLabel*         welcomeLabel_    = nullptr;
    QSplitter*      mainSplitter_    = nullptr;
};

} } // namespace chronos::gui
