#ifndef EDITVARIABLESDIALOG_H
#define EDITVARIABLESDIALOG_H

#include "core/Iaito.h"
#include <QDialog>

namespace Ui {
class EditVariablesDialog;
}

class EditVariablesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditVariablesDialog(
        RVA offset, QString initialVar = QString(), QWidget *parent = nullptr);
    ~EditVariablesDialog();

    bool empty() const;
private slots:
    void applyFields();
    void updateFields();

private:
    Ui::EditVariablesDialog *ui;
    RVA functionAddress;
    QList<VariableDescription> variables;

    void populateTypesComboBox();
};

#endif // EDITVARIABLESDIALOG_H
