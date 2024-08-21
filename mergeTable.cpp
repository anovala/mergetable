#include "mergeTable.h"
#include "./ui_mergeTable.h"

mergeTable::mergeTable(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::mergeTable)
    , m_model(new mergeModel(this))
{
    ui->setupUi(this);
    ui->tableView->setModel(m_model);
    ui->tableView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->tableView,&QWidget::customContextMenuRequested,this,&mergeTable::showMenu);
    auto mergeAction = new QAction("merge",this);
    auto removeRowAction = new QAction("removeRow",this);
    auto insertRowAction = new QAction("insertRow",this);
    auto removeColAction = new QAction("removeColumn",this);
    auto insertColAction = new QAction("insertColumn",this);
    auto saveDbAction = new QAction("savetoDb",this);
    auto saveJsonAction = new QAction("savetoJson",this);

    auto splitAction = new QAction("split",this);
    menu.addActions({mergeAction,removeRowAction,insertRowAction,saveDbAction,
                     removeColAction,insertColAction,splitAction,saveJsonAction});

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
                m_model->printTable();
            }
        }
    });

    connect(m_model,&mergeModel::cancelMerge,this,[this](int row, int col){
        ui->tableView->setSpan(row,col,1,1);
    });

    connect(m_model,&mergeModel::mergeRequest,this,[this](int row, int col ,int rowSpan, int colSpan){
        ui->tableView->setSpan(row,col,rowSpan,colSpan);
    });

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
        ui->tableView->setSpan(top,left,height,width);
    });

    connect(removeColAction,&QAction::triggered,this,[this]{
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        for(auto &index: selectIndexes)
        {
            //remove selectCols
            auto col = index.column();
            m_model->removeColumn_(col);
        }
    });

    connect(insertColAction,&QAction::triggered,this,[this]{
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        int col = selectIndexes.first().column();
        for(auto &index: selectIndexes)
        {
            //insertCol before the first select column
            if(index.column() < col)
                col = index.column();
        }
        m_model->insertColumn_(col);
    });

    connect(removeRowAction,&QAction::triggered,this,[this]()
    {
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        for(auto &index: selectIndexes)
        {
            //remove selectRows
            auto row = index.row();
            m_model->removeRow_(row);
        }
    });
    connect(insertRowAction,&QAction::triggered,this,[this](){
        auto selectIndexes = ui->tableView->selectionModel()->selectedIndexes();
        int row = selectIndexes.first().row();
        for(auto &index: selectIndexes)
        {
            //insertRow before the first select row
            if(index.row() < row)
                row = index.row();
        }
        m_model->insertRow_(row);
    });

    // m_model->initTable("cellTable");
    // m_model->loadFromDb("cellTable");
    m_model->loadFromJson("data.json");
    m_model->expandAll();
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
