#include "mainwindow.h"

#include <complex>
#include <qdebug.h>
#include "tsfilehandler.h"
#include "tstreewidget.h"
#include "../include/translationservice.h"
#include "../include/translationsettingsdialog.h"
#include <QProgressDialog>
#include <QMessageBox>
#include <QTimer>
#include <QStatusBar>
#include <QPropertyAnimation>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    m_translationService = new TranslationService(this);
    connect(m_translationService, &TranslationService::batchTranslationCompleted,
            this, &MainWindow::onBatchTranslationCompleted);
    connect(m_translationService, &TranslationService::errorOccurred,
            this, &MainWindow::onTranslationError);
    setupUi();
}

void MainWindow::setupUi() {

    resize(800,300);
    m_logWidget = new LogOutputWidget(this);
    m_treeWidget = new TsTreeWidget(this);
    m_treeWidget->setMinimumWidth(300);
    m_detailWidget = new TsDetailWidget(this);
    QSplitter *mainSplitter = new QSplitter(Qt::Vertical, this);

    QSplitter *workSplitter = new QSplitter(Qt::Horizontal, this);
    workSplitter->addWidget(m_treeWidget);
    workSplitter->addWidget(m_detailWidget);
    workSplitter->setSizes({300, 500});

    mainSplitter->addWidget(workSplitter);
    mainSplitter->addWidget(m_logWidget);
    mainSplitter->setSizes({400, 100});

    setCentralWidget(mainSplitter);

    connect(this, &MainWindow::logMessage, m_logWidget, &LogOutputWidget::appendMessage);
    connect(this, &MainWindow::logError, m_logWidget, &LogOutputWidget::appendError);


    connect(m_treeWidget, &TsTreeWidget::contextSelected, [this](const QString &contextName){
        int messageCount = m_fileHandler.getContextMessageCount(contextName);
        m_detailWidget->showContextInfo(contextName, messageCount);
    });

    connect(m_treeWidget, &TsTreeWidget::messageSelected, [this](const QString &contextName, const QString &source){
        TsEntry entry = m_fileHandler.findEntry(contextName, source);
        if (!entry.source.isEmpty()) {
            m_detailWidget->showMessageInfo(contextName, entry.source,
                                            entry.translation,
                                            stateToString(entry.state),
                                            entry.locations,
                                            entry.comments);
        }
    });

    connect(m_detailWidget, &TsDetailWidget::translationChanged,
            [this](const QString &context, const QString &source, const QString &newTranslation){
                m_fileHandler.updateEntryTranslation(context, source, newTranslation);
            });

    connect(m_detailWidget, &TsDetailWidget::stateChanged,
            [this](const QString &context, const QString &source, const QString &newState){
                m_fileHandler.updateEntryState(context, source, stringToState(newState));
            });

    connect(m_detailWidget, &TsDetailWidget::saveRequested, [this]{
        if (!m_currentFilePath.isEmpty()) {
            m_fileHandler.save(m_currentFilePath);
            m_treeWidget->loadTsFile(m_currentFilePath);
        }
    });

    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *fileMenu = menuBar->addMenu("文件");
    QAction *openAction = fileMenu->addAction("打开");
    QAction *saveAction = fileMenu->addAction("保存");

    connect(openAction, &QAction::triggered, [this]{
        QString filePath = QFileDialog::getOpenFileName(this, "打开TS文件", "", "TS文件 (*.ts)");
        if (!filePath.isEmpty()) {
            loadTsFile(filePath);
        }
    });

    connect(saveAction, &QAction::triggered, [this]{
        if (!m_currentFilePath.isEmpty()) {
            m_fileHandler.save(m_currentFilePath);
        }
    });
    m_translationToolBar = addToolBar("翻译");
    m_translationToolBar->setMovable(false);
    m_engineCombo = new QComboBox(this);
    for (TranslationService::Engine engine : TranslationService::supportedEngines()) {
        m_engineCombo->addItem(TranslationService::engineName(engine), engine);
    }
    connect(m_engineCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onEngineChanged);

    m_translationToolBar->addWidget(new QLabel("引擎:", this));
    m_translationToolBar->addWidget(m_engineCombo);
    QAction *autoTranslateAction = new QAction(QIcon(":/icons/translate.svg"), "自动翻译", this);
    connect(autoTranslateAction, &QAction::triggered, [this]{
        if (m_detailWidget) {
            m_detailWidget->onAutoTranslateClicked();
        }
    });
    m_translationToolBar->addAction(autoTranslateAction);
    QAction *batchTranslateAction = new QAction(QIcon(":/icons/batch.svg"), "批量翻译", this);
    connect(batchTranslateAction, &QAction::triggered,
            this, &MainWindow::onBatchTranslationRequested);
    m_translationToolBar->addAction(batchTranslateAction);
    QAction *settingsAction = new QAction(QIcon(":/icons/settings.svg"), "设置", this);
    connect(settingsAction, &QAction::triggered,
            this, &MainWindow::openTranslationSettings);
    m_translationToolBar->addAction(settingsAction);

    QSettings settings;
    int savedEngine = settings.value("Translation/currentEngine", TranslationService::GoogleTranslate).toInt();
    m_engineCombo->setCurrentIndex(m_engineCombo->findData(savedEngine));
    setMenuBar(menuBar);


    QStatusBar *statusBar = new QStatusBar(this);
    setStatusBar(statusBar);
    m_statusLabel = new QLabel("就绪", statusBar);
    statusBar->addPermanentWidget(m_statusLabel);
    m_statusProgressBar = new QProgressBar(statusBar);
    m_statusProgressBar->setMinimum(0);
    m_statusProgressBar->setMaximum(100);
    m_statusProgressBar->setTextVisible(false);
    m_statusProgressBar->setFixedWidth(150);
    m_statusProgressBar->setVisible(false); // 初始隐藏
    statusBar->addPermanentWidget(m_statusProgressBar);
    createProgressBarMenu();
    connect(m_translationService, &TranslationService::batchProgress,
            this, &MainWindow::onBatchProgress);
    connect(m_translationService, &TranslationService::batchTranslationCompleted,
            this, &MainWindow::onBatchTranslationCompleted);
    connect(m_translationService, &TranslationService::batchCanceled,
            this, &MainWindow::onBatchTranslationCanceled);
}

void MainWindow::loadTsFile(const QString &filePath) {
    if (m_fileHandler.load(filePath)) {
        m_currentFilePath = filePath;
        m_treeWidget->loadTsFile(filePath);
        m_detailWidget->clear();

        QFileInfo fileInfo(filePath);
        setWindowTitle(QString("QtTsTranslator - %1").arg(fileInfo.fileName()));
    }
}

QString MainWindow::stateToString(TranslationState state) const {
    switch (state) {
        case TranslationState::Unfinished: return "unfinished";
        case TranslationState::Finished: return "finished";
        case TranslationState::Vanished: return "vanished";
        case TranslationState::Obsolete: return "obsolete";
        default: return "finished";
    }
}

TranslationState MainWindow::stringToState(const QString &stateStr) const {
    if (stateStr == "unfinished") return TranslationState::Unfinished;
    if (stateStr == "vanished") return TranslationState::Vanished;
    if (stateStr == "obsolete") return TranslationState::Obsolete;
    return TranslationState::Finished;
}
void MainWindow::openTranslationSettings() {
    TranslationSettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        // 设置当前引擎
        m_translationService->setCurrentEngine(dialog.currentEngine());
        // 设置各引擎的配置
        for (TranslationService::Engine engine : TranslationService::supportedEngines()) {
            m_translationService->setApiKey(engine, dialog.apiKey(engine));
            m_translationService->setLanguages(engine,
                dialog.sourceLanguage(engine),
                dialog.targetLanguage(engine));
        }
    }
}

void MainWindow::onBatchTranslationRequested() {
    QList<int> untranslatedIndices = m_fileHandler.getUntranslatedEntries();
    if (untranslatedIndices.isEmpty()) {
        logMessage("信息: 没有需要翻译的条目");
        return;
    }
    // 收集源文本
    QStringList sourceTexts;
    for (int index : untranslatedIndices) {
        TsEntry entry = m_fileHandler.entryAt(index);
        sourceTexts.append(entry.source);
    }

    QProgressDialog progressDialog("正在批量翻译...", "取消", 0, sourceTexts.size(), this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(0);

    connect(m_translationService, &TranslationService::batchTranslationCompleted,
            &progressDialog, &QProgressDialog::cancel);
    connect(m_translationService, &TranslationService::singleTranslationCompleted,
        this, &MainWindow::onSingleTranslationCompleted);
    m_translationService->translateBatch(sourceTexts);
}

void MainWindow::onSingleTranslationCompleted(const QString &context, const QString &source, const QString &translation) {
    // 查找包含这个源文本的所有条目
    QList<int> indices = m_fileHandler.findEntriesBySource(source);

    if (indices.isEmpty()) {
        logError("警告: 未找到源文本" + source);
        // qWarning() << "未找到源文本:" << source;
        return;
    }

    for (int index : indices) {
        TsEntry entry = m_fileHandler.entryAt(index);
        if (entry.state == TranslationState::Unfinished) {
            m_fileHandler.updateEntryTranslation(index, translation);
            m_fileHandler.updateEntryState(index, TranslationState::Finished);

            // 更新进度
            // m_progressDialog.setValue(m_progressDialog.value() + 1);
        }
    }

    if (!m_currentFilePath.isEmpty()) {
        m_fileHandler.save(m_currentFilePath);
    }
    m_treeWidget->loadTsFile(m_currentFilePath);
}
void MainWindow::onBatchTranslationCompleted(const QMap<QString, QString> &results) {
    QList<int> untranslatedIndices = m_fileHandler.getUntranslatedEntries();

    for (int index : untranslatedIndices) {
        TsEntry entry = m_fileHandler.entryAt(index);
        if (results.contains(entry.source)) {
            m_fileHandler.updateEntryTranslation(index, results[entry.source]);
            m_fileHandler.updateEntryState(index, TranslationState::Finished);
        }
    }
    if (!m_currentFilePath.isEmpty()) {
        m_fileHandler.save(m_currentFilePath);
    }
    m_treeWidget->loadTsFile(m_currentFilePath);
    logMessage(QString("信息: 批量翻译完成，已翻译 %1 个条目").arg(results.size()));
    m_statusProgressBar->setVisible(false);
    m_statusLabel->setText("翻译完成");
    QTimer::singleShot(5000, this, [this]() {
        m_statusLabel->setText("就绪");
    });
}

void MainWindow::onTranslationError(const QString &errorMessage, const QString &sourceText) {
    if (sourceText.isEmpty())
        logError("翻译错误: " + errorMessage);
    else
        logError("翻译错误: " + errorMessage + " (源文本: " + sourceText + ")");
    // QMessageBox::critical(this, "翻译错误", errorMessage);
}

void MainWindow::onEngineChanged(int index) {
    TranslationService::Engine engine = static_cast<TranslationService::Engine>(
        m_engineCombo->itemData(index).toInt());
    m_translationService->setCurrentEngine(engine);
}

void MainWindow::logMessage(const QString &message) {
    m_logWidget->appendMessage(message);
}

void MainWindow::logError(const QString &error) {
    m_logWidget->appendError(error);
}

void MainWindow::onBatchProgress(int current, int total) {
    if (!m_statusProgressBar->isVisible()) {
        m_statusProgressBar->setVisible(true);
    }

    int progress = total > 0 ? (current * 100) / total : 0;

    QPropertyAnimation *animation = new QPropertyAnimation(m_statusProgressBar, "value");
    animation->setDuration(500);
    animation->setStartValue(m_statusProgressBar->value());
    animation->setEndValue(progress);
    animation->setEasingCurve(QEasingCurve::OutQuad);
    animation->start(QAbstractAnimation::DeleteWhenStopped);

    m_statusLabel->setText(QString("翻译中: %1/%2 (%3%)")
                          .arg(current)
                          .arg(total)
                          .arg(progress));

    m_statusProgressBar->setToolTip(QString("已翻译: %1/%2 (%3%)")
                                   .arg(current)
                                   .arg(total)
                                   .arg(progress));
    if (progress < 30) {
        setProgressBarColor("#FF6B6B"); // 红色
    } else if (progress < 70) {
        setProgressBarColor("#FFD166"); // 黄色
    } else {
        setProgressBarColor("#06D6A0"); // 绿色
    }
}

void MainWindow::setProgressBarColor(const QString &color) {
    QString style = QString(
        "QProgressBar {"
        "   border: 1px solid #c0c0c0;"
        "   border-radius: 3px;"
        "   background: #f0f0f0;"
        "   text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "   background: %1;"
        "   border-radius: 2px;"
        "}"
    ).arg(color);

    m_statusProgressBar->setStyleSheet(style);
}

void MainWindow::onBatchTranslationCanceled() {
    m_statusProgressBar->setVisible(false);
    m_statusLabel->setText("翻译已取消");
    QTimer::singleShot(5000, this, [this]() {
        m_statusLabel->setText("就绪");
    });
}

void MainWindow::createProgressBarMenu() {
    m_statusProgressBar->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_statusProgressBar, &QProgressBar::customContextMenuRequested,
            this, &MainWindow::showProgressBarMenu);
}

void MainWindow::showProgressBarMenu(const QPoint &pos) {
    if (!m_translationService || !m_translationService->isBatchRunning()) {
        return;
    }
    QMenu menu(this);
    QAction *cancelAction = menu.addAction("取消翻译");
    connect(cancelAction, &QAction::triggered, [this]() {
        m_translationService->cancelBatch();
    });
    menu.exec(m_statusProgressBar->mapToGlobal(pos));
}