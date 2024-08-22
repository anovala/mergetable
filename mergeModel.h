#pragma once

#include <QAbstractTableModel>
#include <QSqlDatabase>
#include <QStack>

#define MAXSTACKSIZE 100
struct Cell{
    QString val = "temp";
    // int row;
    // int column;
    int rowSpan = 1;
    int colSpan = 1;
    int row;
    int col;

    bool operator==(const Cell&temp) const
    {
        return this->rowSpan == temp.rowSpan && this->colSpan == temp.colSpan &&
               this->col == temp.col && this->row == temp.row && this->val == temp.val;
    }
};

struct TableState {
    QList<Cell> cells;
    QList<QPair<int,int>> mergedCells;
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
    void savetoJson(const QString &fileName);
    void loadFromJson(const QString &fileName);
    void restoreTableMergeState(bool init = false);
    void clearTableMergeState();
    void initTable(const QString& tableName);
    Cell* find(int row, int col);

private:
    void increaseCol(int col, int rowBegin,int totalRow);
    void increaseRow(int row, int colBegin,int totalColumn);
    void print(Cell cell);
    void printTable();
    void sortTable();
    void appendAndIncreseRow(int row, int col,int totalCol,
                             int rowSpan = 1,int colSpan = 1,const QString& val = "Cell");
    void appendAndIncreaseCol(int row, int col, int totalRow,
                             int rowSpan = 1, int colSpan =1, const QString& val = "Cell");

    Cell* findSpanOnCol(int row,int col);
    Cell* findSpanOnRow(int row,int col);
    void saveCurrentState();
public slots:

//operate need to store
    void removeRow_(int row);
    void removeColumn_(int col);
    void insertRows_(int row, int count);
    void insertRow_(int row);
    void insertColumn_(int col);
    void insertColumns_(int col, int count);
    void split(int row, int col);
    void merge(int top, int left, int width, int height);
    void undo();
    void redo();

signals:
    // void cancelMerge(int row, int col);
    // void mergeRequest(int row, int col, int rowSpan, int colSpan);
    void mergeSig(int row, int col, int rowSpan = 1, int colSpan = 1);
    void enableRedo(bool);
    void enableUndo(bool);

private:
    TableState m_state;
    // QList<Cell> m_cells;
    QStack<TableState> m_undoStack;
    QStack<TableState> m_redoStack;
    QSqlDatabase m_db;
};
