#ifndef FORMPLOT_H
#define FORMPLOT_H

#include <QMainWindow>
#include <QCompleter>

#include <qcustomplot.h>

namespace Ui {
class FormPlot;
}

class FormPlot : public QMainWindow
{
    Q_OBJECT

public:
    explicit FormPlot(QStringList pvs, QDateTime from, QDateTime to, QWidget *parent = nullptr);
    ~FormPlot();

private:
    Ui::FormPlot *ui;

    QStringList pvList;
    QCompleter* completer;
};

#endif // FORMPLOT_H
