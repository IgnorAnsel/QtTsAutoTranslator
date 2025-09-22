#include "tsfilehandler.h"
TsFileHandler::TsFileHandler(QObject *parent) : QObject(parent) {}

bool TsFileHandler::load(const QString &filePath) {
    m_entries.clear();
    m_filePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit fileLoaded(false);
        return false;
    }

    QXmlStreamReader reader(&file);

    // 解析XML
    parseXml(reader);

    file.close();

    if (reader.hasError()) {
        emit fileLoaded(false);
        return false;
    }

    emit fileLoaded(true);
    return true;
}

bool TsFileHandler::save(const QString &filePath) {
    if (!filePath.isEmpty()) {
        m_filePath = filePath;
    }

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit fileSaved(false);
        return false;
    }

    QXmlStreamWriter writer(&file);
    writer.setAutoFormatting(true);
    writer.setAutoFormattingIndent(2);

    // 生成XML
    generateXml(writer);

    file.close();

    emit fileSaved(true);
    return true;
}

TsEntry TsFileHandler::entryAt(int index) const {
    if (index >= 0 && index < m_entries.size()) {
        return m_entries.at(index);
    }
    return TsEntry();
}

void TsFileHandler::updateEntryTranslation(int index, const QString &translation) {
    if (index >= 0 && index < m_entries.size()) {
        m_entries[index].translation = translation;
        emit entryUpdated(index);
    }
}

void TsFileHandler::updateEntryState(int index, TranslationState state) {
    if (index >= 0 && index < m_entries.size()) {
        m_entries[index].state = state;
        emit entryUpdated(index);
    }
}

void TsFileHandler::addEntry(const TsEntry &entry) {
    m_entries.append(entry);
    emit entryAdded(m_entries.size() - 1);
}

void TsFileHandler::removeEntry(int index) {
    if (index >= 0 && index < m_entries.size()) {
        m_entries.removeAt(index);
        emit entryRemoved(index);
    }
}

QList<int> TsFileHandler::findEntries(const QString &searchText, bool searchSource, bool searchTranslation) {
    QList<int> results;

    if (searchText.isEmpty()) return results;

    for (int i = 0; i < m_entries.size(); ++i) {
        const TsEntry &entry = m_entries.at(i);

        if (searchSource && entry.source.contains(searchText, Qt::CaseInsensitive)) {
            results.append(i);
        } else if (searchTranslation && entry.translation.contains(searchText, Qt::CaseInsensitive)) {
            results.append(i);
        }
    }

    return results;
}

QList<int> TsFileHandler::getUntranslatedEntries() {
    QList<int> results;

    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).state == TranslationState::Unfinished ||
            m_entries.at(i).translation.isEmpty()) {
            results.append(i);
        }
    }

    return results;
}

QList<int> TsFileHandler::getNeedsReviewEntries() {
    QList<int> results;

    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries.at(i).state == TranslationState::Unfinished &&
            !m_entries.at(i).translation.isEmpty()) {
            results.append(i);
        }
    }

    return results;
}

TsFileHandler::Statistics TsFileHandler::getStatistics() const {
    Statistics stats;
    stats.totalEntries = m_entries.size();

    for (const TsEntry &entry : m_entries) {
        switch (entry.state) {
        case TranslationState::Finished: stats.translated++; break;
        case TranslationState::Unfinished: stats.unfinished++; break;
        case TranslationState::Vanished: stats.vanished++; break;
        case TranslationState::Obsolete: stats.obsolete++; break;
        }
    }

    return stats;
}

void TsFileHandler::parseXml(QXmlStreamReader &reader) {
    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();

        if (token == QXmlStreamReader::StartDocument) continue;

        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == "TS") {
                m_version = reader.attributes().value("version").toString();
                m_language = reader.attributes().value("language").toString();
                m_sourceLanguage = reader.attributes().value("sourcelanguage").toString();
            } else if (reader.name() == "context") {
                QString contextName;

                while (!(reader.tokenType() == QXmlStreamReader::EndElement &&
                         reader.name() == "context")) {
                    if (reader.tokenType() == QXmlStreamReader::StartElement) {
                        if (reader.name() == "name") {
                            contextName = reader.readElementText();
                        } else if (reader.name() == "message") {
                            TsEntry entry;
                            entry.context = contextName;

                            while (!(reader.tokenType() == QXmlStreamReader::EndElement &&
                                     reader.name() == "message")) {
                                if (reader.tokenType() == QXmlStreamReader::StartElement) {
                                    if (reader.name() == "source") {
                                        entry.source = reader.readElementText();
                                    } else if (reader.name() == "translation") {
                                        QXmlStreamAttributes attrs = reader.attributes();
                                        if (attrs.hasAttribute("type")) {
                                            entry.state = stringToState(attrs.value("type").toString());
                                        } else {
                                            entry.state = TranslationState::Finished;
                                        }
                                        entry.translation = reader.readElementText();
                                    } else if (reader.name() == "comment") {
                                        entry.comments.append(reader.readElementText());
                                    } else if (reader.name() == "location") {
                                        QXmlStreamAttributes attrs = reader.attributes();
                                        QString location = attrs.value("filename").toString() +
                                                           ":" + attrs.value("line").toString();
                                        entry.locations.append(location);
                                        reader.readNext(); // Skip to end of location
                                    }
                                }
                                reader.readNext();
                            }
                            m_entries.append(entry);
                        }
                    }
                    reader.readNext();
                }
            }
        }
    }
}

void TsFileHandler::generateXml(QXmlStreamWriter &writer) {
    writer.writeStartDocument();
    writer.writeStartElement("TS");
    writer.writeAttribute("version", m_version);
    writer.writeAttribute("language", m_language);
    if (!m_sourceLanguage.isEmpty()) {
        writer.writeAttribute("sourcelanguage", m_sourceLanguage);
    }
    // 按上下文分组条目
    QMap<QString, QList<TsEntry>> contextMap;
    for (const TsEntry &entry : m_entries) {
        contextMap[entry.context].append(entry);
    }
    for (auto it = contextMap.begin(); it != contextMap.end(); ++it) {
        writer.writeStartElement("context");
        writer.writeTextElement("name", it.key());
        for (const TsEntry &entry : it.value()) {
            writer.writeStartElement("message");
            for (const QString &location : entry.locations) {
                QStringList parts = location.split(":");
                if (parts.size() >= 2) {
                    writer.writeStartElement("location");
                    writer.writeAttribute("filename", parts[0]);
                    writer.writeAttribute("line", parts[1]);
                    writer.writeEndElement(); // location
                }
            }
            for (const QString &comment : entry.comments) {
                writer.writeTextElement("comment", comment);
            }
            writer.writeTextElement("source", entry.source);
            writer.writeStartElement("translation");
            if (entry.state != TranslationState::Finished) {
                writer.writeAttribute("type", stateToString(entry.state));
            }
            writer.writeCharacters(entry.translation);
            writer.writeEndElement(); // translation
            writer.writeEndElement(); // message
        }
        writer.writeEndElement(); // context
    }
    writer.writeEndElement(); // TS
    writer.writeEndDocument();
}

QString TsFileHandler::stateToString(TranslationState state) const {
    switch (state) {
    case TranslationState::Unfinished: return "unfinished";
    case TranslationState::Vanished: return "vanished";
    case TranslationState::Obsolete: return "obsolete";
    default: return "";
    }
}

TranslationState TsFileHandler::stringToState(const QString &stateStr) const {
    if (stateStr == "unfinished") return TranslationState::Unfinished;
    if (stateStr == "vanished") return TranslationState::Vanished;
    if (stateStr == "obsolete") return TranslationState::Obsolete;
    return TranslationState::Finished;
}

int TsFileHandler::getContextMessageCount(const QString &contextName) const {
    int count = 0;
    for (const TsEntry &entry : m_entries) {
        if (entry.context == contextName) {
            count++;
        }
    }
    return count;
}

TsEntry TsFileHandler::findEntry(const QString &contextName, const QString &source) const {
    for (const TsEntry &entry : m_entries) {
        if (entry.context == contextName && entry.source == source) {
            return entry;
        }
    }
    return TsEntry(); // 返回一个空的TsEntry
}

void TsFileHandler::updateEntryTranslation(const QString &contextName, const QString &source, const QString &translation) {
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].context == contextName && m_entries[i].source == source) {
            m_entries[i].translation = translation;
            break;
        }
    }
}

void TsFileHandler::updateEntryState(const QString &contextName, const QString &source, TranslationState state) {
    for (int i = 0; i < m_entries.size(); ++i) {
        if (m_entries[i].context == contextName && m_entries[i].source == source) {
            m_entries[i].state = state;
            break;
        }
    }
}
