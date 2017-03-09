#include <iostream>
#include <stdio.h>
#include <QApplication>
#include <QTreeView>
#include <QtTest/QtTest>

#include "gui/mainwindow.h"
#include "tests/testcsvreader.h"
#include "tests/testqstring.h"
#include "tests/testtreeitem.h"
#include "tests/testqsortlist.h"
#include "tests/testcharts.h"

using namespace std;

int main(int argc, char *argv[]) {
    int test = 0;   /* 0: test off 1: test on*/

    if (test) {
        TestTreeItem testTree;
        TestCSVReader testCSVReader;
        TestQSortList testQSortList;
        TestCharts testCharts;

        std::cout << "\n\n";
        QTest::qExec(&testCSVReader, argc, argv);
        std::cout << "\n\n";
        QTest::qExec(&testTree, argc, argv);
        std::cout << "\n\n";
        std::cout << "\n\n";
        QTest::qExec(&testQSortList, argc, argv);
        std::cout << "\n\n";
        QTest::qExec(&testCharts, argc, argv);
    }

    QApplication app(argc, argv);
    MainWindow w;
    w.show();
    //w.showMaximized();

    return app.exec();
}
