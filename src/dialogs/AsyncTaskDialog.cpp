
#include "AsyncTaskDialog.h"
#include "common/AsyncTask.h"
#include "ui_AsyncTaskDialog.h"

AsyncTaskDialog::AsyncTaskDialog(AsyncTask::Ptr task, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::AsyncTaskDialog)
    , task(task)
{
    ui->setupUi(this);

    QString title = task->getTitle();
    if (!title.isNull()) {
        setWindowTitle(title);
    }

    // Update log when the task reports text output
    connect(task.data(), &AsyncTask::logChanged, this, &AsyncTaskDialog::updateLog);
    // Also handle tasks that emit a finished(QString) signal with a result string
    connect(task.data(), SIGNAL(finished(QString)), this, SLOT(updateLog(QString)));
    // Close dialog when the task signals completion (parameterless)
    connect(task.data(), &AsyncTask::finished, this, [this]() { close(); });

    updateLog(task->getLog());

    connect(&timer, &QTimer::timeout, this, &AsyncTaskDialog::updateProgressTimer);
    timer.setInterval(1000);
    timer.setSingleShot(false);
    timer.start();

    updateProgressTimer();
}

AsyncTaskDialog::~AsyncTaskDialog() {}

void AsyncTaskDialog::updateLog(const QString &log)
{
    ui->logTextEdit->setPlainText(log);
}

void AsyncTaskDialog::updateProgressTimer()
{
    int secondsElapsed = (task->getElapsedTime() + 500) / 1000;
    int minutesElapsed = secondsElapsed / 60;
    int hoursElapsed = minutesElapsed / 60;

    QString label = tr("Running for") + " ";
    if (hoursElapsed) {
        label += tr("%n hour", "%n hours", hoursElapsed);
        label += " ";
    }
    if (minutesElapsed) {
        label += tr("%n minute", "%n minutes", minutesElapsed % 60);
        label += " ";
    }
    label += tr("%n seconds", "%n second", secondsElapsed % 60);
    ui->timeLabel->setText(label);
}

void AsyncTaskDialog::closeEvent(QCloseEvent *event)
{
    if (interruptOnClose) {
        task->interrupt();
        task->wait();
    }

    QWidget::closeEvent(event);
}

void AsyncTaskDialog::reject()
{
    task->interrupt();
}
