#include "FormPlot.h"
#include "ui_FormPlot.h"

FormPlot::FormPlot(QStringList pvs, QDateTime from, QDateTime to, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FormPlot)
{
    ui->setupUi(this);

    this->pvList = pvs;
    this->ui->dtFrom->setDateTime(from);
    this->ui->dtTo->setDateTime(to);

    this->completer = new QCompleter(this->pvList, this);
    this->completer->setCaseSensitivity(Qt::CaseInsensitive);
    this->ui->txtPV->setCompleter(this->completer);
}

FormPlot::~FormPlot()
{
    delete ui;
}
