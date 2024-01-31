#include "msys2dialog.h"
#include "common.h"
#include "ui_msys2dialog.h"
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
Msys2Dialog::Msys2Dialog(QWidget *parent) : QDialog(parent), ui(new Ui::Msys2Dialog)
{
    ui->setupUi(this);
    this->on_lineEdit_textChanged("");
}
Msys2Dialog::~Msys2Dialog()
{
    delete ui;
}
QString Msys2Dialog::name() const
{
    return ui->lineEdit->text();
}
void Msys2Dialog::on_lineEdit_textChanged(const QString &arg1)
{
}
