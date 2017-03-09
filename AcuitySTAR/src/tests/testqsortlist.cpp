#include "testqsortlist.h"
#include "database/QSortListIO.h"

#include <QtTest>

TestQSortList::TestQSortList(QObject *parent) : QObject(parent)
{

}

void TestQSortList::testSaveAndReadList()
{
    // create a QSortListIO object with filename = test.dat
    QSortListIO saveTestSort("test.dat");

    // create a two QStringList
    QString str1 = "Tony";
    QString str2 = "Bob";
    QString str3 = "Jim";

    QStringList testList_1;
    testList_1.append(str1);
    testList_1.append(str2);
    testList_1.append(str3);

    QStringList testList_2;
    testList_2.append(str1);

    // create a QList of QStringLists
    QList<QStringList> testQList;
    testQList.append(testList_1);
    testQList.append(testList_2);

    // save the QList of QStringList
    saveTestSort.saveList(testQList);

    QList<QStringList> readList;
    readList = saveTestSort.readList();

    QVERIFY(readList == testQList);
}
