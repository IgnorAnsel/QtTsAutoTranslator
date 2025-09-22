#ifndef TSDETAILWIDGET_H
#define TSDETAILWIDGET_H
#include <QWidget>
#include <QFormLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QGroupBox>
#include <QFileInfo>

class TsDetailWidget : public QWidget {
    Q_OBJECT
public:
    explicit TsDetailWidget(QWidget *parent = nullptr);
    void showContextInfo(const QString &contextName, int messageCount);
    void showMessageInfo(const QString &contextName, const QString &source,
                         const QString &translation, const QString &state,
                         const QStringList &locations, const QStringList &comments);

    void clear();
    QString currentTranslation() const;
    QString currentState() const;

signals:
    void translationChanged(const QString &context, const QString &source,
                            const QString &newTranslation);
    void stateChanged(const QString &context, const QString &source,
                      const QString &newState);
    void saveRequested();

private:
    void setupUi();

    // 上下文信息区域
    QGroupBox *m_contextGroup;
    QLabel *m_contextNameLabel;
    QLabel *m_messageCountLabel;

    // 消息信息区域
    QGroupBox *m_messageGroup;
    QLabel *m_sourceLabel;
    QTextEdit *m_translationEdit;
    QComboBox *m_stateCombo;
    QTextEdit *m_locationsEdit;
    QTextEdit *m_commentsEdit;

    // 操作按钮
    QPushButton *m_saveButton;
    QPushButton *m_copySourceButton;

    QString m_currentContext;
    QString m_currentSource;
};
#endif
