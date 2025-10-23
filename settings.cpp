#include "settings.h"

#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDir>
#include <QFile>
#include <QDataStream>
#include <QStandardPaths>
#include <QCheckBox>

// Placeholder for Constants if not available
class Constants {
public:
    QString getQString(const QString& key) {
        if (key == "APP_FOLDER") return ".Quish";
        if (key == "SETTINGS_FILE") return "settings.dat";
        return QString();
    }
};

//******************************************************************************
// Settings()
//******************************************************************************
Settings::Settings(QObject *parent)
    : QObject(parent)
{
    // Set the defaults values...
    defaults["minimizeToTray"] = QVariant(false);
    defaults["confirmExit"] = QVariant(true);

    // Read the settings from user's settings
    read();

    // Check if all defaults settings are set and set them if any
    foreach (const auto& defaultKey, defaults.keys()) {
        if (!settings.contains(defaultKey)) {
            settings[defaultKey] = defaults[defaultKey];
        }
    }

    // Delete all keys that are no more in the defaults settings values
    foreach (const auto& settingKey, settings.keys()) {
        if (!defaults.contains(settingKey)) {
            settings.erase(settings.find(settingKey));
        }
    }

    // Save the settings to user's settings
    write();
}

//******************************************************************************
// get()
//******************************************************************************
QVariant Settings::get(QString param) {
    return this->settings[param];
}

//******************************************************************************
// set()
//******************************************************************************
void Settings::set(QString param, QVariant value) {
    this->settings[param] = value;
    emit settingChanged(param, value);
    write();
}

//******************************************************************************
// write()
//******************************************************************************
void Settings::write() {
    Constants appConstants; // Use placeholder Constants
    QString appFolderPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/" + appConstants.getQString("APP_FOLDER");
    QDir appDir(appFolderPath);
    if (!appDir.exists()) {
        appDir.mkpath(".");
    }

    QFile fSettings(appDir.filePath(appConstants.getQString("SETTINGS_FILE")));
    if (fSettings.open(QIODevice::WriteOnly)) {
        QDataStream out(&fSettings);
        out.setVersion(QDataStream::Qt_5_3);
        out << settings;
        fSettings.close();
    }
}

//******************************************************************************
// read()
//******************************************************************************
void Settings::read() {
    Constants appConstants; // Use placeholder Constants
    QString appFolderPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/" + appConstants.getQString("APP_FOLDER");
    QDir appDir(appFolderPath);

    QFile fSettings(appDir.filePath(appConstants.getQString("SETTINGS_FILE")));
    if (fSettings.open(QIODevice::ReadOnly)) {
        QDataStream in(&fSettings);
        in.setVersion(QDataStream::Qt_5_3);
        in >> settings;
        fSettings.close();
    }
}

//******************************************************************************
// form()
//******************************************************************************
void Settings::form(QWidget *w) {
    QFormLayout *form = qobject_cast<QFormLayout*>(w->layout());
    if (!form) {
        form = new QFormLayout(w);
        w->setLayout(form);
    }

    // Add settings for Quish
    QLabel *lblMinimizeToTray = new QLabel(tr("Minimize to system tray on close"));
    QCheckBox *chkMinimizeToTray = new QCheckBox();
    chkMinimizeToTray->setChecked(get("minimizeToTray").toBool());
    connect(chkMinimizeToTray, &QCheckBox::toggled, this, [this, chkMinimizeToTray]() {
        handleCheckBoxChanged(chkMinimizeToTray, "minimizeToTray");
    });
    form->addRow(lblMinimizeToTray, chkMinimizeToTray);

    QLabel *lblConfirmExit = new QLabel(tr("Confirm on exit"));
    QCheckBox *chkConfirmExit = new QCheckBox();
    chkConfirmExit->setChecked(get("confirmExit").toBool());
    connect(chkConfirmExit, &QCheckBox::toggled, this, [this, chkConfirmExit]() {
        handleCheckBoxChanged(chkConfirmExit, "confirmExit");
    });
    form->addRow(lblConfirmExit, chkConfirmExit);

    // Example of other settings (commented out for now)
    /*
    form->addRow(new QLabel("<b>ᐅ</b>"), new QLabel("<b>VOSTOK'S SETTINGS</b>"));
    form->addRow(new QLabel(""), new QLabel(""));

    for(auto e : settings.keys())
    {
        if (e != "minimizeToTray" && e != "confirmExit") { // Avoid duplicating our specific settings
            QLabel *lblSetting = new QLabel(e);
            QLineEdit *txtSetting = new QLineEdit(settings.value(e).toString());
            connect(txtSetting, &QLineEdit::textChanged, [=]{handleTextChanged(lblSetting, txtSetting);});
            form->addRow(lblSetting, txtSetting);
        }
    }

    form->addRow(new QLabel(""), new QLabel(""));
    form->addRow(new QLabel("<b>ᐅ</b>"), new QLabel("<b>RPN HELP</b>"));
    form->addRow(new QLabel(""), new QLabel(""));

    // Placeholder for RPN if not available
    // RPN *rpn = new RPN(nullptr);
    // for(auto e : rpn->help.keys())
    // {
    //     QLabel *lbl = new QLabel(" " + e + " ");
    //     lbl->setStyleSheet("QLabel { font: 10pt 'monospace'; background-color : lightgrey; }");
    //     QLabel *txtHelp = new QLabel(rpn->help.value(e));
    //     form->addRow(lbl, txtHelp);
    // }
    */
}

//******************************************************************************
// handleTextChanged()
//******************************************************************************
void Settings::handleTextChanged(QLabel *lbl, QLineEdit *txt) {
    settings[lbl->text()] = QVariant(txt->text());
    write();
}

//******************************************************************************
// handleCheckBoxChanged()
//******************************************************************************
void Settings::handleCheckBoxChanged(QCheckBox *chk, const QString &param) {
    QVariant value = QVariant(chk->isChecked());
    settings[param] = value;
    emit settingChanged(param, value);
    write();
}
