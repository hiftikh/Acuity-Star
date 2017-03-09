#ifndef TESTCHARTS_H
#define TESTCHARTS_H

#include <QObject>
#include <vector>
#include <QObject>
#include <QtTest/QtTest>
#include <QtDebug>

class TestCharts : public QObject
{
    Q_OBJECT
public:
    explicit TestCharts(QObject *parent = 0);

signals:

private slots:
    void testInvalidLineChart();
    void testInvalidBarChart();
    void testInvalidPieChart();
    void testInvalidStackedBarChart();
};

#endif // TESTCHARTS_H
