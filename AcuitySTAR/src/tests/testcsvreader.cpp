#include "testcsvreader.h"

TestCSVReader::TestCSVReader(QObject *parent) : QObject(parent)
{

}

void TestCSVReader::testCSVReader_data()
{
    QTest::addColumn<QString>("pathName");

    //Test the new files with the added division and department fields.
    QTest::newRow("Grants_New") << "../../../../../AcuitySTAR/Project Information/Sample Data/Grants_expanded.csv";
    QTest::newRow("Teaching_New") << "../../../../../AcuitySTAR/Project Information/Sample Data/Program_Teaching_expanded.csv";
    QTest::newRow("Presentations_New") << "../../../../../AcuitySTAR/Project Information/Sample Data/Presentations_expanded.csv";
    QTest::newRow("Publications_New") << "../../../../../AcuitySTAR/Project Information/Sample Data/Publications_expanded.csv";

    //Test the original sample files
    QTest::newRow("GrantsClinicalFunding_sample") << "../../../../../AcuitySTAR/Project Information/Sample Data/GrantsClinicalFunding_sample.csv";
    QTest::newRow("Presentation_Sample") << "../../../../../AcuitySTAR/Project Information/Sample Data/Presentations_sample.csv";
    QTest::newRow("Publications_Sample") << "../../../../../AcuitySTAR/Project Information/Sample Data/Publications_sample.csv";
    QTest::newRow("Teaching_Sample") << "../../../../../AcuitySTAR/Project Information/Sample Data/Teaching_sample.csv";
}

void TestCSVReader::testCSVReader()
{
    QFETCH(QString, pathName);
    std::string utf8_text = pathName.toUtf8().constData();
    CSVReader reader(utf8_text);
    std::vector<std::string> header = reader.getHeaders();
    QVERIFY(header.size() != 0);
}



void TestCSVReader::testInvalidData()
{
    CSVReader reader("../../../../../AcuitySTAR/Project Information/Sample Data/invaliddata.txt");
    std::vector<std::string> header = reader.getHeaders();
    QVERIFY(header.size() == 0);
}

void TestCSVReader::testInvalidFilePath()
{
    CSVReader reader("thisfiledoesnotexist.cvs");
    std::vector<std::string> header = reader.getHeaders();
    QVERIFY(header.size() == 0);
}

void TestCSVReader::testDeptDivisionTeach()
{
   CSVReader reader("../../../../../AcuitySTAR/Project Information/Sample Data/Program_Teaching_expanded.csv");
   std::vector<std::string> header = reader.getHeaders();
   bool hasDep = false;
   bool hasDiv = false;
   for (int i = 0; i < (int)header.size(); i++) {
       if (header.at(i) == "Division") {
           hasDiv = true;
       }

       if (header.at(i) == "Department") {
           hasDep = true;
       }
   }
   QVERIFY(hasDep && hasDiv);
}

void TestCSVReader::testDeptDivisionPres()
{
    CSVReader reader("../../../../../AcuitySTAR/Project Information/Sample Data/Presentations_expanded.csv");
    std::vector<std::string> header = reader.getHeaders();
    bool hasDep = false;
    bool hasDiv = false;
    for (int i = 0;  i < (int)header.size(); i++) {
        if (header.at(i) == "Division") {
            hasDiv = true;
        }

        if (header.at(i) == "Department") {
            hasDep = true;
        }
    }
    QVERIFY(hasDep && hasDiv);
}

void TestCSVReader::testDeptDivisionPub()
{
    CSVReader reader("../../../../../AcuitySTAR/Project Information/Sample Data/Publications_expanded.csv");
    std::vector<std::string> header = reader.getHeaders();
    bool hasDep = false;
    bool hasDiv = false;
    for (int i = 0; i < (int)header.size(); i++) {
        if (header.at(i) == "Division") {
            hasDiv = true;
        }

        if (header.at(i) == "Department") {
            hasDep = true;
        }
    }
    QVERIFY(hasDep && hasDiv);
}

void TestCSVReader::testDeptDivisionGrant()
{
    CSVReader reader("../../../../../AcuitySTAR/Project Information/Sample Data/Grants_expanded.csv");
    std::vector<std::string> header = reader.getHeaders();
    bool hasDep = false;
    bool hasDiv = false;
    for (int i = 0; i < (int)header.size(); i++) {
        if (header.at(i) == "Division") {
            hasDiv = true;
        }

        if (header.at(i) == "Department") {
            hasDep = true;
        }
    }
    QVERIFY(hasDep && hasDiv);
}
