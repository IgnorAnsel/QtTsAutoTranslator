#ifndef TSFILEHANDLER_H
#define TSFILEHANDLER_H
#include <QObject>
#include <QFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QList>
#include <QMap>

// TS文件条目状态枚举
enum class TranslationState {
    Unfinished,  // 未完成
    Finished,    // 已完成
    Vanished,    // 已消失
    Obsolete     // 已废弃
};

// TS文件条目结构
struct TsEntry {
    QString source;             // 源文本
    QString translation;        // 翻译文本
    TranslationState state;      // 翻译状态
    QStringList comments;       // 注释
    QStringList locations;      // 位置信息 (filename:line)
    QString context;            // 上下文信息
};

// TS文件处理类
class TsFileHandler : public QObject {
    Q_OBJECT

public:
    explicit TsFileHandler(QObject *parent = nullptr);
    bool load(const QString &filePath);
    bool save(const QString &filePath);
    const QList<TsEntry> &entries() const { return m_entries; }
    TsEntry entryAt(int index) const;
    void updateEntryTranslation(int index, const QString &translation);
    void updateEntryState(int index, TranslationState state);
    void addEntry(const TsEntry &entry);
    void removeEntry(int index);
    QString language() const { return m_language; }
    QString version() const { return m_version; }
    QString sourceLanguage() const { return m_sourceLanguage; }
    void setLanguage(const QString &language) { m_language = language; }

    QList<int> findEntries(const QString &searchText, bool searchSource = true, bool searchTranslation = true);
    QList<int> getUntranslatedEntries();

    QList<int> findEntriesBySource(const QString &source);

    QList<int> getNeedsReviewEntries();

    int getContextMessageCount(const QString &contextName) const;

    TsEntry findEntry(const QString &contextName, const QString &source) const;

    void updateEntryTranslation(const QString &contextName, const QString &source, const QString &translation);

    void updateEntryState(const QString &contextName, const QString &source, TranslationState state);
    struct Statistics {
        int totalEntries = 0;
        int translated = 0;
        int unfinished = 0;
        int vanished = 0;
        int obsolete = 0;
    };

    Statistics getStatistics() const;

signals:
    void fileLoaded(bool success);
    void fileSaved(bool success);
    void entryUpdated(int index);
    void entryAdded(int index);
    void entryRemoved(int index);

private:
    QList<TsEntry> m_entries;      // 所有条目
    QString m_filePath;            // 文件路径
    QString m_language;            // 目标语言
    QString m_version;             // TS文件版本
    QString m_sourceLanguage;      // 源语言

    void parseXml(QXmlStreamReader &reader);
    void generateXml(QXmlStreamWriter &writer);
    QString stateToString(TranslationState state) const;
    TranslationState stringToState(const QString &stateStr) const;


};

#endif
