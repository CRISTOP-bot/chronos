#pragma once
#include <QMainWindow>
#include <QString>
#include <QSplitter>
#include <QComboBox>
#include "chronos/storage/writer.h"
#include "chronos/events/event_log.h"
#include "chronos/timeline/timeline.h"
#include "chronos/analysis/state_diff.h"

class QTableWidget;
class QSlider;
class QLabel;
class QTreeWidget;
class QPlainTextEdit;
class QPushButton;

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
    void runStateDiff();
    void setDiffA();
    void setDiffB();
    void showExecGraph();

private:
    void buildUi();
    void applyDarkTheme();
    void populateEvents();

    chronos::storage::Reader::Recording recording_{};
    chronos::EventLog eventLog_{};
    chronos::timeline::Timeline timeline_{};

    QTableWidget* eventTable_ = nullptr;
    QSplitter* mainSplitter_ = nullptr;
    QLabel* welcomeLabel_ = nullptr;
    QSlider* timelineSlider_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* eventDetail_ = nullptr;
    QTreeWidget* memoryTree_ = nullptr;
    QComboBox* checkpointCombo_ = nullptr;
    QTreeWidget* callStackTree_ = nullptr;
    QTreeWidget* execGraphTree_ = nullptr;
    QPlainTextEdit* diffEdit_ = nullptr;
    QPlainTextEdit* hexView_ = nullptr;
    QPushButton* diffButton_ = nullptr;
    QPushButton* diffAButton_ = nullptr;
    QPushButton* diffBButton_ = nullptr;
    int selectedEventRow_ = -1;
    int diffA_ = -1;
    int diffB_ = -1;
};

} } // namespace
