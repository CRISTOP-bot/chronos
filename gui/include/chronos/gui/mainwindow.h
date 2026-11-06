#pragma once
#include <QMainWindow>
#include <QString>
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

private:
    void buildUi();
    void applyDarkTheme();
    void populateEvents();

    chronos::storage::Reader::Recording recording_{};
    chronos::EventLog eventLog_{};
    chronos::timeline::Timeline timeline_{};

    QTableWidget* eventTable_ = nullptr;
    QSlider* timelineSlider_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLabel* eventDetail_ = nullptr;
    QTreeWidget* memoryTree_ = nullptr;
    QTreeWidget* callStackTree_ = nullptr;
    QTreeWidget* execGraphTree_ = nullptr;
    QPlainTextEdit* diffEdit_ = nullptr;
    QPlainTextEdit* hexView_ = nullptr;
    QPushButton* diffButton_ = nullptr;
    int diffA_ = 0;
    int diffB_ = 0;
};

} } // namespace
