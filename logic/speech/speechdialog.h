#ifndef SPEECHDIALOG_H
#define SPEECHDIALOG_H

#include <QDialog>
#include <QTextToSpeech>
#include <QLoggingCategory>

//#include "sphinxrecognizer.h"

namespace Ui {
class SpeechDialog;
}

class SpeechDialog : public QDialog
{
    Q_OBJECT

    Ui::SpeechDialog *ui;

    int* rate;
    int* pitch;
    int* volume;
    bool* started;

    QString engineName;
    QTextToSpeech * speech;
    QVector<QVoice> * voices;
    QVector<QString> * engines;
    QVector<QString> * listRows;
    QVector<QVariant> * languages;

public:
    explicit SpeechDialog(QWidget *parent = nullptr);
    ~SpeechDialog();

public slots:
    void speak();
    void stop();

    void setRate(int);
    void setPitch(int);
    void setVolume(int volume);

    void stateChanged(QTextToSpeech::State state);
    void engineSelected(int index);
    void languageSelected(int language);
    void voiceSelected(int index);

    void localeChanged(const QLocale &locale);

private slots:
    void on_buttonBox_accepted();
};

#endif // SPEECHDIALOG_H
