#-------------------------------------------------
#
# Project created by QtCreator 2015-10-21T14:49:31
#
#-------------------------------------------------

QT              += core gui testlib
CONFIG          += c++11
CONFIG          += console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

TARGET = TeamPisces
TEMPLATE = app

DISTFILES += \
    TeamPisces.pro.user \

HEADERS += \
    gui/CustomSort.h \
    gui/ErrorEditDialog.h \
    gui/piechartwidget.h \
    gui/mainwindow.h \
    database/CSVReader.h \
    database/RecordsManager.h \
    database/QSortListIO.h\
    datamodel/TreeItem.h \
    datamodel/TreeModel.h \
    datamodel/PresentationTreeModel.h \
    datamodel/PublicationTreeModel.h \
    datamodel/GrantFundingTreeModel.h \
    datamodel/TeachingTreeModel.h \
    datamodel/mysortfilterproxymodel.h \
    gui/qcustomplot.h \
    tests/testqstring.h \
    tests/testcsvreader.h \
    tests/testtreeitem.h \
    tests/testqsortlist.h \
    tests/testcharts.h \
    datamodel/sortfilterproxymodel.h


SOURCES += \
    gui/CustomSort.cpp \
    gui/ErrorEditDialog.cpp \
    gui/piechartwidget.cpp \
    gui/main.cpp \
    gui/mainwindow.cpp \
    database/CSVReader.cpp \
    database/RecordsManager.cpp \
    database/QSortListIO.cpp \
    datamodel/TreeItem.cpp \
    datamodel/TreeModel.cpp \
    datamodel/PresentationTreeModel.cpp \
    datamodel/PublicationTreeModel.cpp \
    datamodel/GrantFundingTreeModel.cpp \
    datamodel/TeachingTreeModel.cpp \
    datamodel/mysortfilterproxymodel.cpp \
    gui/qcustomplot.cpp \
    tests/testqstring.cpp \
    tests/testcsvreader.cpp \
    tests/testtreeitem.cpp \
    tests/testqsortlist.cpp \
    tests/testcharts.cpp \
    datamodel/sortfilterproxymodel.cpp

FORMS += \
    gui/customsort.ui \
    gui/mainwindow.ui \
    gui/ErrorEditDialog.ui

RESOURCES += \
    images.qrc \
