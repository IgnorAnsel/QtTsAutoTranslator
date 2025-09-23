#include "tstreewidget.h"
#include <QXmlStreamReader>
#include <QDebug>
#include <QHeaderView>

TsTreeWidget::TsTreeWidget(QWidget *parent)
    : QTreeWidget(parent) {

    setColumnCount(3);
    setHeaderLabels({"上下文", "源文本", "状态"});
    setSelectionMode(QAbstractItemView::SingleSelection);
    setAnimated(true);
    setIndentation(15);
    header()->setSectionResizeMode(0, QHeaderView::Interactive);
    header()->setSectionResizeMode(1, QHeaderView::Interactive);
    header()->setSectionResizeMode(2, QHeaderView::Interactive);

    connect(this, &QTreeWidget::itemSelectionChanged,
            this, &TsTreeWidget::onItemSelectionChanged);
}

bool TsTreeWidget::loadTsFile(const QString &filePath) {
    clear();
    m_currentFilePath = filePath;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "无法打开文件:" << filePath;
        return false;
    }

    QXmlStreamReader reader(&file);

    while (!reader.atEnd() && !reader.hasError()) {
        QXmlStreamReader::TokenType token = reader.readNext();

        if (token == QXmlStreamReader::StartElement && reader.name() == "context") {
            parseContext(reader);
        }
    }

    if (reader.hasError()) {
        qWarning() << "XML解析错误:" << reader.errorString();
        file.close();
        return false;
    }

    file.close();
    expandAll(); // 默认展开所有节点
    return true;
}

void TsTreeWidget::parseContext(QXmlStreamReader &reader) {
    QString contextName;

    QTreeWidgetItem *contextItem = new QTreeWidgetItem(this);
    contextItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
    int totalMessages = 0;
    int finishedMessages = 0;

    while (!(reader.atEnd() && !reader.hasError())) {
        QXmlStreamReader::TokenType token = reader.readNext();

        if (token == QXmlStreamReader::StartElement) {
            if (reader.name() == "name") {
                contextName = reader.readElementText();
                contextItem->setText(0, contextName);
                contextItem->setData(0, Qt::UserRole, contextName);
            } else if (reader.name() == "message") {
                totalMessages++;
                TranslationState state = parseMessage(reader, contextItem);
                // 如果状态是已完成，增加计数
                if (state == TranslationState::Finished) {
                    finishedMessages++;
                }
            }
        } else if (token == QXmlStreamReader::EndElement && reader.name() == "context") {
            break;
        }
    }

    contextItem->setText(2, QString("%1/%2").arg(finishedMessages).arg(totalMessages));
    // qDebug() << "Context:" << contextName << "Finished:" << finishedMessages << "Total:" << totalMessages;

    double progress = totalMessages > 0 ? (double)finishedMessages / totalMessages : 0;
    if (progress < 0.3) {
        contextItem->setForeground(2, QColor(200, 0, 0));
    } else if (progress < 0.7) {
        contextItem->setForeground(2, QColor(200, 150, 0));
    } else {
        contextItem->setForeground(2, QColor(0, 150, 0));
    }
}

TranslationState TsTreeWidget::parseMessage(QXmlStreamReader &reader, QTreeWidgetItem *contextItem) {
    QString sourceText;
    QString translation;
    TranslationState state = TranslationState::Unfinished;

    // 创建消息节点
    QTreeWidgetItem *messageItem = new QTreeWidgetItem(contextItem);
    messageItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

    while (!(reader.tokenType() == QXmlStreamReader::EndElement &&
             reader.name() == "message")) {

        if (reader.tokenType() == QXmlStreamReader::StartElement) {
            if (reader.name() == "source") {
                sourceText = reader.readElementText();
                messageItem->setText(1, sourceText);
                messageItem->setData(1, Qt::UserRole, sourceText);
            } else if (reader.name() == "translation") {
                QXmlStreamAttributes attrs = reader.attributes();
                if (attrs.hasAttribute("type")) {
                    QString stateStr = attrs.value("type").toString();
                    if (stateStr == "unfinished") state = TranslationState::Unfinished;
                    else if (stateStr == "vanished") state = TranslationState::Vanished;
                    else if (stateStr == "obsolete") state = TranslationState::Obsolete;
                    else state = TranslationState::Finished; // 默认完成
                } else {
                    state = TranslationState::Finished;
                }
                translation = reader.readElementText();
                messageItem->setText(2, stateToString(state));

                if (state == TranslationState::Unfinished) {
                    // messageItem->setIcon(2, QIcon(":/icons/warning.svg"));
                    messageItem->setForeground(2, QColor(200, 0, 0));
                } else if (state == TranslationState::Obsolete) {
                    // messageItem->setIcon(2, QIcon(":/icons/obsolete.svg"));
                    messageItem->setForeground(2, QColor(150, 150, 150));
                } else {
                    // messageItem->setIcon(2, QIcon(":/icons/check.svg"));
                    messageItem->setForeground(2, QColor(0, 150, 0));
                }
            } else if (reader.name() == "comment") {
                reader.readElementText();
            } else if (reader.name() == "location") {
                reader.readNext();
            }
        }
        reader.readNext();
    }

    return state;
}

void TsTreeWidget::calculateContextProgress(QTreeWidgetItem *contextItem) {
    int totalMessages = contextItem->childCount();
    int finishedMessages = 0;

    for (int i = 0; i < totalMessages; i++) {
        QTreeWidgetItem *child = contextItem->child(i);
        QString stateText = child->text(2);

        if (stateText == "已完成" || stateText == "finished") {

            finishedMessages++;
        }
    }

    contextItem->setText(2, QString("%1/%2").arg(finishedMessages).arg(totalMessages));
    qDebug() << finishedMessages;
    double progress = totalMessages > 0 ? (double)finishedMessages / totalMessages : 0;
    if (progress < 0.3) {
        contextItem->setForeground(2, QColor(200, 0, 0));
    } else if (progress < 0.7) {
        contextItem->setForeground(2, QColor(200, 150, 0));
    } else {
        contextItem->setForeground(2, QColor(0, 150, 0));
    }
}

void TsTreeWidget::onItemSelectionChanged() {
    QList<QTreeWidgetItem*> selectedItems = this->selectedItems();
    if (selectedItems.isEmpty()) return;

    QTreeWidgetItem *item = selectedItems.first();

    if (item->parent() == nullptr) {
        QString contextName = item->data(0, Qt::UserRole).toString();
        emit contextSelected(contextName);
    }
    else {
        QString contextName = item->parent()->data(0, Qt::UserRole).toString();
        QString sourceText = item->data(1, Qt::UserRole).toString();
        emit messageSelected(contextName, sourceText);
    }
}

QString TsTreeWidget::selectedContext() const {
    QList<QTreeWidgetItem*> selectedItems = this->selectedItems();
    if (selectedItems.isEmpty()) return "";

    QTreeWidgetItem *item = selectedItems.first();
    if (item->parent() == nullptr) {
        return item->data(0, Qt::UserRole).toString();
    }
    return "";
}

QString TsTreeWidget::selectedSource() const {
    QList<QTreeWidgetItem*> selectedItems = this->selectedItems();
    if (selectedItems.isEmpty()) return "";

    QTreeWidgetItem *item = selectedItems.first();
    if (item->parent() != nullptr) {
        return item->data(1, Qt::UserRole).toString();
    }
    return "";
}

QString TsTreeWidget::stateToString(TranslationState state) const {
    switch (state) {
        case TranslationState::Unfinished: return "未完成";
        case TranslationState::Finished: return "已完成";
        case TranslationState::Vanished: return "已消失";
        case TranslationState::Obsolete: return "已废弃";
        default: return "未知";
    }
}