#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "SaveCommandDialog.h"

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
#include <QIntValidator>
#include <QTimer>
#include "settings.h"
#include "JsonHighlighter.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_trayIcon(nullptr)
    , m_sudoCheckBox(nullptr)
    , m_clearOutputCheckBox(nullptr)
    , m_workingDirectoryLabel(nullptr)
    , m_workingDirectoryLineEdit(nullptr)
    , m_themeComboBox(nullptr)
    , m_lblFileSize(nullptr)
    , m_lblExitCode(nullptr)
    , m_lblElapsedTime(nullptr)
{
    ui->setupUi(this);

    // Programmatically add Help tab
    QWidget *helpTab = new QWidget();
    ui->tabWidget->insertTab(1, helpTab, QIcon(":/icons/Question.png"), "Help");
    QGridLayout *helpLayout = new QGridLayout(helpTab);
    m_txtHelp = new QTextEdit(helpTab);
    m_txtHelp->setReadOnly(true);
    helpLayout->addWidget(m_txtHelp);

    QFont monospaceFont("Monospace");
    monospaceFont.setStyleHint(QFont::Monospace);
    ui->txtOutput->setFont(monospaceFont);
    m_txtHelp->setFont(monospaceFont);
    m_highlighter = new JsonHighlighter(ui->txtEditFile->document());
    m_statusLabel = new QLabel(this);
    ui->statusbar->addWidget(m_statusLabel);

    m_lblExitCode = new QLabel(tr("Exit Code: N/A"), this);
    m_lblExitCode->setStyleSheet("border: 1px solid gray; padding: 1px;");
    m_lblElapsedTime = new QLabel(tr("Elapsed: N/A"), this);
    m_lblElapsedTime->setStyleSheet("border: 1px solid gray; padding: 1px;");

    if (ui->statusbar) {
        ui->statusbar->addPermanentWidget(m_lblExitCode);
        ui->statusbar->addPermanentWidget(m_lblElapsedTime);
    }

    lblCommand = findChild<QLineEdit*>(tr("lblCommand"));
    m_lblCursorPosition = findChild<QLabel*>("lblCursorPosition");
    m_lblFileSize = findChild<QLabel*>("lblFileSize");

    m_statusBarTimer = new QTimer(this);
    m_statusBarTimer->setSingleShot(true);
    connect(m_statusBarTimer, &QTimer::timeout, this, &MainWindow::clearStatusBarMessage);
    m_statusBarTimer->setInterval(m_appSettings.get("statusBarTimeout").toInt());

    connect(&m_appSettings, &Settings::settingChanged, this, [this](const QString &param, const QVariant &value) {
        if (param == "statusBarTimeout") {
            m_statusBarTimer->setInterval(value.toInt());
        }
    });

    if (m_lblCursorPosition) {
        connect(ui->txtEditFile, &CodeEditor::cursorPositionChanged, this, &MainWindow::updateCursorPositionLabel);
    }

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

    m_saveAction = new QAction(this);
    m_saveAction->setShortcut(QKeySequence("Ctrl+S"));
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::on_btnSaveFile_clicked);
    this->addAction(m_saveAction);

    m_helpAction = new QAction(this);
    m_helpAction->setShortcut(QKeySequence(Qt::Key_F1));
    connect(m_helpAction, &QAction::triggered, this, [this]() {
        ui->tabWidget->setCurrentIndex(1); // Help tab is at index 1
    });
    this->addAction(m_helpAction);

    m_quitAction_2 = new QAction(tr("&Quit"), this);
    m_quitAction_2->setShortcut(QKeySequence("Ctrl+Q"));
    connect(m_quitAction_2, &QAction::triggered, this, &MainWindow::close);
    ui->menuFile->addAction(m_quitAction_2);

    connect(ui->cmbTopics,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &MainWindow::on_cmbTopics_currentIndexChanged);

    connect(ui->tabWidget, &QTabWidget::currentChanged, this, &MainWindow::on_tabWidget_currentChanged);

    m_process = nullptr;
    m_btnBreak = findChild<QPushButton*>(tr("btnBreak"));
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

    // Theme selection
    m_themeComboBox = new QComboBox(settingsContainerWidget);
    settingsFormLayout->addRow(tr("Theme"), m_themeComboBox);
    QStringList themeFiles;
    themeFiles << "dark" << "light" << "AMOLED" << "Aqua" << "ConsoleStyle" << "ElegantDark" << "MacOS" << "ManjaroMix" << "MaterialDark" << "NeonButtons" << "Ubuntu";
    for (const QString &theme : themeFiles) {
        m_themeComboBox->addItem(theme);
    }
    connect(m_themeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::handleThemeChange);

    // System Tray Icon setup
    if (m_appSettings.get("minimizeToTray").toBool()) {
        createTrayIcon();
    }

    // Load saved theme
    QString savedTheme = settings.value("theme", "light").toString();
    int index = m_themeComboBox->findText(savedTheme);
    if (index != -1) {
        m_themeComboBox->setCurrentIndex(index);
        applyTheme(savedTheme);
    } else {
        applyTheme("light"); // Fallback to light theme if saved theme is not found
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
    QString version = QString(APP_VERSION_STRING);
    ui->lblVersion->setText(version);

    checkForNewVersion();

    setStatusBarMessage(tr("Application started."));
}

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
    setStatusBarMessage(tr("Setting '%1' changed to '%2'.").arg(param).arg(value.toString()));
    if (param == "minimizeToTray") {
        if (value.toBool()) {
            createTrayIcon();
        } else {
            destroyTrayIcon();
        }
    }
}

void MainWindow::checkForNewVersion()
{
    QString localCommitHash = QString(APP_VERSION_STRING).section('-', 1, 1); // Extract hash from "0.X-hash"
    setStatusBarMessage(tr("Checking for new version. Current version: %1").arg(localCommitHash));

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

void MainWindow::updateCursorPositionLabel()
{
    if (m_lblCursorPosition) {
        QTextCursor cursor = ui->txtEditFile->textCursor();
        int line = cursor.blockNumber() + 1;
        int column = cursor.columnNumber() + 1;
        m_lblCursorPosition->setText(QString("Ln %1, Col %2").arg(line).arg(column));
    }
}

void MainWindow::handleThemeChange(int index)
{
    QString selectedTheme = m_themeComboBox->itemText(index);
    if (selectedTheme == "Default") {
        qApp->setStyleSheet("");
    } else {
        applyTheme(selectedTheme);
    }
    QSettings settings("MyCompany", "Quish");
    settings.setValue("theme", selectedTheme);
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
    updateFileSizeLabel();

    setStatusBarMessage(tr("Configuration loaded from %1").arg(filePath));
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

#include <QDateTime>
#include <QTextStream>

void MainWindow::setStatusBarMessage(const QString &message)
{
    if (m_statusLabel) {
        m_statusLabel->setText(message);
    }

    if (m_statusBarTimer) {
        m_statusBarTimer->start();
    }

    QString logFilePath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.Quish/quish.log";
    QFile logFile(logFilePath);
    if (logFile.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&logFile);
        out << QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss") << " - " << message << "\n";
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

    QString contentToSave = ui->txtEditFile->toPlainText();
    QTextStream out(&file);
    out << contentToSave;
    file.close();

    setStatusBarMessage(tr("Configuration saved to %1").arg(m_currentConfigFilePath));
    loadConfigFile(m_currentConfigFilePath);
    updateFileSizeLabel();
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
    updateCommandLineLabel();
    if (ui->tabWidget->currentIndex() == 2) { // Check if Edit tab is active
        scrollToCurrentCommandInEditor();
    }

    m_txtHelp->clear();
    if (m_currentConfig.contains("man")) {
        QString manCommand = "man " + m_currentConfig["man"].toString();
        QProcess *process = new QProcess(this);
        process->setProcessChannelMode(QProcess::MergedChannels);
        connect(process, &QProcess::readyReadStandardOutput, this, [this, process]() {
            QString output = process->readAllStandardOutput();
            QStringList lines = output.split('\n');
            QString filteredOutput;
            for (const QString &line : lines) {
                if (!line.startsWith("troff:")) {
                    filteredOutput += line + '\n';
                }
            }
            m_txtHelp->insertPlainText(filteredOutput);
        });
        connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, [this, process](int exitCode, QProcess::ExitStatus exitStatus) {
            Q_UNUSED(exitStatus);
            if (exitCode != 0) {
                m_txtHelp->setPlainText(QString("Error executing man command. Exit code: %1").arg(exitCode));
            }
            m_txtHelp->moveCursor(QTextCursor::Start);
            process->deleteLater();
        });
        process->start(manCommand);
    } else {
        m_txtHelp->clear();
    }
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
    updateCommandLineLabel();
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
    m_sudoCheckBox->setObjectName("m_sudoCheckBox");
    m_sudoCheckBox->setChecked(config.value("sudo").toBool(false));
    layout->addRow(m_sudoCheckBox);
    connect(m_sudoCheckBox, &QCheckBox::toggled, this, &MainWindow::updateCommandLineLabel);

    // Add "Clear Output Before Run" checkbox
    m_clearOutputCheckBox = new QCheckBox(tr("Clear Output Before Run"), ui->scrollAreaWidgetContents);
    m_clearOutputCheckBox->setObjectName("m_clearOutputCheckBox");
    m_clearOutputCheckBox->setChecked(config.value("clear_output").toBool(false));
    layout->addRow(m_clearOutputCheckBox);
    connect(m_clearOutputCheckBox, &QCheckBox::toggled, this, &MainWindow::updateCommandLineLabel);

    m_workingDirectoryLabel = new QLabel(tr("Folder"), ui->scrollAreaWidgetContents);
    m_workingDirectoryLineEdit = new QLineEdit(ui->scrollAreaWidgetContents);
    m_workingDirectoryLineEdit->setObjectName("m_workingDirectoryLineEdit");
    if (config.contains("working_directory")) {
        m_workingDirectoryLineEdit->setText(config.value("working_directory").toString());
    } else {
        m_workingDirectoryLineEdit->setText(QDir::homePath());
    }
    connect(m_workingDirectoryLineEdit, &QLineEdit::textChanged, this, &MainWindow::updateCommandLineLabel);

    QWidget *widget = new QWidget(this);
    QHBoxLayout *hLayout = new QHBoxLayout(widget);
    QPushButton *button = new QPushButton("...", ui->scrollAreaWidgetContents);
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
        bool mandatory = arg["mandatory"].toBool();

        QString styleSheet = mandatory ? "color: red;" : "";

        if (type == "boolean" && !exclusiveGroup.isEmpty()) {
            QRadioButton *radioButton = new QRadioButton(name, ui->scrollAreaWidgetContents);
            radioButton->setStyleSheet(styleSheet);
            layout->addRow(radioButton);
            radioButton->setProperty("argType", type);
            radioButton->setProperty("argFlag", arg["flag"].toString());
            radioButton->setProperty("exclusiveGroup", exclusiveGroup);
            radioButton->setProperty("argName", name);
            radioButton->setProperty("mandatory", mandatory);

            if (!m_buttonGroups.contains(exclusiveGroup)) {
                m_buttonGroups.insert(exclusiveGroup, new QButtonGroup(this));
            }
            m_buttonGroups[exclusiveGroup]->addButton(radioButton);
            m_exclusiveGroupWidgets[exclusiveGroup].append(radioButton);
            connect(radioButton, &QRadioButton::toggled, this, &MainWindow::updateCommandLineLabel);
            if (arg.contains("default")) {
                radioButton->setChecked(arg["default"].toBool());
            }
        } else if (type == "file" || type == "folder" || type == "newfile" || type == "newfolder") {
            QLabel *label = new QLabel(name, ui->scrollAreaWidgetContents);
            label->setStyleSheet(styleSheet);
            QWidget *widget = new QWidget(ui->scrollAreaWidgetContents);
            QHBoxLayout *hLayout = new QHBoxLayout(widget);
            QLineEdit *lineEdit = new QLineEdit(this);
            if (arg.contains("default")) {
                lineEdit->setText(arg["default"].toString());
            }
            lineEdit->setStyleSheet(styleSheet);
            connect(lineEdit, &QLineEdit::textChanged, this, &MainWindow::updateCommandLineLabel);
            QPushButton *button = new QPushButton("...", ui->scrollAreaWidgetContents);
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
            } else if (type == "newfile") {
                connect(button, &QPushButton::clicked, this, [this, lineEdit]() {
                    QString path = QFileDialog::getSaveFileName(this, "Select File");
                    if (!path.isEmpty()) {
                        lineEdit->setText(path);
                    }
                });
            } else if (type == "newfolder") {
                connect(button, &QPushButton::clicked, this, [this, lineEdit]() {
                    QString path = QFileDialog::getExistingDirectory(this, "Select Folder");
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
            widget->setProperty("mandatory", mandatory);
        } else if (type == "files") {
            QLabel *label = new QLabel(name, ui->scrollAreaWidgetContents);
            label->setStyleSheet(styleSheet);
            QListWidget *listWidget = new QListWidget(ui->scrollAreaWidgetContents);
            listWidget->setStyleSheet(styleSheet);
            QPushButton *button = new QPushButton("...", ui->scrollAreaWidgetContents);
            connect(button, &QPushButton::clicked, this, [this, listWidget]() {
                QStringList files = QFileDialog::getOpenFileNames(this, "Select Files");
                if (!files.isEmpty()) {
                    listWidget->addItems(files);
                    updateCommandLineLabel(); // Update after adding files
                }
            });
            connect(listWidget, &QListWidget::itemChanged, this, &MainWindow::updateCommandLineLabel);
            layout->addRow(label, listWidget);
            layout->addRow(button);
            listWidget->setProperty("argType", type);
            listWidget->setProperty("argFlag", arg["flag"].toString());
            listWidget->setProperty("argName", name);
            listWidget->setProperty("mandatory", mandatory);
            if (arg.contains("default")) {
                QJsonArray defaultFiles = arg["default"].toArray();
                for (const QJsonValue &fileVal : defaultFiles) {
                    listWidget->addItem(fileVal.toString());
                }
            }
        } else if (type == "string" || type == "raw_string") {
            QLabel *label = new QLabel(name, ui->scrollAreaWidgetContents);
            label->setStyleSheet(styleSheet);
            QLineEdit *lineEdit = new QLineEdit(ui->scrollAreaWidgetContents);
            if (arg.contains("default")) {
                lineEdit->setText(arg["default"].toString());
            }
            lineEdit->setStyleSheet(styleSheet);
            layout->addRow(label, lineEdit);
            connect(lineEdit, &QLineEdit::textChanged, this, &MainWindow::updateCommandLineLabel);
            lineEdit->setProperty("argType", type);
            lineEdit->setProperty("argFlag", arg["flag"].toString());
            lineEdit->setProperty("argName", name);
            lineEdit->setProperty("mandatory", mandatory);
        } else if (type == "integer") {
            QLabel *label = new QLabel(name, ui->scrollAreaWidgetContents);
            label->setStyleSheet(styleSheet);
            QLineEdit *lineEdit = new QLineEdit(ui->scrollAreaWidgetContents);
            if (arg.contains("default")) {
                lineEdit->setText(QString::number(arg["default"].toInt()));
            }
            lineEdit->setStyleSheet(styleSheet);
            lineEdit->setValidator(new QIntValidator(this));
            layout->addRow(label, lineEdit);
            connect(lineEdit, &QLineEdit::textChanged, this, &MainWindow::updateCommandLineLabel);
            lineEdit->setProperty("argType", type);
            lineEdit->setProperty("argFlag", arg["flag"].toString());
            lineEdit->setProperty("argName", name);
            lineEdit->setProperty("mandatory", mandatory);
        } else if (type == "boolean") { // Handle boolean without exclusive group
            QCheckBox *checkBox = new QCheckBox(name, ui->scrollAreaWidgetContents);
            if (arg.contains("default")) {
                checkBox->setChecked(arg["default"].toBool());
            }
            checkBox->setStyleSheet(styleSheet);
            layout->addRow(checkBox);
            connect(checkBox, &QCheckBox::toggled, this, &MainWindow::updateCommandLineLabel);
            checkBox->setProperty("argType", type);
            checkBox->setProperty("argFlag", arg["flag"].toString());
            checkBox->setProperty("argName", name);
            checkBox->setProperty("mandatory", mandatory);
        }
    }
}

void MainWindow::applyTheme(const QString &themeName)
{
    QFile file(QString(":/themes/%1.qss").arg(themeName));
    if (file.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(styleSheet);
        file.close();
    } else {
        qWarning() << "Could not open theme file:" << themeName;
    }
}

void MainWindow::on_btnRun_clicked()

{

    QString commandLineForDisplay = buildCommandLine();
    if (commandLineForDisplay.isEmpty()) {
        return;
    }

    setStatusBarMessage(tr("Executing command: %1").arg(commandLineForDisplay));

    if (lblCommand) {
        lblCommand->setText(commandLineForDisplay);
    }

    QString commandLineForExecution = "stdbuf -o L " + commandLineForDisplay;



    if (m_clearOutputCheckBox && m_clearOutputCheckBox->isChecked()) {

        ui->txtOutput->clear();

    }



    ui->tabWidget->setCurrentIndex(0);

    m_timer.restart();

    if (m_btnBreak) {

        m_btnBreak->setEnabled(true);

    }

    m_process = new QProcess(this);

    m_process->setProcessChannelMode(QProcess::MergedChannels);

        connect(m_process, &QProcess::readyReadStandardOutput, this, [this]() {

            ui->txtOutput->insertPlainText(m_process->readAll());

            ui->txtOutput->verticalScrollBar()->setValue(ui->txtOutput->verticalScrollBar()->maximum());

        });

    connect(m_process,

            QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),

            this,

            [this](int exitCode, QProcess::ExitStatus exitStatus) {

                Q_UNUSED(exitStatus);

                ui->txtOutput->append(m_process->readAllStandardOutput());

                QString color = (exitCode == 0) ? "green" : "red";

                ui->txtOutput->append(

                    QString("<span style=\"color:%1;\">\nProcess finished " "with exit code " "%2</span><br><br>")

                        .arg(color)

                        .arg(exitCode));

                setStatusBarMessage(

                    QString("Finished with exit code %1 in %2 ms").arg(exitCode).arg(m_timer.elapsed()));

                if (m_lblExitCode) {
                    m_lblExitCode->setText(QString("Exit Code: %1").arg(exitCode));
                }
                if (m_lblElapsedTime) {
                    m_lblElapsedTime->setText(QString("Elapsed: %1 ms").arg(m_timer.elapsed()));
                }

                if (m_btnBreak) {

                    m_btnBreak->setEnabled(false);

                }

                m_process->deleteLater();

                m_process = nullptr;

            });



    QStringList commandParts = commandLineForExecution.split(" ");

    QString program = commandParts.takeFirst();

    m_process->setWorkingDirectory(m_workingDirectoryLineEdit->text());

    m_process->start(program, commandParts);
}



void MainWindow::updateCommandLineLabel()

{

    QString commandLine = buildCommandLine();

    if (lblCommand) {

        lblCommand->setText(commandLine);

    }

}

void MainWindow::scrollToCurrentCommandInEditor()
{
    if (!m_currentConfig.isEmpty()) {
        QString commandName = m_currentConfig["name"].toString();
        if (commandName.isEmpty()) {
            return;
        }

        QString searchPattern = QString("\"name\": \"%1\"").arg(commandName);
        QTextDocument *document = ui->txtEditFile->document();
        QTextCursor highlightCursor(document);
        QTextCursor cursor(document);

        cursor.beginEditBlock();

        QTextCharFormat plainFormat(highlightCursor.charFormat());
        QTextCharFormat colorFormat = plainFormat;
        colorFormat.setBackground(Qt::yellow);

        while (!highlightCursor.isNull() && !highlightCursor.atEnd()) {
            highlightCursor = document->find(searchPattern, highlightCursor, QTextDocument::FindWholeWords);

            if (!highlightCursor.isNull()) {
                // Found the name, now find the start of the JSON object
                QTextCursor jsonStartCursor = highlightCursor;
                int braceCount = 0;
                while (jsonStartCursor.position() > 0) {
                    jsonStartCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 1);
                    if (jsonStartCursor.selectedText() == "{") {
                        braceCount++;
                    } else if (jsonStartCursor.selectedText() == "}") {
                        braceCount--;
                    }
                    if (braceCount == 1) { // Found the opening brace of the object
                        break;
                    }
                    jsonStartCursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::MoveAnchor, 1);
                }

                if (jsonStartCursor.position() > 0) {
                    cursor.setPosition(jsonStartCursor.position());
                    ui->txtEditFile->setTextCursor(cursor);
                    ui->txtEditFile->ensureCursorVisible();
                    break; // Found and positioned, exit loop
                }
            }
        }
        cursor.endEditBlock();
    }
}

void MainWindow::updateFileSizeLabel()
{
    if (m_lblFileSize && !m_currentConfigFilePath.isEmpty()) {
        QFile file(m_currentConfigFilePath);
        if (file.exists()) {
            qint64 size = file.size();
            QString sizeString;
            if (size < 1024) {
                sizeString = QString("%1 B").arg(size);
            } else if (size < 1024 * 1024) {
                sizeString = QString("%1 KB").arg(size / 1024.0, 0, 'f', 1);
            } else if (size < 1024 * 1024 * 1024) {
                sizeString = QString("%1 MB").arg(size / (1024.0 * 1024.0), 0, 'f', 1);
            } else {
                sizeString = QString("%1 GB").arg(size / (1024.0 * 1024.0 * 1024.0), 0, 'f', 1);
            }
            m_lblFileSize->setText(QString("Size: %1").arg(sizeString));
        } else {
            m_lblFileSize->setText("Size: N/A");
        }
    } else if (m_lblFileSize) {
        m_lblFileSize->setText("Size: N/A");
    }
}

void MainWindow::clearStatusBarMessage()
{
    if (m_statusLabel) {
        m_statusLabel->clear();
    }
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    setStatusBarMessage(tr("Switched to tab: %1").arg(ui->tabWidget->tabText(index)));
    // Assuming 'Edit' tab is at index 2 (Output is 0, Help is 1)
    if (index == 2) {
        scrollToCurrentCommandInEditor();
        updateFileSizeLabel(); // Update file size when switching to Edit tab
    }
}



QString MainWindow::buildCommandLine()

{

    if (m_currentConfig.isEmpty()) {

        QMessageBox::warning(this, tr("Warning"), tr("No configuration loaded"));

        return "";

    }



                QString executable = m_currentConfig["executable"].toString();



                if (executable.isEmpty()) {



                    QMessageBox::critical(this, tr("Error"), tr("No executable specified in configuration"));



                    return "";



                }



            



                QString commandLine = executable;



    if (m_sudoCheckBox && m_sudoCheckBox->isChecked()) {

        commandLine = "sudo " + commandLine;

    }



    QFormLayout *formLayout = qobject_cast<QFormLayout*>(ui->scrollAreaWidgetContents->layout());

    QJsonArray configArgs = m_currentConfig["arguments"].toArray();



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

                QList<QCheckBox*> checkBoxes = ui->scrollAreaWidgetContents->findChildren<QCheckBox*>();

                for (QCheckBox *checkBox : checkBoxes) {

                    if (checkBox->property("argFlag").toString() == flag && checkBox->isChecked()) {

                        commandLine += " " + flag;

                        break;

                    }

                }

            }

        } else { // string, integer, file, folder

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

                } else if (QWidget *container = qobject_cast<QWidget*>(fieldWidget)) {

                    QLineEdit* lineEdit = container->findChild<QLineEdit*>();

                    if(lineEdit) {

                        paramValue = lineEdit->text();

                    }

                } else if (QListWidget *listWidget = qobject_cast<QListWidget*>(fieldWidget)) {

                    for (int j = 0; j < listWidget->count(); ++j) {

                        paramValue += " " + listWidget->item(j)->text();

                    }

                }



                if (!paramValue.isEmpty()) {

                    if (!flag.isEmpty()) {

                        commandLine += " " + flag;

                    }

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

    return commandLine;

}

void MainWindow::on_btnBreak_clicked()
{
    if (m_process && m_process->state() == QProcess::Running) {
        m_process->terminate();
        setStatusBarMessage(tr("Process terminated."));
    }
}

void MainWindow::on_btnSaveCommand_clicked()
{
    SaveCommandDialog dialog(this);
    dialog.setWidgets(ui->scrollAreaWidgetContents);
    dialog.setCommandName(m_currentConfig["name"].toString());

    if (dialog.exec() == QDialog::Accepted) {
        QJsonObject newCommand = dialog.getNewCommand();
        // Add the executable from the current command to the new preset
        newCommand["executable"] = m_currentConfig["executable"].toString();

        QString selectedTopic = ui->cmbTopics->currentText();

        QFile file(m_currentConfigFilePath);
        if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QMessageBox::critical(this, "Error", "Could not open config file for writing.");
            return;
        }

        QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll());
        QJsonObject rootObj = jsonDoc.object();
        QJsonObject topicsObj = rootObj["topics"].toObject();
        QJsonArray commandsArray = topicsObj[selectedTopic].toArray();

        // Append the new command to the array
        commandsArray.append(newCommand);
        topicsObj[selectedTopic] = commandsArray;
        rootObj["topics"] = topicsObj;

        file.resize(0); // Clear existing content
        file.write(QJsonDocument(rootObj).toJson(QJsonDocument::Indented));
        file.close();

        loadConfigFile(m_currentConfigFilePath);
        // After saving and reloading, select the newly added command
        ui->cmbTopics->setCurrentText(selectedTopic);
        ui->cmbCommands->setCurrentText(newCommand["name"].toString());
    }
}

void MainWindow::on_btnImportJSON_clicked()
{
    QString importFilePath = QFileDialog::getOpenFileName(this, tr("Import JSON File"), "", tr("JSON Files (*.json)"));
    if (importFilePath.isEmpty()) {
        return;
    }

    QFile importFile(importFilePath);
    if (!importFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open import file: %1").arg(importFilePath));
        return;
    }

    QByteArray importData = importFile.readAll();
    QJsonDocument importDoc = QJsonDocument::fromJson(importData);
    if (importDoc.isNull() || !importDoc.isObject()) {
        QMessageBox::critical(this, tr("Error"), tr("Invalid JSON in import file: %1").arg(importFilePath));
        return;
    }

    QJsonObject importObject = importDoc.object();

    // Assuming m_rootConfig is the currently loaded config
    for (auto it = importObject.begin(); it != importObject.end(); ++it) {
        m_rootConfig[it.key()] = it.value();
    }

    // Save the merged configuration back to the current config file
    if (m_currentConfigFilePath.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("No main configuration file loaded. Cannot save merged configuration."));
        return;
    }

    QFile configFile(m_currentConfigFilePath);
    if (!configFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open configuration file for writing: %1").arg(m_currentConfigFilePath));
        return;
    }

    QJsonDocument newConfigDoc(m_rootConfig);
    configFile.write(newConfigDoc.toJson(QJsonDocument::Indented));
    configFile.close();

    // Reload the configuration to update the UI
    loadConfigFile(m_currentConfigFilePath);

    setStatusBarMessage(tr("Successfully imported and merged from %1").arg(importFilePath));
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
                } else if (item->widget() == m_themeComboBox) {
                    m_themeComboBox = nullptr;
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
    updateCommandLineLabel();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    setStatusBarMessage(tr("Application closing."));
    QSettings settings("MyCompany", "Quish");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("currentTab", ui->tabWidget->currentIndex());
    settings.setValue("splitterSizes", ui->splitter->saveState());
    settings.setValue("theme", m_themeComboBox->currentText());

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
