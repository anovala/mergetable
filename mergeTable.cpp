#include "mergeTable.h"
#include "./ui_mergeTable.h"
#include <QMenuBar>

mergeTable::mergeTable(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mergeTable)
    , m_model(new mergeModel(this))
    , m_delegate(new headerDelegate(this))
{
    ui->setupUi(this);
    ui->tableView->setModel(m_model);
    ui->tableView->setItemDelegate(m_delegate);
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);

    createConnection();
    m_model->initTable("cellTable");
    m_model->loadFromDb("cellTable");
    // m_model->loadFromJson("data.json");
    m_model->restoreTableMergeState(true);
}

mergeTable::~mergeTable()
{
    delete ui;
}

void mergeTable::showMenu(const QPoint &pos)
{
    menu.move(mapToParent(pos));
    menu.show();
}

void mergeTable::createConnection()
{
    connect(ui->tableView,&QWidget::customContextMenuRequested,this,&mergeTable::showMenu);
    auto mergeAction = new QAction("merge",this);
    auto removeRowAction = new QAction("removeRow",this);
    auto insertRowFrontAction = new QAction("insertRow_Front",this);
    auto insertRowBackAction = new QAction("insertRow_Back",this);
    auto removeColAction = new QAction("removeColumn",this);
    auto insertColFrontAction = new QAction("insertColumn_Front",this);
    auto insertColBackAction = new QAction("insertColumn_Back",this);
    auto saveDbAction = new QAction("savetoDb",this);
    auto saveJsonAction = new QAction("savetoJson",this);
    auto redoAction = new QAction("redo",this);
    auto undoAction = new QAction("undo",this);
    auto firstRow = new QAction("First Row");
    auto firstCol = new QAction("First Column");
    firstRow->setCheckable(true);
    firstCol->setCheckable(true);

    redoAction->setEnabled(false);
    undoAction->setEnabled(false);
    redoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    undoAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Z));
    ui->tableView->addActions({redoAction,undoAction});

    auto splitAction = new QAction("split",this);
    menu.addActions({mergeAction,removeRowAction,insertRowFrontAction,
                    insertRowBackAction,removeColAction,insertColFrontAction,
                     insertColBackAction,splitAction});
    menu.addSeparator();
    menu.addActions({redoAction,undoAction,saveDbAction,saveJsonAction});
    menu.addSeparator();
    menu.addActions({firstRow,firstCol});

    connect(firstRow,&QAction::triggered,this,[this](bool b){
        if(b){
            m_delegate->setColoredRow(0);
        }else
        {
            m_delegate->removeColoredRow(0);
        }
        m_model->setFirstRowHeader(b);
    });

    connect(firstCol, &QAction::triggered,this,[this](bool b){
        if(b){
            m_delegate->setColoredCol(0);
        }else{
            m_delegate->removeColoredCol(0);
        }
        m_model->setFirstColHeader(b);
    });

    connect(m_model,&mergeModel::enableRedo,this,[redoAction](bool b){
        redoAction->setEnabled(b);
    });

    connect(m_model,&mergeModel::enableUndo,this,[undoAction](bool b){
        undoAction->setEnabled(b);
    });

    connect(redoAction, &QAction::triggered,m_model,&mergeModel::redo);
    connect(undoAction,&QAction::triggered,m_model,&mergeModel::undo);

    connect(saveJsonAction,&QAction::triggered,this,[this]{
        m_model->savetoJson("data.json");
    });

    connect(splitAction,&QAction::triggered,this,[this](){
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        for(auto &index : selectIndexes)
        {
            auto cell = m_model->find(index.row(),index.column());
            if(cell && (cell->colSpan >1 || cell->rowSpan>1))
            {
                m_model->split(cell->row,cell->col);
            }
        }
    });

    connect(m_model,&mergeModel::mergeSig,this,[this](int row, int col, int rowSpan, int colSpan){
        ui->tableView->setSpan(row,col,rowSpan,colSpan);
    });

    // connect(m_model,&mergeModel::mergeRequest,this,[this](int row, int col ,int rowSpan, int colSpan){
    //     ui->tableView->setSpan(row,col,rowSpan,colSpan);
    // });

    connect(saveDbAction,&QAction::triggered,this,[this](){
        m_model->savetoDb("cellTable");
    });

    connect(mergeAction,&QAction::triggered,this,[this]
    {
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        int top;
        int left;
        int width = 1;
        int height = 1;
        auto topLeftIndex = selectIndexes.first();
        top = topLeftIndex.row();
        left = topLeftIndex.column();

        for(auto &index: selectIndexes)
        {
            if(index.row() < top)
                top = index.row();
            if(index.column() < left)
                left = index.column();
            int tempWidth = index.column() - left+1;
            int tempHeight = index.row() - top+1;
            if(tempWidth > width)
                width = tempWidth;
            if(tempHeight > height)
                height = tempHeight;
        }

        qDebug () << top<<left<<width<<height;
        m_model->merge(top,left,width,height);
        // ui->tableView->setSpan(top,left,height,width);
    });

    connect(removeColAction,&QAction::triggered,this,[this]{
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        QSet<int> temp;
        for(auto &index: selectIndexes)
        {
            //remove selectCols
            auto col = index.column();
            temp.insert(col);
        }

        for(auto col:temp)
        {
            m_model->removeColumn_(col);
        }
    });

    connect(insertColFrontAction,&QAction::triggered,this,[this]{
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        QSet<int> temp;
        for(auto &index: selectIndexes)
        {
            temp.insert(index.column());
        }

        auto minimum = std::min_element(temp.begin(),temp.end());
        if(minimum != temp.end())
        {
            m_model->insertColumns_(*minimum,temp.size());
        }
    });

    connect(insertColBackAction,&QAction::triggered,this,[this]{
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        QSet<int> temp;
        for(auto &index: selectIndexes)
        {
            temp.insert(index.column());
        }

        auto maximum = std::max_element(temp.begin(),temp.end());
        if(maximum != temp.end())
        {
            m_model->insertColumns_(*maximum+1,temp.size());
        }
    });

    connect(removeRowAction,&QAction::triggered,this,[this]()
    {
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        QSet<int> temp;
        for(auto &index: selectIndexes)
        {
            //remove selectRows
            auto row = index.row();
            temp.insert(row);
        }

        for(auto row:temp)
        {
            m_model->removeRow_(row);
        }
    });

    connect(insertRowFrontAction,&QAction::triggered,this,[this](){
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        QSet<int> temp;
        for(auto &index: selectIndexes)
        {
            temp.insert(index.row());
        }
        auto minimum = std::min_element(temp.begin(),temp.end());
        if(minimum != temp.end())
        {
            m_model->insertRows_(*minimum,temp.size());
        }
    });

    connect(insertRowBackAction,&QAction::triggered,this,[this](){
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        QSet<int> temp;
        for(auto &index: selectIndexes)
        {
            temp.insert(index.row());
        }
        auto maxmimum = std::max_element(temp.begin(),temp.end());
        if(maxmimum != temp.end())
        {
            m_model->insertRows_(*maxmimum+1,temp.size());
        }
    });

}

