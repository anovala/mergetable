#include "headerDelegate.h"
#include <QPainter>

headerDelegate::headerDelegate(QObject *parent):
    QStyledItemDelegate(parent)
{

}

void headerDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    auto rect = option.rect;
    // auto pen = painter->pen();

    if(m_coloredRows.contains(index.row()) || m_coloredCols.contains(index.column()))
    {
        // pen.setWidth(0);
        painter->setPen(Qt::NoPen);
        painter->setBrush(QBrush(QColor(224,224,224)));
        painter->drawRect(rect);
    }

    QStyledItemDelegate::paint(painter,option,index);
}

void headerDelegate::setColoredRow(int row)
{
    m_coloredRows.insert(row);
}

void headerDelegate::setColoredCol(int col)
{
    m_coloredCols.insert(col);
}

void headerDelegate::removeColoredRow(int row)
{
    m_coloredRows.remove(row);
}

void headerDelegate::removeColoredCol(int col)
{
    m_coloredCols.remove(col);
}

void headerDelegate::clear()
{
    m_coloredRows.clear();
    m_coloredCols.clear();
}
