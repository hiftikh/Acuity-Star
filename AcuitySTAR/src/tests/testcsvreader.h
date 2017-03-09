#ifndef TESTCSVREADER_H
#define TESTCSVREADER_H

#include <string>
#include <vector>
#include <QObject>
#include <QtTest/QtTest>
#include <QtDebug>
#include <iostream>
#include <fstream>
#include <sstream>
#include "database/CSVReader.h"

class TestCSVReader : public QObject
{
    Q_OBJECT
public:
    explicit TestCSVReader(QObject *parent = 0);

signals:

private slots:
    void testCSVReader_data();
    void testCSVReader();
    void testInvalidData();
    void testInvalidFilePath();
    void testDeptDivisionTeach();
    void testDeptDivisionPres();
    void testDeptDivisionPub();
    void testDeptDivisionGrant();
};

#endif // TESTCSVREADER_H
