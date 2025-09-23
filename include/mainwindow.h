#include <QMainWindow>
#include <QMenuBar>
#include <QSplitter>
#include "tstreewidget.h"
#include "tsdetailwidget.h"
#include "tsfilehandler.h"
#include "logoutputwidget.h"
#include <QFileDialog>
#include <QProgressDialog>
#include <QToolBar>
#include <QProgressBar>
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
private slots:
    void openTranslationSettings();
    void onBatchTranslationRequested();

    void onSingleTranslationCompleted(const QString &context, const QString &source, const QString &translation);

    void onBatchTranslationCompleted(const QMap<QString, QString> &results);
    void onTranslationError(const QString &errorMessage, const QString &sourceText);
    void onEngineChanged(int index);
    void logMessage(const QString &message);
    void logError(const QString &error);
private slots:
    void onBatchProgress(int current, int total);

    void setProgressBarColor(const QString &color);

    void onBatchTranslationCanceled();

    void createProgressBarMenu();

    void showProgressBarMenu(const QPoint &pos);

private:
    void setupUi();
    void loadTsFile(const QString &filePath);

    QString stateToString(TranslationState state) const;

    TranslationState stringToState(const QString &stateStr) const;

    TsTreeWidget *m_treeWidget;
    TsDetailWidget *m_detailWidget;
    LogOutputWidget *m_logWidget;
    TsFileHandler m_fileHandler;
    QString m_currentFilePath;
    TranslationService *m_translationService;
    QToolBar *m_translationToolBar;
    QComboBox *m_engineCombo;

    QProgressBar *m_statusProgressBar;
    QLabel *m_statusLabel;
};




