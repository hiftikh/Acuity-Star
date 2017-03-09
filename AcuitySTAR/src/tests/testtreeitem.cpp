#include "testtreeitem.h"

TestTreeItem::TestTreeItem(QObject *parent) : QObject(parent)
{

}

//Tests interting items into a tree
void TestTreeItem::testInsert()
{
    QVariant item1(1);
    QVariant item2(2);
    QVariant item3(3);
    QVariant item4(4);

    QList<QVariant> list1;
    list1.append(item1);
    QList<QVariant> list2;
    list2.append(item2);
    QList<QVariant> list3;
    list3.append(item3);
    QList<QVariant> list4;
    list4.append(item4);

    TreeItem *null = nullptr;

    TreeItem *parentItem = new TreeItem(list1, null);
    TreeItem *child1 = new TreeItem(list2, parentItem);
    TreeItem *child2 = new TreeItem(list3, parentItem);
    TreeItem *child3 = new TreeItem(list4, parentItem);

    parentItem->appendChild(child1);
    parentItem->appendChild(child2);
    parentItem->appendChild(child3);

    QVERIFY(parentItem->childCount() == 3);
    parentItem->~TreeItem();
}

//Tests getting items parent
void TestTreeItem::testParent()
{
    QVariant item1(1);
    QVariant item2(2);
    QVariant item3(3);
    QVariant item4(4);

    QList<QVariant> list1;
    list1.append(item1);
    QList<QVariant> list2;
    list2.append(item2);
    QList<QVariant> list3;
    list3.append(item3);
    QList<QVariant> list4;
    list4.append(item4);

    TreeItem *null = nullptr;

    TreeItem *parentItem = new TreeItem(list1, null);
    TreeItem *child1 = new TreeItem(list2, parentItem);
    TreeItem *child2 = new TreeItem(list3, parentItem);
    TreeItem *child3 = new TreeItem(list4, parentItem);

    parentItem->appendChild(child1);
    parentItem->appendChild(child2);
    parentItem->appendChild(child3);

    QVERIFY(child1->parentItem() == parentItem);
    parentItem->~TreeItem();
}

//Tests getting a items child
void TestTreeItem::testChild()
{
    QVariant item1(1);
    QVariant item2(2);
    QVariant item3(3);
    QVariant item4(4);

    QList<QVariant> list1;
    list1.append(item1);
    QList<QVariant> list2;
    list2.append(item2);
    QList<QVariant> list3;
    list3.append(item3);
    QList<QVariant> list4;
    list4.append(item4);

    TreeItem *null = nullptr;

    TreeItem *parentItem = new TreeItem(list1, null);
    TreeItem *child1 = new TreeItem(list2, parentItem);
    TreeItem *child2 = new TreeItem(list3, parentItem);
    TreeItem *child3 = new TreeItem(list4, parentItem);

    parentItem->appendChild(child1);
    parentItem->appendChild(child2);
    parentItem->appendChild(child3);

    QVERIFY(parentItem->child(0) == child1);
    parentItem->~TreeItem();
}

void TestTreeItem::testRow()
{
    QVariant item1(1);
    QVariant item2(2);
    QVariant item3(3);
    QVariant item4(4);

    QList<QVariant> list1;
    list1.append(item1);
    QList<QVariant> list2;
    list2.append(item2);
    QList<QVariant> list3;
    list3.append(item3);
    QList<QVariant> list4;
    list4.append(item4);

    TreeItem *null = nullptr;

    TreeItem *parentItem = new TreeItem(list1, null);
    TreeItem *child1 = new TreeItem(list2, parentItem);
    TreeItem *child2 = new TreeItem(list3, parentItem);
    TreeItem *child3 = new TreeItem(list4, parentItem);

    parentItem->appendChild(child1);
    parentItem->appendChild(child2);
    parentItem->appendChild(child3);

    QVERIFY((child2->row() == 1) && (parentItem->row() == 0));
    parentItem->~TreeItem();
}
