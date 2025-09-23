#include "../include/translationsettingsdialog.h"
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSettings>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>

TranslationSettingsDialog::TranslationSettingsDialog(QWidget *parent)
    : QDialog(parent) {

    setWindowTitle("翻译设置");
    setWindowIcon(QIcon(":/icons/settings.svg"));
    resize(600, 400);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QGroupBox *engineGroup = new QGroupBox("当前翻译引擎", this);
    QFormLayout *engineLayout = new QFormLayout(engineGroup);

    m_engineCombo = new QComboBox(this);
    for (TranslationService::Engine engine : TranslationService::supportedEngines()) {
        m_engineCombo->addItem(TranslationService::engineName(engine), engine);
    }
    engineLayout->addRow("选择引擎:", m_engineCombo);

    mainLayout->addWidget(engineGroup);

    m_tabWidget = new QTabWidget(this);

    for (TranslationService::Engine engine : TranslationService::supportedEngines()) {
        setupEngineTab(engine, TranslationService::engineName(engine));
    }

    mainLayout->addWidget(m_tabWidget);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    QSettings settings;
    int savedEngine = settings.value("Translation/currentEngine", TranslationService::GoogleTranslate).toInt();
    m_engineCombo->setCurrentIndex(m_engineCombo->findData(savedEngine));
}

void TranslationSettingsDialog::setupEngineTab(TranslationService::Engine engine, const QString &name) {
    QWidget *tab = new QWidget(this);
    QFormLayout *formLayout = new QFormLayout(tab);

    EngineSettings settings;

    settings.apiKeyEdit = new QLineEdit(tab);
    settings.apiKeyEdit->setPlaceholderText("输入API密钥");
    formLayout->addRow("API密钥:", settings.apiKeyEdit);

    // 语言选择
    settings.sourceLangCombo = new QComboBox(tab);
    settings.targetLangCombo = new QComboBox(tab);

    // 添加常用语言
    QStringList languages = {
        "自动检测", "en", "zh-CN", "zh-TW", "ja", "ko", "fr", "de", "es", "ru", "ar"
    };

    QStringList languageNames = {
        "自动检测", "英语", "简体中文", "繁体中文", "日语", "韩语", "法语", "德语", "西班牙语", "俄语", "阿拉伯语"
    };

    for (int i = 0; i < languages.size(); i++) {
        settings.sourceLangCombo->addItem(languageNames[i], languages[i]);
        settings.targetLangCombo->addItem(languageNames[i], languages[i]);
    }

    formLayout->addRow("源语言:", settings.sourceLangCombo);
    formLayout->addRow("目标语言:", settings.targetLangCombo);

    QLabel *hintLabel = new QLabel(tab);
    switch (engine) {
        case TranslationService::BaiduTranslate:
            hintLabel->setText("提示：百度翻译API密钥格式为 appid:secretKey");
            break;
        case TranslationService::YoudaoTranslate:
            hintLabel->setText("提示：有道翻译API密钥格式为 appKey:secretKey");
            break;
        default:
            hintLabel->setText("");
    }
    formLayout->addRow(hintLabel);

    m_tabWidget->addTab(tab, name);
    m_engineSettings[engine] = settings;

    QSettings settingsStore;
    QString engineKey = QString("Translation/%1/").arg(static_cast<int>(engine));

    settings.apiKeyEdit->setText(settingsStore.value(engineKey + "apiKey", "").toString());

    QString sourceLang = settingsStore.value(engineKey + "sourceLang", "en").toString();
    int sourceIndex = settings.sourceLangCombo->findData(sourceLang);
    if (sourceIndex >= 0) settings.sourceLangCombo->setCurrentIndex(sourceIndex);

    QString targetLang = settingsStore.value(engineKey + "targetLang", "zh-CN").toString();
    int targetIndex = settings.targetLangCombo->findData(targetLang);
    if (targetIndex >= 0) settings.targetLangCombo->setCurrentIndex(targetIndex);
}

TranslationService::Engine TranslationSettingsDialog::currentEngine() const {
    return static_cast<TranslationService::Engine>(m_engineCombo->currentData().toInt());
}

QString TranslationSettingsDialog::apiKey(TranslationService::Engine engine) const {
    return m_engineSettings.value(engine).apiKeyEdit->text();
}

QString TranslationSettingsDialog::sourceLanguage(TranslationService::Engine engine) const {
    return m_engineSettings.value(engine).sourceLangCombo->currentData().toString();
}

QString TranslationSettingsDialog::targetLanguage(TranslationService::Engine engine) const {
    return m_engineSettings.value(engine).targetLangCombo->currentData().toString();
}