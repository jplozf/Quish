#include "SaveCommandDialog.h"
#include <QFormLayout>
#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QLabel>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonValue>
#include <QVariant>
#include <QScrollArea>

SaveCommandDialog::SaveCommandDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Save Command Preset");
    setMinimumWidth(450);
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Command Name Input
    QHBoxLayout* nameLayout = new QHBoxLayout();
    nameLayout->addWidget(new QLabel("Preset Name:", this));
    m_nameLineEdit = new QLineEdit(this);
    nameLayout->addWidget(m_nameLineEdit);
    mainLayout->addLayout(nameLayout);

    // Parameters Area
    mainLayout->addWidget(new QLabel("Select parameters to include in the preset:", this));
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    QWidget *scrollAreaWidgetContents = new QWidget();
    m_dialogGridLayout = new QGridLayout(scrollAreaWidgetContents);
    scrollArea->setWidget(scrollAreaWidgetContents);
    mainLayout->addWidget(scrollArea);

    // OK/Cancel Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &SaveCommandDialog::onOkClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void SaveCommandDialog::setCommandName(const QString &name)
{
    m_nameLineEdit->setText(name + " - ");
    m_nameLineEdit->setFocus();
    m_nameLineEdit->setCursorPosition(m_nameLineEdit->text().length());
}

void SaveCommandDialog::setWidgets(QWidget *scrollAreaWidgetContents)
{
    int currentRow = 0;

    // Find and add special widgets first
    QCheckBox *sudoCheckBox = scrollAreaWidgetContents->findChild<QCheckBox*>("m_sudoCheckBox");
    if (sudoCheckBox) {
        QCheckBox *dialogCheckBox = new QCheckBox(this);
        m_widgets.append(sudoCheckBox);
        m_checkBoxes.append(dialogCheckBox);

        QLabel *nameLabel = new QLabel(sudoCheckBox->text(), this);
        QLabel *valueLabel = new QLabel(sudoCheckBox->isChecked() ? "Checked" : "Unchecked", this);
        valueLabel->setStyleSheet("font-style: italic;");

        m_dialogGridLayout->addWidget(nameLabel, currentRow, 0);
        m_dialogGridLayout->addWidget(valueLabel, currentRow, 1);
        m_dialogGridLayout->addWidget(dialogCheckBox, currentRow, 2);
        currentRow++;
    }

    QCheckBox *clearOutputCheckBox = scrollAreaWidgetContents->findChild<QCheckBox*>("m_clearOutputCheckBox");
    if (clearOutputCheckBox) {
        QCheckBox *dialogCheckBox = new QCheckBox(this);
        m_widgets.append(clearOutputCheckBox);
        m_checkBoxes.append(dialogCheckBox);

        QLabel *nameLabel = new QLabel(clearOutputCheckBox->text(), this);
        QLabel *valueLabel = new QLabel(clearOutputCheckBox->isChecked() ? "Checked" : "Unchecked", this);
        valueLabel->setStyleSheet("font-style: italic;");

        m_dialogGridLayout->addWidget(nameLabel, currentRow, 0);
        m_dialogGridLayout->addWidget(valueLabel, currentRow, 1);
        m_dialogGridLayout->addWidget(dialogCheckBox, currentRow, 2);
        currentRow++;
    }

    // The working directory line edit is inside a QWidget container
    QWidget *workingDirectoryContainer = nullptr;
    QFormLayout *sourceLayout = qobject_cast<QFormLayout*>(scrollAreaWidgetContents->layout());
    if (sourceLayout) {
        for (int i = 0; i < sourceLayout->rowCount(); ++i) {
            QLayoutItem *labelItem = sourceLayout->itemAt(i, QFormLayout::LabelRole);
            if (labelItem && qobject_cast<QLabel*>(labelItem->widget()) && qobject_cast<QLabel*>(labelItem->widget())->text() == tr("Folder")) {
                QLayoutItem *fieldItem = sourceLayout->itemAt(i, QFormLayout::FieldRole);
                if (fieldItem && fieldItem->widget()) {
                    workingDirectoryContainer = fieldItem->widget();
                    break;
                }
            }
        }
    }

    if (workingDirectoryContainer) {
        QLineEdit *workingDirectoryLineEdit = workingDirectoryContainer->findChild<QLineEdit*>("m_workingDirectoryLineEdit");
        if (workingDirectoryLineEdit) {
            QCheckBox *dialogCheckBox = new QCheckBox(this);
            m_widgets.append(workingDirectoryLineEdit);
            m_checkBoxes.append(dialogCheckBox);

            QLabel *nameLabel = new QLabel(tr("Working Directory"), this);
            QLabel *valueLabel = new QLabel(workingDirectoryLineEdit->text().isEmpty() ? "[empty]" : workingDirectoryLineEdit->text(), this);
            valueLabel->setStyleSheet("font-style: italic;");

            m_dialogGridLayout->addWidget(nameLabel, currentRow, 0);
            m_dialogGridLayout->addWidget(valueLabel, currentRow, 1);
            m_dialogGridLayout->addWidget(dialogCheckBox, currentRow, 2);
            currentRow++;
        }
    }

    // Now, continue with the existing logic for regular arguments
    if (!sourceLayout) return;

    // This is the robust logic to iterate through all rows and identify the widgets
    for (int i = 0; i < sourceLayout->rowCount(); ++i) {
        QLayoutItem *labelItem = sourceLayout->itemAt(i, QFormLayout::LabelRole);
        QLayoutItem *fieldItem = sourceLayout->itemAt(i, QFormLayout::FieldRole);

        QWidget *sourceWidget = nullptr;
        QString labelText;
        QString valueText;

        // Prioritize checking for QCheckBox or QRadioButton in either role
        if (labelItem && labelItem->widget()) {
            if (auto cb = qobject_cast<QCheckBox*>(labelItem->widget())) {
                sourceWidget = cb;
                labelText = cb->text();
                valueText = cb->isChecked() ? "Checked" : "Unchecked";
            } else if (auto rb = qobject_cast<QRadioButton*>(labelItem->widget())) {
                if (!rb->isChecked()) continue; // Only process selected radio buttons
                sourceWidget = rb;
                labelText = rb->text();
                valueText = "Selected";
            }
        }
        if (!sourceWidget && fieldItem && fieldItem->widget()) {
            if (auto cb = qobject_cast<QCheckBox*>(fieldItem->widget())) {
                sourceWidget = cb;
                // Try to get label from labelItem if available, otherwise use checkbox text
                if (labelItem && labelItem->widget() && qobject_cast<QLabel*>(labelItem->widget())) {
                    labelText = qobject_cast<QLabel*>(labelItem->widget())->text();
                } else {
                    labelText = cb->text();
                }
                valueText = cb->isChecked() ? "Checked" : "Unchecked";
            } else if (auto rb = qobject_cast<QRadioButton*>(fieldItem->widget())) {
                if (!rb->isChecked()) continue; // Only process selected radio buttons
                sourceWidget = rb;
                // Try to get label from labelItem if available, otherwise use radio button text
                if (labelItem && labelItem->widget() && qobject_cast<QLabel*>(labelItem->widget())) {
                    labelText = qobject_cast<QLabel*>(labelItem->widget())->text();
                } else {
                    labelText = rb->text();
                }
                valueText = "Selected";
            }
        }

        // If not a checkbox/radio button, then check for a standard label-field pair
        if (!sourceWidget && labelItem && labelItem->widget() && fieldItem && fieldItem->widget()) {
            if (auto label = qobject_cast<QLabel*>(labelItem->widget())) {
                sourceWidget = fieldItem->widget();
                labelText = label->text();

                if (auto lineEdit = qobject_cast<QLineEdit*>(sourceWidget)) {
                    valueText = lineEdit->text();
                } else if (auto listWidget = qobject_cast<QListWidget*>(sourceWidget)) {
                    QStringList items;
                    for (int j = 0; j < listWidget->count(); ++j) items.append(listWidget->item(j)->text());
                    valueText = items.join(", ");
                } else if (auto container = qobject_cast<QWidget*>(sourceWidget)) { // For composite widgets
                    if (auto lineEdit = container->findChild<QLineEdit*>()) {
                        valueText = lineEdit->text();
                    }
                }
            }
        }

        // If we successfully identified a widget and its label, add it to the dialog
        if (sourceWidget && !labelText.isEmpty()) {
            QCheckBox *dialogCheckBox = new QCheckBox(this);
            m_widgets.append(sourceWidget);
            m_checkBoxes.append(dialogCheckBox);

            QLabel *nameLabel = new QLabel(labelText, this);
            QLabel *valueLabel = new QLabel(valueText.isEmpty() ? "[empty]" : valueText, this);
            valueLabel->setStyleSheet("font-style: italic;");
            valueLabel->setWordWrap(true);

            m_dialogGridLayout->addWidget(nameLabel, currentRow, 0);
            m_dialogGridLayout->addWidget(valueLabel, currentRow, 1);
            m_dialogGridLayout->addWidget(dialogCheckBox, currentRow, 2);

            // Pre-check and disable if the original widget was mandatory
            if (sourceWidget->property("mandatory").isValid() && sourceWidget->property("mandatory").toBool()) {
                dialogCheckBox->setChecked(true);
                dialogCheckBox->setEnabled(false);
            }
            currentRow++;
        }
    }
}

void SaveCommandDialog::onOkClicked()
{
    m_newCommand["name"] = m_nameLineEdit->text();
    QJsonArray arguments;

    for (int i = 0; i < m_widgets.size(); ++i) {
        if (!m_checkBoxes[i]->isChecked()) continue;

        QWidget *widget = m_widgets[i];

        // Handle special, non-argument widgets by their object name
        if (widget->objectName() == "m_sudoCheckBox") {
            m_newCommand["sudo"] = qobject_cast<QCheckBox*>(widget)->isChecked();
            continue;
        }
        if (widget->objectName() == "m_clearOutputCheckBox") {
            m_newCommand["clear_output"] = qobject_cast<QCheckBox*>(widget)->isChecked();
            continue;
        }
        // The working directory is a QWidget container holding the actual QLineEdit
        if (widget->objectName() == "m_workingDirectoryLineEdit") {
            m_newCommand["working_directory"] = qobject_cast<QLineEdit*>(widget)->text();
            continue;
        }

        // Handle regular argument widgets, identified by having the "argType" property
        if (!widget->property("argType").isValid()) continue;

        QJsonObject arg;
        arg["name"] = widget->property("argName").toString();
        arg["type"] = widget->property("argType").toString();
        arg["flag"] = widget->property("argFlag").toString();
        arg["exclusive_group"] = widget->property("exclusiveGroup").toString();
        arg["mandatory"] = widget->property("mandatory").toBool();

        QVariant defaultValue;
        if (auto rb = qobject_cast<QRadioButton*>(widget)) defaultValue = rb->isChecked();
        else if (auto cb = qobject_cast<QCheckBox*>(widget)) defaultValue = cb->isChecked();
        else if (auto le = qobject_cast<QLineEdit*>(widget)) defaultValue = le->text();
        else if (auto lw = qobject_cast<QListWidget*>(widget)) {
            QStringList items;
            for (int j = 0; j < lw->count(); ++j) items.append(lw->item(j)->text());
            defaultValue = QJsonArray::fromStringList(items);
        } else if (auto container = qobject_cast<QWidget*>(widget)) {
            if (auto le = container->findChild<QLineEdit*>()) defaultValue = le->text();
        }

        if (defaultValue.isValid()) {
            arg["default"] = QJsonValue::fromVariant(defaultValue);
        }
        arguments.append(arg);
    }

    m_newCommand["arguments"] = arguments;
    accept();
}

QJsonObject SaveCommandDialog::getNewCommand()
{
    return m_newCommand;
}
