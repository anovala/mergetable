#pragma once

#include <QStyledItemDelegate>

class headerDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    headerDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void setColoredRow(int row);
    void setColoredCol(int col);
    void removeColoredRow(int row);
    void removeColoredCol(int col);
    void clear();
private:
    QSet<int> m_coloredRows;
    QSet<int> m_coloredCols;
};
