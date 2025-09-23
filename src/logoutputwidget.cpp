#include "../include/logoutputwidget.h"
#include <QDateTime>

LogOutputWidget::LogOutputWidget(QWidget *parent)
    : QWidget(parent) {

    // 创建日志文本框
    m_logTextEdit = new QTextEdit(this);
    m_logTextEdit->setReadOnly(true);
    m_logTextEdit->setFont(QFont("Consolas", 10));

    // 创建清除按钮
    m_clearButton = new QPushButton("清除日志", this);

    // 布局
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_logTextEdit);
    layout->addWidget(m_clearButton);

    setLayout(layout);

    // 连接信号
    connect(m_clearButton, &QPushButton::clicked, this, &LogOutputWidget::clearLog);
}

void LogOutputWidget::appendMessage(const QString &message) {
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    m_logTextEdit->append("<span style='color:blue'>" + timestamp + message + "</span>");
}

void LogOutputWidget::appendError(const QString &error) {
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    m_logTextEdit->append("<span style='color:red'>" + timestamp + error + "</span>");
}

void LogOutputWidget::clearLog() {
    m_logTextEdit->clear();
}