#ifndef SORTFILTERPROXYMODEL_H
#define SORTFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>
#include <QDate>

class SortFilterProxyModel : public QSortFilterProxyModel
    {
        Q_OBJECT
    public:
        explicit SortFilterProxyModel(QObject *parent = 0);

   public slots:

   protected:
        bool filterAcceptsRow( int source_row, const QModelIndex &source_parent) const;

    private:
    };

#endif // SORTFILTERPROXYMODEL_H
