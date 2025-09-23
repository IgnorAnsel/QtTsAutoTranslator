#ifndef LOGOUTPUTWIDGET_H
#define LOGOUTPUTWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

class LogOutputWidget : public QWidget {
    Q_OBJECT
public:
    explicit LogOutputWidget(QWidget *parent = nullptr);

    void appendMessage(const QString &message);
    void appendError(const QString &error);
    void clearLog();

private:
    QTextEdit *m_logTextEdit;
    QPushButton *m_clearButton;
};

#endif // LOGOUTPUTWIDGET_H