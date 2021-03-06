/*************************************************************************************
 *  Copyright (C) 2010-2012 by Percy Camilo T. Aucahuasi <percy.camilo.ta@gmail.com> *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#include "mainwindow.h"

#include <analitzaplot/plotsdictionarymodel.h>
#include <analitzaplot/planecurve.h>
#include <analitzaplot/functiongraph.h>
#include <analitzagui/plotsview2d.h>
#include <analitzagui/plotsview3d.h>
#include <dictionaryitem.h>
#include <analitza/expression.h>
#include <analitzaplot/plotsmodel.h>
#include <KDE/KApplication>
#include <QBuffer>
#include <QtGui/QDockWidget>
#include <QtGui/QLayout>
#include <QLineEdit>
#include <qpushbutton.h>
#include <QToolButton>
#include <QDebug>
#include <QFileDialog>
#include <KDE/KLocale>
#include <KDE/KLocalizedString>
#include <KDE/KStandardDirs>
#include <KDE/KAction>
#include <KDE/KActionCollection>
#include <KDE/KStandardAction>
#include <KDE/KStatusBar>
#include <KDE/KFileDialog>
#include <KDE/KMessageBox>
#include <KIO/NetAccess>
#include <KDE/KMessageBox>
#include <KDE/KStandardDirs>
#include <KDE/KToolInvocation>
#include <KToolBar>
#include "dictionariesmodel.h"
#include <KMenuBar>
#include "dashboard.h"
#include "document.h"
#include "plotseditor.h"
#include "datastore.h"
#include "plotsbuilder.h"
#include "spaceinformation.h"
#include "spaceoptions.h"
#include "filter.h"
#include <qjson/serializer.h>
#include <qjson/parser.h>

MainWindow::MainWindow(QWidget *parent)
    : KXmlGuiWindow(parent)
{
    m_document = new DataStore(this);

    m_dashboard = new Dashboard(this);
    m_dashboard->setDocument(m_document);
    
    //para main de dashboard
    connect(m_dashboard, SIGNAL(spaceActivated(int)), SLOT(activateSpace(int)));
    
    //para document de dashboard
    connect(m_dashboard, SIGNAL(spaceActivated(int)), m_document , SIGNAL(spaceActivated(int)));
    
    setupDocks();
    setupActions();
    setupGUI(Keys | StatusBar | Save | Create, "khipu.rc");

    m_filter = new Filter(this);
    
    connect(m_filter, SIGNAL(filterByDimension(Dimensions)), m_dashboard, SLOT(filterByDimension(Dimensions)));
    connect(m_filter, SIGNAL(filterByText(QString)), m_dashboard, SLOT(filterByText(QString)));
    
    toolBar("mainToolBar")->addWidget(m_filter);

    setCentralWidget(m_dashboard);
    setupToolBars();
    activateDashboardUi();
    
    updateTittleWhenOpenSaveDoc();
}

MainWindow::~MainWindow()
{
}

KAction* MainWindow::createAction(const char* name, const QString& text, const QString& iconName, const QKeySequence& shortcut, const QObject* recvr, const char* slot, bool isCheckable, bool checked)
{
    KAction* act = new KAction(this);
    act->setText(text);
    act->setIcon(KIcon(iconName));
    act->setShortcut(shortcut);
    act->setCheckable(isCheckable);
    
    if (isCheckable) 
    {
        act->setChecked(checked);
        
        QObject::connect(act, SIGNAL(toggled(bool)), recvr, slot);
    }
    else
        QObject::connect(act, SIGNAL(triggered()), recvr, slot);

        
    actionCollection()->addAction(name, act);

    return act;
}

void MainWindow::setupDocks()
{
    PlotsBuilder *plotsBuilder = new PlotsBuilder(this);
    m_plotsBuilderDock = new QDockWidget(i18n("Shortcuts"), this);
    m_plotsBuilderDock->setWidget(plotsBuilder); // plotsbuilder debe ser miembro
    m_plotsBuilderDock->setObjectName("dsfs");
//     m_plotsBuilderDock->setFloating(false);
    m_plotsBuilderDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_plotsBuilderDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    m_plotsBuilderDock->hide();

    plotsBuilder->mapConnection(PlotsBuilder::CartesianGraphCurve, this, SLOT(buildCartesianGraphCurve()));
    plotsBuilder->mapConnection(PlotsBuilder::CartesianImplicitCurve, this, SLOT(buildCartesianImplicitCurve()));
    plotsBuilder->mapConnection(PlotsBuilder::CartesianParametricCurve2D, this, SLOT(buildCartesianParametricCurve2D()));
    plotsBuilder->mapConnection(PlotsBuilder::PolarGraphCurve, this, SLOT(buildPolarGraphCurve()));
    plotsBuilder->mapConnection(PlotsBuilder::CartesianParametricCurve3D, this, SLOT(buildCartesianParametricCurve3D()));
    plotsBuilder->mapConnection(PlotsBuilder::CartesianGraphSurface, this, SLOT(buildCartesianGraphSurface()));
    plotsBuilder->mapConnection(PlotsBuilder::CartesianImplicitSurface, this, SLOT(buildCartesianImplicitSurface()));
    plotsBuilder->mapConnection(PlotsBuilder::CartesianParametricSurface, this, SLOT(buildCartesianParametricSurface()));
    plotsBuilder->mapConnection(PlotsBuilder::CylindricalGraphSurface, this, SLOT(buildCylindricalGraphSurface()));
    plotsBuilder->mapConnection(PlotsBuilder::SphericalGraphSurface, this, SLOT(buildSphericalGraphSurface()));
    
    ///
    
    m_spacePlotsDock = new PlotsEditor(this);
    m_spacePlotsDock->setDocument(m_document);
//     m_spacePlotsDock->setFloating(false);
    m_spacePlotsDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_spacePlotsDock->setFeatures(QDockWidget::DockWidgetClosable | QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);

    connect(m_spacePlotsDock, SIGNAL(goHome()), SLOT(goHome()));
    connect(m_spacePlotsDock, SIGNAL(sendStatus(QString,int)), statusBar(),SLOT(showMessage(QString,int)));
    
    connect(m_dashboard, SIGNAL(spaceActivated(int)), m_spacePlotsDock, SLOT(setCurrentSpace(int)));
    
    m_spaceInfoDock = new SpaceInformation(this);
    
    m_spaceOptionsDock = new SpaceOptions(this);
    connect(m_document, SIGNAL(gridStyleChanged(int)), m_spaceOptionsDock, SLOT(setGridStyleIndex(int)));
    //2d view
    connect(m_spaceOptionsDock, SIGNAL(updateGridStyle(int)), m_dashboard->view2d(), SLOT(useCoorSys(int)));
    connect(m_spaceOptionsDock, SIGNAL(updateGridColor(QColor)), m_dashboard->view2d(), SLOT(updateGridColor(QColor)));
    connect(m_spaceOptionsDock, SIGNAL(setXAxisLabel(QString)), m_dashboard->view2d(), SLOT(setXAxisLabel(QString)));
    connect(m_spaceOptionsDock, SIGNAL(setYAxisLabel(QString)), m_dashboard->view2d(), SLOT(setYAxisLabel(QString)));
    connect(m_spaceOptionsDock, SIGNAL(updateTickScale(QString,qreal,int,int)), m_dashboard->view2d(), SLOT(updateTickScale(QString,qreal,int,int)));
    connect(m_spaceOptionsDock, SIGNAL(ticksShown(QFlags<Qt::Orientation>)), m_dashboard->view2d(), SLOT(setTicksShown(QFlags<Qt::Orientation>)));
    connect(m_spaceOptionsDock, SIGNAL(axesShown(QFlags<Qt::Orientation>)), m_dashboard->view2d(), SLOT(setAxesShown(QFlags<Qt::Orientation>)));
    //3d view
    connect(m_spaceOptionsDock, SIGNAL(axisIsDrawn(bool)), m_dashboard->view3d(), SLOT(setAxisIsDrawn(bool)));
    connect(m_spaceOptionsDock, SIGNAL(gridIsDrawn(bool)), m_dashboard->view3d(), SLOT(setGridIsDrawn(bool)));
    connect(m_spaceOptionsDock, SIGNAL(sceneResized(int)), m_dashboard->view3d(), SLOT(resizeScene(int)));

    addDockWidget(Qt::LeftDockWidgetArea, m_plotsBuilderDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_spacePlotsDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_spaceOptionsDock);
    addDockWidget(Qt::LeftDockWidgetArea, m_spaceInfoDock);
    
    tabifyDockWidget(m_plotsBuilderDock, m_spacePlotsDock);
    tabifyDockWidget(m_spacePlotsDock, m_spaceOptionsDock);
    tabifyDockWidget(m_spaceOptionsDock, m_spaceInfoDock);
}

void MainWindow::setupActions()
{
    //file
    KStandardAction::openNew(this, SLOT(newFile()), actionCollection());
    KStandardAction::open(this, SLOT(openFile()), actionCollection());
    KStandardAction::openRecent(this, SLOT(fooSlot()), actionCollection());    
    KStandardAction::save(this, SLOT(saveFile()), actionCollection());
    KStandardAction::saveAs(this, SLOT(fooSlot()), actionCollection());
    KStandardAction::close(this, SLOT(close()), actionCollection());
    KStandardAction::quit(this, SLOT(close()), actionCollection());
    //TODO
//     KStandardAction::showMenubar(menuBar(), SLOT(setVisible(bool)), actionCollection());

    //edit - dashboard
    createAction("add_space2d", i18n("&Add Space 2D"), "add-space2d", Qt::CTRL + Qt::Key_W, this, SLOT(addSpace2D()));
    createAction("add_space3d", i18n("&Add Space 3D"), "add-space3d", Qt::CTRL + Qt::Key_W, this, SLOT(addSpace3D()));
    createAction("add_random_plot", i18n("&Add Random Plot"), "roll", Qt::CTRL + Qt::Key_W, this, SLOT(fooSlot()));
    //view - dashboard //TODO Show Plots Dictionary
    m_plotsBuilderDock->toggleViewAction()->setIcon(KIcon("formula"));
    m_plotsBuilderDock->toggleViewAction()->setShortcut(Qt::CTRL + Qt::Key_W);
    m_plotsBuilderDock->toggleViewAction()->setToolTip(i18n("Create a plot in a new space"));
    actionCollection()->addAction("show_plotsbuilder", m_plotsBuilderDock->toggleViewAction());

    createAction("show_plots", i18n("&Show Plots"), "view-list-details", Qt::CTRL + Qt::Key_W, this, SLOT(fooSlot()));
    createAction("show_spaces", i18n("&Show Spaces"), "view-list-icons", Qt::CTRL + Qt::Key_W, this, SLOT(fooSlot()));
    createAction("show_plotsdictionary", i18n("&Mathematical Objects"), "functionhelp", Qt::CTRL + Qt::Key_W, this, 
                 SLOT(setVisibleDictionary()));

    //view - space
//     createAction("show_plots_editor", i18n("S&how Space Plots"), "address-book-new", Qt::CTRL + Qt::Key_W, this, SLOT(fooSlot()), true);
    m_spacePlotsDock->toggleViewAction()->setIcon(KIcon("editplots"));
    m_spacePlotsDock->toggleViewAction()->setShortcut(Qt::CTRL + Qt::Key_W);
    actionCollection()->addAction("show_plots_editor", m_spacePlotsDock->toggleViewAction());    
    
//     createAction("show_space_info", i18n("&Show Space Information"), "document-properties", Qt::CTRL + Qt::Key_W, this, SLOT(fooSlot()), true);
    m_spaceInfoDock->toggleViewAction()->setIcon(KIcon("dialog-information"));
    m_spaceInfoDock->toggleViewAction()->setShortcut(Qt::CTRL + Qt::Key_W);
    actionCollection()->addAction("show_space_info", m_spaceInfoDock->toggleViewAction());       
    
//     createAction("show_plotter_options", i18n("&Show Space Options"), "configure", Qt::CTRL + Qt::Key_W, this, SLOT(fooSlot()), true);
    m_spaceOptionsDock->toggleViewAction()->setIcon(KIcon("coords"));
    m_spaceOptionsDock->toggleViewAction()->setShortcut(Qt::CTRL + Qt::Key_W);
    actionCollection()->addAction("show_plotter_options", m_spaceOptionsDock->toggleViewAction());       
    
    
    //go
    KAction *act = KStandardAction::firstPage(this, SLOT(fooSlot()), actionCollection());
    act->setText(i18n("&Go First Space"));
    act->setIcon(KIcon("go-first-view"));
    act->setEnabled(false);

    act = KStandardAction::prior(this, SLOT(fooSlot()), actionCollection());
    act->setText(i18n("&Go Previous Space"));
    act->setIcon(KIcon("go-previous-view"));
    act->setEnabled(false);

    act = KStandardAction::next(this, SLOT(fooSlot()), actionCollection());
    act->setText(i18n("&Go Next Space"));
    act->setIcon(KIcon("go-next-view"));
    act->setEnabled(false);
    
    act = KStandardAction::lastPage(this, SLOT(fooSlot()), actionCollection());
    act->setText(i18n("&Go Last Space"));
    act->setIcon(KIcon("go-last-view"));
    act->setEnabled(false);

    KStandardAction::home(this, SLOT(goHome()), actionCollection());
    //tools dashboard
    createAction("delete_currentspace", i18n("&Remove Current Space"), "list-remove", Qt::CTRL + Qt::Key_W, this, SLOT(removeCurrentSpace()))->setVisible(false);;
    //tools space
    createAction("copy_snapshot", i18n("&Take Snapshot"), "edit-copy", Qt::CTRL + Qt::Key_W, this, SLOT(copySnapshot()));
//     createAction("export_snapshot", i18n("&Export Space Snapshot"), "view-preview", Qt::CTRL + Qt::Key_W, this, SLOT(fooSlot()));
    //settings
    KStandardAction::showMenubar(this, SLOT(fooSlot()), actionCollection());
    KStandardAction::fullScreen(this, SLOT(fooSlot()), this ,actionCollection());

//     connect(m_dashboard, SIGNAL(saveRequest()), SLOT(saveFile()));
//     connect(m_dashboard, SIGNAL(openRequest()), SLOT(openFile()));

}

void MainWindow::fooSlot(bool t)
{
//     qDebug() << "test slot" << t;
}


void MainWindow::setupToolBars()
{
//     hideSpaceToolBar();
    
//     qDebug() << action("add_space2d")->isCheckable();
}


bool MainWindow::queryClose()
{
//     if (m_dashboard->isModified())
//     {
//         QString paletteFileName = m_dashboard->fileName();
// 
//         if (paletteFileName.isEmpty())
//             paletteFileName = i18n("Untitled");
// 
//         switch (KMessageBox::warningYesNoCancel(this,
//                                                 i18n( "The document \"%1\" has been modified.\n"
//                                                         "Do you want to save your changes or discard them?", paletteFileName),
//                                                 i18n( "Close Document" ), KStandardGuiItem::save(), KStandardGuiItem::discard()))
//         {
//         case KMessageBox::Yes:
//         {
// 
// 
// 
// 
// 
//             m_dashboard->showDashboard();
// 
//             saveFile();
// 
//             return m_dashboard->isSaved();
//         }
//         case KMessageBox::No :
//             return true;
// 
//         default :
//             return false;
//         }
//     }

    return true;
}

void MainWindow::newFile()
{
  //  if(modified && asked for save)
    //KToolInvocation::kdeinitExec("khipu");
    QVariantList people;

    QVariantMap bob;
    bob.insert("Name", "Bob");
    bob.insert("Phonenumber", 123);

    QVariantMap alice;
    alice.insert("Name", "Alice");
    alice.insert("Phonenumber", 321);

    people << bob << alice;

    QJson::Serializer serializer;
  //  bool ok;
    QByteArray json = serializer.serialize(people);

  //  if (ok) {
      qDebug() << json;
  //  } else {
  //    qCritical() << "Something went wrong:";
   // }
      qDebug() << "parsing.....";

      QJson::Parser parser;
     // bool ok;

      QVariantList result = parser.parse(json).toList();
      foreach(QVariant record, result) {
          QVariantMap map = record.toMap();
          qDebug() << map.value("Name").toString() << "\t" << map.value("Phonenumber").toInt();
      }
    //  qDebug() << result.value("Name").toString() << " and " << result["Phonenumber"].toInt();
     // if (!ok) {
      //  qFatal("An error occurred during parsing");
      //  exit (1);
     // }

      //qDebug() << "1st:" << result["Name"].toString();
  //    qDebug() << "plugins:";

      /*foreach (QVariant plugin, result["plug-ins"].toList()) {
        qDebug() << "\t-" << plugin.toString();
      }
*/
  //    QVariantMap nestedMap = result["indent"].toMap();
   //   qDebug() << "length:" << nestedMap["length"].toInt();
    //  qDebug() << "use_space:" << nestedMap["use_space"].toBool();

}

void MainWindow::openFile()
{ // temp data
// Replacement of qDebugs by writing into the file

    QMap<DictionaryItem*, Analitza::PlotItem*> map=m_document->currentDataMap();
    if(map.empty())
    {
        qDebug() << "map is empty";
        // just starting #no plot is available so no need to save
          return;
    }
    QList<DictionaryItem*> spaceList=map.uniqueKeys();
    if(spaceList.empty()){
            qDebug() << "list is empty";
            return;
    }
    if(m_savedSpaces > 0){ // home is clicked or not.!
    // do this for the whole list and write at that time into a file
    int i,j;
    for(i=0;i<spaceList.size();i++){
             DictionaryItem* space=spaceList.at(i);
             QString title = space->title();
             QString descr = space->description();
             Analitza::Dimension dim = space->dimension();
             qDebug() << title;
             qDebug() << descr;
             qDebug() << dim;

             /*QMap<QString, int>::const_iterator i = map.find("HDR");
             while (i != map.end() && i.key() == "HDR") {
                 cout << i.value() << endl;
                 ++i;
             }*/
             qDebug() << "coming";
       /*      QMap<DictionaryItem*, Analitza::PlotItem*>::const_iterator j = map.find(space);
             while (j != map.end() && j.key() == space) {
                 qDebug() << j.value();
                 ++j;
             }
       */      QList<Analitza::PlotItem*> plotList=map.values(space);
             for (j=0;j<plotList.size();j++) {
             Analitza::PlotItem* plotitem=plotList.at(j);
      //       Analitza::PlotItem* plotitem =
             if(plotitem==0)qDebug() << "null";
             else qDebug() << "not null";
             QString name=plotitem->name();
             QString expression=plotitem->expression().toString();
             qDebug() << name;
             qDebug() << expression;
             }
        }
    }

    if(m_totalSpaces > m_savedSpaces) {  // if addspaces button is hit more than home
    // need to save the current space
        qDebug() << "correct approach";
       QString activeSpaceTitle=m_spaceInfoDock->title();
        QString activeSpacedescr=m_spaceInfoDock->description();
        Analitza::PlotsModel* plotsmodel=m_document->plotsModel();
        qDebug() << activeSpaceTitle;
        qDebug() << activeSpacedescr;
        qDebug() << m_currentSpaceDim;

        // iterate through the whole model (two columns are there)
        int nRows= plotsmodel->rowCount();
        int i,j;
        for (i=0;i<nRows;i++){

                QString plotname=plotsmodel->data(plotsmodel->index(i,0)).toString();
                QString plotequation=plotsmodel->data(plotsmodel->index(i,1)).toString();
                    if(plotequation==""){
                          qDebug() << "empty eqaution";
                          return;
                     }
                qDebug() << plotname ;
                qDebug() << plotequation;
        }
     }

return;
}
void MainWindow::saveFile() {
    qDebug() << "working..";

    /*QVariantList people;
    QVariantMap bob;
    bob.insert("Name", "Bob");
    bob.insert("Phonenumber", 123);
    QVariantMap alice;
    alice.insert("Name", "Alice");
    alice.insert("Phonenumber", 321);
    people << bob << alice;
    */
    QMap<DictionaryItem*, Analitza::PlotItem*> map=m_document->currentDataMap();
    if(map.empty())
    {
        qDebug() << "map is empty";
        // just starting #no plot is available so no need to save
        return;
    }
    QList<DictionaryItem*> spaceList=map.uniqueKeys();
    if(spaceList.empty()){
            qDebug() << "list is empty";
            return;
    }

    QVariantList plotspace_list;
////////////
    if(m_savedSpaces > 0){ // home is clicked or not.!
    // do this for the whole list and write at that time into a file

///////
    int i,j;
    for(i=0;i<m_savedSpaces;i++){
        DictionaryItem* space=spaceList.at(i);
        QString spaceName = space->title();
        QString spaceDesc = space->description();
        int dim = space->dimension();

   /* qDebug() << title;
    qDebug() << descr;
    qDebug() << dim;
*/

        QVariantList subplot_list;
        subplot_list.clear();

        QVariantMap plotspace;
        plotspace.insert("name",spaceName);
        plotspace.insert("description",spaceDesc);
        plotspace.insert("dimension",dim);
        plotspace.insert("image",imageList.at(i));

        QList<Analitza::PlotItem*> plotList=map.values(space);
                   for (j=0;j<plotList.size();j++) {
                   Analitza::PlotItem* plotitem=plotList.at(j);
                   if(plotitem==0)qDebug() << "null";
                   else qDebug() << "not null";
                   QString plotName=plotitem->name();
                   QString plotExpression=plotitem->expression().toString();
                   QVariantMap plot;
                   plot.insert("name",plotName);
                   plot.insert("expression",plotExpression);
                   if(dim==2){
                    Analitza::FunctionGraph*functiongraph=static_cast<Analitza::FunctionGraph*> (plotList.at(j));
                    double arg1min=functiongraph->interval(functiongraph->parameters().at(0)).first;
                    double arg1max=functiongraph->interval(functiongraph->parameters().at(0)).second;
                    plot.insert("arg1min",arg1min);
                    plot.insert("arg1max",arg1max);
                   }
               /*    qDebug() << name;
                   qDebug() << expression;
                 */
                   //writehere
                   subplot_list << plot;
                   }
                   QJson::Serializer subserializer;
                   QByteArray subjson = subserializer.serialize(subplot_list);

                   plotspace.insert("plots",subjson);
                   plotspace_list << plotspace;
              }
        }


 ///////////////////////////////////////
          if(m_totalSpaces > m_savedSpaces) {  // if addspaces button is hit more than home
          // need to save the current space
            //  qDebug() << "correct approach";
              DictionaryItem* space=spaceList.at(m_totalSpaces-1); // use this just to get the plots because it does not contain correct space information
              int dim = space->dimension();
              QString activeSpaceTitle=m_spaceInfoDock->title();
              QString activeSpacedescr=m_spaceInfoDock->description();
     //         Analitza::PlotsModel* plotsmodel=m_document->plotsModel();
             // qDebug() << activeSpaceTitle;
             // qDebug() << activeSpacedescr;
             // qDebug() << m_currentSpaceDim;

              QVariantList subplot_list;
              subplot_list.clear();

              QVariantMap plotspace;
              plotspace.insert("name",activeSpaceTitle);
              plotspace.insert("description",activeSpacedescr);
              plotspace.insert("dimension",dim);

              // to save the current space thumbnail....
              QPixmap thumbnail;
              switch (space->dimension())
              {
                  case Analitza::Dim2D:
                      thumbnail = QPixmap::grabWidget(m_dashboard->view2d());
                      break;
                  case Analitza::Dim3D:
                  {
                      m_dashboard->view3d()->updateGL();
                      m_dashboard->view3d()->setFocus();
                      m_dashboard->view3d()->makeCurrent();
                      m_dashboard->view3d()->raise();

                      QImage image(m_dashboard->view3d()->grabFrameBuffer(true));

                      thumbnail = QPixmap::fromImage(image, Qt::ColorOnly);

                      break;
                  }
              }
              QString imageInText=thumbnailtoString(&thumbnail);
              plotspace.insert("image",imageInText);


              QList<Analitza::PlotItem*> plotList=map.values(space);
                         for (int j=0;j<plotList.size();j++) {
                         Analitza::PlotItem* plotitem=plotList.at(j);
                         if(plotitem==0)qDebug() << "null";
                         else qDebug() << "not null";
                         QString plotName=plotitem->name();
                         QString plotExpression=plotitem->expression().toString();
                         QVariantMap plot;
                         plot.insert("name",plotName);
                         plot.insert("expression",plotExpression);
                         if(dim==2){
                          Analitza::FunctionGraph*functiongraph=static_cast<Analitza::FunctionGraph*> (plotList.at(j));
                          double arg1min=functiongraph->interval(functiongraph->parameters().at(0)).first;
                          double arg1max=functiongraph->interval(functiongraph->parameters().at(0)).second;
                          plot.insert("arg1min",arg1min);
                          plot.insert("arg1max",arg1max);
                         }
                         /*    qDebug() << name;
                         qDebug() << expression;
                       */
                         //writehere

                         subplot_list << plot;
                         }

              // iterate through the whole model (two columns are there)
            /*  int nRows= plotsmodel->rowCount();
              int i,j;
              for (i=0;i<nRows;i++){

                      QString plotname=plotsmodel->data(plotsmodel->index(i,0)).toString();
                      QString plotequation=plotsmodel->data(plotsmodel->index(i,1)).toString();
                          if(plotequation==""){
                                qDebug() << "empty eqaution";
                                return;
                           }
                      qDebug() << plotname ;
                      qDebug() << plotequation;

                      QVariantMap plot;
                      plot.insert("name",plotname);
                      plot.insert("expression",plotequation);

                      subplot_list << plot;
              }
        */
              QJson::Serializer subserializer;
              QByteArray subjson = subserializer.serialize(subplot_list);

              plotspace.insert("plots",subjson);
              plotspace_list << plotspace;
           }

/////////////////////////////////////////////



          QJson::Serializer serializer;
          QByteArray json = serializer.serialize(plotspace_list);
         // qDebug() << json;
          QString path;
          path = QFileDialog::getSaveFileName(this, tr("Save File (Please save with extension .khipu) "),"/");
          if(path==""){
              qDebug() << "error in saving file...may be path not found." ;
              return;
          }
          qDebug() << "path: " << path;
          QFile *file = new QFile(path,this);
          if(!file->open(QFile::WriteOnly | QFile::Text)){
              qDebug() << "Error in writing";
              exit(0);
          }
          QTextStream out(file);
          out << json;
          file->close();




/*



     /////////////////////////////////////////////////////////////////
    QVariantList plotspace_list;
    QVariantList subplot_list;

    QVariantMap plotspace;
    plotspace.insert("name",spaceName);
    plotspace.insert("description",spaceDesc);

    QVariantMap plot;
    plot.insert("name",plotName);
    plot.insert("expression",plotExpression);

    subplot_list << plot;
    QJson::Serializer subserializer;
    QByteArray subjson = subserializer.serialize(subplot_list);

    plotspace.insert("plots",subjson);
    //qDebug() << subjson;
    plotspace_list << plotspace;
    QJson::Serializer serializer;
    QByteArray json = serializer.serialize(plotspace_list);
   // qDebug() << json;
    out << json;
      file->close();


      //////////////////  Opening a file and parsing //////////////////


      if(!file->open(QFile::ReadOnly)){
          qDebug() << "error in reading";
          exit(0);
   }

    qDebug() << "parsing....";
    QJson::Parser parser;
    QVariantList result = parser.parse(file).toList();
    foreach(QVariant record, result) {
        QVariantMap map = record.toMap();
        qDebug() << map.value("name").toString() << "\t" << map.value("description").toString();
        QVariantList sublist= parser.parse(map.value("plots").toByteArray()).toList();
        qDebug() << "plots data";
        foreach(QVariant subrecord, sublist) {
            QVariantMap submap = subrecord.toMap();
            qDebug() << submap.value("name").toString() << "\t" << submap.value("expression").toString();
    }
  }
*/
}

void MainWindow::activateSpace(int spaceidx)
{
    activateSpaceUi();
    
    m_spacePlotsDock->reset(true);
    
    //clear space infor widget 
//     m_spaceInfoDock->clear();
    DictionaryItem *space = m_document->spacesModel()->space(spaceidx);
    m_spaceInfoDock->setInformation(space->title(), space->description());
    
    m_spaceOptionsDock->setDimension(space->dimension());
}

void MainWindow::activateDashboardUi()
{
    //menubar
    //edit
    action("add_space2d")->setVisible(true);
    action("add_space3d")->setVisible(true);
    
    if (m_document->spacesModel()->rowCount()>0)
        action("delete_currentspace")->setVisible(true);        
    
    action("add_random_plot")->setVisible(false);
    //view
    action("show_plotsbuilder")->setVisible(true);
    action("show_plots")->setVisible(true);
    action("show_spaces")->setVisible(true);
    action("show_plotsdictionary")->setVisible(true);    
    action("show_plots_editor")->setVisible(false);
    action("show_space_info")->setVisible(false);
    action("show_plotter_options")->setVisible(false);
    //go
    action("go_home")->setVisible(false);
    //tools
    action("copy_snapshot")->setVisible(false);
//     action("export_snapshot")->setVisible(false);
    
    //toolbars
//     toolBar("mainToolBar")->show();
//     toolBar("spaceToolBar")->hide();

    //docks
    // primero oculto los widgets sino el size de los que voy a ocultar interfieren y la mainwnd se muestra muy grande
    m_spacePlotsDock->hide();
    m_spaceInfoDock->hide();
    m_spaceOptionsDock->hide();
    m_plotsBuilderDock->hide(); 
}

void MainWindow::activateSpaceUi()
{
    m_dashboard->setCurrentIndex(1);

    //menu
    //edit
    action("add_space2d")->setVisible(false);
    action("add_space3d")->setVisible(false);
    action("delete_currentspace")->setVisible(false);        
    action("add_random_plot")->setVisible(true);
    //view
    action("show_plotsbuilder")->setVisible(false);
    action("show_plots")->setVisible(false);
    action("show_spaces")->setVisible(false);
    action("show_plotsdictionary")->setVisible(false); 
    action("show_plots_editor")->setVisible(true);
    action("show_space_info")->setVisible(true);
    action("show_plotter_options")->setVisible(true);
    //go
    action("go_home")->setVisible(true);    
    //tools
    action("copy_snapshot")->setVisible(true);
//     action("export_snapshot")->setVisible(true);
    
    //toolbars
//     toolBar("mainToolBar")->hide();
//     toolBar("spaceToolBar")->show();

    //docks
    //lo mismo ... primero hides luego show
    m_plotsBuilderDock->hide();
    m_spacePlotsDock->show();
    m_spaceInfoDock->show();
    m_spaceOptionsDock->show();
    
    ///
}

void MainWindow::copySnapshot()
{
    DictionaryItem *space = m_document->spacesModel()->space(m_document->currentSpace());

    switch (space->dimension())
    {
        case Analitza::Dim2D: m_dashboard->view2d()->snapshotToClipboard(); break;
//         case Dim3D: m_dashboard->view3d()->snapshotToClipboard(); break;
#warning port to the new plotviewer
    }
    
    statusBar()->showMessage(i18n("The diagram was copied to clipboard"), 2500);
}

void MainWindow::exportSnapShot()
{

}

void MainWindow::setVisibleDictionary()
{
    
        //menu
        //edit 
        action("add_space2d")->setVisible(false);
        action("add_space3d")->setVisible(false);
        action("delete_currentspace")->setVisible(false);        
        action("add_random_plot")->setVisible(false);
        //view
        action("show_plotsbuilder")->setVisible(false);
        action("show_plots")->setVisible(false);
        action("show_spaces")->setVisible(false);
        action("show_plots_editor")->setVisible(false);
        action("show_space_info")->setVisible(false);
        action("show_plotter_options")->setVisible(false);
        //go
        action("show_plotsdictionary")->setVisible(false); 
        action("go_home")->setVisible(true);  
        
        //tools
        action("copy_snapshot")->setVisible(false);
//         action("export_snapshot")->setVisible(false);
        
        //toolbars
//         toolBar("mainToolBar")->show();
//         toolBar("spaceToolBar")->show();

        //docks
        //lo mismo ... primero hides luego show
        m_plotsBuilderDock->hide();
        m_spacePlotsDock->hide();
        m_spaceInfoDock->hide();
        m_spaceOptionsDock->hide();
    
    m_dashboard->showDictionary();
}

void MainWindow::addSpace2D()
{
    m_currentSpaceDim=2;
    m_totalSpaces++;
    activateSpaceUi();
    m_dashboard->showPlotsView2D();
    m_document->spacesModel()->addSpace(Analitza::Dim2D, i18n("Untitled %1", m_document->spacesModel()->rowCount()+1));
}

void MainWindow::addSpace3D()
{
    m_totalSpaces++;
    activateSpaceUi();
    m_currentSpaceDim=3;
    m_dashboard->showPlotsView3D();
    m_document->spacesModel()->addSpace(Analitza::Dim3D, i18n("Untitled %1", m_document->spacesModel()->rowCount()+1));
}

void MainWindow::removeCurrentSpace()
{
    m_document->removeCurrentSpace();
    
    if (m_document->spacesModel()->rowCount() == 0)
        action("delete_currentspace")->setVisible(false);
}


//NOTE se emite cuando se regresa de un space ... aqui se debe guardar la imforacion del space
void MainWindow::goHome()
{
    m_savedSpaces++;
    if (m_dashboard->currentIndex() != 0) // si no esta en modo dashboard
    {
        ///guardando space info
        
        DictionaryItem *space = m_document->spacesModel()->space(m_document->currentSpace());

        space->stamp(); // marcamos la fecha y hora de ingreso al space
        space->setTitle(m_spaceInfoDock->title());
        space->setDescription(m_spaceInfoDock->description());
        
        QPixmap thumbnail; 

        switch (space->dimension())
        {
            case Analitza::Dim2D:
                thumbnail = QPixmap::grabWidget(m_dashboard->view2d());
                break;
            case Analitza::Dim3D:
            {
                m_dashboard->view3d()->updateGL();
                m_dashboard->view3d()->setFocus();
                m_dashboard->view3d()->makeCurrent();
                m_dashboard->view3d()->raise();
        
                QImage image(m_dashboard->view3d()->grabFrameBuffer(true));

                thumbnail = QPixmap::fromImage(image, Qt::ColorOnly);

                break;
            }
        }
        QString imageInText=thumbnailtoString(&thumbnail);
        imageList.append(imageInText);
        thumbnail = thumbnail.scaled(QSize(PreviewWidth, PreviewHeight), Qt::IgnoreAspectRatio,Qt::SmoothTransformation);   
        space->setThumbnail(thumbnail);
   }
    m_dashboard->goHome();
    activateDashboardUi();
}

QString MainWindow::thumbnailtoString(QPixmap *thumbnail){
    QImage image = thumbnail->toImage();
    QByteArray imageArray;
    QBuffer buffer(&imageArray);
    buffer.open(QIODevice::WriteOnly);
    image.save(&buffer, "PNG");
    QByteArray compressedImage = qCompress(buffer.data(), 9).toHex();
    QString str(compressedImage);
return str;
}

void MainWindow::buildCartesianGraphCurve()
{
    addSpace2D();
    m_spacePlotsDock->buildCartesianGraphCurve(true);
}

void MainWindow::buildCartesianImplicitCurve()
{
    addSpace2D();
    m_spacePlotsDock->buildCartesianImplicitCurve(true);
    
}


void MainWindow::buildCartesianParametricCurve2D()
{
    addSpace2D();
    m_spacePlotsDock->buildCartesianParametricCurve2D(true);
    
}

void MainWindow::buildPolarGraphCurve()
{
    addSpace2D();
    m_spacePlotsDock->buildPolarGraphCurve(true);
    
}


void MainWindow::buildCartesianParametricCurve3D()
{
    addSpace3D();
    m_spacePlotsDock->buildCartesianParametricCurve3D(true);
    
}


void MainWindow::buildCartesianGraphSurface()
{
    addSpace3D();
    m_spacePlotsDock->buildCartesianGraphSurface(true);
    
}


void MainWindow::buildCartesianImplicitSurface()
{
    addSpace3D();
    m_spacePlotsDock->buildCartesianImplicitSurface(true);
    
}


void MainWindow::buildCartesianParametricSurface()
{
    addSpace3D();
    m_spacePlotsDock->buildCartesianParametricSurface(true);
    
}


void MainWindow::buildCylindricalGraphSurface()
{
    addSpace3D();
    m_spacePlotsDock->buildCylindricalGraphSurface(true);
    
}


void MainWindow::buildSphericalGraphSurface()
{
    addSpace3D();
    
    m_spacePlotsDock->buildSphericalGraphSurface(true);
    
}


void MainWindow::updateTittleWhenChangeDocState()
{
//     QString paletteFileName = m_dashboard->fileName();
// 
//     if (paletteFileName.isEmpty())
//         paletteFileName = i18n("Untitled");
// 
//     setWindowTitle(QString("%1 - GPLACS " + i18n("[modificado]")).arg(paletteFileName));
}

void MainWindow::updateTittleWhenOpenSaveDoc()
{
//     QString paletteFileName = m_dashboard->fileName();
// 
//     if (paletteFileName.isEmpty())
//         paletteFileName = i18n("Untitled");
// 
//     setWindowTitle(QString("%1 - GPLACS").arg(paletteFileName));
}

