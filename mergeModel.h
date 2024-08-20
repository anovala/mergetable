#pragma once

#include <QAbstractTableModel>
#include <QSqlDatabase>
struct oneLine {
    int a;
    int b;
    QString c;
    float d;
};


struct Cell{
    QString val = "temp";
    // int row;
    // int column;
    int rowSpan = 1;
    int colSpan = 1;
    int row;
    int col;
};


class mergeModel : public QAbstractTableModel{
    Q_OBJECT

public:
    mergeModel(QObject *parent);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &parent = QModelIndex(), int role = Qt::DisplayRole)const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Q_INVOKABLE virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole)override;
    virtual QSize span(const QModelIndex &index) const override;

    bool savetoDb(const QString& tableName);
    bool loadFromDb(const QString& tableName);
    void initTable(const QString& tableName);
    Cell* find(int row, int col);
    void increaseCol(int col, int rowBegin,int totalRow);
    void increaseRow(int row, int colBegin,int totalColumn);
    void print(Cell cell);

public slots:
    void removeRow_(int row);
    void removeColumn_(int col);
    void insertRow_(int row);
    void insertColumn_(int col);
    void merge(int top, int left, int width, int height);

private:
    QList<Cell> m_cells;
    QSqlDatabase m_db;
};
