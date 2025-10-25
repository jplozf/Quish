#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QScrollBar>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QCheckBox>
#include <QJsonArray>
#include <QJsonObject>
#include <QMainWindow>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QTextEdit>
#include <QElapsedTimer>
#include <QTime>
#include <QRadioButton>
#include <QButtonGroup>
#include "settings.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_actionOpen_triggered();
    void on_btnRun_clicked();
    void on_cmbCommands_currentIndexChanged(int index);
    void on_cmbTopics_currentIndexChanged(int index);
    void on_btnSaveFile_clicked();
    void on_btnBreak_clicked();
    void on_btnClear_clicked();
    void on_btnCopy_clicked();
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void restoreActionTriggered();
    void onSettingChanged(const QString &param, const QVariant &value);

private:
    void buildUi(const QJsonObject &config);
    void clearForm();
    bool loadConfigFile(const QString &filePath);

    void createTrayIcon();
    void destroyTrayIcon();
    void checkForNewVersion();

    Ui::MainWindow *ui;
    QJsonObject m_currentConfig;
    QJsonObject m_rootConfig;
    QLabel *m_statusLabel;
    QElapsedTimer m_timer;
    QLabel *lblCommand;
    QTextEdit *txtEditFile;
    QString m_currentConfigFilePath;
    QLabel *lblEditFile;
    QAction *m_runAction;
    QAction *m_breakAction;
    QProcess *m_process;
    QPushButton *m_btnBreak;
    QSystemTrayIcon *m_trayIcon;
    QMenu *m_trayMenu;
    QAction *m_restoreAction;
    QAction *m_quitAction;
    Settings m_appSettings;
    bool m_isQuitting = false;
    QCheckBox *m_sudoCheckBox;
    QCheckBox *m_clearOutputCheckBox;
    QLabel *m_workingDirectoryLabel;
    QLineEdit *m_workingDirectoryLineEdit;
    QMap<QString, QList<QWidget*>> m_exclusiveGroupWidgets;
    QMap<QString, QButtonGroup*> m_buttonGroups;
};
#endif // MAINWINDOW_H
