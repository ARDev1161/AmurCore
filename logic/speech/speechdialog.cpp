#include "speechdialog.h"
#include "ui_speechdialog.h"


SpeechDialog::SpeechDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SpeechDialog),
    speech(nullptr),
    voices{new QVector<QVoice>},
    engines(new QVector<QString>),
    listRows(nullptr),
    languages(nullptr)
{
    ui->setupUi(this);
   QLoggingCategory::setFilterRules(QStringLiteral("qt.speech.tts=true \n qt.speech.tts.*=true"));
    ui->engine->addItem("Default", QString("default"));

    foreach (QString eng, QTextToSpeech::availableEngines()){
        ui->engine->addItem(eng, eng);
        this->engines->append(eng);
    }

//    foreach (QVoice voi, QTextToSpeech::availableVoices()){
//        ui->voice->addItem(voi, voi);
//        this->voices->append(voi);
//    }
    ui->engine->setCurrentIndex(0);
    engineSelected(0);

    connect(ui->speakButton, &QPushButton::clicked, this, &SpeechDialog::speak);
    connect(ui->pitch, &QSlider::valueChanged, this, &SpeechDialog::setPitch);
    connect(ui->rate, &QSlider::valueChanged, this, &SpeechDialog::setRate);
    connect(ui->volume, &QSlider::valueChanged, this, &SpeechDialog::setVolume);
    connect(ui->engine, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SpeechDialog::engineSelected);
}

SpeechDialog::~SpeechDialog()
{

    disconnect(ui->speakButton, &QPushButton::clicked, this, &SpeechDialog::speak);
    disconnect(ui->pitch, &QSlider::valueChanged, this, &SpeechDialog::setPitch);
    disconnect(ui->rate, &QSlider::valueChanged, this, &SpeechDialog::setRate);
    disconnect(ui->volume, &QSlider::valueChanged, this, &SpeechDialog::setVolume);

    disconnect(ui->engine, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SpeechDialog::engineSelected);
    disconnect(ui->voice, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SpeechDialog::voiceSelected);

    connect(ui->stopButton, &QPushButton::clicked, speech, &QTextToSpeech::stop);
    connect(ui->pauseButton, &QPushButton::clicked, speech, &QTextToSpeech::pause);
    connect(ui->resumeButton, &QPushButton::clicked, speech, &QTextToSpeech::resume);

    connect(speech, &QTextToSpeech::stateChanged, this, &SpeechDialog::stateChanged);
    connect(speech, &QTextToSpeech::localeChanged, this, &SpeechDialog::localeChanged);

    connect(ui->language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SpeechDialog::languageSelected);

    engines->resize(0);
    engines->removeLast();

    voices->resize(1);
    voices->removeLast();

    languages->resize(1);
    languages->removeLast();

    listRows->resize(1);
    listRows->removeLast();

    engines->resize(1);
    engines->removeLast();

    engines = nullptr;
    speech->deleteLater();
    delete ui;
}

void SpeechDialog::speak()
{
    speech->say(ui->plainTextEdit->toPlainText());
}

void SpeechDialog::stop()
{
    speech->stop();
}

void SpeechDialog::setRate(int rate)
{
    speech->setRate(rate / 10.0);
}

void SpeechDialog::setPitch(int pitch)
{
    speech->setPitch(pitch / 10.0);
}

void SpeechDialog::setVolume(int volume)
{
    speech->setVolume(volume / 100.0);
}

void SpeechDialog::stateChanged(QTextToSpeech::State state)
{
    if (state == QTextToSpeech::Speaking) {
        ui->statusbar->showMessage("Speech started...");
    } else if (state == QTextToSpeech::Ready)
        ui->statusbar->showMessage("Speech stopped...", 2000);
    else if (state == QTextToSpeech::Paused)
        ui->statusbar->showMessage("Speech paused...");
    else
        ui->statusbar->showMessage("Speech error!");

    ui->pauseButton->setEnabled(state == QTextToSpeech::Speaking);
    ui->resumeButton->setEnabled(state == QTextToSpeech::Paused);
    ui->stopButton->setEnabled(state == QTextToSpeech::Speaking || state == QTextToSpeech::Paused);
}

void SpeechDialog::engineSelected(int index)
{
    QString engineName = ui->engine->itemData(index).toString();
    speech = nullptr;

    if (engineName == "default")
        speech = new QTextToSpeech(this);
    else
        speech = new QTextToSpeech(engineName, this);

    disconnect(ui->language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SpeechDialog::languageSelected);
    ui->language->clear();
    // Populate the languages combobox before connecting its signal.
    QVector<QLocale> locales = speech->availableLocales();
    QLocale current = speech->locale();

    engines->squeeze();

    foreach (const QLocale &locale, locales) {

        QString name(QString("%1 (%2)")
                     .arg(QLocale::languageToString(locale.language()))
                     .arg(QLocale::countryToString(locale.country())));

        QVariant localeVariant(locale);
        ui->language->addItem(name, localeVariant);
        if (locale.name() == current.name())current = locale;
    }
    setRate(ui->rate->value());
    setPitch(ui->pitch->value());
    setVolume(ui->volume->value());

    connect(ui->stopButton, &QPushButton::clicked, speech, &QTextToSpeech::stop);
    connect(ui->pauseButton, &QPushButton::clicked, speech, &QTextToSpeech::pause);
    connect(ui->resumeButton, &QPushButton::clicked, speech, &QTextToSpeech::resume);

    connect(speech, &QTextToSpeech::stateChanged, this, &SpeechDialog::stateChanged);
    connect(speech, &QTextToSpeech::localeChanged, this, &SpeechDialog::localeChanged);

    connect(ui->language, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SpeechDialog::languageSelected);
    localeChanged(current);
}

void SpeechDialog::languageSelected(int language)
{
    QLocale locale = ui->language->itemData(language).toLocale();
    speech->setLocale(locale);
}

void SpeechDialog::voiceSelected(int index)
{
    speech->setVoice(voices->at(index));
}

void SpeechDialog::localeChanged(const QLocale &locale)
{
    QVariant localeVariant(locale);
    ui->language->setCurrentIndex(ui->language->findData(localeVariant));

    disconnect(ui->voice, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SpeechDialog::voiceSelected);
    ui->voice->clear();

    *voices = speech->availableVoices();

    foreach (const QVoice &voice, *voices) {
        ui->voice->addItem(QString("%1 - %2 - %3").arg(voice.name())
                          .arg(QVoice::genderName(voice.gender()))
                          .arg(QVoice::ageName(voice.age())));
        if (voice.name() == speech->voice().name())
            ui->voice->setCurrentIndex(ui->voice->count() - 1);
    }

    connect(ui->voice, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &SpeechDialog::voiceSelected);
}

void SpeechDialog::on_buttonBox_accepted()
{

}
