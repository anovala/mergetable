#pragma once

#include <QWidget>
#include <QMenu>
#include "mergeModel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class mergeTable;
}
QT_END_NAMESPACE

class mergeTable : public QWidget
{
    Q_OBJECT

public:
    mergeTable(QWidget *parent = nullptr);
    ~mergeTable();

private:
    void showMenu(const QPoint &pos);

private:
    Ui::mergeTable *ui;
    mergeModel *m_model;
    QMenu menu;
};
