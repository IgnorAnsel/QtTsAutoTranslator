#include <QMainWindow>
#include <QMenuBar>
#include <QSplitter>
#include "tstreewidget.h"
#include "tsdetailwidget.h"
#include "tsfilehandler.h"
#include <QFileDialog>

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);

private:
    void setupUi();
    void loadTsFile(const QString &filePath);

    QString stateToString(TranslationState state) const;

    TranslationState stringToState(const QString &stateStr) const;

    TsTreeWidget *m_treeWidget;
    TsDetailWidget *m_detailWidget;
    TsFileHandler m_fileHandler;
    QString m_currentFilePath;
};




