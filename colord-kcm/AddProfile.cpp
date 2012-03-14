#include "AddProfile.h"
#include "ui_AddProfile.h"

AddProfile::AddProfile(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AddProfile)
{
    ui->setupUi(this);
}

AddProfile::~AddProfile()
{
    delete ui;
}
