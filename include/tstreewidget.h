#ifndef TSTREEWIDGET_H
#define TSTREEWIDGET_H

#include <QTreeWidget>
#include <QXmlStreamReader>
#include "tsfilehandler.h"

class TsTreeWidget : public QTreeWidget {
    Q_OBJECT
public:
    explicit TsTreeWidget(QWidget *parent = nullptr);

    bool loadTsFile(const QString &filePath);
    QString selectedContext() const;
    QString selectedSource() const;

    signals:
        void contextSelected(const QString &contextName);
    void messageSelected(const QString &contextName, const QString &source);

private slots:
    void onItemSelectionChanged();

private:
    void parseContext(QXmlStreamReader &reader);
    TranslationState parseMessage(QXmlStreamReader &reader, QTreeWidgetItem *contextItem);
    void calculateContextProgress(QTreeWidgetItem *contextItem);
    QString stateToString(TranslationState state) const;

    QString m_currentFilePath;
};

#endif // TSTREEWIDGET_H