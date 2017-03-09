#include <QDate>
#include <QDebug>
#include <QDirModel>
#include <QFileDialog>
#include <QHeaderView>
#include <QMessageBox>
#include <QPainter>
#include <QPdfWriter>
#include <QPrintDialog>
#include <QPrinter>
#include <QString>
#include <QDebug>

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "CustomSort.h"
#include "ErrorEditDialog.h"

#include "database/CSVReader.h"
#include "database/QSortListIO.h"
#include "database/RecordsManager.h"
#include "datamodel/TreeModel.h"
#include "datamodel/GrantFundingTreeModel.h"
#include "datamodel/PresentationTreeModel.h"
#include "datamodel/PublicationTreeModel.h"
#include "datamodel/TeachingTreeModel.h"
#include "datamodel/mysortfilterproxymodel.h"
#include "datamodel/sortfilterproxymodel.h"

#define FUNDORDER_SAVE  "fundsortorder.dat"
#define PRESORDER_SAVE  "pressortorder.dat"
#define PUBORDER_SAVE   "pubsortorder.dat"
#define TEACHORDER_SAVE "teachsortorder.dat"

std::vector<std::string> MainWindow::GRANTS_MANFIELDS = {"Member Name", "Funding Type", "Status", "Peer Reviewed?", "Role", "Title", "Start Date", "Division"};
std::vector<std::string> MainWindow::PRES_MANFIELDS = {"Member Name", "Date", "Type", "Role", "Title", "Division"};
std::vector<std::string> MainWindow::PUBS_MANFIELDS = {"Member Name", "Type", "Status Date", "Role", "Title", "Division"};
std::vector<std::string> MainWindow::TEACH_MANFIELDS = {"Member Name", "Start Date", "Program", "Division"};

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow),
    fundTree(NULL), presTree(NULL), pubTree(NULL), teachTree(NULL),
    funddb(NULL), presdb(NULL), pubdb(NULL), teachdb(NULL) {

    refreshOnUserSelect = false;

    // draw GUI elements
    ui->setupUi(this);

    this->showMaximized();

    // set up the logo
    QPixmap logo(":/logo.png");
    ui->logo_label->setPixmap(logo);

    // set up application icon
    QIcon icon(":/icon32.ico");
    MainWindow::setWindowIcon(icon);

    // set the date format
    QString dFormat("yyyy");
    ui->FromDate->setDisplayFormat(dFormat);
    ui->ToDate->setDisplayFormat(dFormat);

    // set start year to be 1950
    QDate startDate(1950, 1, 1);

    // set ranges for spinbox
    ui->FromDate->setDateRange(startDate, QDate::currentDate());
    ui->ToDate->setDateRange(startDate, QDate::currentDate());

    // set default dates (1950->current)
    ui->FromDate->setDate(startDate);
    ui->ToDate->setDate(QDate::currentDate());

    // set some member variables to the current date values
    yearStart = 1950;
    yearEnd = QDate::currentDate().year();

    //default print is disabled
    ui->teachPrintButton->setEnabled(false);
    ui->fundPrintButton->setEnabled(false);
    ui->pubPrintButton->setEnabled(false);
    ui->presPrintButton->setEnabled(false);

    ui->teachExportButton->setEnabled(false);
    ui->fundExportButton->setEnabled(false);
    ui->pubExportButton->setEnabled(false);
    ui->presExportButton->setEnabled(false);



    //Setup printer
    printer = new QPrinter();

    dateChanged = {false, false, false, false};
    QApplication::setStyle(QStyleFactory::create("Fusion"));
    setAcceptDrops(true);

    QFile file("state.dat");
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    int currTab = 0;
    QString fileName;

    in >> currTab;
    in >> fileName;

    if (fileName != "") {

        QMessageBox prompt;
        QString mainText = "Would you like to reopen the last file you were working on?";
        prompt.setText(mainText);
        prompt.setInformativeText(fileName);
        prompt.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        prompt.setDefaultButton(QMessageBox::Yes);
        prompt.setWindowIcon(QIcon(":/icon32.ico"));
        int ret = prompt.exec();

        if (ret == QMessageBox::Yes) {
            ui->categoryTab->setCurrentIndex(currTab);
            if (currTab == 0) {
                load_teach(fileName);
            } else if (currTab == 1) {
                load_pub(fileName);
            } else if (currTab == 2) {
                load_pres(fileName);
            } else {
                load_fund(fileName);
            }
        } else {

        }
    }
}

MainWindow::~MainWindow() {
    delete ui;

    delete fundTree;
    delete presTree;
    delete pubTree;
    delete teachTree;

    delete funddb;
    delete presdb;
    delete pubdb;
    delete teachdb;
    delete printer;
}

void MainWindow::on_actionLoad_file_triggered() {
    QStringList filePaths = QFileDialog::getOpenFileNames(this,
                                                          "Select one or more files to load",
                                                          QDir::currentPath(),
                                                          tr("CSV (*.csv);; All files (*.*)"));
    if (filePaths.size() > 0) {
        const int NUM_TABS = 4;
        bool all_loaded[NUM_TABS] = {false, false, false, false};
        int sum = std::accumulate(all_loaded, all_loaded + NUM_TABS, 0);
        QStringList::Iterator it = filePaths.begin();
        while (sum != NUM_TABS && it != filePaths.end()) {
            QString path = it[0];
            //note short-circuit eval
            if (!all_loaded[FUNDING] && load_fund(path, true)) {
                all_loaded[FUNDING] = true;
            } else if (!all_loaded[TEACH] && load_teach(path, true)) {
                all_loaded[TEACH] = true;
            } else if (!all_loaded[PUBLICATIONS] && load_pub(path, true)) {
                all_loaded[PUBLICATIONS] = true;
            } else if (!all_loaded[PRESENTATIONS] && load_pres(path, true)) {
                all_loaded[PRESENTATIONS] = true;
            }
            sum = std::accumulate(all_loaded, all_loaded + NUM_TABS, 0);
            ++it;
        }
    }
}


/*
 * When the load file button is clicked a file open dialog box
 * is opened.  If a file name is successfully returned, makeTree()
 * is called.
 */
QString MainWindow::load_file() {
    QString filePath = QFileDialog::getOpenFileName(this, "Open File", QDir::currentPath(),
                                                    tr("CSV (*.csv);; All files (*.*)"));

    if (!filePath.isEmpty()) {
        return filePath;
    } else {
        return "";
    }
}

void MainWindow::refresh(int tabIndex) {
    // if we've loaded in a file, update that data
    switch (tabIndex) {
    case FUNDING:
        if (!fundPath.isEmpty()) {
            makeTree(FUNDING);
            ui->fund_graph_stackedWidget->hide();
            ui->fundGraphTitle->clear();
        }
        break;

    case PRESENTATIONS:
        if (!presPath.isEmpty()) {
            makeTree(PRESENTATIONS);
            ui->pres_graph_stackedWidget->hide();
            ui->presGraphTitle->clear();
        }
        break;
    case PUBLICATIONS:
        if (!pubPath.isEmpty()) {
            makeTree(PUBLICATIONS);
            ui->pub_graph_stackedWidget->hide();
            ui->pubGraphTitle->clear();
        }
        break;

    case TEACH:
        if (!teachPath.isEmpty()) {
            makeTree(TEACH);
            ui->teach_graph_stackedWidget->hide();
            ui->teachGraphTitle->clear();
        }
        break;
    }
}

int MainWindow::checkFile(int index, QString filePath) {
    CSVReader reader;
    std::vector<std::string> header;
    std::string searchstring;

    int sortHeaderIndex = 2;

    switch (index) {
    case TEACH:
        // read if first or if a new file is loaded
        if (teachPath.isEmpty() || (!teachPath.isEmpty() && filePath.compare(teachPath))) {
            // create a new reader to read in the file
            reader = CSVReader(filePath.toStdString());
            header = reader.getHeaders();

            // create a new manager for the data
            delete teachdb;
            teachdb = new RecordsManager(&header);

            // check for right file type by searching for unique header
            searchstring = "Program";
            if (std::find(header.begin(), header.end(), searchstring) != header.end()) {
                // load in data into the manager, with the date as the key
                sortHeaderIndex = teachdb->getHeaderIndex("Start Date");
                teachData = reader.getData();
                std::vector<std::vector<std::string>*> f_errs;
                unsigned int j;
                for (int i = 0; i < (int) teachData.size(); i++) {

                    if (teachData[i].size() != header.size()) {
                        teachData.erase(teachData.begin() + i);
                    } else {

                    for (j = 0; j < TEACH_MANFIELDS.size(); j++) {
                        int index = teachdb->getHeaderIndex(TEACH_MANFIELDS[j]);
                        if (teachData[i][index].compare("") == 0) {
                            f_errs.push_back(&teachData[i]);
                            break;
                        }
                    }



                    // if all mandatory fields are okay
                    if (j == TEACH_MANFIELDS.size()) {
                        // date interpretation
                        int yrIndex = teachdb->getHeaderIndex("Start Date");
                        int year;
                        sscanf(teachData[i][yrIndex].c_str(), "%4d", &year);
                        teachData[i][yrIndex] = std::to_string(year);

                        teachdb->addRecord(reader.parseDateString(teachData[i][sortHeaderIndex]), &teachData[i]);
                    }

                    }
                }

                if (f_errs.size() > 0) {
                    if(handle_field_errors(f_errs, header, TEACH_MANFIELDS)) {
                        for (unsigned int i = 0; i < f_errs.size(); i++) {
                            teachdb->addRecord(reader.parseDateString((*(f_errs[i]))[sortHeaderIndex]), f_errs[i]);
                        }
                    }
                }
            } else {
                return EXIT_FAILURE;
            }
        } else {
            return EXIT_SUCCESS;
        }
        break;

    case PUBLICATIONS:
        // read if first or if a new file is loaded
        if (presPath.isEmpty() || (!presPath.isEmpty() && filePath.compare(presPath))) {
            // create a new reader to read in the file
            reader = CSVReader(filePath.toStdString());
            header = reader.getHeaders();

            // create a new manager for the data
            delete pubdb;
            pubdb = new RecordsManager(&header);

            // check for right file type by searching for unique header
            searchstring = "Publication Status";
            if (std::find(header.begin(), header.end(), searchstring) != header.end()) {
                // load in data into the manager, with the date as the key
                sortHeaderIndex = pubdb->getHeaderIndex("Status Date");
                pubData = reader.getData();
                std::vector<std::vector<std::string>*> f_errs;
                unsigned int j;
                for (int i = 0; i < (int) pubData.size(); i++) {

                    if (pubData[i].size() != header.size()) {
                        pubData.erase(pubData.begin() + i);
                    } else {

                        for (j = 0; j < PUBS_MANFIELDS.size(); j++) {
                            int index = pubdb->getHeaderIndex(PUBS_MANFIELDS[j]);
                            if (pubData[i][index].compare("") == 0) {
                                f_errs.push_back(&pubData[i]);
                                break;
                            }
                        }


                    // if all mandatory fields are okay
                    if (j == PUBS_MANFIELDS.size()) {
                        // date interpretation
                        int yrIndex = pubdb->getHeaderIndex("Status Date");
                        int year;
                        sscanf(pubData[i][yrIndex].c_str(), "%4d", &year);
                        pubData[i][yrIndex] = std::to_string(year);

                        pubdb->addRecord(reader.parseDateString(pubData[i][sortHeaderIndex]), &pubData[i]);
                    }
                    }
                }

                if (f_errs.size() > 0) {
                    if(handle_field_errors(f_errs, header, PUBS_MANFIELDS)) {
                        for (unsigned int i = 0; i < f_errs.size(); i++) {
                            pubdb->addRecord(reader.parseDateString((*(f_errs[i]))[sortHeaderIndex]), f_errs[i]);
                        }
                    }
                }
            } else {
                return EXIT_FAILURE;
            }
        } else {
            return EXIT_SUCCESS;
        }
        break;

    case PRESENTATIONS:
        // read if first or if a new file is loaded
        if (presPath.isEmpty() || (!presPath.isEmpty() && filePath.compare(presPath))) {
            // create a new reader to read in the file
            reader = CSVReader(filePath.toStdString());
            header = reader.getHeaders();

            // create a new manager for the data
            delete presdb;
            presdb = new RecordsManager(&header);

            // check for right file type by searching for unique header
            searchstring = "Activity Type";
            if (std::find(header.begin(), header.end(), searchstring) != header.end()) {
                // load in data into the manager, with the date as the key
                sortHeaderIndex = presdb->getHeaderIndex("Date");
                presData = reader.getData();
                std::vector<std::vector<std::string>*> f_errs;
                unsigned int j = 0;
                for (int i = 0; i < (int) presData.size(); i++) {

                    if (presData[i].size() != header.size()) {
                        presData.erase(presData.begin() + i);
                    } else {

                        for (j = 0; j < PRES_MANFIELDS.size(); j++) {

                            int index = presdb->getHeaderIndex(PRES_MANFIELDS[j]);
                            if (presData[i][index].compare("") == 0) {
                                f_errs.push_back(&presData[i]);
                                break;
                            }
                        }


                    // if all mandatory fields are okay
                    if (j == PRES_MANFIELDS.size()) {
                        // date interpretation
                        int yrIndex = presdb->getHeaderIndex("Date");
                        int year;
                        sscanf(presData[i][yrIndex].c_str(), "%4d", &year);
                        presData[i][yrIndex] = std::to_string(year);

                        presdb->addRecord(reader.parseDateString(presData[i][sortHeaderIndex]), &presData[i]);
                    }
                    }
                }

                if (f_errs.size() > 0) {
                    if(handle_field_errors(f_errs, header, PRES_MANFIELDS)) {
                        for (unsigned int i = 0; i < f_errs.size(); i++) {
                            presdb->addRecord(reader.parseDateString((*(f_errs[i]))[sortHeaderIndex]), f_errs[i]);
                        }
                    }
                }
            } else {
                return EXIT_FAILURE;
            }
        } else {
            return EXIT_SUCCESS;
        }
        break;

    case FUNDING:
        // read if first or if a new file is loaded
        if (fundPath.isEmpty() || (!fundPath.isEmpty() && filePath.compare(fundPath))) {
            // create a new reader to read in the file
            reader = CSVReader(filePath.toStdString());
            header = reader.getHeaders();

            // create a new manager for the data
            delete funddb;
            funddb = new RecordsManager(&header);

            // check for right file type by searching for unique header
            searchstring = "Funding Type";

            if (std::find(header.begin(), header.end(), searchstring) != header.end()) {
                // load in data into the manager, with the date as the key
                sortHeaderIndex = funddb->getHeaderIndex("Start Date");
                fundData = reader.getData();
                std::vector<std::vector<std::string>*> f_errs;
                unsigned int j;
                for (int i = 0; i < (int) fundData.size(); i++) {

                    if (fundData[i].size() != header.size()) {
                        fundData.erase(fundData.begin() + i);
                    } else {

                        for (j = 0; j < GRANTS_MANFIELDS.size(); j++) {
                            int index = funddb->getHeaderIndex(GRANTS_MANFIELDS[j]);
                            if (fundData[i][index].compare("") == 0) {
                                f_errs.push_back(&fundData[i]);
                                break;
                            }
                        }



                        // if all mandatory fields are okay
                        if (j == GRANTS_MANFIELDS.size()) {
                            // date interpretation
                            int yrIndex = funddb->getHeaderIndex("Start Date");
                            int year;
                            sscanf(fundData[i][yrIndex].c_str(), "%4d", &year);
                            fundData[i][yrIndex] = std::to_string(year);

                            // boolean interpretation
                            int prIndex = funddb->getHeaderIndex("Peer Reviewed?");
                            if (fundData[i][prIndex].compare("True") == 0) {
                                fundData[i][prIndex] = "Peer Reviewed";
                            } else {
                                fundData[i][prIndex] = "Not Peer Reviewed";
                            }
                            funddb->addRecord(reader.parseDateString(fundData[i][sortHeaderIndex]), &fundData[i]);
                        }
                    }
                }

                if (f_errs.size() > 0) {
                    if(handle_field_errors(f_errs, header, GRANTS_MANFIELDS)) {
                        for (unsigned int i = 0; i < f_errs.size(); i++) {
                            funddb->addRecord(reader.parseDateString((*(f_errs[i]))[sortHeaderIndex]), f_errs[i]);
                        }
                    }
                }
            } else {
                return EXIT_FAILURE;
            }
        } else {
            return EXIT_SUCCESS;
        }
        break;
    }
    return EXIT_SUCCESS;
}

void MainWindow::createDefaultSortOrder(int tabIndex) {
    QStringList defaultOrder;
    defaultOrder << "Default";

    switch (tabIndex) {
    case FUNDING:
        // specify default sort order
        defaultOrder << "Member Name" << "Funding Type" << "Peer Reviewed?" << "Status" << "Role" << "Title";

        // add default list to member variable
        allFundOrders << defaultOrder;

    {
        // save the default for the user
        QSortListIO saveFundSort(FUNDORDER_SAVE);
        saveFundSort.saveList(allFundOrders);
    }

        break;
    case PRESENTATIONS:
        // specify default sort order
        defaultOrder << "Member Name" << "Type" << "Role" << "Title";

        // add default list to member variable
        allPresOrders << defaultOrder;

    {
        // save the default for the user
        QSortListIO savePresSort(PRESORDER_SAVE);
        savePresSort.saveList(allPresOrders);
    }

        break;
    case PUBLICATIONS:
        // specify default sort order
        defaultOrder << "Member Name" << "Type" << "Role" << "Title";

        // add default list to member variable
        allPubOrders << defaultOrder;

    {
        // save the default for the user
        QSortListIO savePubSort(PUBORDER_SAVE);
        savePubSort.saveList(allPubOrders);
    }

        break;
    case TEACH:
        // specify default sort order
        defaultOrder << "Member Name" << "Program";

        // add default list to member variable
        allTeachOrders << defaultOrder;

    {
        // save the default for the user
        QSortListIO saveTeachSort(FUNDORDER_SAVE);
        saveTeachSort.saveList(allFundOrders);
    }

        break;
    }
}

/*
 * err: vector of pointers to record entries.
 * headers: vector of strings containing the db headers
 * mandatory: vector of strings containing the mandatory fields in the db
 *
 * Returns true when the passed err vector contains edited fields.
 * Returns false when no fields have been edited.
 */
bool MainWindow::handle_field_errors(std::vector<std::vector<std::string>*>& err,
                                     std::vector<std::string>& headers,
                                     std::vector<std::string>& mandatory) {
    //Since CSVReader alldata contains completely empty records
    //remove them first.
    std::vector<std::vector<std::string>*>::iterator it;
    for (it = err.begin(); it != err.end(); it++) {
        bool allEmpty = true;
        for (int col = 0; col < (int) (*it)->size(); col++) {
            if ((*it)->at(col).compare("") != 0) {
                allEmpty = false;
            }
        }
        if (allEmpty) {
            it = err.erase(it);
            it--;
        }
    }
    //Return false; there are no errors to correct
    if (err.size() == 0) {
        return false;
    }
    QMessageBox prompt;
    QString mainText = "File contains ";
    mainText.append(QString::number(err.size()));
    mainText.append(" records with missing mandatory fields.");
    prompt.setText(mainText);
    prompt.setInformativeText("Do you want to edit these entries or discard?");
    prompt.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    prompt.setDefaultButton(QMessageBox::Yes);
    prompt.setButtonText(QMessageBox::Yes, "Edit");
    prompt.setButtonText(QMessageBox::No, "Discard");
    prompt.setWindowIcon(QIcon(":/icon32.ico"));
    int ret = prompt.exec();

    switch (ret) {
    case QMessageBox::Yes: {
        ErrorEditDialog diag(this, err, headers, mandatory);
        if(diag.exec()) {
            return true;
        }
        return false;
    }

    case QMessageBox::No:
    default:
        return false;
    }
}

/*
 * Builds a TreeModel for the ui's embedded treeview.
 */
void MainWindow::makeTree(int tabIndex) {

    QString filterString = "";

    switch (tabIndex) {
    case TEACH:
        // set up some member variables to point to the current data in use
        currentdb = teachdb;
        delete teachTree;
        teachTree = new TeachingTreeModel(currentdb);
        currentTree = teachTree;

        // get some UI elements so we can put the tree and graphs into them
        currentView = ui->teachTreeView;

        // analyze the data into a tree
        currentTree->setupModel(yearStart, yearEnd, teachSortOrder, getFilterStartChar(TEACH), getFilterEndChar(TEACH));

        ui->teach_pie_button->toggle();

        filterString = teachFilter;

        break;

    case PUBLICATIONS:
        // set up some member variables to point to the current data in use
        currentdb = pubdb;
        delete pubTree;
        pubTree = new PublicationTreeModel(currentdb);
        currentTree = pubTree;

        // get some UI elements so we can put the tree and graphs into them
        currentView = ui->pubTreeView;

        // analyze the data into a tree
        currentTree->setupModel(yearStart, yearEnd, pubSortOrder, getFilterStartChar(PUBLICATIONS), getFilterEndChar(PUBLICATIONS));

        ui->pub_pie_button->toggle();

        filterString = pubFilter;

        break;
    case PRESENTATIONS:
        // set up some member variables to point to the current data in use
        currentdb = presdb;
        delete presTree;

        presTree = new PresentationTreeModel(currentdb);
        currentTree = presTree;

        // get some UI elements so we can put the tree and graphs into them
        currentView = ui->presTreeView;

        // analyze the data into a tree
        currentTree->setupModel(yearStart, yearEnd, presSortOrder, getFilterStartChar(PRESENTATIONS), getFilterEndChar(PRESENTATIONS));

        ui->pres_pie_button->toggle();

        filterString = presFilter;

        break;

    case FUNDING:
        // set up some member variables to point to the current data in use
        currentdb = funddb;
        delete fundTree;
        fundTree = new GrantFundingTreeModel(currentdb);
        currentTree = fundTree;

        // get some UI elements so we can put the tree and graphs into them
        currentView = ui->fundTreeView;

        // analyze the data into a tree
        currentTree->setupModel(yearStart, yearEnd, fundSortOrder, getFilterStartChar(FUNDING), getFilterEndChar(FUNDING));

        ui->fund_pie_button->toggle();

        filterString = grantFilter;

        break;
    }

    if(!refreshOnUserSelect) {
        QSortFilterProxyModel *filterModel = new QSortFilterProxyModel(this);
        filterModel->setSourceModel(currentTree);
        filterModel->setFilterRegExp(QRegExp(filterString, Qt::CaseInsensitive, QRegExp::FixedString));
        filterModel->setFilterKeyColumn(0);
        currentView->setModel(filterModel);
    }
    else {
        filterString = "";
        MySortFilterProxyModel *filterModel = new MySortFilterProxyModel();
        filterModel->setTheRows(userSelectedRows);
        filterModel->setSourceModel(currentTree);
        filterModel->setFilterRegExp(QRegExp(filterString, Qt::CaseInsensitive, QRegExp::FixedString));

//    SortFilterProxyModel *filterModel = new SortFilterProxyModel(this);
//    filterModel->setSourceModel(currentTree);
//    filterModel->setFilterRegExp(QRegExp(filterString, Qt::CaseInsensitive,
//
        filterModel->setFilterKeyColumn(0);
        currentView->setModel(filterModel);
    }

    // set resize property to stretch
    currentView->header()->resizeSections(QHeaderView::Stretch);
}

bool MainWindow::setupPieChart(PieChartWidget* pieChart, QListWidget *pieListWidget, std::vector<std::pair <std::string, double>> pieChartList) {

    if (pieChart == nullptr) { return false; }

    // draws the pie graph by sending piechartwidget a vector of name, presentation count
    QVector<QColor> defaultColors(9);
    defaultColors[0]  = QColor(93,165,218);   // blue
    defaultColors[1]  = QColor(250,164,58);   // orange
    defaultColors[2]  = QColor(77,77,77);   // grey
    defaultColors[3]  = QColor(96,189,104);   // green
    defaultColors[4]  = QColor(241,124,176);   // pink
    defaultColors[5]  = QColor(139,35,35); // brown
    defaultColors[6]  = QColor(240,240,0);  // yellow
    defaultColors[7]  = QColor(241,88,84);   // red
    defaultColors[8]  = QColor(178,118,178);   // purple

    int pieSize = (int) pieChartList.size();
    QVector<QColor> colorList(pieSize);

//    if(pieSize == 1) {
//        colorList[0] = defaultColors[1];
//    }
//    else if(pieSize == 2) {
//        colorList[0] = defaultColors[1];colorList[1] = defaultColors[2];
//    }
//    else if(pieSize == 3) {
//        colorList[0] = defaultColors[1];colorList[1] = defaultColors[2];colorList[2] = defaultColors[0];
//    }
//    else if(pieSize == 4) {
//        colorList[0] = defaultColors[0];colorList[1] = defaultColors[7];colorList[2] = defaultColors[1];
//        colorList[3] = defaultColors[2];
//    }
//    else if(pieSize == 5) {
//        colorList[0] = defaultColors[0];colorList[1] = defaultColors[7];colorList[2] = defaultColors[1];
//        colorList[3] = defaultColors[2];colorList[4] = defaultColors[3];
//    }
//    else if(pieSize == 6) {
//        colorList[0] = defaultColors[0];colorList[1] = defaultColors[7];colorList[2] = defaultColors[1];
//        colorList[3] = defaultColors[2];colorList[4] = defaultColors[3];colorList[5] = defaultColors[8];
//    }
//    else if(pieSize == 7) {
//        colorList[0] = defaultColors[0];colorList[1] = defaultColors[7];colorList[2] = defaultColors[1];
//        colorList[3] = defaultColors[2];colorList[4] = defaultColors[3];colorList[5] = defaultColors[8];
//        colorList[6] = defaultColors[6];
//    }
//    else if(pieSize == 8) {
//        colorList[0] = defaultColors[0];colorList[1] = defaultColors[7];colorList[2] = defaultColors[1];
//        colorList[3] = defaultColors[2];colorList[4] = defaultColors[3];colorList[5] = defaultColors[8];
//        colorList[6] = defaultColors[6];colorList[7] = defaultColors[4];
//    }
//    else if(pieSize == 9) {
//        colorList[0] = defaultColors[0];colorList[1] = defaultColors[7];colorList[2] = defaultColors[1];
//        colorList[3] = defaultColors[2];colorList[4] = defaultColors[3];colorList[5] = defaultColors[8];
//        colorList[6] = defaultColors[6];colorList[7] = defaultColors[4];colorList[8] = defaultColors[5];
//    }
//    else {
        for(int i = 0; i < pieSize; i++) {
            colorList[i] = defaultColors[i % 9];
        }
//    }

    pieListWidget->clear();

    for (int i = 0; i < pieSize; i++) { 
        QString identifier = QString::fromStdString(pieChartList[i].first) + QString(",   ") + QString::number(pieChartList[i].second);
        QListWidgetItem item = QListWidgetItem(identifier);
        item.setTextAlignment(Qt::AlignHCenter);
        pieListWidget->addItem(identifier);

        // set legend colors
        QPixmap pixmap(100, 100);
        colorList[i].setAlpha(150);
        pixmap.fill( colorList[i]);
        QIcon tempIcon(pixmap);
        pieListWidget->item(i)->setIcon(tempIcon);
    }
    pieChart->setData(pieChartList, colorList); //passes vector list to piechartwidget
    return true;
}

bool MainWindow::setupLineChart(QCustomPlot *lineChart, std::vector<std::pair <std::string, double>> lineChartList) {

    if (lineChart == nullptr) { return false; }


     QCPScatterStyle style = QCPScatterStyle::ssDisc;
     QPen pointsColour;
     pointsColour.setColor(QColor(93,165,218));
     style.setPen(pointsColour);

     QPen pen;

     lineChart->addGraph();
     pen.setColor(QColor(255, 131, 0));
     // generate data:

     int lineSize = (int)lineChartList.size();
     QVector<double> x(lineSize), y(lineSize);
     QVector<QString> xAxisLables;
     QVector<double> xAxisTicks;

     double maxVal = lineChartList[0].second;
     double minVal = lineChartList[0].second;

     for (int k=0; k<lineSize; ++k)
     {
         x[k] = k + 1;
         y[k] = lineChartList[k].second;

         QString line = QString::fromStdString(lineChartList[k].first);
         QStringList words = line.split(" ");

         QString lable = "";

         for (int i = 0; i < words.length(); i++) {
             lable.append(words.at(i));
             lable.append("\n");
         }

         xAxisLables.append(lable);
         xAxisTicks.append(k + 1);
         if(y[k] > maxVal) {
             maxVal = y[k];
         }
         if (y[k] < minVal) {
             minVal = y[k];
         }
     }
     lineChart->graph()->setData(x, y);
     lineChart->graph()->rescaleAxes(true);
     lineChart->graph()->setPen(pen);
     lineChart->graph()->setName("Line Graph");
     lineChart->graph()->setLineStyle(QCPGraph::lsLine);
     // set scatter style:
     lineChart->graph()->setScatterStyle(style);

     // set blank axis lines:
     //lineChart->rescaleAxes();
     lineChart->xAxis->setRange(0, lineSize + 1);
     lineChart->yAxis->setRange(minVal - (maxVal / 10), maxVal + (maxVal / 10));
     lineChart->yAxis->setTicks(true);
     //lineChart->xAxis->setTickLabels(true);
     lineChart->yAxis->setTickLabels(true);

     lineChart->xAxis->setAutoTicks(false);
     lineChart->xAxis->setAutoTickLabels(false);
     lineChart->xAxis->setTickVector(xAxisTicks);
     lineChart->xAxis->setTickVectorLabels(xAxisLables);
     // make top right axes clones of bottom left axes:
     lineChart->axisRect()->setupFullAxesBox();
     return true;
}

bool MainWindow::setupStackedBarChart(QCustomPlot *stackedBarChart, std::vector<std::pair<std::string, double> > stackedBarCharList)
{
    if (stackedBarChart == nullptr) { return false; }

    int stackedBarSize = (int) stackedBarCharList.size();
    QVector<QColor> defaultColors(9);
    defaultColors[0]  = QColor(93,165,218);   // blue
    defaultColors[1]  = QColor(250,164,58);   // orange
    defaultColors[2]  = QColor(77,77,77);   // grey
    defaultColors[3]  = QColor(96,189,104);   // green
    defaultColors[4]  = QColor(241,124,176);   // pink
    defaultColors[5]  = QColor(139,35,35); // brown
    defaultColors[6]  = QColor(240,240,0);  // yellow
    defaultColors[7]  = QColor(241,88,84);   // red
    defaultColors[8]  = QColor(178,118,178);   // purple

    QCPBars *prev;

    double totalCount = 0;
    for (int i = 0; i < stackedBarSize; i++) {
        QCPBars *bar = new QCPBars(stackedBarChart->yAxis, stackedBarChart->xAxis);
        stackedBarChart->addPlottable(bar);
        bar->setAntialiased(false);
        //bar->setStackingGap(1);
        std::pair<std::string, double> curr = stackedBarCharList.at(i);
        bar->setName(QString::fromStdString((curr.first)));
        QColor fillColor = defaultColors.at(i % 9).lighter(130);
        fillColor.setAlpha(150);
        QPen strokeColor = QPen(defaultColors.at(i % 9));
        strokeColor.setWidth(2);
        bar->setPen(strokeColor);
        bar->setBrush(fillColor);

        QVector<double> ticks;
        QVector<QString> ylabels;
        QVector<double> count;
        ticks << 1;
        ylabels << QString::fromStdString(stackedBarCharList[i].first);
        count << stackedBarCharList[i].second;
        totalCount += stackedBarCharList[i].second;
        bar->setData(ticks, count);

        if (i > 0) {
            bar->moveAbove(prev);
        }
        prev = bar;
    }

    QVector<double> empty;
    //setup Y Axis
    stackedBarChart->yAxis->setAutoTicks(false);
    stackedBarChart->yAxis->setAutoTickLabels(false);
    stackedBarChart->yAxis->setTickLabelPadding(1);
    stackedBarChart->yAxis->setSubTickCount(0);
    stackedBarChart->yAxis->setTickLength(0, 1);
    stackedBarChart->yAxis->grid()->setVisible(true);
    stackedBarChart->yAxis->setRange(0, 3);
    stackedBarChart->yAxis->setTickVector(empty);

    // setup X Axis
    stackedBarChart->xAxis->setAutoTicks(true);
    stackedBarChart->xAxis->setRange(0,totalCount+(totalCount*.05));
    stackedBarChart->xAxis->setAutoTickLabels(true);
    stackedBarChart->xAxis->setAutoTickStep(true);
    stackedBarChart->xAxis->grid()->setSubGridVisible(true);

    //setup legend
    stackedBarChart->legend->setVisible(true);
    stackedBarChart->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignTop|Qt::AlignHCenter);
    stackedBarChart->legend->setBrush(QColor(255, 255, 255, 100));

    return true;
}

bool MainWindow::setupBarChart(QCustomPlot *barChart, std::vector<std::pair <std::string, double>> barChartList) {

    if (barChart == nullptr) { return false; }

    // create empty bar chart objects:
    QCPBars *yLabels = new QCPBars(barChart->yAxis, barChart->xAxis);
    barChart->addPlottable(yLabels);
    yLabels->setAntialiased(false);

    // set names and colors:
    QPen pen;
    pen.setWidthF(1.2);
    yLabels->setName("Type");
    pen.setColor(QColor(255, 131, 0));
    yLabels->setPen(pen);
    yLabels->setBrush(QColor(255, 131, 0, 50));

    //get label list
    int barSize = (int) barChartList.size();
    double maxCount = 0;
    double scaledCount;
    QVector<double> ticks;
    QVector<QString> ylabels;
    QVector<double> count;

    //add label list to y axis labels
    for (int i = 0; i < barSize; i++){
        ticks << (i+1);
        ylabels << QString::fromStdString(barChartList[i].first);
        if (barChartList[i].second>1000000){
            scaledCount = barChartList[i].second/1000000;
        } else if (barChartList[i].second>1000){
            scaledCount = barChartList[i].second/1000;
        } else{
            scaledCount = barChartList[i].second;
        }
        count <<scaledCount;

        if (maxCount < barChartList[i].second)
            maxCount = barChartList[i].second;
    }

    //setup Y Axis
    barChart->yAxis->setAutoTicks(false);
    barChart->yAxis->setAutoTickLabels(false);
    barChart->yAxis->setTickVector(ticks);
    barChart->yAxis->setTickVectorLabels(ylabels);
    barChart->yAxis->setTickLabelPadding(1);
    barChart->yAxis->setSubTickCount(0);
    barChart->yAxis->setTickLength(0, 1);
    barChart->yAxis->grid()->setVisible(true);
    barChart->yAxis->setRange(0, barSize+1);

    if(maxCount>1000000){
        maxCount = maxCount/1000000;
        barChart->xAxis->setLabel("Total (in Millions)");
    }else if (maxCount>1000){
        maxCount = maxCount/1000;
        barChart->xAxis->setLabel("Total (in Thousands)");
    }else{
        barChart->xAxis->setLabel("Total");
    }

    // setup X Axis
    barChart->xAxis->setAutoTicks(true);
    barChart->xAxis->setRange(0,maxCount+(maxCount*.05));
    barChart->xAxis->setAutoTickLabels(true);
    barChart->xAxis->setAutoTickStep(true);
    barChart->xAxis->grid()->setSubGridVisible(true);

    QPen gridPen;
    gridPen.setStyle(Qt::SolidLine);
    gridPen.setColor(QColor(0, 0, 0, 25));
    barChart->xAxis->grid()->setPen(gridPen);
    gridPen.setStyle(Qt::DotLine);
    barChart->xAxis->grid()->setSubGridPen(gridPen);

    yLabels->setData(ticks, count);
    return true;
}


void MainWindow::on_teach_new_sort_clicked() {
    if (teachdb != NULL) {
        CustomSort* sortdialog = new CustomSort();
        sortdialog->setFields(TEACH_MANFIELDS);
        int ret = sortdialog->exec();
        if (ret) {
            QStringList newSortOrder = sortdialog->getSortFields();
            allTeachOrders << newSortOrder;
            ui->teach_sort->addItem(newSortOrder.at(0));

            // save the sort fields to file
            QSortListIO saveTeachSort(TEACHORDER_SAVE);
            saveTeachSort.saveList(allTeachOrders);
        }
        delete sortdialog;
    } else {
        QMessageBox::critical(this, "Missing File", "Please load a file first.");
    }
}

void MainWindow::on_teach_edit_sort_clicked()
{
    if (teachdb != NULL) {
        CustomSort* sortdialog = new CustomSort();
        sortdialog->setFields(TEACH_MANFIELDS);
        QStringList sortOrder = allTeachOrders[ui->teach_sort->currentIndex()];
        if (sortOrder.at(0) == "Default") {
            QMessageBox::critical(this, "Error", "Cannot edit the default sort order.");
            return;
        }
        sortdialog->setSortFields(sortOrder);
        int ret = sortdialog->exec();
        if (ret) {
            QStringList newSortOrder = sortdialog->getSortFields();

            for (int i = 0; i < allTeachOrders.length(); i++) {
                if (allTeachOrders.at(i).at(0) == newSortOrder.at(0)) {
                    allTeachOrders.removeAt(i);
                    allTeachOrders << newSortOrder;
                }
            }

            int index = ui->teach_sort->findText(newSortOrder.at(0));
            ui->teach_sort->removeItem(index);
            ui->teach_sort->addItem(newSortOrder.at(0));

            // save the sort fields to file
            QSortListIO saveTeachSort(TEACHORDER_SAVE);
            saveTeachSort.saveList(allTeachOrders);
        }
        delete sortdialog;
    } else {
        QMessageBox::critical(this, "Missing File", "Please load a file first.");
    }
}

void MainWindow::on_teach_sort_selection_clicked() {
    ui->teach_sort->setEnabled(false);
    ui->teach_filter->setEnabled(false);
    ui->teach_edit_sort->setEnabled(false);
    ui->teach_delete_sort->setEnabled(false);
    ui->teach_new_sort->setEnabled(false);
    ui->teach_filter_from->setEnabled(false);
    ui->teach_filter_to->setEnabled(false);
    ui->teach_pie_button->setEnabled(false);
    ui->teach_bar_button->setEnabled(false);
    ui->teach_stacked_bar_button->setEnabled(false);
    ui->teach_to_label->setEnabled(false);
    ui->teach_sort_label->setEnabled(false);
    ui->teach_filter_label->setEnabled(false);
    ui->teach_line_button->setEnabled(false);
    ui->teach_search->setEnabled(false);
    ui->teach_search_label->setEnabled(false);
    ui->teach_sort_selection->setEnabled(false);
    ui->teach_sort_cancel->setEnabled(true);
    ui->teach_sort_apply->setEnabled(true);
    ui->teachTreeView->setSelectionMode(QTreeView::MultiSelection);
}

void MainWindow::on_teach_sort_cancel_clicked() {
    ui->teach_sort->setEnabled(true);
    ui->teach_edit_sort->setEnabled(true);
    ui->teach_delete_sort->setEnabled(true);
    ui->teach_new_sort->setEnabled(true);
    ui->teach_filter_from->setEnabled(true);
    ui->teach_filter_to->setEnabled(true);
    ui->teach_pie_button->setEnabled(true);
    ui->teach_bar_button->setEnabled(true);
    ui->teach_stacked_bar_button->setEnabled(true);
    ui->teach_to_label->setEnabled(true);
    ui->teach_sort_label->setEnabled(true);
    ui->teach_filter_label->setEnabled(true);
    ui->teach_line_button->setEnabled(true);
    ui->teach_search->setEnabled(true);
    ui->teach_search_label->setEnabled(true);
    ui->teach_sort_selection->setEnabled(true);
    ui->teach_filter->setEnabled(true);
    ui->teach_sort_cancel->setEnabled(false);
    ui->teach_sort_apply->setEnabled(false);

    userSelectedRows.clear();
    ui->teachTreeView->setSelectionMode(QTreeView::SingleSelection);

    refresh(TEACH);
}

void MainWindow::on_teach_sort_apply_clicked() {
    ui->teach_edit_sort->setEnabled(true);
    ui->teach_delete_sort->setEnabled(true);
    ui->teach_new_sort->setEnabled(true);
    ui->teach_pie_button->setEnabled(true);
    ui->teach_bar_button->setEnabled(true);
    ui->teach_stacked_bar_button->setEnabled(true);
    ui->teach_filter_label->setEnabled(true);
    ui->teach_line_button->setEnabled(true);
    ui->teach_sort_apply->setEnabled(false);


    ui->teachTreeView->setSelectionMode(QTreeView::SingleSelection);

    QModelIndexList list = ui->teachTreeView->selectionModel()->selectedIndexes();
    userSelectedRows.clear();
    int row = -1;
    foreach (QModelIndex index, list)
    {
        if (index.row()!=row && index.column()==0)
        {
            userSelectedRows.append(index.row());
            row = index.row();
        }
    }
    refreshOnUserSelect = true;
    refresh(TEACH);
    refreshOnUserSelect = false;
}


void MainWindow::on_pub_sort_selection_clicked() {
    ui->pub_sort->setEnabled(false);
    ui->pub_filter->setEnabled(false);
    ui->pub_edit_sort->setEnabled(false);
    ui->pub_delete_sort->setEnabled(false);
    ui->pub_new_sort->setEnabled(false);
    ui->pub_filter_from->setEnabled(false);
    ui->pub_filter_to->setEnabled(false);
    ui->pub_pie_button->setEnabled(false);
    ui->pub_bar_button->setEnabled(false);
    ui->pub_stacked_bar_button->setEnabled(false);
    ui->pub_to_label->setEnabled(false);
    ui->pub_sort_label->setEnabled(false);
    ui->pub_filter_label->setEnabled(false);
    ui->pub_line_button->setEnabled(false);
    ui->pub_search->setEnabled(false);
    ui->pub_search_label->setEnabled(false);
    ui->pub_sort_selection->setEnabled(false);
    ui->pub_sort_cancel->setEnabled(true);
    ui->pub_sort_apply->setEnabled(true);
    ui->pubTreeView->setSelectionMode(QTreeView::MultiSelection);
}

void MainWindow::on_pub_sort_cancel_clicked() {
    ui->pub_sort->setEnabled(true);
    ui->pub_edit_sort->setEnabled(true);
    ui->pub_delete_sort->setEnabled(true);
    ui->pub_new_sort->setEnabled(true);
    ui->pub_filter_from->setEnabled(true);
    ui->pub_filter_to->setEnabled(true);
    ui->pub_pie_button->setEnabled(true);
    ui->pub_bar_button->setEnabled(true);
    ui->pub_stacked_bar_button->setEnabled(true);
    ui->pub_to_label->setEnabled(true);
    ui->pub_sort_label->setEnabled(true);
    ui->pub_filter_label->setEnabled(true);
    ui->pub_line_button->setEnabled(true);
    ui->pub_search->setEnabled(true);
    ui->pub_search_label->setEnabled(true);
    ui->pub_sort_selection->setEnabled(true);
    ui->pub_filter->setEnabled(true);
    ui->pub_sort_cancel->setEnabled(false);
    ui->pub_sort_apply->setEnabled(false);

    userSelectedRows.clear();
    ui->pubTreeView->setSelectionMode(QTreeView::SingleSelection);

     refresh(PUBLICATIONS);
}

void MainWindow::on_pub_sort_apply_clicked() {
    ui->pub_edit_sort->setEnabled(true);
    ui->pub_delete_sort->setEnabled(true);
    ui->pub_new_sort->setEnabled(true);
    ui->pub_pie_button->setEnabled(true);
    ui->pub_bar_button->setEnabled(true);
    ui->pub_stacked_bar_button->setEnabled(true);
    ui->pub_filter_label->setEnabled(true);
    ui->pub_line_button->setEnabled(true);
    ui->pub_sort_apply->setEnabled(false);


    ui->pubTreeView->setSelectionMode(QTreeView::SingleSelection);

    QModelIndexList list = ui->pubTreeView->selectionModel()->selectedIndexes();
    userSelectedRows.clear();
    int row = -1;
    foreach (QModelIndex index, list)
    {
        if (index.row()!=row && index.column()==0)
        {
            userSelectedRows.append(index.row());
            row = index.row();
        }
    }
    refreshOnUserSelect = true;
    refresh(PUBLICATIONS);
    refreshOnUserSelect = false;
}

void MainWindow::on_pres_sort_selection_clicked() {
    ui->pres_sort->setEnabled(false);
    ui->pres_filter->setEnabled(false);
    ui->pres_edit_sort->setEnabled(false);
    ui->pres_delete_sort->setEnabled(false);
    ui->pres_new_sort->setEnabled(false);
    ui->pres_filter_from->setEnabled(false);
    ui->pres_filter_to->setEnabled(false);
    ui->pres_pie_button->setEnabled(false);
    ui->pres_bar_button->setEnabled(false);
    ui->pres_stacked_bar_button->setEnabled(false);
    ui->pres_to_label->setEnabled(false);
    ui->pres_sort_label->setEnabled(false);
    ui->pres_filter_label->setEnabled(false);
    ui->pres_line_button->setEnabled(false);
    ui->pres_search->setEnabled(false);
    ui->pres_search_label->setEnabled(false);
    ui->pres_sort_selection->setEnabled(false);
    ui->pres_sort_cancel->setEnabled(true);
    ui->pres_sort_apply->setEnabled(true);
    ui->presTreeView->setSelectionMode(QTreeView::MultiSelection);
}

void MainWindow::on_pres_sort_cancel_clicked() {
    ui->pres_sort->setEnabled(true);
    ui->pres_edit_sort->setEnabled(true);
    ui->pres_delete_sort->setEnabled(true);
    ui->pres_new_sort->setEnabled(true);
    ui->pres_filter_from->setEnabled(true);
    ui->pres_filter_to->setEnabled(true);
    ui->pres_pie_button->setEnabled(true);
    ui->pres_bar_button->setEnabled(true);
    ui->pres_stacked_bar_button->setEnabled(true);
    ui->pres_to_label->setEnabled(true);
    ui->pres_sort_label->setEnabled(true);
    ui->pres_filter_label->setEnabled(true);
    ui->pres_line_button->setEnabled(true);
    ui->pres_search->setEnabled(true);
    ui->pres_search_label->setEnabled(true);
    ui->pres_sort_selection->setEnabled(true);
    ui->pres_filter->setEnabled(true);
    ui->pres_sort_cancel->setEnabled(false);
    ui->pres_sort_apply->setEnabled(false);

    userSelectedRows.clear();
    ui->presTreeView->setSelectionMode(QTreeView::SingleSelection);

    refresh(PRESENTATIONS);
}

void MainWindow::on_pres_sort_apply_clicked() {
    ui->pres_edit_sort->setEnabled(true);
    ui->pres_delete_sort->setEnabled(true);
    ui->pres_new_sort->setEnabled(true);
    ui->pres_pie_button->setEnabled(true);
    ui->pres_bar_button->setEnabled(true);
    ui->pres_stacked_bar_button->setEnabled(true);
    ui->pres_filter_label->setEnabled(true);
    ui->pres_line_button->setEnabled(true);
    ui->pres_sort_apply->setEnabled(false);


    ui->presTreeView->setSelectionMode(QTreeView::SingleSelection);

    QModelIndexList list = ui->presTreeView->selectionModel()->selectedIndexes();
    userSelectedRows.clear();
    int row = -1;
    foreach (QModelIndex index, list)
    {
        if (index.row()!=row && index.column()==0)
        {
            userSelectedRows.append(index.row());
            row = index.row();
        }
    }
    refreshOnUserSelect = true;
    refresh(PRESENTATIONS);
    refreshOnUserSelect = false;
}

void MainWindow::on_fund_sort_selection_clicked() {
    ui->fund_sort->setEnabled(false);
    ui->fund_filter->setEnabled(false);
    ui->fund_edit_sort->setEnabled(false);
    ui->fund_delete_sort->setEnabled(false);
    ui->fund_new_sort->setEnabled(false);
    ui->fund_filter_from->setEnabled(false);
    ui->fund_filter_to->setEnabled(false);
    ui->fund_pie_button->setEnabled(false);
    ui->fund_bar_button->setEnabled(false);
    ui->fund_stacked_bar_button->setEnabled(false);
    ui->fund_to_label->setEnabled(false);
    ui->fund_sort_label->setEnabled(false);
    ui->fund_filter_label->setEnabled(false);
    ui->fund_line_button->setEnabled(false);
    ui->grant_search->setEnabled(false);
    ui->grants_search_label->setEnabled(false);
    ui->fund_sort_selection->setEnabled(false);
    ui->fund_sort_cancel->setEnabled(true);
    ui->fund_sort_apply->setEnabled(true);
    ui->fundTreeView->setSelectionMode(QTreeView::MultiSelection);
}

void MainWindow::on_fund_sort_cancel_clicked() {
    ui->fund_sort->setEnabled(true);
    ui->fund_edit_sort->setEnabled(true);
    ui->fund_delete_sort->setEnabled(true);
    ui->fund_new_sort->setEnabled(true);
    ui->fund_filter_from->setEnabled(true);
    ui->fund_filter_to->setEnabled(true);
    ui->fund_pie_button->setEnabled(true);
    ui->fund_bar_button->setEnabled(true);
    ui->fund_stacked_bar_button->setEnabled(true);
    ui->fund_to_label->setEnabled(true);
    ui->fund_sort_label->setEnabled(true);
    ui->fund_filter_label->setEnabled(true);
    ui->fund_line_button->setEnabled(true);
    ui->grant_search->setEnabled(true);
    ui->grants_search_label->setEnabled(true);
    ui->fund_sort_selection->setEnabled(true);
    ui->fund_filter->setEnabled(true);
    ui->fund_sort_cancel->setEnabled(false);
    ui->fund_sort_apply->setEnabled(false);

    userSelectedRows.clear();
    ui->fundTreeView->setSelectionMode(QTreeView::SingleSelection);

    refresh(FUNDING);
}

void MainWindow::on_fund_sort_apply_clicked() {
    ui->fund_edit_sort->setEnabled(true);
    ui->fund_delete_sort->setEnabled(true);
    ui->fund_new_sort->setEnabled(true);
    ui->fund_pie_button->setEnabled(true);
    ui->fund_bar_button->setEnabled(true);
    ui->fund_stacked_bar_button->setEnabled(true);
    ui->fund_filter_label->setEnabled(true);
    ui->fund_line_button->setEnabled(true);
    ui->fund_sort_apply->setEnabled(false);

    ui->fundTreeView->setSelectionMode(QTreeView::SingleSelection);

    QModelIndexList list = ui->fundTreeView->selectionModel()->selectedIndexes();
    userSelectedRows.clear();
    int row = -1;
    foreach (QModelIndex index, list)
    {
        if (index.row()!=row && index.column()==0)
        {
            userSelectedRows.append(index.row());
            row = index.row();
        }
    }
    refreshOnUserSelect = true;
    refresh(FUNDING);
    refreshOnUserSelect = false;
}

void MainWindow::on_pub_new_sort_clicked() {
    if (pubdb != NULL) {
        CustomSort* sortdialog = new CustomSort();
        sortdialog->setFields(PUBS_MANFIELDS);
        int ret = sortdialog->exec();
        if (ret) {
            QStringList newSortOrder = sortdialog->getSortFields();
            allPubOrders << newSortOrder;
            ui->pub_sort->addItem(newSortOrder.at(0));

            // save the sort fields to file
            QSortListIO savePubSort(PUBORDER_SAVE);
            savePubSort.saveList(allPubOrders);
        }
        delete sortdialog;
    } else {
        QMessageBox::critical(this, "Missing File", "Please load a file first.");
    }
}

void MainWindow::on_pub_edit_sort_clicked()
{


    if (pubdb != NULL) {
        CustomSort* sortdialog = new CustomSort();
        sortdialog->setFields(PUBS_MANFIELDS);
        QStringList sortOrder = allPubOrders[ui->pub_sort->currentIndex()];
        if (sortOrder.at(0) == "Default") {
            QMessageBox::critical(this, "Error", "Cannot edit the default sort order.");
            return;
        }
        sortdialog->setSortFields(sortOrder);
        int ret = sortdialog->exec();
        if (ret) {
            QStringList newSortOrder = sortdialog->getSortFields();

            for (int i = 0; i < allPubOrders.length(); i++) {
                if (allPubOrders.at(i).at(0) == newSortOrder.at(0)) {
                    allPubOrders.removeAt(i);
                    allPubOrders << newSortOrder;
                }
            }

            int index = ui->pub_sort->findText(newSortOrder.at(0));
            ui->pub_sort->removeItem(index);
            ui->pub_sort->addItem(newSortOrder.at(0));

            // save the sort fields to file
            QSortListIO saveTeachSort(PUBORDER_SAVE);
            saveTeachSort.saveList(allPubOrders);
        }
        delete sortdialog;
    } else {
        QMessageBox::critical(this, "Missing File", "Please load a file first.");
    }
}

void MainWindow::on_pres_new_sort_clicked() {
    if (presdb != NULL) {
        CustomSort* sortdialog = new CustomSort();
        sortdialog->setFields(PRES_MANFIELDS);
        int ret = sortdialog->exec();
        if (ret) {
            QStringList newSortOrder = sortdialog->getSortFields();
            allPresOrders << newSortOrder;
            ui->pres_sort->addItem(newSortOrder.at(0));

            // save the sort fields to file
            QSortListIO savePresSort(PRESORDER_SAVE);
            savePresSort.saveList(allPresOrders);
        }
        delete sortdialog;
    } else {
        QMessageBox::critical(this, "Missing File", "Please load a file first.");
    }
}

void MainWindow::on_pres_edit_sort_clicked()
{


    if (presdb != NULL) {
        CustomSort* sortdialog = new CustomSort();
        sortdialog->setFields(PRES_MANFIELDS);
        QStringList sortOrder = allPresOrders[ui->pres_sort->currentIndex()];
        if (sortOrder.at(0) == "Default") {
            QMessageBox::critical(this, "Error", "Cannot edit the default sort order.");
            return;
        }
        sortdialog->setSortFields(sortOrder);
        int ret = sortdialog->exec();
        if (ret) {
            QStringList newSortOrder = sortdialog->getSortFields();

            for (int i = 0; i < allPresOrders.length(); i++) {
                if (allPresOrders.at(i).at(0) == newSortOrder.at(0)) {
                    allPresOrders.removeAt(i);
                    allPresOrders << newSortOrder;
                }
            }

            int index = ui->pres_sort->findText(newSortOrder.at(0));
            ui->pres_sort->removeItem(index);
            ui->pres_sort->addItem(newSortOrder.at(0));

            // save the sort fields to file
            QSortListIO saveTeachSort(PRESORDER_SAVE);
            saveTeachSort.saveList(allPresOrders);
        }
        delete sortdialog;
    } else {
        QMessageBox::critical(this, "Missing File", "Please load a file first.");
    }
}

void MainWindow::on_fund_new_sort_clicked() {
    if (funddb != NULL) {
        CustomSort* sortdialog = new CustomSort();
        sortdialog->setFields(GRANTS_MANFIELDS);
        int ret = sortdialog->exec();
        if (ret) {
            QStringList newSortOrder = sortdialog->getSortFields();
            allFundOrders << newSortOrder;
            ui->fund_sort->addItem(newSortOrder.at(0));

            // save the sort fields to file
            QSortListIO saveFundSort(FUNDORDER_SAVE);
            saveFundSort.saveList(allFundOrders);
        }
        delete sortdialog;
    } else {
        QMessageBox::critical(this, "Missing File", "Please load a file first.");
    }
}

void MainWindow::on_fund_edit_sort_clicked()
{
    if (funddb != NULL) {
        CustomSort* sortdialog = new CustomSort();
        sortdialog->setFields(GRANTS_MANFIELDS);
        QStringList sortOrder = allFundOrders[ui->fund_sort->currentIndex()];
        if (sortOrder.at(0) == "Default") {
            QMessageBox::critical(this, "Error", "Cannot edit the default sort order.");
            return;
        }
        sortdialog->setSortFields(sortOrder);
        int ret = sortdialog->exec();
        if (ret) {
            QStringList newSortOrder = sortdialog->getSortFields();

            for (int i = 0; i < allFundOrders.length(); i++) {
                if (allFundOrders.at(i).at(0) == newSortOrder.at(0)) {
                    allFundOrders.removeAt(i);
                    allFundOrders << newSortOrder;
                }
            }

            int index = ui->fund_sort->findText(newSortOrder.at(0));
            ui->fund_sort->removeItem(index);
            ui->fund_sort->addItem(newSortOrder.at(0));

            // save the sort fields to file
            QSortListIO saveTeachSort(FUNDORDER_SAVE);
            saveTeachSort.saveList(allFundOrders);
        }
        delete sortdialog;
    } else {
        QMessageBox::critical(this, "Missing File", "Please load a file first.");
    }
}

void MainWindow::on_teach_sort_currentIndexChanged(int index) {
    if(index != -1) {
        QStringList sortOrder = allTeachOrders[index];
        teachSortOrder.clear();
        for (int i = 1; i < sortOrder.size(); i++) {
            teachSortOrder.emplace_back(sortOrder[i].toStdString());
        }
        ui->teach_filter->setText(QString::fromStdString(teachSortOrder[0]));
        refresh(TEACH);
    }
}

void MainWindow::on_pub_sort_currentIndexChanged(int index) {
    if(index != -1) {
        QStringList sortOrder = allPubOrders[index];
        pubSortOrder.clear();
        for (int i = 1; i < sortOrder.size(); i++) {
            pubSortOrder.emplace_back(sortOrder[i].toStdString());
        }
        ui->pub_filter->setText(QString::fromStdString(pubSortOrder[0]));
        refresh(PUBLICATIONS);
    }
}

void MainWindow::on_pres_sort_currentIndexChanged(int index) {
    if(index != -1) {
        QStringList sortOrder = allPresOrders[index];
        presSortOrder.clear();
        for (int i = 1; i < sortOrder.size(); i++) {
            presSortOrder.emplace_back(sortOrder[i].toStdString());
        }
        ui->pres_filter->setText(QString::fromStdString(presSortOrder[0]));
        refresh(PRESENTATIONS);
    }
}

void MainWindow::on_fund_sort_currentIndexChanged(int index) {
    if(index != -1) {
        QStringList sortOrder = allFundOrders[index];
        fundSortOrder.clear();
        for (int i = 1; i < sortOrder.size(); i++) {
            fundSortOrder.emplace_back(sortOrder[i].toStdString());
        }
        ui->fund_filter->setText(QString::fromStdString(fundSortOrder[0]));
        refresh(FUNDING);
    }
}

void MainWindow::on_teach_delete_sort_clicked() {
    if (ui->teach_sort->currentIndex()!=0) {
        QMessageBox prompt;
        prompt.setText("Are you sure you want to delete " + ui->teach_sort->currentText() + "?");
        prompt.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (prompt.exec()==QMessageBox::Yes) {
            allTeachOrders.removeAt(ui->teach_sort->currentIndex());
            ui->teach_sort->removeItem(ui->teach_sort->currentIndex());

            QSortListIO saveTeachSort(TEACHORDER_SAVE);
            saveTeachSort.saveList(allTeachOrders);
        }
    } else {
        QMessageBox::critical(this, "", "Cannot delete Default");
    }
}

void MainWindow::on_pub_delete_sort_clicked() {
    if (ui->pub_sort->currentIndex()!=0) {
        QMessageBox prompt;
        prompt.setText("Are you sure you want to delete " + ui->pub_sort->currentText() + "?");
        prompt.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (prompt.exec()==QMessageBox::Yes) {
            allPubOrders.removeAt(ui->pub_sort->currentIndex());
            ui->pub_sort->removeItem(ui->pub_sort->currentIndex());

            QSortListIO savePubSort(PUBORDER_SAVE);
            savePubSort.saveList(allPubOrders);
        }
    } else {
        QMessageBox::critical(this, "", "Cannot delete Default");
    }
}

void MainWindow::on_pres_delete_sort_clicked() {
    if (ui->pres_sort->currentIndex()!=0) {
        QMessageBox prompt;
        prompt.setText("Are you sure you want to delete " + ui->pres_sort->currentText() + "?");
        prompt.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (prompt.exec()==QMessageBox::Yes) {
            allPresOrders.removeAt(ui->pres_sort->currentIndex());
            ui->pres_sort->removeItem(ui->pres_sort->currentIndex());

            QSortListIO savePresSort(PRESORDER_SAVE);
            savePresSort.saveList(allPresOrders);
        }
    } else {
        QMessageBox::critical(this, "", "Cannot delete Default");
    }
}

void MainWindow::on_fund_delete_sort_clicked() {
    if (ui->fund_sort->currentIndex()!=0) {
        QMessageBox prompt;
        prompt.setText("Are you sure you want to delete " + ui->fund_sort->currentText() + "?");
        prompt.setStandardButtons(QMessageBox::Yes | QMessageBox::No);

        if (prompt.exec()==QMessageBox::Yes) {
            allFundOrders.removeAt(ui->fund_sort->currentIndex());
            ui->fund_sort->removeItem(ui->fund_sort->currentIndex());

            QSortListIO saveFundSort(FUNDORDER_SAVE);
            saveFundSort.saveList(allFundOrders);
        }
    } else {
        QMessageBox::critical(this, "", "Cannot delete Default");
    }
}

void MainWindow::on_teach_bar_button_toggled() { ui->teach_graph_stackedWidget->setCurrentIndex(1);}
void MainWindow::on_teach_pie_button_toggled() { ui->teach_graph_stackedWidget->setCurrentIndex(0);}
void MainWindow::on_pub_bar_button_toggled() { ui->pub_graph_stackedWidget->setCurrentIndex(1);}
void MainWindow::on_pub_pie_button_toggled() { ui->pub_graph_stackedWidget->setCurrentIndex(0);}
void MainWindow::on_pres_bar_button_toggled() { ui->pres_graph_stackedWidget->setCurrentIndex(1);}
void MainWindow::on_pres_pie_button_toggled() { ui->pres_graph_stackedWidget->setCurrentIndex(0);}
void MainWindow::on_fund_bar_button_toggled() { ui->fund_graph_stackedWidget->setCurrentIndex(1);}
void MainWindow::on_fund_pie_button_toggled() { ui->fund_graph_stackedWidget->setCurrentIndex(0);}
void MainWindow::on_fund_line_button_toggled() { ui->fund_graph_stackedWidget->setCurrentIndex(2);}
void MainWindow::on_pres_line_button_toggled() { ui->pres_graph_stackedWidget->setCurrentIndex(2);}
void MainWindow::on_pub_line_button_toggled() { ui->pub_graph_stackedWidget->setCurrentIndex(2);}
void MainWindow::on_teach_line_button_toggled() { ui->teach_graph_stackedWidget->setCurrentIndex(2);}
void MainWindow::on_teach_stacked_bar_button_clicked() { ui->teach_graph_stackedWidget->setCurrentIndex(3); }
void MainWindow::on_pub_stacked_bar_button_clicked() { ui->pub_graph_stackedWidget->setCurrentIndex(3); }
void MainWindow::on_pres_stacked_bar_button_clicked() { ui->pres_graph_stackedWidget->setCurrentIndex(3); }
void MainWindow::on_fund_stacked_bar_button_clicked() {  ui->fund_graph_stackedWidget->setCurrentIndex(3); }

void MainWindow::on_teach_load_file_clicked() {
    QString path = load_file();
    if (!path.isEmpty()) {
        load_teach(path);
    }
}

bool MainWindow::load_teach(QString path, bool multi_file) {
    if (!checkFile(TEACH, path)) {
        // enable gui elements

        lastTeachFile = path;

        ui->teach_sort->setEnabled(true);
        ui->teach_edit_sort->setEnabled(true);
        ui->teach_delete_sort->setEnabled(true);
        ui->teach_new_sort->setEnabled(true);
        ui->teach_filter_from->setEnabled(true);
        ui->teach_filter_to->setEnabled(true);
        ui->teach_pie_button->setEnabled(true);
        ui->teach_bar_button->setEnabled(true);
        ui->teach_stacked_bar_button->setEnabled(true);
        ui->teach_to_label->setEnabled(true);
        ui->teach_sort_label->setEnabled(true);
        ui->teach_filter->setEnabled(true);
        ui->teach_filter_label->setEnabled(true);
        ui->teach_line_button->setEnabled(true);
        ui->teach_search->setEnabled(true);
        ui->teach_search_label->setEnabled(true);
        ui->teach_sort_selection->setEnabled(true);

        // load save order
        QSortListIO teachSaveOrder(TEACHORDER_SAVE);
        allTeachOrders = teachSaveOrder.readList();
        ui->teach_sort->clear();
        for (int i = 0; i < allTeachOrders.size(); i++) {
            ui->teach_sort->addItem(allTeachOrders.at(i).at(0));
        }

        // create default sort order if none are loaded
        if (ui->teach_sort->currentIndex() < 0) {
            createDefaultSortOrder(TEACH);
            ui->teach_sort->addItem(allTeachOrders[0][0]);
        }

        // create the tree
        teachPath = path;
        makeTree(TEACH);
        ui->teach_file_label->setText(teachPath);

        return true;
    } else {
        if (!multi_file) {
            QMessageBox::critical(this, "Invalid File", "Not a valid teaching file.");
            on_teach_load_file_clicked();
        }
    }
    return false;
}

void MainWindow::on_pub_load_file_clicked() {
    QString path = load_file();
    if (!path.isEmpty()) {
        load_pub(path);
    }
}

bool MainWindow::load_pub(QString path, bool multi_file) {
    if (!checkFile(PUBLICATIONS, path)) {
        // enable gui elements

        lastPubFile = path;

        ui->pub_sort->setEnabled(true);
        ui->pub_edit_sort->setEnabled(true);
        ui->pub_delete_sort->setEnabled(true);
        ui->pub_new_sort->setEnabled(true);
        ui->pub_filter_from->setEnabled(true);
        ui->pub_filter_to->setEnabled(true);
        ui->pub_pie_button->setEnabled(true);
        ui->pub_bar_button->setEnabled(true);
        ui->pub_line_button->setEnabled(true);
        ui->pub_to_label->setEnabled(true);
        ui->pub_sort_label->setEnabled(true);
        ui->pub_filter->setEnabled(true);
        ui->pub_filter_label->setEnabled(true);
        ui->pub_stacked_bar_button->setEnabled(true);
        ui->pub_search->setEnabled(true);
        ui->pub_search_label->setEnabled(true);
        ui->pub_sort_selection->setEnabled(true);


        // load save order
        QSortListIO pubSaveOrder(PUBORDER_SAVE);
        allPubOrders = pubSaveOrder.readList();
        ui->pub_sort->clear();
        for (int i = 0; i < allPubOrders.size(); i++) {
            ui->pub_sort->addItem(allPubOrders.at(i).at(0));
        }

        // create default sort order if none are loaded
        if (ui->pub_sort->currentIndex() < 0) {
            createDefaultSortOrder(PUBLICATIONS);
            ui->pub_sort->addItem(allPubOrders[0][0]);
        }

        // create the tree
        pubPath = path;
        makeTree(PUBLICATIONS);
        ui->pub_file_label->setText(pubPath);

        return true;
    } else {
        if (!multi_file) {
            QMessageBox::critical(this, "Invalid File", "Not a valid publications file.");
            on_pub_load_file_clicked();
        }
    }
    return false;
}

void MainWindow::on_pres_load_file_clicked() {
    QString path = load_file();
    if (!path.isEmpty()) {
        load_pres(path);
    }
}

bool MainWindow::load_pres(QString path, bool multi_file) {
    if (!checkFile(PRESENTATIONS, path)) {

        lastPresFile = path;

        // enable gui elements
        ui->pres_sort->setEnabled(true);
        ui->pres_delete_sort->setEnabled(true);
        ui->pres_new_sort->setEnabled(true);
        ui->pres_edit_sort->setEnabled(true);
        ui->pres_filter_from->setEnabled(true);
        ui->pres_filter_to->setEnabled(true);
        ui->pres_pie_button->setEnabled(true);
        ui->pres_bar_button->setEnabled(true);
        ui->pres_line_button->setEnabled(true);
        ui->pres_to_label->setEnabled(true);
        ui->pres_sort_label->setEnabled(true);
        ui->pres_filter->setEnabled(true);
        ui->pres_filter_label->setEnabled(true);
        ui->pres_stacked_bar_button->setEnabled(true);
        ui->pres_search->setEnabled(true);
        ui->pres_search_label->setEnabled(true);
        ui->pres_sort_selection->setEnabled(true);

        // load save order
        QSortListIO presSaveOrder(PRESORDER_SAVE);
        allPresOrders = presSaveOrder.readList();
        ui->pres_sort->clear();
        for (int i = 0; i < allPresOrders.size(); i++) {
            ui->pres_sort->addItem(allPresOrders.at(i).at(0));
        }

        // create default sort order if none are loaded
        if (ui->pres_sort->currentIndex() < 0) {
            createDefaultSortOrder(PRESENTATIONS);
            ui->pres_sort->addItem(allPresOrders[0][0]);
        }

        // create the tree
        presPath = path;
        makeTree(PRESENTATIONS);
        ui->pres_file_label->setText(presPath);

        return true;
    } else {
        if (!multi_file) {
            QMessageBox::critical(this, "Invalid File", "Not a valid presentations file.");
            on_pres_load_file_clicked();
        }
    }
    return false;
}

void MainWindow::on_fund_load_file_clicked() {
    QString path = load_file();
    if (!path.isEmpty()) {
        load_fund(path);
    }
}

bool MainWindow::load_fund(QString path, bool multi_file) {
    if (!checkFile(FUNDING, path)) {

        lastGrantFile = path;

        // enable gui elements
        ui->fund_sort->setEnabled(true);
        ui->fund_delete_sort->setEnabled(true);
        ui->fund_new_sort->setEnabled(true);
        ui->fund_filter_from->setEnabled(true);
        ui->fund_edit_sort->setEnabled(true);
        ui->fund_filter_to->setEnabled(true);
        ui->fund_pie_button->setEnabled(true);
        ui->fund_bar_button->setEnabled(true);
        ui->fund_line_button->setEnabled(true);
        ui->fund_to_label->setEnabled(true);
        ui->fund_sort_label->setEnabled(true);
        ui->fund_filter->setEnabled(true);
        ui->fund_filter_label->setEnabled(true);
        ui->fund_stacked_bar_button->setEnabled(true);
        ui->grant_search->setEnabled(true);
        ui->grants_search_label->setEnabled(true);
        ui->fund_sort_selection->setEnabled(true);

        // load save order
        QSortListIO fundSaveOrder(FUNDORDER_SAVE);
        allFundOrders = fundSaveOrder.readList();
        ui->fund_sort->clear();
        for (int i = 0; i < allFundOrders.size(); i++) {
            ui->fund_sort->addItem(allFundOrders.at(i).at(0));
        }

        // create default sort order if none are loaded
        if (ui->fund_sort->currentIndex() < 0) {
            createDefaultSortOrder(FUNDING);
            ui->fund_sort->addItem(allFundOrders[0][0]);
        }

        // create the tree
        fundPath = path;
        makeTree(FUNDING);
        ui->fund_file_label->setText(fundPath);

        return true;
    } else {
        if (!multi_file) {
            QMessageBox::critical(this, "Invalid File", "Not a valid grants and funding file.");
            on_fund_load_file_clicked();
        }
    }
    return false;
}

void MainWindow::on_FromDate_dateChanged(const QDate &date) {
    // set the member variable to the new date
    yearStart = date.year();

    // update end date spinbox to not fall below that year
    ui->ToDate->setMinimumDate(date);

    dateChanged = {true, true, true, true};

    // refresh the GUI
    refresh(ui->categoryTab->currentIndex());
}

void MainWindow::on_ToDate_dateChanged(const QDate &date) {
    // set the member variable to the new date
    yearEnd = date.year();

    // update end date spinbox to not fall below that year
    ui->ToDate->setMinimumDate(ui->FromDate->date());

    dateChanged = {true, true, true, true};

    // refresh the GUI
    refresh(ui->categoryTab->currentIndex());
}

void MainWindow::on_categoryTab_currentChanged() {
    if (dateChanged[ui->categoryTab->currentIndex()] == true) {
        refresh(ui->categoryTab->currentIndex());
        dateChanged[ui->categoryTab->currentIndex()] = false;
    }
}

void MainWindow::on_teachTreeView_clicked(const QModelIndex &index) {
    QString clickedName = index.data(Qt::DisplayRole).toString();
    if (clickedName==teachClickedName) { return;}
    ui->teachPrintButton->setEnabled(true);
    ui->teachExportButton->setEnabled(true);
    std::vector<std::string> parentsList;
    QModelIndex current = index.sibling(index.row(), 0);
    QString name;
    while (true) {
        name = current.data(Qt::DisplayRole).toString();
        if(name!="") {
            auto it = parentsList.begin();
            it = parentsList.insert(it, name.toStdString());
        } else {
            break;
        }
        current = current.parent();
    }

    if (parentsList.size()!=teachSortOrder.size()) {
        teachClickedName = clickedName;
        std::vector<std::string> sortOrder(teachSortOrder.begin(), teachSortOrder.begin()+parentsList.size()+1);
        std::vector<std::pair <std::string, int>> list =
                teachdb->getCountTuple(yearStart, yearEnd, sortOrder, parentsList, getFilterStartChar(TEACH), getFilterEndChar(TEACH));
        std::vector<std::pair <std::string, double>> chartList;
        for (int i = 0; i < (int) list.size(); i++) {
            chartList.emplace_back(list[i].first, static_cast<double>(list[i].second));
        }

        if (!chartList.empty()) {
            ui->teachBarChart->clearPlottables();
            MainWindow::setupBarChart(ui->teachBarChart, chartList);
            ui->teachBarChart->replot();

            ui->teachLineChart->clearPlottables();
            MainWindow::setupLineChart(ui->teachLineChart, chartList);
            ui->teachLineChart->replot();

            ui->teachStackedBarChart->clearPlottables();
            MainWindow::setupStackedBarChart(ui->teachStackedBarChart, chartList);
            ui->teachStackedBarChart->replot();

            MainWindow::setupPieChart(ui->teachPieChart, ui->teachPieList, chartList);

            if (parentsList.size()>1) {
                ui->teachGraphTitle->setText("Total " + clickedName + " Teaching by " +
                                             QString::fromStdString(teachSortOrder[parentsList.size()]) + " for " + QString::fromStdString(parentsList[0]));
            } else {
                ui->teachGraphTitle->setText("Total Teaching by " + QString::fromStdString(parentsList[0]));
            }
            ui->teach_graph_stackedWidget->show();
        }
    } else {
        ui->teach_graph_stackedWidget->hide();
        ui->teachGraphTitle->clear();
        teachClickedName.clear();
    }
}

void MainWindow::on_pubTreeView_clicked(const QModelIndex &index) {
    QString clickedName = index.data(Qt::DisplayRole).toString();
    if (clickedName==pubClickedName) { return;}
    ui->pubPrintButton->setEnabled(true);
    ui->pubExportButton->setEnabled(true);
    std::vector<std::string> parentsList;
    QModelIndex current = index.sibling(index.row(), 0);
    QString name;
    while (true) {
        name = current.data(Qt::DisplayRole).toString();
        if(name!="") {
            auto it = parentsList.begin();
            it = parentsList.insert(it, name.toStdString());
        } else {
            break;
        }
        current = current.parent();
    }

    if (parentsList.size()!=pubSortOrder.size()) {
        pubClickedName = clickedName;
        std::vector<std::string> sortOrder(pubSortOrder.begin(), pubSortOrder.begin()+parentsList.size()+1);
        std::vector<std::pair <std::string, int>> list =
                pubdb->getCountTuple(yearStart, yearEnd, sortOrder, parentsList, getFilterStartChar(PUBLICATIONS), getFilterEndChar(PUBLICATIONS));
        std::vector<std::pair <std::string, double>> chartList;
        for (int i = 0; i < (int) list.size(); i++) {
            chartList.emplace_back(list[i].first, static_cast<double>(list[i].second));
        }

        if (!chartList.empty()) {
            ui->pubBarChart->clearPlottables();
            MainWindow::setupBarChart(ui->pubBarChart, chartList);
            ui->pubBarChart->replot();

            ui->pubLineChart->clearPlottables();
            MainWindow::setupLineChart(ui->pubLineChart, chartList);
            ui->pubLineChart->replot();

            ui->pubStackedBarChart->clearPlottables();
            MainWindow::setupStackedBarChart(ui->pubStackedBarChart, chartList);
            ui->pubStackedBarChart->replot();

            MainWindow::setupPieChart(ui->pubPieChart, ui->pubPieList, chartList);

            if (parentsList.size()>1) {
                ui->pubGraphTitle->setText("Total " + clickedName + " Publications by " +
                                           QString::fromStdString(pubSortOrder[parentsList.size()]) + " for " + QString::fromStdString(parentsList[0]));
            } else {
                ui->pubGraphTitle->setText("Total Publications by " + QString::fromStdString(parentsList[0]));
            }
            ui->pub_graph_stackedWidget->show();
        }
    } else {
        ui->pub_graph_stackedWidget->hide();
        ui->pubGraphTitle->clear();
        pubClickedName.clear();
    }
}

void MainWindow::on_presTreeView_clicked(const QModelIndex &index) {
    QString clickedName = index.data(Qt::DisplayRole).toString();
    if (clickedName==presClickedName) { return;}
    ui->presPrintButton->setEnabled(true);
    ui->presExportButton->setEnabled(true);
    std::vector<std::string> parentsList;
    QModelIndex current = index.sibling(index.row(), 0);
    QString name;
    while (true) {
        name = current.data(Qt::DisplayRole).toString();
        if(name!="") {
            auto it = parentsList.begin();
            it = parentsList.insert(it, name.toStdString());
        } else {
            break;
        }
        current = current.parent();
    }

    if (parentsList.size()!=presSortOrder.size()) {
        presClickedName = clickedName;
        std::vector<std::string> sortOrder(presSortOrder.begin(), presSortOrder.begin()+parentsList.size()+1);
        std::vector<std::pair <std::string, int>> list =
                presdb->getCountTuple(yearStart, yearEnd, sortOrder, parentsList, getFilterStartChar(PRESENTATIONS), getFilterEndChar(PRESENTATIONS));
        std::vector<std::pair <std::string, double>> chartList;
        for (int i = 0; i < (int) list.size(); i++) {
            chartList.emplace_back(list[i].first, static_cast<double>(list[i].second));
        }

        if (!chartList.empty()) {
            ui->presBarChart->clearPlottables();
            MainWindow::setupBarChart(ui->presBarChart, chartList);
            ui->presBarChart->replot();

            ui->presLineChart->clearPlottables();
            MainWindow::setupLineChart(ui->presLineChart, chartList);
            ui->presLineChart->replot();

            ui->presStackedBarChart->clearPlottables();
            MainWindow::setupStackedBarChart(ui->presStackedBarChart, chartList);
            ui->presStackedBarChart->replot();

            MainWindow::setupPieChart(ui->presPieChart, ui->presPieList, chartList);

            if (parentsList.size()>1) {
                ui->presGraphTitle->setText("Total " + clickedName + " Presentations by " +
                                            QString::fromStdString(presSortOrder[parentsList.size()]) + " for " + QString::fromStdString(parentsList[0]));
            } else {
                ui->presGraphTitle->setText("Total Presentations by " + QString::fromStdString(parentsList[0]));
            }
            ui->pres_graph_stackedWidget->show();
        }
    } else {
        ui->pres_graph_stackedWidget->hide();
        ui->presGraphTitle->clear();
        presClickedName.clear();
    }
}

void MainWindow::on_fundTreeView_clicked(const QModelIndex &index) {
    QString clickedName = index.data(Qt::DisplayRole).toString();
    if (clickedName==fundClickedName) { return;}
    ui->fundPrintButton->setEnabled(true);
    ui->fundExportButton->setEnabled(true);
    std::vector<std::string> parentsList;
    QModelIndex current = index.sibling(index.row(), 0);
    QString name;
    while (true) {
        name = current.data(Qt::DisplayRole).toString();
        if(name!="") {
            auto it = parentsList.begin();
            it = parentsList.insert(it, name.toStdString());
        } else {
            break;
        }
        current = current.parent();
    }

    if (parentsList.size()!=fundSortOrder.size()) {
        if (clickedName != fundClickedName) {
            fundClickedName = clickedName;
            std::vector<std::string> sortOrder(fundSortOrder.begin(), fundSortOrder.begin()+parentsList.size()+1);
            std::vector<std::pair <std::string, double>> chartList =
                    funddb->getTotalsTuple(yearStart, yearEnd, sortOrder, parentsList, "Total Amount", getFilterStartChar(FUNDING), getFilterEndChar(FUNDING));

            if (!chartList.empty()) {
                ui->fundBarChart->clearPlottables();
                MainWindow::setupBarChart(ui->fundBarChart, chartList);
                ui->fundBarChart->replot();

                ui->fundLineChart->clearPlottables();
                MainWindow::setupLineChart(ui->fundLineChart, chartList);
                ui->fundLineChart->replot();

                ui->fundStackedBarChart->clearPlottables();
                MainWindow::setupStackedBarChart(ui->fundStackedBarChart, chartList);
                ui->fundStackedBarChart->replot();

                MainWindow::setupPieChart(ui->fundPieChart, ui->fundPieList, chartList);

                if (parentsList.size()>1) {
                    ui->fundGraphTitle->setText("Total " + clickedName + " Grants & Funding by " +
                                                QString::fromStdString(fundSortOrder[parentsList.size()]) + " for " + QString::fromStdString(parentsList[0]));
                } else {
                    ui->fundGraphTitle->setText("Total Grants & Funding by " + QString::fromStdString(parentsList[0]));
                }
                ui->fund_graph_stackedWidget->show();
            }
        } else {
            ui->fund_graph_stackedWidget->hide();
            ui->fundGraphTitle->clear();
            fundClickedName.clear();
        }
    }
}

void MainWindow::on_teachPrintButton_clicked()
{
    printTeach();
}

void MainWindow::printTeach()
{
    QPrintDialog printDialog(printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        QCPPainter painter;
        painter.begin(printer);
        ui->teachChartFrame->render(&painter);
    }
}

void MainWindow::on_fundPrintButton_clicked()
{
    printGrant();
}

void MainWindow::printGrant()
{
    QPrintDialog printDialog(printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        QCPPainter painter;
        painter.begin(printer);
        ui->fundChartFrame->render(&painter);
    }
}

void MainWindow::on_presPrintButton_clicked()
{
    printPres();
}

void MainWindow::printPres()
{
    QPrintDialog printDialog(printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        QCPPainter painter;
        painter.begin(printer);
        ui->presChartFrame->render(&painter);
    }
}


void MainWindow::on_pubPrintButton_clicked()
{
    printPub();
}

void MainWindow::printPub()
{
    QPrintDialog printDialog(printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        QCPPainter painter;
        painter.begin(printer);
        ui->pubChartFrame->render(&painter);
    }
}

void MainWindow::on_teachExportButton_clicked()
{
    exportTeach();
}

void MainWindow::exportTeach() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export File"),
                                                    QDir::homePath(),
                                                    tr("PDF (*.pdf)"));
    if (fileName.contains("") != 0) {
        QPdfWriter writer(fileName);
        writer.setPageOrientation(QPageLayout::Landscape);
        QPainter painter;
        painter.begin(&writer);
        painter.scale(10.0, 10.0);
        ui->teachChartFrame->render(&painter);

        /* Another option for bar chart since it is QCustom plot
        if (ui->teach_bar_button->isChecked()) {
            ui->teachBarChart->savePdf(fileName);
        } */
    }
}

void MainWindow::on_fundExportButton_clicked()
{
    exportGrant();
}

void MainWindow::exportGrant() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export File"),
                                                    QDir::homePath(),
                                                    tr("PDF (*.pdf)"));
    if (fileName.contains("") != 0) {
        QPdfWriter writer(fileName);
        writer.setPageOrientation(QPageLayout::Landscape);
        QPainter painter;
        painter.begin(&writer);
        painter.scale(10.0, 10.0);
        ui->fundChartFrame->render(&painter);
    }
}

void MainWindow::on_presExportButton_clicked()
{
    exportPres();
}

void MainWindow::exportPres() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export File"),
                                                    QDir::homePath(),
                                                    tr("PDF (*.pdf)"));
    if (fileName.contains("") != 0) {
        QPdfWriter writer(fileName);
        writer.setPageOrientation(QPageLayout::Landscape);
        QPainter painter;
        painter.begin(&writer);
        painter.scale(10.0, 10.0);
        ui->presChartFrame->render(&painter);
    }
}

void MainWindow::on_pubExportButton_clicked()
{
    exportPub();
}

void MainWindow::exportPub() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Export File"),
                                                    QDir::homePath(),
                                                    tr("PDF (*.pdf)"));
    if (fileName.contains("") != 0) {
        QPdfWriter writer(fileName);
        writer.setPageOrientation(QPageLayout::Landscape);
        QPainter painter;
        painter.begin(&writer);
        painter.scale(10.0, 10.0);
        ui->pubChartFrame->render(&painter);
    }
}



char MainWindow::getFilterStartChar(int type) {
    char charInField;

    // get char from the text field
    switch (type) {
    case FUNDING:
        charInField = ui->fund_filter_from->text().toStdString()[0];
        break;
    case PRESENTATIONS:
        charInField = ui->pres_filter_from->text().toStdString()[0];
        break;
    case PUBLICATIONS:
        charInField = ui->pub_filter_from->text().toStdString()[0];
        break;
    case TEACH:
        charInField = ui->teach_filter_from->text().toStdString()[0];
        break;
    }

    // convert the char to uppercase
    char uppercase = charInField & ~0x20;

    // if alphabetical, return that letter
    if ('A' <= uppercase && uppercase <= 'Z') {
        return uppercase;
    } else {
        // otherwise, default is '*'
        return '*';
    }
}

char MainWindow::getFilterEndChar(int type) {
    char charInField;

    // get char from the text field
    switch (type) {
    case FUNDING:
        charInField = ui->fund_filter_to->text().toStdString()[0];
        break;
    case PRESENTATIONS:
        charInField = ui->pres_filter_to->text().toStdString()[0];
        break;
    case PUBLICATIONS:
        charInField = ui->pub_filter_to->text().toStdString()[0];
        break;
    case TEACH:
        charInField = ui->teach_filter_to->text().toStdString()[0];
        break;
    }

    // convert the char to uppercase
    char uppercase = charInField & ~0x20;

    // if alphabetical, return that letter
    if ('A' <= uppercase && uppercase <= 'Z') {
        return uppercase;
    } else if (charInField == '*') {
        // otherwise, is it '*'?
        return charInField;
    } else {
        // otherwise, default is 'Z'
        return 'Z';
    }
}

void MainWindow::on_teach_filter_from_textChanged() { refresh(TEACH);}
void MainWindow::on_teach_filter_to_textChanged() { refresh(TEACH);}
void MainWindow::on_pub_filter_from_textChanged() { refresh(PUBLICATIONS);}
void MainWindow::on_pub_filter_to_textChanged() { refresh(PUBLICATIONS);}
void MainWindow::on_pres_filter_from_textChanged() { refresh(PRESENTATIONS);}
void MainWindow::on_pres_filter_to_textChanged() { refresh(PRESENTATIONS);}
void MainWindow::on_fund_filter_from_textChanged() { refresh(FUNDING);}
void MainWindow::on_fund_filter_to_textChanged() { refresh(FUNDING);}

void MainWindow::on_teach_search_textChanged(const QString &arg1)
{
    teachFilter = arg1;
    refresh(TEACH);
}

void MainWindow::on_pub_search_textChanged(const QString &arg1)
{
    pubFilter = arg1;
    refresh(PUBLICATIONS);
}

void MainWindow::on_pres_search_textChanged(const QString &arg1)
{
    presFilter = arg1;
    refresh(PRESENTATIONS);
}

void MainWindow::on_grant_search_textChanged(const QString &arg1)
{
    grantFilter = arg1;
    refresh(FUNDING);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls() && e->mimeData()->urls().length() == 1) {
        e->acceptProposedAction();
    } else {
        QMessageBox::critical(this, "Invalid File", "You can only drag and drop one valid csv file at a time.");
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    foreach (const QUrl &url, e->mimeData()->urls()) {
        QString path = url.toLocalFile();
        if(ui->categoryTab->currentIndex() == 0) {
            if (!path.isEmpty()) {
                load_teach(path);
            }
        } else if (ui->categoryTab->currentIndex() == 1) {
            if (!path.isEmpty()) {
                load_pub(path);
            }
        } else if (ui->categoryTab->currentIndex() == 2) {
            if (!path.isEmpty()) {
                load_pres(path);
            }
        } else {
            if (!path.isEmpty()) {
                load_fund(path);
            }
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveProgramState();
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch ( event->key() )
        {
    case Qt::Key_Control:
        cntrlPressed = true;
        break;

    case Qt::Key_P:
        if (cntrlPressed) {
            if (ui->categoryTab->currentIndex() == 0) {
                printTeach();
            } else if (ui->categoryTab->currentIndex() == 1) {
                printPub();
            } else if (ui->categoryTab->currentIndex() == 2) {
                printPres();
            } else {
                printGrant();
            }
        }
        break;

    case Qt::Key_E:
        if (cntrlPressed) {
            if (ui->categoryTab->currentIndex() == 0) {
                exportTeach();
            } else if (ui->categoryTab->currentIndex() == 1) {
                exportPub();
            } else if (ui->categoryTab->currentIndex() == 2) {
                exportPres();
            } else {
                exportGrant();
            }
        }
        break;

    case Qt::Key_L:
        if (cntrlPressed) {
            if (ui->categoryTab->currentIndex() == 0) {
                QString path = load_file();
                if (!path.isEmpty()) {
                    load_teach(path);
                }
            } else if (ui->categoryTab->currentIndex() == 1) {
                QString path = load_file();
                if (!path.isEmpty()) {
                    load_pub(path);
                }
            } else if (ui->categoryTab->currentIndex() == 2) {
                QString path = load_file();
                if (!path.isEmpty()) {
                    load_pres(path);
                }
            } else {
                QString path = load_file();
                if (!path.isEmpty()) {
                    load_fund(path);
                }
            }
        }
        break;

    case Qt::Key_1:
        if (cntrlPressed) {
            ui->categoryTab->setCurrentIndex(0);
        }
        break;

    case Qt::Key_2:
        if (cntrlPressed) {
            ui->categoryTab->setCurrentIndex(1);
        }
        break;

    case Qt::Key_3:
        if (cntrlPressed) {
            ui->categoryTab->setCurrentIndex(2);
        }
        break;

    case Qt::Key_4:
        if (cntrlPressed) {
            ui->categoryTab->setCurrentIndex(3);
        }
        break;




        }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    switch ( event->key() )
        {
        case Qt::Key_Control:
            cntrlPressed = false;
            break;
        }
}

bool MainWindow::saveProgramState()
{
    QFile file("state.dat");
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);

    int currTab = ui->categoryTab->currentIndex();

    out << currTab;

    if (currTab == 0) {
        out << lastTeachFile;
    } else if (currTab == 1) {
        out << lastPubFile;
    } else if (currTab == 2) {
        out << lastPresFile;
    } else {
        out << lastGrantFile;
    }
    return true;
}


