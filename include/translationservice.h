#ifndef TRANSLATION_SERVICE_H
#define TRANSLATION_SERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QSettings>
#include <QMap>

class TranslationService : public QObject {
    Q_OBJECT
public:
    enum Engine {
        GoogleTranslate,
        BaiduTranslate,
        DeepLTranslate,
        YoudaoTranslate
    };
    Q_ENUM(Engine)

    bool isBatchRunning() const;

    explicit TranslationService(QObject *parent = nullptr);
    void setCurrentEngine(Engine engine);
    void setApiKey(Engine engine, const QString &apiKey);
    void setLanguages(Engine engine, const QString &sourceLang, const QString &targetLang);
    void translateText(const QString &text);
    void translateBatch(const QStringList &texts);

    void translateNextInBatch();

    void cancelBatch();

    static QList<Engine> supportedEngines();
    static QString engineName(Engine engine);

    Engine currentEngine() const {
        return m_currentEngine;
    };


signals:
    void translationCompleted(const QString &original, const QString &translated);
    void singleTranslationCompleted(const QString &context, const QString &source, const QString &translation);
    void batchTranslationCompleted(const QMap<QString, QString> &results);
    void errorOccurred(const QString &errorMessage, const QString &sourceText = "");
    void batchProgress(int current, int total);
    void batchCanceled();
private slots:
    void onTranslationFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    Engine m_currentEngine;

    struct EngineConfig {
        QString apiKey;
        QString sourceLang;
        QString targetLang;
    };

    QMap<Engine, EngineConfig> m_engineConfigs;

    // 翻译方法
    void translateWithGoogle(const QString &text);
    void translateWithBaidu(const QString &text);
    void translateWithDeepL(const QString &text);
    void translateWithYoudao(const QString &text);

    void batchTranslateWithGoogle(const QStringList &texts);
    void batchTranslateWithBaidu(const QStringList &texts);
    void batchTranslateWithDeepL(const QStringList &texts);
    void batchTranslateWithYoudao(const QStringList &texts);

    // 解析响应
    QString parseGoogleResponse(const QByteArray &response);
    QString parseBaiduResponse(const QByteArray &response);
    QString parseDeepLResponse(const QByteArray &response);
    QString parseYoudaoResponse(const QByteArray &response);

    // 语言代码转换
    QString toGoogleLanguageCode(const QString &lang);
    QString toBaiduLanguageCode(const QString &lang);
    QString toDeepLLanguageCode(const QString &lang);
    QString toYoudaoLanguageCode(const QString &lang);
};

#endif // TRANSLATION_SERVICE_H