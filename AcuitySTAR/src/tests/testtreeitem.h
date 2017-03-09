#ifndef TESTTREEITEM_H
#define TESTTREEITEM_H

#include <QObject>
#include "../datamodel/TreeItem.h"
#include <QtTest>

class TestTreeItem : public QObject
{
    Q_OBJECT
public:
    explicit TestTreeItem(QObject *parent = 0);

signals:

private slots:
    void testInsert();
    void testParent();
    void testChild();
    void testRow();
};

#endif // TESTTREEITEM_H
