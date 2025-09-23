#ifndef TRANSLATIONSETTINGSDIALOG_H
#define TRANSLATIONSETTINGSDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include "translationservice.h"

class TranslationSettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit TranslationSettingsDialog(QWidget *parent = nullptr);

    TranslationService::Engine currentEngine() const;
    QString apiKey(TranslationService::Engine engine) const;
    QString sourceLanguage(TranslationService::Engine engine) const;
    QString targetLanguage(TranslationService::Engine engine) const;

private:
    QTabWidget *m_tabWidget;
    QComboBox *m_engineCombo;

    struct EngineSettings {
        QLineEdit *apiKeyEdit;
        QComboBox *sourceLangCombo;
        QComboBox *targetLangCombo;
    };

    QMap<TranslationService::Engine, EngineSettings> m_engineSettings;

    void setupEngineTab(TranslationService::Engine engine, const QString &name);
};

#endif // TRANSLATIONSETTINGSDIALOG_H