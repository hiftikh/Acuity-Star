#ifndef TESTQSTRING_H
#define TESTQSTRING_H

#include <QObject>
#include <QtTest/QtTest>


class TestQString : public QObject
{
    Q_OBJECT
public:
    explicit TestQString(QObject *parent = 0);

signals:

private slots:
    void toUpper();
};

#endif // TESTQSTRING_H
