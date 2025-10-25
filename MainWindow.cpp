#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QProcess>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QSizePolicy>
#include <QFontMetrics>
#include <QCloseEvent>
#include <QStandardPaths>
#include <QDir>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QCoreApplication>
#include <QApplication>
#include <QClipboard>
#include "settings.h"

void MainWindow::createTrayIcon()
{
    if (m_trayIcon) return;

    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QApplication::windowIcon());
    m_trayIcon->setToolTip(tr("Quish Application"));

    m_restoreAction = new QAction(tr("Restore"), this);
    connect(m_restoreAction, &QAction::triggered, this, &MainWindow::restoreActionTriggered);

    m_quitAction = new QAction(tr("Quit"), this);
    connect(m_quitAction, &QAction::triggered, this, [this]() {
        m_isQuitting = true;
        close();
    });

    m_trayMenu = new QMenu(this);
    m_trayMenu->addAction(m_restoreAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_quitAction);
    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &MainWindow::trayIconActivated);
    m_trayIcon->show();
}

void MainWindow::destroyTrayIcon()
{
    if (!m_trayIcon) return;

    m_trayIcon->hide();
    delete m_trayIcon;
    m_trayIcon = nullptr;

    delete m_trayMenu;
    m_trayMenu = nullptr;

    delete m_restoreAction;
    m_restoreAction = nullptr;

    delete m_quitAction;
    m_quitAction = nullptr;
}

void MainWindow::onSettingChanged(const QString &param, const QVariant &value)
{
    if (param == "minimizeToTray") {
        if (value.toBool()) {
            createTrayIcon();
        } else {
            destroyTrayIcon();
        }
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_trayIcon(nullptr)
    , m_sudoCheckBox(nullptr)
    , m_clearOutputCheckBox(nullptr)
    , m_workingDirectoryLabel(nullptr)
    , m_workingDirectoryLineEdit(nullptr)
{
    ui->setupUi(this);
    QFont monospaceFont("Monospace");
    monospaceFont.setStyleHint(QFont::Monospace);
    ui->txtOutput->setFont(monospaceFont);
    m_statusLabel = new QLabel(this);
    ui->statusbar->addWidget(m_statusLabel);

    lblCommand = findChild<QLabel*>("lblCommand");

    QSettings settings("MyCompany", "Quish");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    ui->tabWidget->setCurrentIndex(settings.value("currentTab", 0).toInt());
    ui->splitter->restoreState(settings.value("splitterSizes").toByteArray());

    m_runAction = new QAction(this);
    m_runAction->setShortcut(QKeySequence(Qt::Key_F3));
    connect(m_runAction, &QAction::triggered, this, &MainWindow::on_btnRun_clicked);
    this->addAction(m_runAction);

    m_breakAction = new QAction(this);
    m_breakAction->setShortcut(QKeySequence(Qt::Key_F4));
    connect(m_breakAction, &QAction::triggered, this, &MainWindow::on_btnBreak_clicked);
    this->addAction(m_breakAction);

    connect(ui->cmbTopics, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::on_cmbTopics_currentIndexChanged);

    m_process = nullptr;
    m_btnBreak = findChild<QPushButton*>("btnBreak");
    if (m_btnBreak) {
        m_btnBreak->setEnabled(false);
    }

    // Settings Tab setup
    QWidget *settingsContainerWidget = new QWidget(ui->tabSettings);
    QFormLayout *settingsFormLayout = new QFormLayout(settingsContainerWidget);
    settingsContainerWidget->setLayout(settingsFormLayout);

    QGridLayout *tabSettingsGridLayout = qobject_cast<QGridLayout*>(ui->tabSettings->layout());
    if (!tabSettingsGridLayout) {
        tabSettingsGridLayout = new QGridLayout(ui->tabSettings);
        ui->tabSettings->setLayout(tabSettingsGridLayout);
    }
    tabSettingsGridLayout->addWidget(settingsContainerWidget, 0, 0);

    m_appSettings.form(settingsContainerWidget);
    connect(&m_appSettings, &Settings::settingChanged, this, &MainWindow::onSettingChanged);

    // System Tray Icon setup
    if (m_appSettings.get("minimizeToTray").toBool()) {
        createTrayIcon();
    }

    // Create .Quish folder in home directory if it doesn't exist
    QString homePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    QDir quishDir(homePath + "/.Quish");
    if (!quishDir.exists()) {
        quishDir.mkpath(".");
    }

    // Load default config file from .Quish folder
    QString defaultConfigPath = homePath + "/.Quish/config.json";
    if (QFile::exists(defaultConfigPath)) {
        loadConfigFile(defaultConfigPath);
    }

    // Set version label
    QProcess gitProcess;
    QDir buildDir(QCoreApplication::applicationDirPath());
    buildDir.cdUp();
    buildDir.cdUp(); // Navigate up one more level to reach the project root
    gitProcess.setWorkingDirectory(buildDir.absolutePath());
    gitProcess.start("git", QStringList() << "rev-list" << "--count" << "HEAD");
    gitProcess.waitForFinished();
    QString commitCount = gitProcess.readAllStandardOutput().trimmed();

    gitProcess.start("git", QStringList() << "rev-parse" << "--short" << "HEAD");
    gitProcess.waitForFinished();
    QString commitHash = gitProcess.readAllStandardOutput().trimmed();

    QString version = QString("0.%1-%2").arg(commitCount).arg(commitHash);
    ui->lblVersion->setText(version);

    checkForNewVersion();
}

void MainWindow::checkForNewVersion()
{
    QProcess gitProcess;
    QDir buildDir(QCoreApplication::applicationDirPath());
    buildDir.cdUp();
    buildDir.cdUp();
    gitProcess.setWorkingDirectory(buildDir.absolutePath());

    gitProcess.start("git", QStringList() << "rev-parse" << "--short" << "HEAD");
    gitProcess.waitForFinished();
    QString localCommitHash = gitProcess.readAllStandardOutput().trimmed();

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, [=](QNetworkReply *reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            if (!doc.isNull() && doc.isObject()) {
                QJsonObject obj = doc.object();
                QString remoteCommitHash = obj["sha"].toString().mid(0, 7); // Get short hash

                if (remoteCommitHash != localCommitHash) {
                    QMessageBox::information(this, tr("New Version Available"),
                                             tr("A new version of Quish is available on GitHub!\nLocal: %1\nRemote: %2")
                                                 .arg(localCommitHash).arg(remoteCommitHash));
                }
            }
        } else {
            qDebug() << "Network error:" << reply->errorString();
        }
        reply->deleteLater();
        manager->deleteLater();
    });

    QNetworkRequest request(QUrl("https://api.github.com/repos/jplozf/Quish/commits/main"));
    manager->get(request);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::loadConfigFile(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open file: %1").arg(filePath));
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(data));
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::critical(this, tr("Error"), tr("Invalid JSON file: %1").arg(filePath));
        return false;
    }

    m_rootConfig = doc.object();
    if (!m_rootConfig.contains("topics") || !m_rootConfig["topics"].isObject()) {
        QMessageBox::critical(this, tr("Error"), tr("Invalid configuration format: 'topics' object not found in %1.").arg(filePath));
        return false;
    }

    ui->cmbTopics->clear();
    ui->cmbCommands->clear();

    QJsonObject topics = m_rootConfig["topics"].toObject();
    ui->cmbTopics->addItems(topics.keys());

    if (ui->cmbTopics->count() > 0) {
        ui->cmbTopics->setCurrentIndex(0);
        on_cmbTopics_currentIndexChanged(0);
    }

    ui->txtEditFile->setPlainText(QString(data));
    m_currentConfigFilePath = filePath;
    ui->lblEditFile->setText(filePath);

    return true;
}

void MainWindow::on_actionOpen_triggered()
{
    qDebug() << "on_actionOpen_triggered called.";
    QString filePath = QFileDialog::getOpenFileName(this, tr("Open Configuration"), "", tr("JSON Files (*.json)"));
    qDebug() << "Selected file path:" << filePath;
    if (!filePath.isEmpty()) {
        loadConfigFile(filePath);
    }
}

void MainWindow::on_btnSaveFile_clicked()
{
    if (m_currentConfigFilePath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No configuration file loaded to save."));
        return;
    }

    QFile file(m_currentConfigFilePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open file for writing: %1").arg(m_currentConfigFilePath));
        return;
    }

    qDebug() << "Saving to file:" << m_currentConfigFilePath;
    QString contentToSave = ui->txtEditFile->toPlainText();
    qDebug() << "Content to save:\n" << contentToSave;
    QTextStream out(&file);
    out << contentToSave;
    file.close();

    m_statusLabel->setText(tr("Configuration saved to %1").arg(m_currentConfigFilePath));
    loadConfigFile(m_currentConfigFilePath);
}

void MainWindow::on_cmbCommands_currentIndexChanged(int index)
{
    if (index < 0) {
        clearForm();
        return;
    }
    QString selectedTopic = ui->cmbTopics->currentText();
    QJsonArray commands = m_rootConfig["topics"].toObject()[selectedTopic].toArray();

    if (index >= commands.size()) {
        return;
    }

    m_currentConfig = commands[index].toObject();
    buildUi(m_currentConfig);
}

void MainWindow::on_cmbTopics_currentIndexChanged(int index)
{
    if (index < 0) {
        return;
    }
    ui->cmbCommands->clear();

    QString selectedTopic = ui->cmbTopics->currentText();
    QJsonObject topics = m_rootConfig["topics"].toObject();
    QJsonArray commands = topics[selectedTopic].toArray();

    for (const QJsonValue &value : commands) {
        ui->cmbCommands->addItem(value.toObject()["name"].toString());
    }

    if (ui->cmbCommands->count() > 0) {
        ui->cmbCommands->setCurrentIndex(0);
        on_cmbCommands_currentIndexChanged(0);
    } else {
        clearForm();
    }
}

void MainWindow::buildUi(const QJsonObject &config)
{
    clearForm();
    m_exclusiveGroupWidgets.clear();
    for (QButtonGroup* group : m_buttonGroups.values()) {
        delete group;
    }
    m_buttonGroups.clear();

    if (!config.contains("arguments") || !config["arguments"].isArray()) {
        return;
    }

    QJsonArray arguments = config["arguments"].toArray();
    QFormLayout *layout = qobject_cast<QFormLayout*>(ui->scrollAreaWidgetContents->layout());
    if (!layout) {
        layout = new QFormLayout(ui->scrollAreaWidgetContents);
        ui->scrollAreaWidgetContents->setLayout(layout);
    }

    // Add "Run as Sudo" checkbox
    m_sudoCheckBox = new QCheckBox(tr("Run as Sudo"), ui->scrollAreaWidgetContents);
    layout->addRow(m_sudoCheckBox);

    // Add "Clear Output Before Run" checkbox
    m_clearOutputCheckBox = new QCheckBox(tr("Clear Output Before Run"), ui->scrollAreaWidgetContents);
    layout->addRow(m_clearOutputCheckBox);

    m_workingDirectoryLabel = new QLabel(tr("Folder"), ui->scrollAreaWidgetContents);
    m_workingDirectoryLineEdit = new QLineEdit(QDir::homePath(), ui->scrollAreaWidgetContents);

    QWidget *widget = new QWidget(this);
    QHBoxLayout *hLayout = new QHBoxLayout(widget);
    QPushButton *button = new QPushButton("...", this);
    QFontMetrics fm(button->font());
    int width = fm.horizontalAdvance("...") + 20;
    button->setFixedWidth(width);
    hLayout->addWidget(m_workingDirectoryLineEdit, 1);
    hLayout->addWidget(button);
    hLayout->setContentsMargins(0, 0, 0, 0);
    widget->setLayout(hLayout);
    layout->addRow(m_workingDirectoryLabel, widget);

    connect(button, &QPushButton::clicked, this, [this]() {
        QString path = QFileDialog::getExistingDirectory(this, "Select Folder");
        if (!path.isEmpty()) {
            m_workingDirectoryLineEdit->setText(path);
        }
    });

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);
    layout->addRow(line);

    for (const QJsonValue &value : arguments) {
        QJsonObject arg = value.toObject();
        QString name = arg["name"].toString();
        QString type = arg["type"].toString();
        QString exclusiveGroup = arg["exclusive_group"].toString();

        QLabel *label = new QLabel(name, this);

        if (type == "boolean" && !exclusiveGroup.isEmpty()) {
            QRadioButton *radioButton = new QRadioButton(name, this);
            layout->addRow(radioButton);
            radioButton->setProperty("argType", type);
            radioButton->setProperty("argFlag", arg["flag"].toString());
            radioButton->setProperty("exclusiveGroup", exclusiveGroup);

            if (!m_buttonGroups.contains(exclusiveGroup)) {
                m_buttonGroups.insert(exclusiveGroup, new QButtonGroup(this));
            }
            m_buttonGroups[exclusiveGroup]->addButton(radioButton);
            m_exclusiveGroupWidgets[exclusiveGroup].append(radioButton);
        } else if (type == "file" || type == "folder") {
            QWidget *widget = new QWidget(this);
            QHBoxLayout *hLayout = new QHBoxLayout(widget);
            QLineEdit *lineEdit = new QLineEdit(this);
            QPushButton *button = new QPushButton("...", this);
            hLayout->addWidget(lineEdit);
            hLayout->addWidget(button);
            hLayout->setContentsMargins(0, 0, 0, 0);
            widget->setLayout(hLayout);
            layout->addRow(label, widget);

            if (type == "file") {
                connect(button, &QPushButton::clicked, this, [this, lineEdit]() {
                    QString path = QFileDialog::getOpenFileName(this, "Select File");
                    if (!path.isEmpty()) {
                        lineEdit->setText(path);
                    }
                });
            } else { // folder
                connect(button, &QPushButton::clicked, this, [this, lineEdit]() {
                    QString path = QFileDialog::getExistingDirectory(this, "Select Folder");
                    if (!path.isEmpty()) {
                        lineEdit->setText(path);
                    }
                });
            }
            widget->setProperty("argType", type);
            widget->setProperty("argFlag", arg["flag"].toString());
            widget->setProperty("argName", name);
        } else if (type == "string" || type == "raw_string") {
            QLineEdit *lineEdit = new QLineEdit(this);
            layout->addRow(label, lineEdit);
            lineEdit->setProperty("argType", type);
            lineEdit->setProperty("argFlag", arg["flag"].toString());
            lineEdit->setProperty("argName", name);
        } else if (type == "integer") {
            QSpinBox *spinBox = new QSpinBox(this);
            spinBox->setRange(-999999, 999999);
            layout->addRow(label, spinBox);
            spinBox->setProperty("argType", type);
            spinBox->setProperty("argFlag", arg["flag"].toString());
            spinBox->setProperty("argName", name);
        } else if (type == "boolean") { // Handle boolean without exclusive group
            QCheckBox *checkBox = new QCheckBox(name, this);
            layout->addRow(checkBox);
            checkBox->setProperty("argType", type);
            checkBox->setProperty("argFlag", arg["flag"].toString());
            checkBox->setProperty("argName", name);
        }
    }
}

void MainWindow::on_btnRun_clicked()
{
    if (m_currentConfig.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No configuration loaded"));
        return;
    }

    QString executable = m_currentConfig["executable"].toString();
    if (executable.isEmpty()) {
        QMessageBox::critical(this, tr("Error"), tr("No executable specified in configuration"));
        return;
    }

            QString commandLine = executable;
        
                if (m_sudoCheckBox && m_sudoCheckBox->isChecked()) {
        
                    commandLine = "sudo " + commandLine;
        
                }
        
            
        
                if (m_clearOutputCheckBox && m_clearOutputCheckBox->isChecked()) {
        
                    ui->txtOutput->clear();
        
                }
        
            
        
                QFormLayout *formLayout = qobject_cast<QFormLayout*>(ui->scrollAreaWidgetContents->layout());        QJsonArray configArgs = m_currentConfig["arguments"].toArray();

        for (const QJsonValue &value : configArgs) {
            QJsonObject arg = value.toObject();
            QString name = arg["name"].toString();
            QString type = arg["type"].toString();
            QString flag = arg["flag"].toString();
            QString exclusiveGroup = arg["exclusive_group"].toString();

            if (type == "boolean") {
                if (!exclusiveGroup.isEmpty()) {
                    if (m_buttonGroups.contains(exclusiveGroup)) {
                        QAbstractButton *checkedButton = m_buttonGroups[exclusiveGroup]->checkedButton();
                        if (checkedButton && checkedButton->property("argFlag").toString() == flag) {
                            commandLine += " " + flag;
                        }
                    }
                } else { // Non-exclusive boolean (checkbox)
                    // Find the QCheckBox by its flag or name
                    QList<QCheckBox*> checkBoxes = ui->scrollAreaWidgetContents->findChildren<QCheckBox*>();
                    for (QCheckBox *checkBox : checkBoxes) {
                        if (checkBox->property("argFlag").toString() == flag && checkBox->isChecked()) {
                            commandLine += " " + flag;
                            break;
                        }
                    }
                }
            } else { // string, integer, file, folder
                // Find the corresponding widget in the form layout
                QWidget *fieldWidget = nullptr;
                for (int i = 0; i < formLayout->rowCount(); ++i) {
                    QLayoutItem *fieldItem = formLayout->itemAt(i, QFormLayout::FieldRole);
                    if (fieldItem && fieldItem->widget()) {
                        if (!flag.isEmpty() && fieldItem->widget()->property("argFlag").toString() == flag) {
                            fieldWidget = fieldItem->widget();
                            break;
                        } else if (flag.isEmpty() && fieldItem->widget()->property("argName").toString() == name) {
                            fieldWidget = fieldItem->widget();
                            break;
                        } else if (QWidget *container = qobject_cast<QWidget*>(fieldItem->widget())) {
                            QLineEdit* lineEdit = container->findChild<QLineEdit*>();
                            if (lineEdit) {
                                if (!flag.isEmpty() && container->property("argFlag").toString() == flag) {
                                    fieldWidget = container;
                                    break;
                                } else if (flag.isEmpty() && container->property("argName").toString() == name) {
                                    fieldWidget = container;
                                    break;
                                }
                            }
                        }
                    }
                }

                if (fieldWidget) {
                    QString paramValue;
                    if (QLineEdit *lineEdit = qobject_cast<QLineEdit*>(fieldWidget)) {
                        paramValue = lineEdit->text();
                    } else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(fieldWidget)) {
                        paramValue = QString::number(spinBox->value());
                    } else if (QWidget *container = qobject_cast<QWidget*>(fieldWidget)) {
                        QLineEdit* lineEdit = container->findChild<QLineEdit*>();
                        if(lineEdit) {
                            paramValue = lineEdit->text();
                        }
                    }

                    if (!flag.isEmpty()) {
                        commandLine += " " + flag;
                    }
                    if (!paramValue.isEmpty()) {
                        if (type == "raw_string") {
                            commandLine += " " + paramValue;
                        } else {
                            if (paramValue.contains(' ')) {
                                commandLine += " \"" + paramValue + "\"";
                            } else {
                                commandLine += " " + paramValue;
                            }
                        }
                    }
                }
            }
        }
    
        ui->tabWidget->setCurrentIndex(0);
        m_timer.restart();
        if (m_btnBreak) {
            m_btnBreak->setEnabled(true);
        }
        m_process = new QProcess(this);
        m_process->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {
            ui->txtOutput->insertPlainText(m_process->readAllStandardOutput());
            ui->txtOutput->verticalScrollBar()->setValue(ui->txtOutput->verticalScrollBar()->maximum());
        });
        connect(m_process,
                QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this,
                [this](int exitCode, QProcess::ExitStatus exitStatus) {
                    Q_UNUSED(exitStatus);
                    ui->txtOutput->append(m_process->readAllStandardOutput());
                    QString color = (exitCode == 0) ? "green" : "red";
                    ui->txtOutput->append(QString("<span style=\"color:%1;\">\nProcess finished with exit code %2</span>").arg(color).arg(exitCode));
                    m_statusLabel->setText(QString("Finished with exit code %1 in %2 ms")
                                               .arg(exitCode)
                                               .arg(m_timer.elapsed()));
                    if (m_btnBreak) {
                        m_btnBreak->setEnabled(false);
                    }
                    m_process->deleteLater();
                    m_process = nullptr;
                });
    
        QStringList commandParts = commandLine.split(" ");
        QString program = commandParts.takeFirst();
        m_process->setWorkingDirectory(m_workingDirectoryLineEdit->text());
        m_process->start(program, commandParts);

    if (lblCommand) {
        lblCommand->setText(commandLine);
    }}

void MainWindow::on_btnBreak_clicked()
{
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->terminate();
        m_statusLabel->setText(tr("Process terminated."));
    }
}

void MainWindow::on_btnClear_clicked()
{
    ui->txtOutput->clear();
    ui->lblCommand->setText("Output");
}

void MainWindow::on_btnCopy_clicked()
{
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(ui->txtOutput->toPlainText());
}

void MainWindow::restoreActionTriggered()
{
    qDebug() << "on_restoreAction_triggered called.";
    qDebug() << "Before - isVisible:" << isVisible() << "isMinimized:" << isMinimized() << "windowState:" << windowState();
    if (isMinimized()) {
        showNormal(); // Restore from minimized state
    } else if (!isVisible()) {
        show(); // Show if hidden
    }
    raise();
    activateWindow();
    qDebug() << "After - isVisible:" << isVisible() << "isMinimized:" << isMinimized() << "windowState:" << windowState();
}

void MainWindow::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    Q_UNUSED(reason); // The 'reason' parameter is no longer used, so mark it as unused to avoid compiler warnings.
    qDebug() << "on_trayIcon_activated called with reason:" << reason;
    restoreActionTriggered();
}

void MainWindow::clearForm()
{
    QFormLayout *layout = qobject_cast<QFormLayout*>(ui->scrollAreaWidgetContents->layout());
    if (layout) {
        while (layout->count() > 0) {
            QLayoutItem* item = layout->takeAt(0);
            if (item->widget()) {
                if (item->widget() == m_sudoCheckBox) {
                    m_sudoCheckBox = nullptr; // Set to nullptr if it's the sudo checkbox
                } else if (item->widget() == m_clearOutputCheckBox) {
                    m_clearOutputCheckBox = nullptr; // Set to nullptr if it's the clear output checkbox
                } else if (item->widget() == m_workingDirectoryLabel) {
                    m_workingDirectoryLabel = nullptr;
                } else if (item->widget() == m_workingDirectoryLineEdit) {
                    m_workingDirectoryLineEdit = nullptr;
                }
                delete item->widget();
            }
            delete item;
        }
    }
    for (QButtonGroup* group : m_buttonGroups.values()) {
        delete group;
    }
    m_buttonGroups.clear();
    m_exclusiveGroupWidgets.clear();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("MyCompany", "Quish");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("currentTab", ui->tabWidget->currentIndex());
    settings.setValue("splitterSizes", ui->splitter->saveState());

    if (m_trayIcon && m_appSettings.get("minimizeToTray").toBool() && !m_isQuitting) {
        hide();
        event->ignore();
        return;
    }

    if (m_appSettings.get("confirmExit").toBool()) {
        if (QMessageBox::question(this, tr("Confirm Exit"), tr("Are you sure you want to exit?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
            event->accept();
        } else {
            m_isQuitting = false;
            event->ignore();
        }
    } else {
        event->accept();
    }
}
