#include "../include/translationservice.h"

#include <iostream>
#include <ostream>
#include <QSettings>
#include <QDebug>
#include <QCryptographicHash>
#include <QUrl>
#include <QRandomGenerator>
#include <QTimer>

struct {
    QStringList batchQueue;
    QMap<QString, QString> batchResults;
    int currentBatchIndex = 0;
    int batchTotal = 0;
} batchState;
bool TranslationService::isBatchRunning() const {
    return !batchState.batchQueue.isEmpty();
}
TranslationService::TranslationService(QObject *parent)
    : QObject(parent), m_networkManager(new QNetworkAccessManager(this)),
      m_currentEngine(GoogleTranslate) {
    QSettings settings;
    Engine engines[] = {GoogleTranslate, BaiduTranslate, DeepLTranslate, YoudaoTranslate};
    for (Engine engine : engines) {
        QString engineKey = QString("Translation/%1/").arg(static_cast<int>(engine));

        EngineConfig config;
        config.apiKey = settings.value(engineKey + "apiKey", "").toString();
        config.sourceLang = settings.value(engineKey + "sourceLang", "en").toString();
        config.targetLang = settings.value(engineKey + "targetLang", "zh-CN").toString();

        m_engineConfigs[engine] = config;
    }

    int savedEngine = settings.value("Translation/currentEngine", GoogleTranslate).toInt();
    m_currentEngine = static_cast<Engine>(savedEngine);

    connect(m_networkManager, &QNetworkAccessManager::finished,
            this, &TranslationService::onTranslationFinished);
}

void TranslationService::setCurrentEngine(Engine engine) {
    m_currentEngine = engine;
    QSettings settings;
    settings.setValue("Translation/currentEngine", static_cast<int>(engine));
}

void TranslationService::setApiKey(Engine engine, const QString &apiKey) {
    m_engineConfigs[engine].apiKey = apiKey;

    QSettings settings;
    QString engineKey = QString("Translation/%1/apiKey").arg(static_cast<int>(engine));
    settings.setValue(engineKey, apiKey);
}

void TranslationService::setLanguages(Engine engine, const QString &sourceLang, const QString &targetLang) {
    m_engineConfigs[engine].sourceLang = sourceLang;
    m_engineConfigs[engine].targetLang = targetLang;

    QSettings settings;
    QString engineKey = QString("Translation/%1/").arg(static_cast<int>(engine));
    settings.setValue(engineKey + "sourceLang", sourceLang);
    settings.setValue(engineKey + "targetLang", targetLang);
}

void TranslationService::translateText(const QString &text) {
    if (text.isEmpty()) {
        emit errorOccurred("源文本为空");
        return;
    }

    QString apiKey = m_engineConfigs[m_currentEngine].apiKey;
    if (apiKey.isEmpty()) {
        emit errorOccurred(QString("%1 API密钥未设置").arg(engineName(m_currentEngine)));
        return;
    }

    switch (m_currentEngine) {
        case GoogleTranslate:
            translateWithGoogle(text);
            break;
        case BaiduTranslate:
            translateWithBaidu(text);
            break;
        case DeepLTranslate:
            translateWithDeepL(text);
            break;
        case YoudaoTranslate:
            translateWithYoudao(text);
            break;
    }
}

void TranslationService::translateBatch(const QStringList &texts) {
    if (texts.isEmpty()) {
        emit errorOccurred("没有要翻译的文本");
        return;
    }

    QString apiKey = m_engineConfigs[m_currentEngine].apiKey;
    if (apiKey.isEmpty()) {
        emit errorOccurred(QString("%1 API密钥未设置").arg(engineName(m_currentEngine)));
        return;
    }

    // 重置批量翻译状态
    batchState.batchQueue = texts;
    batchState.batchResults.clear();
    batchState.currentBatchIndex = 0;
    batchState.batchTotal = texts.size();
    emit batchProgress(0, texts.size());
    translateNextInBatch();
}

void TranslationService::translateNextInBatch() {
    if (batchState.currentBatchIndex >= batchState.batchQueue.size()) {
        emit batchTranslationCompleted(batchState.batchResults);
        return;
    }

    QString text = batchState.batchQueue[batchState.currentBatchIndex];
    batchState.currentBatchIndex++;

    int delay = 0;
    switch (m_currentEngine) {
        case BaiduTranslate:
        case YoudaoTranslate:
            delay = 1000; // 1000ms 延迟避免频率限制
            break;
        default:
            delay = 100;
    }

    // 延迟后翻译
    QTimer::singleShot(delay, this, [this, text]() {
        translateText(text);
        emit batchProgress(batchState.currentBatchIndex, batchState.batchTotal);
    });
}

void TranslationService::cancelBatch() {
    batchState.batchQueue.clear();
    batchState.batchResults.clear();
    batchState.currentBatchIndex = 0;
    batchState.batchTotal = 0;
    emit batchCanceled();
}

QList<TranslationService::Engine> TranslationService::supportedEngines() {
    return {GoogleTranslate, BaiduTranslate, DeepLTranslate, YoudaoTranslate};
}

QString TranslationService::engineName(Engine engine) {
    switch (engine) {
        case GoogleTranslate: return "Google Translate";
        case BaiduTranslate: return "百度翻译";
        case DeepLTranslate: return "DeepL";
        case YoudaoTranslate: return "有道翻译";
    }
    return "未知";
}

// Google翻译实现
void TranslationService::translateWithGoogle(const QString &text) {
    QUrl url("https://translation.googleapis.com/language/translate/v2");
    QUrlQuery query;
    query.addQueryItem("key", m_engineConfigs[GoogleTranslate].apiKey);
    query.addQueryItem("q", text);
    query.addQueryItem("source", toGoogleLanguageCode(m_engineConfigs[GoogleTranslate].sourceLang));
    query.addQueryItem("target", toGoogleLanguageCode(m_engineConfigs[GoogleTranslate].targetLang));
    query.addQueryItem("format", "text");
    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);
    reply->setProperty("originalText", text);
    reply->setProperty("engine", GoogleTranslate);
}

void TranslationService::batchTranslateWithGoogle(const QStringList &texts) {
    QUrl url("https://translation.googleapis.com/language/translate/v2");
    QUrlQuery query;
    query.addQueryItem("key", m_engineConfigs[GoogleTranslate].apiKey);
    query.addQueryItem("source", toGoogleLanguageCode(m_engineConfigs[GoogleTranslate].sourceLang));
    query.addQueryItem("target", toGoogleLanguageCode(m_engineConfigs[GoogleTranslate].targetLang));
    query.addQueryItem("format", "text");

    for (const QString &text : texts) {
        query.addQueryItem("q", text);
    }

    url.setQuery(query);

    QNetworkRequest request(url);
    QNetworkReply *reply = m_networkManager->get(request);
    reply->setProperty("engine", GoogleTranslate);
    reply->setProperty("isBatch", true);
}

// 百度翻译实现
void TranslationService::translateWithBaidu(const QString &text) {
    QUrl url("https://fanyi-api.baidu.com/api/trans/vip/translate");

    QString appId = m_engineConfigs[BaiduTranslate].apiKey.split(':').value(0);
    QString secretKey = m_engineConfigs[BaiduTranslate].apiKey.split(':').value(1);

    if (appId.isEmpty() || secretKey.isEmpty()) {
        emit errorOccurred("百度翻译API密钥格式不正确");
        return;
    }

    QString salt = QString::number(QRandomGenerator::global()->generate());
    QString sign = QCryptographicHash::hash(
        (appId + text + salt + secretKey).toUtf8(),
        QCryptographicHash::Md5
    ).toHex();

    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("from", toBaiduLanguageCode(m_engineConfigs[BaiduTranslate].sourceLang));
    query.addQueryItem("to", toBaiduLanguageCode(m_engineConfigs[BaiduTranslate].targetLang));
    query.addQueryItem("appid", appId);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("originalText", text);
    reply->setProperty("engine", BaiduTranslate);
}

void TranslationService::batchTranslateWithBaidu(const QStringList &texts) {
    translateBatch(texts);
}

void TranslationService::translateWithDeepL(const QString &text) {
    QUrl url("https://api-free.deepl.com/v2/translate");

    QUrlQuery query;
    query.addQueryItem("auth_key", m_engineConfigs[DeepLTranslate].apiKey);
    query.addQueryItem("text", text);
    query.addQueryItem("source_lang", toDeepLLanguageCode(m_engineConfigs[DeepLTranslate].sourceLang));
    query.addQueryItem("target_lang", toDeepLLanguageCode(m_engineConfigs[DeepLTranslate].targetLang));

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("originalText", text);
    reply->setProperty("engine", DeepLTranslate);
}

void TranslationService::batchTranslateWithDeepL(const QStringList &texts) {
    QUrl url("https://api-free.deepl.com/v2/translate");

    QUrlQuery query;
    query.addQueryItem("auth_key", m_engineConfigs[DeepLTranslate].apiKey);
    query.addQueryItem("source_lang", toDeepLLanguageCode(m_engineConfigs[DeepLTranslate].sourceLang));
    query.addQueryItem("target_lang", toDeepLLanguageCode(m_engineConfigs[DeepLTranslate].targetLang));

    for (const QString &text : texts) {
        query.addQueryItem("text", text);
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("engine", DeepLTranslate);
    reply->setProperty("isBatch", true);
}

void TranslationService::translateWithYoudao(const QString &text) {
    QUrl url("https://openapi.youdao.com/api");

    QString appKey = m_engineConfigs[YoudaoTranslate].apiKey.split(':').value(0);
    QString secretKey = m_engineConfigs[YoudaoTranslate].apiKey.split(':').value(1);

    if (appKey.isEmpty() || secretKey.isEmpty()) {
        emit errorOccurred("有道翻译API密钥格式不正确");
        return;
    }

    QString salt = QString::number(QRandomGenerator::global()->generate());
    QString sign = QCryptographicHash::hash(
        (appKey + text + salt + secretKey).toUtf8(),
        QCryptographicHash::Sha256
    ).toHex();

    QUrlQuery query;
    query.addQueryItem("q", text);
    query.addQueryItem("from", toYoudaoLanguageCode(m_engineConfigs[YoudaoTranslate].sourceLang));
    query.addQueryItem("to", toYoudaoLanguageCode(m_engineConfigs[YoudaoTranslate].targetLang));
    query.addQueryItem("appKey", appKey);
    query.addQueryItem("salt", salt);
    query.addQueryItem("sign", sign);
    query.addQueryItem("signType", "v3");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_networkManager->post(request, query.toString(QUrl::FullyEncoded).toUtf8());
    reply->setProperty("originalText", text);
    reply->setProperty("engine", YoudaoTranslate);
}

void TranslationService::batchTranslateWithYoudao(const QStringList &texts) {
    translateBatch(texts);
}

void TranslationService::onTranslationFinished(QNetworkReply *reply) {
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = QString("网络错误: %1").arg(reply->errorString());
        emit errorOccurred(errorMsg);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    Engine engine = static_cast<Engine>(reply->property("engine").toInt());
    bool isBatch = reply->property("isBatch").toBool();
    QString originalText = reply->property("originalText").toString();

    QString translatedText;
    QMap<QString, QString> batchResults;

    switch (engine) {
        case GoogleTranslate:
            if (isBatch) {
                QJsonDocument doc = QJsonDocument::fromJson(responseData);
                QJsonObject obj = doc.object().value("data").toObject();
                QJsonArray translations = obj.value("translations").toArray();

                for (const QJsonValue &val : translations) {
                    QJsonObject transObj = val.toObject();
                    QString original = transObj.value("original").toString();
                    QString translated = transObj.value("translatedText").toString();
                    batchResults[original] = translated;
                }
            } else {
                translatedText = parseGoogleResponse(responseData);
            }
            break;

        case BaiduTranslate:
            translatedText = parseBaiduResponse(responseData);
            if (!batchState.batchQueue.isEmpty() && !translatedText.isEmpty()) {
                batchState.batchResults[originalText] = translatedText;
                emit singleTranslationCompleted("", originalText, translatedText);
            }
            break;

        case DeepLTranslate:
            if (isBatch) {
                QJsonDocument doc = QJsonDocument::fromJson(responseData);
                QJsonArray translations = doc.object().value("translations").toArray();

                for (const QJsonValue &val : translations) {
                    QJsonObject transObj = val.toObject();
                    QString original = transObj.value("text").toString();
                    QString translated = transObj.value("text").toString();
                    batchResults[original] = translated;
                }
            } else {
                translatedText = parseDeepLResponse(responseData);
            }
            break;

        case YoudaoTranslate:
            translatedText = parseYoudaoResponse(responseData);
            // 保存批量翻译结果
            if (!batchState.batchQueue.isEmpty() && !translatedText.isEmpty()) {
                batchState.batchResults[originalText] = translatedText;
                emit singleTranslationCompleted("", originalText, translatedText);
            }
            break;
    }

    if (isBatch) {
        if (!batchResults.isEmpty()) {
            emit batchTranslationCompleted(batchResults);
        } else {
            emit errorOccurred("批量翻译结果为空");
        }
    } else {
        if (!translatedText.isEmpty()) {
            emit translationCompleted(originalText, translatedText);
        } else {
            emit errorOccurred("翻译结果为空", originalText);
        }
    }

    // 继续批量翻译
    if (!batchState.batchQueue.isEmpty()) {
        translateNextInBatch();
    }

    reply->deleteLater();
}

QString TranslationService::parseGoogleResponse(const QByteArray &response) {
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonObject obj = doc.object().value("data").toObject();
    QJsonArray translations = obj.value("translations").toArray();

    if (!translations.isEmpty()) {
        return translations.first().toObject().value("translatedText").toString();
    }
    return "";
}

QString TranslationService::parseBaiduResponse(const QByteArray &response) {
    QJsonDocument doc = QJsonDocument::fromJson(response);

    // 检查错误
    if (doc.object().contains("error_code")) {
        int errorCode = doc.object().value("error_code").toInt();
        QString errorMsg = doc.object().value("error_msg").toString();

        // 常见错误代码处理
        switch (errorCode) {
            case 52001: errorMsg = "请求超时"; break;
            case 52002: errorMsg = "系统错误"; break;
            case 52003: errorMsg = "未授权用户"; break;
            case 54000: errorMsg = "必填参数为空"; break;
            case 54001: errorMsg = "签名错误"; break;
            case 54003: errorMsg = "访问频率受限"; break;
            case 54004: errorMsg = "账户余额不足"; break;
            case 54005: errorMsg = "长请求频繁"; break;
            case 58000: errorMsg = "客户端IP非法"; break;
            case 58001: errorMsg = "译文语言不支持"; break;
            case 58002: errorMsg = "服务已关闭"; break;
        }

        emit errorOccurred(QString("百度翻译错误 (%1): %2").arg(errorCode).arg(errorMsg));
        return "";
    }

    QJsonArray transResults = doc.object().value("trans_result").toArray();

    if (!transResults.isEmpty()) {
        return transResults.first().toObject().value("dst").toString();
    }
    return "";
}

QString TranslationService::parseDeepLResponse(const QByteArray &response) {
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonArray translations = doc.object().value("translations").toArray();

    if (!translations.isEmpty()) {
        return translations.first().toObject().value("text").toString();
    }
    return "";
}

QString TranslationService::parseYoudaoResponse(const QByteArray &response) {
    QJsonDocument doc = QJsonDocument::fromJson(response);
    QJsonArray translations = doc.object().value("translation").toArray();

    if (!translations.isEmpty()) {
        return translations.first().toString();
    }
    return "";
}

QString TranslationService::toGoogleLanguageCode(const QString &lang) {
    if (lang == "zh-CN") return "zh";
    if (lang == "zh-TW") return "zh-TW";
    return lang;
}

QString TranslationService::toBaiduLanguageCode(const QString &lang) {
    static QMap<QString, QString> langMap = {
        {"zh-CN", "zh"}, {"en", "en"}, {"ja", "jp"}, {"ko", "kor"}, {"fr", "fra"}, {"de", "de"}
    };
    return langMap.value(lang, "auto");
}

QString TranslationService::toDeepLLanguageCode(const QString &lang) {
    static QMap<QString, QString> langMap = {
        {"zh-CN", "ZH"}, {"en", "EN"}, {"ja", "JA"}, {"ko", "KO"}, {"fr", "FR"}, {"de", "DE"}
    };
    return langMap.value(lang, "EN");
}

QString TranslationService::toYoudaoLanguageCode(const QString &lang) {
    static QMap<QString, QString> langMap = {
        {"zh-CN", "zh-CHS"}, {"en", "en"}, {"ja", "ja"}, {"ko", "ko"}, {"fr", "fr"}, {"de", "de"}
    };
    return langMap.value(lang, "auto");
}