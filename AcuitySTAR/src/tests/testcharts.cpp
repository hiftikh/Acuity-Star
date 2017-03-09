#include "testcharts.h"
#include "gui/mainwindow.h"

TestCharts::TestCharts(QObject *parent) : QObject(parent)
{

}

void TestCharts::testInvalidLineChart()
{
    std::vector<std::pair <std::string, double>> lineChartList;
    bool flag = MainWindow::setupLineChart(nullptr,lineChartList);
    QVERIFY(flag == false);
}

void TestCharts::testInvalidBarChart()
{
    std::vector<std::pair <std::string, double>> lineChartList;
    bool flag = MainWindow::setupBarChart(nullptr,lineChartList);
    QVERIFY(flag == false);
}

void TestCharts::testInvalidPieChart()
{
    std::vector<std::pair <std::string, double>> lineChartList;
    bool flag = MainWindow::setupPieChart(nullptr,nullptr, lineChartList);
    QVERIFY(flag == false);
}

void TestCharts::testInvalidStackedBarChart()
{
    std::vector<std::pair <std::string, double>> stackedBarChartList;
    bool flag = MainWindow::setupPieChart(nullptr,nullptr, stackedBarChartList);
    QVERIFY(flag == false);
}

