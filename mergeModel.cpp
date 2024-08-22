#include "mergeModel.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QIODevice>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QSize>

mergeModel::mergeModel(QObject *parent):QAbstractTableModel(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("cell.db");
    if(!m_db.open())
    {
        qDebug() << "Falied to open database!"<<m_db.lastError().text() ;
    }
}

int mergeModel::rowCount(const QModelIndex &parent) const
{
    int maxRows = 0;

    for(auto &&cell : m_state.cells)
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

    for(auto &&cell : m_state.cells)
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
        for(auto &&cell : m_state.cells)
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
        saveCurrentState();
        for(auto &&cell : m_state.cells)
        {
            if(cell.row == row && cell.col == col)
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

    for(auto &&cell : m_state.cells)
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
    for(auto &&cell : m_state.cells)
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
    m_state.cells.clear();

    QSqlQuery query;
    QString selectStr = QString("SELECT value, row, col, rowSpan, colSpan FROM %1").arg(tableName);
    query.prepare(selectStr);

    if(!query.exec())
    {
        qDebug() << "Failed to select: "<<query.lastError().text();
        return false;
    }

    beginResetModel();

    while(query.next())
    {
        Cell cell;
        cell.val = query.value("value").toString();
        cell.row = query.value("row").toInt();
        cell.col = query.value("col").toInt();
        cell.rowSpan = query.value("rowSpan").toInt();
        cell.colSpan = query.value("colSpan").toInt();
        m_state.cells.append(cell);
    }

    endResetModel();
    this->printTable();

    // emit dataChanged(index(0,0),index(this->rowCount()-1,this->columnCount()-1));
    qDebug() << "Load from database successful";
    return true;
}

void mergeModel::savetoJson(const QString &fileName)
{
    QJsonArray cellArray;

    for (const Cell &cell : m_state.cells) {
        QJsonObject cellObject;
        cellObject["row"] = cell.row;
        cellObject["col"] = cell.col;
        cellObject["rowSpan"] = cell.rowSpan;
        cellObject["colSpan"] = cell.colSpan;
        cellObject["val"] = cell.val;

        cellArray.append(cellObject);
    }

    QJsonObject tableObject;
    tableObject["cells"] = cellArray;

    QJsonDocument doc(tableObject);

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open file for writing.");
        return;
    }

    file.write(doc.toJson());
    file.close();
}

void mergeModel::loadFromJson(const QString &fileName)
{
    QFile file(fileName);
    if(!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "Open json file failed!";
        return;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject tableObj = doc.object();

    beginResetModel();

    auto cellArray = tableObj["cells"].toArray();
    m_state.cells.clear();

    for(auto &&element:cellArray)
    {
        auto obj = element.toObject();
        Cell cell;
        cell.row = obj["row"].toInt();
        cell.col = obj["col"].toInt();
        cell.val = obj["val"].toString();
        cell.colSpan = obj["colSpan"].toInt();
        cell.rowSpan = obj["rowSpan"].toInt();
        m_state.cells.append(cell);
    }

    endResetModel();

    file.close();

}

void mergeModel::restoreTableMergeState(bool init)
{
    if(init)
    {
        for(auto &&cell:m_state.cells)
        {
            if(cell.colSpan > 1 || cell.rowSpan > 1)
                m_state.mergedCells.append({cell.row,cell.col});
        }
    }
    for(auto &&[row,col]:m_state.mergedCells)
    {
        auto cell = find(row,col);
        if(cell)
            emit mergeSig(cell->row,cell->col,cell->rowSpan,cell->colSpan);
    }

    emit dataChanged(index(0,0),index(rowCount()-1,columnCount()-1));
}

void mergeModel::clearTableMergeState()
{
    for(auto &&[row,col]:m_state.mergedCells)
    {
        auto cell = find(row,col);
        if(cell)
            emit mergeSig(cell->row,cell->col);
    }
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

    int gridSize = 6; // Define the size of the grid
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
    //use a deep copy, iterator maybe invalid
    m_db.commit();
    emit dataChanged(index(0,0),index(3,3));
    qDebug() << "Cells inserted into table successfully.";
}

//注意如果在操作返回值期间对容器进行修改，需要进行复制
Cell *mergeModel::find(int row, int col)
{
    auto it =std::find_if(m_state.cells.begin(),m_state.cells.end(),[row,col](const Cell&cell)
    {
        return cell.row == row && cell.col == col;
    });

    if(it != m_state.cells.end())
        return &(*it);
    else
        return nullptr;
}

void mergeModel::increaseCol(int col, int rowBegin, int totalRow)
{
    //从当前列最后一个递增起
    for(int curRow = totalRow -1 ; curRow>=rowBegin;)
    {
        auto cell = this->find(curRow,col);
        if(!cell)
        {
            curRow--;
            continue;
        }
        cell->row++;
        curRow--;

        qDebug() << "increase Cell:";
        print(*cell);
    }
}

void mergeModel::increaseRow(int row, int colBegin, int totalColumn)
{
    for(int curCol = totalColumn -1; curCol >= colBegin;)
    {
        auto cell = this->find(row,curCol);
        if(!cell)
        {
            curCol--;
            continue;
        }
        cell->col++;
        curCol--;
        qDebug().nospace() << "increase Cell:";
        print(*cell);
    }
}

void mergeModel::print(Cell cell)
{
    qDebug() << cell.row << cell.col << cell.rowSpan<<cell.colSpan <<cell.val;
}

void mergeModel::printTable()
{
    sortTable();
    for (const Cell &cell : m_state.cells) {
        qDebug().noquote() << "Cell at (" << cell.row << ", " << cell.col << "): "
                                     << "Value = " << cell.val << ", "
                                     << "RowSpan = " << cell.rowSpan << ", "
                                     << "ColSpan = " << cell.colSpan;
    }
    qDebug().noquote()<< QString("Table Contents total %1:").arg(m_state.cells.size());
}

void mergeModel::sortTable()
{
    std::sort(m_state.cells.begin(),m_state.cells.end(),[](const Cell &a, const Cell &b){
        if(a.row != b.row)
            return a.row < b.row;
        else
            return a.col < b.col;
    });
}

void mergeModel::appendAndIncreseRow(int row, int col, int totalCol, int rowSpan, int colSpan, const QString &val)
{
    Cell cell;
    cell.row = row;
    cell.col = col;
    cell.colSpan = colSpan;
    cell.rowSpan = rowSpan;
    cell.val = val;

    qDebug().nospace()<<"append new Cell:";
    print(cell);

    increaseRow(row,col,totalCol);
    m_state.cells.append(cell);
}

void mergeModel::appendAndIncreaseCol(int row, int col, int totalRow, int rowSpan, int colSpan, const QString &val)
{
    Cell cell;
    cell.row = row;
    cell.col = col;
    cell.colSpan = colSpan;
    cell.rowSpan = rowSpan;
    cell.val = val;

    qDebug().nospace()<<"append new Cell:";
    print(cell);

    increaseCol(col,row,totalRow);
    m_state.cells.append(cell);
}

Cell *mergeModel::findSpanOnCol(int row, int col)
{
    for(int curRow = 0;curRow < row;curRow++)
    {
        auto cell = find(curRow,col);
        if(cell && cell->rowSpan+cell->row > row)
            return cell;
    }

    return nullptr;
}

Cell *mergeModel::findSpanOnRow(int row, int col)
{
    for(int curCol=0;curCol < col;curCol++)
    {
        auto cell = find(row,curCol);
        if(cell && cell->colSpan+cell->col > col)
            return cell;
    }
    return nullptr;
}

void mergeModel::saveCurrentState()
{
    if(m_undoStack.size() > MAXSTACKSIZE){
        m_undoStack.pop_front();
    }
    m_undoStack.push_back(m_state);

    m_redoStack.clear();
    emit enableUndo(true);
    emit enableRedo(false);
}

void mergeModel::undo()
{
    if(m_undoStack.isEmpty())
    {
        qDebug() << "Noting to undo";
        emit enableUndo(false);
        return;
    }
    beginResetModel();
    clearTableMergeState();
    if(m_redoStack.size() >= MAXSTACKSIZE)
    {
        m_redoStack.pop_front();
    }
    m_redoStack.push_back(m_state);
    m_state = m_undoStack.takeLast();
    restoreTableMergeState();
    // emit dataChanged(index(0,0),index(rowCount()-1,columnCount()-1)) ;
    endResetModel();
    printTable();

    emit enableRedo(true);
}

void mergeModel::redo()
{
    if(m_redoStack.isEmpty())
    {
        qDebug() << "nothing to redo";
        emit enableRedo(false);
        return;
    }

    beginResetModel();
    clearTableMergeState();
    if(m_undoStack.size() >= MAXSTACKSIZE)
    {
        m_undoStack.pop_front();
    }
    m_undoStack.push_back(m_state);

    m_state = m_redoStack.takeLast();
    restoreTableMergeState();
    endResetModel();

    emit enableUndo(true);

    printTable();
    // emit dataChanged(index(0,0),index(rowCount()-1, columnCount()-1));
}

void mergeModel::removeRow_(int row)
{
    if(row < 0 || row >= rowCount())
        return;
    saveCurrentState();
    beginRemoveRows(QModelIndex(),row,row);
    for(int i = 0; i < m_state.cells.size(); ++i)
    {
        Cell &cell = m_state.cells[i];

        if(cell.row <= row && cell.row+cell.rowSpan > row)
        {
            if(cell.rowSpan > 1)
                cell.rowSpan--;
            else{
                m_state.cells.removeAt(i);
                i--;
            }
        }else if(cell.row > row){
            cell.row--;
        }

    }
    endRemoveRows();
    printTable();
}

void mergeModel::removeColumn_(int col)
{
    if(col < 0 || col >= columnCount())
        return;

    saveCurrentState();
    beginRemoveColumns(QModelIndex(),col,col);
    for(int i = 0; i < m_state.cells.size(); ++i)
    {
        auto &cell = m_state.cells[i];

        //include or single
        if(cell.col <= col && cell.col + cell.colSpan> col)
        {
            if(cell.colSpan > 1)
                cell.colSpan--;
            else{
                m_state.cells.removeAt(i);
                i--;
            }
        }else if(cell.col > col)
            cell.col--;
    }

    endRemoveColumns();

    printTable();
}

void mergeModel::insertRows_(int row, int count)
{
    if(row < 0)
        return;
    saveCurrentState();
    beginInsertRows(QModelIndex(),row,row+count-1);

    for(int i= 0; i < count; i++)
    {
        int columnCount = this->columnCount();
        int rowCount = this->rowCount();

        for(int col=0;col < columnCount;)
        {
            assert(col >= 0);
            Cell *spanCell = nullptr;
            spanCell = findSpanOnCol(row,col);

            if(spanCell)
            {
                spanCell->rowSpan++;
                //expand (col to col+spanCell->colSpan
                for(int spanCol = col;spanCol < col+spanCell->colSpan;spanCol++)
                    increaseCol(spanCol,row,rowCount);

                col+=spanCell->colSpan;
                continue;
            }

            //find the cell on the top of current Cell
            auto it = std::find_if(m_state.cells.begin(),m_state.cells.end(),[row,col](const Cell& cell)
            {
                return cell.row == row -1 && cell.col == col;
            });

            if(it != m_state.cells.end())
            {
                auto temp = *it;
                appendAndIncreaseCol(row,col,rowCount,1,it->colSpan);
                col+=temp.colSpan;
                assert(temp.colSpan >0);
            }else
            {
                appendAndIncreaseCol(row,col,rowCount);
                col++;
            }
        }
    }
    endInsertRows();
    printTable();
}

void mergeModel::insertRow_(int row)
{
    insertRows_(row,1);
}

void mergeModel::insertColumn_(int col)
{
    insertColumns_(col,1);
}

void mergeModel::insertColumns_(int col, int count)
{
    if(col < 0)
        return;

    saveCurrentState();
    beginInsertColumns(QModelIndex(), col,col+count-1);

    for(int i = 0 ; i < count; i++)
    {
        int rowCount = this->rowCount();
        int colCount = this->columnCount();

        for(int row = 0; row < rowCount;)
        {
            Cell *spanCell = nullptr;
            spanCell = findSpanOnRow(row,col);

            if(spanCell)
            {
                spanCell->colSpan++;
                for(int spanRow = row;spanRow < spanCell->rowSpan+row;spanRow++)
                    increaseRow(spanRow,col,colCount);
                row+=spanCell->rowSpan;
                continue;
            }

            auto it = std::find_if(m_state.cells.begin(),m_state.cells.end(),[row,col](const Cell& cell)
            {
                return cell.col == col-1 && cell.row == row;
            });

            if(it != m_state.cells.end())
            {
                auto temp = *it;
                appendAndIncreseRow(row,col,colCount,it->rowSpan,1);
                assert(temp.rowSpan>0);
                row+=temp.rowSpan;
            }else
            {
                appendAndIncreseRow(row,col,colCount);
                row++;
            }
        }
    }

    endInsertColumns();
    printTable();
}

void mergeModel::split(int splitRow, int splitCol)
{
    saveCurrentState();
    //use a deep copy, iterator maybe invalid
    Cell cell = *find(splitRow,splitCol);
    emit mergeSig(splitRow,splitCol);
    QPair<int,int> temp_p= {splitRow,splitCol};
    m_state.mergedCells.removeAll(temp_p);
    m_state.cells.removeAll(cell);

    qDebug() << "split row range: "<<cell.row << "to"<<cell.row+cell.rowSpan;
    qDebug() << "split col range: "<<cell.col<<"to"<<cell.col+cell.colSpan;

    for(int i = cell.row;i < cell.row+cell.rowSpan;i++)
    {
        for(int j =cell.col;j < cell.colSpan + cell.col;j++)
        {
            qDebug() << "pos: "<<i<<","<<j;
            Cell newCell;
            newCell.col = j;
            newCell.row = i;
            newCell.val = "Cell";
            m_state.cells.append(newCell);
            qDebug().nospace() << "append new cell";
            print(newCell);
        }
    }
    emit dataChanged(index(splitRow,splitCol),index(cell.rowSpan-1,cell.colSpan-1));

    printTable();
}

void mergeModel::merge(int top, int left, int width, int height)
{
    saveCurrentState();
    if(top <0 || left < 0 || width<=0 || height <=0)
    {
        qDebug() << "invalid parameter";
        return;
    }

    auto curCellPtr = find(top,left);
    if(!curCellPtr)
        return;
    auto curCell = *curCellPtr;

    for(int row = top; row < top + height;row++)
    {
        for(int col = left; col < left + width;)
        {
            auto removeCell = find(row,col);
            if(removeCell)
            {
                col += removeCell->colSpan;
                if(removeCell->colSpan >1 || removeCell->rowSpan > 1)
                {
                    emit mergeSig(removeCell->row,removeCell->col);
                    QPair<int,int> temp_p = {removeCell->row,removeCell->col};
                    m_state.mergedCells.removeAll(temp_p);
                }
                qDebug().nospace() << "remove: ";
                print(*removeCell);
                m_state.cells.removeOne(*removeCell);
            }else
                col++;
        }
    }

    curCell.colSpan = width;
    curCell.rowSpan = height;
    m_state.cells.append(curCell);
    m_state.mergedCells.append({curCell.row,curCell.col});
    emit mergeSig(curCell.row,curCell.col,curCell.rowSpan,curCell.colSpan);
    emit dataChanged(index(top,left),index(top+height-1,left+width-1),{Qt::DisplayRole});

    printTable();
}




