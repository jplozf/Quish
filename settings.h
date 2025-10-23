#ifndef SETTINGS_H
#define SETTINGS_H

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QObject>
#include <QVariant>
#include <QWidget>

class Settings : public QObject
{
    Q_OBJECT
public:
    explicit Settings(QObject *parent = nullptr);

    QVariant get(QString param);
    void set(QString param, QVariant value); // New method to set a setting
    void write();
    void read();
    void form(QWidget *w);

signals:
    void settingChanged(const QString &param, const QVariant &value);

private:
    QMap<QString, QVariant> settings;
    QMap<QString, QVariant> defaults;

    void handleTextChanged(QLabel *lbl, QLineEdit *txt); // Forward declaration
    void handleCheckBoxChanged(QCheckBox *chk, const QString &param); // New handler for checkboxes
};

#endif // SETTINGS_H
