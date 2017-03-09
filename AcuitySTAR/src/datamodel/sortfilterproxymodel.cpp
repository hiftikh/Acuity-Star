#include "sortfilterproxymodel.h"
#include <qdebug.h>
#include<QStandardItemModel>

SortFilterProxyModel::SortFilterProxyModel(QObject *parent) : QSortFilterProxyModel(parent)
{

}


bool SortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
{

    //Otherwise check if it matches regex
    return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent) ;
}
