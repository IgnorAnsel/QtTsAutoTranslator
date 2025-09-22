#include "tsdetailwidget.h"

#include <QApplication>
#include <QClipboard>

TsDetailWidget::TsDetailWidget(QWidget *parent)
    : QWidget(parent), m_currentContext(""), m_currentSource("") {

    setupUi();
}

void TsDetailWidget::setupUi() {
    m_contextGroup = new QGroupBox("上下文信息", this);
    QFormLayout *contextLayout = new QFormLayout(m_contextGroup);

    m_contextNameLabel = new QLabel("未选择", m_contextGroup);
    m_messageCountLabel = new QLabel("0", m_contextGroup);

    contextLayout->addRow("上下文名称:", m_contextNameLabel);
    contextLayout->addRow("消息数量:", m_messageCountLabel);
    m_messageGroup = new QGroupBox("消息详情", this);
    QFormLayout *messageLayout = new QFormLayout(m_messageGroup);

    m_sourceLabel = new QLabel("未选择", m_messageGroup);
    m_sourceLabel->setWordWrap(true);

    m_translationEdit = new QTextEdit(m_messageGroup);
    m_translationEdit->setPlaceholderText("输入翻译内容...");
    m_translationEdit->setAcceptRichText(false);
    m_translationEdit->setMinimumHeight(80);

    m_stateCombo = new QComboBox(m_messageGroup);
    m_stateCombo->addItem("已完成", "finished");
    m_stateCombo->addItem("未完成", "unfinished");
    m_stateCombo->addItem("已废弃", "obsolete");
    m_stateCombo->addItem("已消失", "vanished");

    m_locationsEdit = new QTextEdit(m_messageGroup);
    m_locationsEdit->setReadOnly(true);
    m_locationsEdit->setMinimumHeight(60);

    m_commentsEdit = new QTextEdit(m_messageGroup);
    m_commentsEdit->setReadOnly(true);
    m_commentsEdit->setMinimumHeight(60);

    messageLayout->addRow("源文本:", m_sourceLabel);
    messageLayout->addRow("翻译:", m_translationEdit);
    messageLayout->addRow("状态:", m_stateCombo);
    messageLayout->addRow("位置:", m_locationsEdit);
    messageLayout->addRow("注释:", m_commentsEdit);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    m_saveButton = new QPushButton("保存修改", this);
    m_saveButton->setEnabled(false);
    m_copySourceButton = new QPushButton("复制源文本", this);

    buttonLayout->addWidget(m_copySourceButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_saveButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_contextGroup);
    mainLayout->addWidget(m_messageGroup);
    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);

    connect(m_translationEdit, &QTextEdit::textChanged, [this]{
        m_saveButton->setEnabled(true);
    });

    connect(m_stateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]{
        m_saveButton->setEnabled(true);
    });

    connect(m_saveButton, &QPushButton::clicked, [this]{
        if (!m_currentContext.isEmpty() && !m_currentSource.isEmpty()) {
            emit translationChanged(m_currentContext, m_currentSource,
                                    m_translationEdit->toPlainText());
            emit stateChanged(m_currentContext, m_currentSource,
                              m_stateCombo->currentData().toString());
            emit saveRequested();
        }
        m_saveButton->setEnabled(false);
    });

    connect(m_copySourceButton, &QPushButton::clicked, [this]{
        QApplication::clipboard()->setText(m_sourceLabel->text());
    });
}

void TsDetailWidget::showContextInfo(const QString &contextName, int messageCount) {
    m_currentContext = contextName;
    m_currentSource = "";

    m_contextGroup->setVisible(true);
    m_messageGroup->setVisible(false);

    m_contextNameLabel->setText(contextName);
    m_messageCountLabel->setText(QString::number(messageCount));

    m_saveButton->setEnabled(false);
}

void TsDetailWidget::showMessageInfo(const QString &contextName, const QString &source,
                                     const QString &translation, const QString &state,
                                     const QStringList &locations, const QStringList &comments) {
    m_currentContext = contextName;
    m_currentSource = source;

    m_contextGroup->setVisible(true);
    m_messageGroup->setVisible(true);

    m_contextNameLabel->setText(contextName);
    m_sourceLabel->setText(source);
    m_translationEdit->setPlainText(translation);

    int index = m_stateCombo->findData(state);
    if (index >= 0) {
        m_stateCombo->setCurrentIndex(index);
    } else {
        m_stateCombo->setCurrentIndex(0); // 默认为已完成
    }

    m_locationsEdit->setPlainText(locations.join("\n"));

    m_commentsEdit->setPlainText(comments.join("\n"));

    m_saveButton->setEnabled(false);
}

void TsDetailWidget::clear() {
    m_currentContext = "";
    m_currentSource = "";

    m_contextGroup->setVisible(false);
    m_messageGroup->setVisible(false);

    m_contextNameLabel->setText("");
    m_messageCountLabel->setText("");
    m_sourceLabel->setText("");
    m_translationEdit->clear();
    m_stateCombo->setCurrentIndex(0);
    m_locationsEdit->clear();
    m_commentsEdit->clear();

    m_saveButton->setEnabled(false);
}

QString TsDetailWidget::currentTranslation() const {
    return m_translationEdit->toPlainText();
}

QString TsDetailWidget::currentState() const {
    return m_stateCombo->currentData().toString();
}
