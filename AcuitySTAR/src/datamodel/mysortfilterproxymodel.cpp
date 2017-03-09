#include "mysortfilterproxymodel.h"

#include <QModelIndex>
#include <QSortFilterProxyModel>

MySortFilterProxyModel::MySortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

void MySortFilterProxyModel::setTheRows(QList<int> tmpList)
{
    theRows.clear();
    for(int i = 0; i < tmpList.count(); i++)
    {
        theRows.append(tmpList.at(i));
    }
}

bool MySortFilterProxyModel::filterAcceptsRow(int sourceRow,
        const QModelIndex &sourceParent) const
{
    if (theRows.count() == 0) {
        return true;
    }

    if (!theRows.contains(sourceRow)) {
        return false;
    }
    return true;
}
