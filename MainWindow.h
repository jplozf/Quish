#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QJsonObject>
#include <QProcess>
#include <QElapsedTimer>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QButtonGroup>
#include <QList>
#include <QWidget>
#include <QAction>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QCheckBox>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QCloseEvent>
#include "settings.h"
#include <QScrollBar>
#include <QNetworkAccessManager>
#include <QRadioButton>
#include <QSettings>
#include <QNetworkReply>

QT_BEGIN_NAMESPACE
#include "CodeEditor.h"

namespace Ui {
class MainWindow;
}

class JsonHighlighter;
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
private slots:
    void on_actionOpen_triggered();
    void on_cmbTopics_currentIndexChanged(int index);
    void on_cmbCommands_currentIndexChanged(int index);
    void on_btnRun_clicked();
    void on_btnBreak_clicked();
    void on_btnClear_clicked();
    void on_btnCopy_clicked();
    void on_btnSaveFile_clicked();
    void handleThemeChange(int index);
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void restoreActionTriggered();
    void onSettingChanged(const QString &param, const QVariant &value);
    void checkForNewVersion();
private:
    void createTrayIcon();
    void destroyTrayIcon();
    bool loadConfigFile(const QString &filePath);
    void buildUi(const QJsonObject &config);
    void clearForm();
    void applyTheme(const QString &themeName);
    void closeEvent(QCloseEvent *event) override;

    Ui::MainWindow *ui;
    QJsonObject m_rootConfig;
    QJsonObject m_currentConfig;
    QProcess *m_process;
    QElapsedTimer m_timer;
    QLabel *m_statusLabel;
    QPushButton *m_btnBreak;
    QMap<QString, QButtonGroup*> m_buttonGroups;
    QMap<QString, QList<QWidget*>> m_exclusiveGroupWidgets;
    QString m_currentConfigFilePath;
    QAction *m_runAction;
    QAction *m_breakAction;
    QAction *m_saveAction;
    JsonHighlighter *m_highlighter;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_restoreAction;
    QAction *m_quitAction;
    bool m_isQuitting = false;
    Settings m_appSettings;
    QCheckBox *m_sudoCheckBox;
    QCheckBox *m_clearOutputCheckBox;
    QLabel *m_workingDirectoryLabel;
    QLineEdit *m_workingDirectoryLineEdit;
    QComboBox *m_themeComboBox;
    QTextEdit *m_txtHelp;
    QLabel *lblCommand;
};
#endif // MAINWINDOW_H
