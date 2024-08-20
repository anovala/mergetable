#include "mergeModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QSize>

mergeModel::mergeModel(QObject *parent):QAbstractTableModel(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("cell.db");
    if(!m_db.open())
    {
        qDebug() << "Falied to open database!"<<m_db.lastError().text() ;
    }

    initTable("cellTable");
    loadFromDb("cellTable");
}

int mergeModel::rowCount(const QModelIndex &parent) const
{
    int maxRows = 0;

    for(auto &&cell : m_cells)
    {
        auto spanEndRow = cell.row+cell.rowSpan-1;
        if(spanEndRow > maxRows)
            maxRows = spanEndRow;
    }
    return maxRows + 1;
}


int mergeModel::columnCount(const QModelIndex &parent) const
{
    int maxCols = 0;

    for(auto &&cell : m_cells)
    {
        auto spanEndCol = cell.col + cell.colSpan-1;
        if(spanEndCol > maxCols)
            maxCols = spanEndCol;
    }
    return maxCols+1;
}

QVariant mergeModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid())
    {
        return QVariant();
    }

    if(role == Qt::DisplayRole || role == Qt::EditRole)
    {
        auto row = index.row();
        auto col = index.column();
        for(auto &&cell : m_cells)
        {
            if(row >= cell.row && row < cell.row+cell.rowSpan &&
                col >= cell.col && col < cell.col+cell.colSpan)
            {
                return cell.val;
            }
        }
    }else if(role == Qt::CheckStateRole)
        return QVariant();

    return QVariant();
}

Qt::ItemFlags mergeModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool mergeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if(!index.isValid())
        return false;
    auto row = index.row();
    auto col = index.column();

    if(role == Qt::EditRole)
    {
        for(auto &&cell : m_cells)
        {
            if(row >= cell.row && row < cell.row+cell.rowSpan &&
                col >= cell.col && col < cell.col+cell.colSpan)
            {
                cell.val = value.toString();
                emit dataChanged(index,index,{role});
                return true;
            }
        }
    }
    return false;
}

QSize mergeModel::span(const QModelIndex &index) const
{
    if(!index.isValid())
        return QSize(1,1);

    auto row = index.row();
    auto col = index.column();

    for(auto &&cell : m_cells)
    {
        if(cell.col== col && cell.row == row)
        {
            return QSize(cell.rowSpan,cell.rowSpan);
        }
    }

    return QSize(1,1);
}

bool mergeModel::savetoDb(const QString &tableName)
{
    if(!m_db.isOpen())
    {
        qDebug() << "database open failed";
        return false;
    }

    QSqlQuery query;
    m_db.transaction();

    query.prepare(QString("DELETE FROM %1").arg(tableName));
    if (!query.exec()) {
        qDebug() << "Failed to clear table:" << query.lastError();
        return false;
    }

    // Insert each row into the database
    QString insertQueryStr = QString("INSERT INTO %1 (value, row, col, rowSpan,colSpan) "
                                     "VALUES (:value, :row, :col, :rowSpan, :colSpan)").arg(tableName);
    query.prepare(insertQueryStr);
    for(auto &&cell : m_cells)
    {
        query.bindValue(":value",cell.val);
        query.bindValue(":row",cell.row);
        query.bindValue(":col",cell.col);
        query.bindValue(":rowSpan",cell.rowSpan);
        query.bindValue(":colSpan",cell.colSpan);
        qDebug() << query.lastQuery();

        if(!query.exec())
        {
            qDebug() << "Failed to insert row:" << query.lastError();
            m_db.rollback(); // Rollback the transaction on failure
            return false;
        }
    }

    // Commit the transaction
    m_db.commit();

    qDebug() << "Data saved to database successfully.";
    return true;
}

bool mergeModel::loadFromDb(const QString &tableName)
{
    if(!m_db.isOpen())
    {
        qDebug() << "db not open!";
        return false;
    }
    m_cells.clear();

    QSqlQuery query;
    QString selectStr = QString("SELECT value, row, col, rowSpan, colSpan FROM %1").arg(tableName);
    query.prepare(selectStr);

    if(!query.exec())
    {
        qDebug() << "Failed to select: "<<query.lastError().text();
        return false;
    }

    while(query.next())
    {
        Cell cell;
        cell.val = query.value("value").toString();
        cell.row = query.value("row").toInt();
        cell.col = query.value("col").toInt();
        cell.rowSpan = query.value("rowSpan").toInt();
        cell.colSpan = query.value("colSpan").toInt();
        m_cells.append(cell);
    }

    emit dataChanged(index(0,0),index(rowCount()-1,columnCount()-1));
    qDebug() << "Load from database successful";
    return true;
}



void mergeModel::initTable(const QString &tableName)
{
    QSqlQuery query;

    // Create the table if it doesn't exist
    QString createTableQuery = QString(
                                   "CREATE TABLE IF NOT EXISTS %1 ("
                                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                                   "value TEXT, "
                                   "row INTEGER, "
                                   "col INTEGER,"
                                   "rowSpan INTEGER,"
                                   "colSpan INTEGER)").arg(tableName);

    if (!query.exec(createTableQuery)) {
        qDebug() << "Failed to create table:" << query.lastError();
    } else {
        qDebug() << "Table created or already exists.";
    }

    // Check if the table is already populated
    QString countQueryStr = QString("SELECT COUNT(*) FROM %1").arg(tableName);
    if (!query.exec(countQueryStr)) {
        qDebug() << "Failed to check table data:" << query.lastError();
        return;
    }

    query.next();
    int count = query.value(0).toInt();

    // If the table is already populated, skip the insertion
    if (count > 0) {
        qDebug() << "Table is already populated. Skipping initial data insertion.";
        return;
    }

    m_db.transaction();

    QString insertQueryStr = QString("INSERT INTO %1 (value, row, col, rowSpan, colSpan) VALUES (?, ?, ?, ?, ?)").arg(tableName);
    query.prepare(insertQueryStr);

    int gridSize = 4; // Define the size of the grid
    for (int row = 0; row < gridSize; ++row) {
        for (int col = 0; col < gridSize; ++col) {
            query.bindValue(0, "Cell");  // Empty value for now
            query.bindValue(1, row);
            query.bindValue(2, col);
            query.bindValue(3, 1);  // rowSpan
            query.bindValue(4, 1);  // colSpan

            if (!query.exec()) {
                qDebug() << "Failed to insert cell:" << query.lastError();
                m_db.rollback(); // Rollback the transaction on failure
                return;
            }
        }
    }
    m_db.commit();
    emit dataChanged(index(0,0),index(3,3));
    qDebug() << "Cells inserted into table successfully.";
}

Cell *mergeModel::find(int row, int col)
{
    auto it =std::find_if(m_cells.begin(),m_cells.end(),[row,col](const Cell&cell)
                 {
        return cell.row == row && cell.col == col;
    });

    if(it != m_cells.end())
        return &(*it);
    else
        return nullptr;
}

void mergeModel::increaseCol(int col, int rowBegin, int totalRow)
{
    for(int curRow = rowBegin;curRow < totalRow;curRow++)
    {
        auto cell = this->find(curRow,col);
        if(!cell)
            continue;
        cell->row++;
        qDebug() << "increase Cell:";
        print(*cell);
    }
}

void mergeModel::increaseRow(int row, int colBegin, int totalColumn)
{
    for(int curCol = colBegin; curCol < totalColumn; curCol++)
    {
        auto cell = this->find(row,curCol);
        if(!cell)
            continue;
        cell->col++;
        qDebug() << "increase Cell:";
        print(*cell);
    }
}

void mergeModel::print(Cell cell)
{
    qDebug() << cell.row << cell.col << cell.rowSpan<<cell.colSpan <<cell.val;
}

void mergeModel::removeRow_(int row)
{
    if(row < 0 || row >= rowCount())
        return;
    beginRemoveRows(QModelIndex(),row,row);
    for(int i = 0; i < m_cells.size(); ++i)
    {
        Cell &cell = m_cells[i];

        if(cell.row <= row && cell.row+cell.rowSpan > row)
        {
            if(cell.rowSpan > 1)
                cell.rowSpan--;
            else{
                m_cells.removeAt(i);
                i--;
            }
        }else if(cell.row > row){
            cell.row--;
        }

        if(cell.row < row && cell.row + cell.rowSpan > row)
        {
            cell.rowSpan--;
        }
    }
    endRemoveRows();
}

void mergeModel::removeColumn_(int col)
{
    if(col < 0 || col >= columnCount())
        return;

    beginRemoveColumns(QModelIndex(),col,col);
    for(int i = 0; i < m_cells.size(); ++i)
    {
        auto &cell = m_cells[i];

        //include or single
        if(cell.col <= col && cell.col + cell.colSpan> col)
        {
            if(cell.colSpan > 1)
                cell.colSpan--;
            else{
                m_cells.removeAt(i);
                i--;
            }
        }else if(cell.col > col)
            cell.col--;

        if(cell.col < col && cell.col + cell.colSpan > col)
        {
            cell.colSpan--;
        }
    }

    endRemoveColumns();
}

void mergeModel::insertRow_(int row)
{
    if(row < 0)
        return;
    beginInsertRows(QModelIndex(),row,row);
    int columnCount = this->columnCount();
    int rowCount = this->rowCount();
    for(int col=0;col < columnCount;)
    {
        //find the cell on the top of current Cell
        auto it = std::find_if(m_cells.begin(),m_cells.end(),[row,col](const Cell& cell)
        {
            qDebug() << "find cell pos: "<<row<<col;
            return cell.row == row -1 && cell.col == col;
        });

        if(it != m_cells.end())
        {
            //expand the top Cell
            if(it->row+it->rowSpan > row)
            {
                it->rowSpan++;
            }else
            {
                //a new cell
                //need to append the below cell row
                Cell newCell;
                newCell.row = row;
                newCell.col = col;
                newCell.colSpan = it->colSpan;
                newCell.rowSpan = 1;
                newCell.val = "Cell";
                qDebug() << "insert: ";
                print(newCell);
                increaseCol(col,newCell.row,rowCount);
                m_cells.append(newCell);
            }
            col += it->colSpan;
            qDebug() << " col += "<<it->colSpan;
        }else
        {
            //insert on first row
            Cell newCell;
            newCell.row = row;
            newCell.col = col;
            newCell.val = "Cell";
            increaseCol(col,newCell.row,rowCount);
            m_cells.append(newCell);
            col += 1;
        }
    }

    endInsertRows();
}

void mergeModel::insertColumn_(int col)
{
    if(col < 0)
        return;

    beginInsertColumns(QModelIndex(), col,col);
    int rowCount = this->rowCount();
    int colCount = this->columnCount();

    for(int row = 0; row < rowCount;)
    {
        auto it = std::find_if(m_cells.begin(),m_cells.end(),[row,col](const Cell& cell)
        {
            return cell.col == col-1 && cell.row == row;
        });

        if(it != m_cells.end())
        {
            if(it->col+it->colSpan > col)
            {
                it->colSpan++;
            }else
            {
                Cell newCell;
                newCell.row = row;
                newCell.col = col;
                newCell.colSpan = 1;
                newCell.rowSpan = it->rowSpan;
                newCell.val = "Cell";
                increaseRow(row,newCell.col,colCount);
                m_cells.append(newCell);
            }
            row += it->rowSpan;
        }else
            throw std::exception("can not find left cell");
    }

    endInsertColumns();
}

void mergeModel::merge(int top, int left, int width, int height)
{
    if(top <0 || left < 0 || width<=0 || height <=0)
    {
        qDebug() << "invalid parameter";
        return;
    }

    auto cell = find(top,left);
    if(!cell)
        return;
    cell->colSpan = width;
    cell->rowSpan = height;

    for(int row = top; row < top + height;row++)
    {
        for(int col = left; col < left + width;col++)
        {
            if(row == top && col == left)
                continue;

            auto newEnd  =std::remove_if(m_cells.begin(),m_cells.end(),[col,row](const Cell &cell){
                return cell.col == col && cell.row == row;
            });
            m_cells.erase(newEnd,m_cells.end());
        }
    }

    emit dataChanged(index(top,left),index(top+height-1,left+width-1),{Qt::DisplayRole});
}




