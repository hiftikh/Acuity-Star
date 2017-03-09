#ifndef TESTQSORTLIST_H
#define TESTQSORTLIST_H

#include <QObject>

class TestQSortList : public QObject
{
    Q_OBJECT
public:
    explicit TestQSortList(QObject *parent = 0);

signals:

private slots:
    void testSaveAndReadList();
};

#endif // TESTQSORTLIST_H
