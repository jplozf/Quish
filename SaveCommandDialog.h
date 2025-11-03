#ifndef SAVECOMMANDDIALOG_H
#define SAVECOMMANDDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QFormLayout>
#include <QLabel>

class SaveCommandDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SaveCommandDialog(QWidget *parent = nullptr);
    void setWidgets(QWidget *scrollAreaWidgetContents);
    void setCommandName(const QString &name);
    QJsonObject getNewCommand();

private slots:
    void onOkClicked();

private:
    QGridLayout *m_dialogGridLayout;
    QLineEdit *m_nameLineEdit;
    QList<QCheckBox*> m_checkBoxes;
    QList<QWidget*> m_widgets;
    QJsonObject m_newCommand;
};

#endif // SAVECOMMANDDIALOG_H
