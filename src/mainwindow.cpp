#include "mainwindow.h"
#include <qdebug.h>
#include "tsfilehandler.h"
#include "tstreewidget.h"
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    setupUi();
}

void MainWindow::setupUi() {
    QSplitter *splitter = new QSplitter(Qt::Horizontal, this);

    m_treeWidget = new TsTreeWidget(this);
    m_treeWidget->setMinimumWidth(300);

    m_detailWidget = new TsDetailWidget(this);

    splitter->addWidget(m_treeWidget);
    splitter->addWidget(m_detailWidget);
    splitter->setSizes({300, 500});

    setCentralWidget(splitter);

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

    setMenuBar(menuBar);
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