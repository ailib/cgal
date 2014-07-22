//#define CGAL_USE_BOOST_BIMAP
//#define CGAL_SDG_VERBOSE

#ifndef CGAL_SDG_VERBOSE
#define CGAL_SDG_DEBUG(a)
#else
#define CGAL_SDG_DEBUG(a) { a }
#endif

#include <fstream>
#include <vector>

// CGAL headers

#include "svd-typedefs.h"

#include <CGAL/Timer.h>

// Qt headers
#include <QtGui>
#include <QString>
#include <QActionGroup>
#include <QFileDialog>
#include <QInputDialog>
#include <QDragEnterEvent>
#include <QDropEvent>

// GraphicsView items and event filters (input classes)
#include <CGAL/Qt/GraphicsViewPolylineInput.h>
#include <CGAL/Qt/SegmentDelaunayGraphLinfGraphicsItem.h>
#include <CGAL/Constraints_loader.h>
//#include <CGAL/Qt/Converter.h>

// the two base classes
#include "ui_Segment_voronoi_2.h"
#include <CGAL/Qt/DemosMainWindow.h>

// for viewportsBbox(QGraphicsScene*)
#include <CGAL/Qt/utility.h>

typedef Rep K;
typedef SDG_2 SVD;

typedef SVD::Vertex_handle Vertex_handle;

// global variables
bool has_file_argument;
QString file_to_open;


class MainWindow :
  public CGAL::Qt::DemosMainWindow,
  public Ui::Segment_voronoi_2
{
  Q_OBJECT
  
private:  
  SVD svd; 
  QGraphicsScene scene;
  std::list<Point_2> seeds;

  CGAL::Qt::SegmentDelaunayGraphLinfGraphicsItem<SVD> * sdggi;

  CGAL::Qt::GraphicsViewPolylineInput<K> * pi;

public:
  MainWindow();

private:
  template <typename Iterator> 
  void insert_polyline(Iterator b, Iterator e)
  {
    Point_2 p, q;
    SVD::Vertex_handle vh, wh;
    Iterator it = b;
    vh = svd.insert(*it);
    p = *it;
    ++it;
    for(; it != e; ++it){
      q = *it;
      if(p != q){
        wh = svd.insert(*it);
        svd.insert(vh,wh);
        std::cout << "s " << p << " " << q << std::endl;
        vh = wh;
        p = q;
      } else {
        std::cout << "duplicate point: " << p << std::endl;
      }
    }
    emit(changed());
  }

protected slots:
 virtual void open(QString);

public slots:

  void processInput(CGAL::Object o);

  void on_actionInsertPolyline_toggled(bool checked);
  
  void on_actionClear_triggered();

  void on_actionRecenter_triggered();

  void on_actionLoadSegments_triggered();

  void loadPolygonConstraints(QString);

  void loadEdgConstraints(QString);

  void loadPoints(QString);

  void loadPointsInput(QString);
  
  void loadSitesInput(QString);

  void on_actionSaveConstraints_triggered();

  void saveConstraints(QString);


signals:
  void changed();
};


MainWindow::MainWindow()
  : DemosMainWindow()
{
  setupUi(this);

  this->graphicsView->setAcceptDrops(false);

  // Add a GraphicItem for the SVD triangulation
  sdggi = new CGAL::Qt::SegmentDelaunayGraphLinfGraphicsItem<SVD>(&svd);
  QColor segmentColor(::Qt::blue);
  QColor voronoiColor(::Qt::black);
  segmentColor.setAlpha(150);
  sdggi->setSegmentPen(segmentColor);
  sdggi->setVoronoiPen(voronoiColor);
    
  QObject::connect(this, SIGNAL(changed()),
		   sdggi, SLOT(modelChanged()));

  sdggi->setVerticesPen(QPen(Qt::red, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

  sdggi->setZValue(-1);
  scene.addItem(sdggi);

  // Setup input handlers. They get events before the scene gets them
  // and the input they generate is passed to the triangulation with 
  // the signal/slot mechanism    
  pi = new CGAL::Qt::GraphicsViewPolylineInput<K>(this, &scene, 0, true); // inputs polylines which are not closed
  QObject::connect(pi, SIGNAL(generate(CGAL::Object)),
		   this, SLOT(processInput(CGAL::Object)));
    


  // 
  // Manual handling of actions
  //
  QObject::connect(this->actionQuit, SIGNAL(triggered()), 
		   this, SLOT(close()));

  // We put mutually exclusive actions in an QActionGroup
  QActionGroup* ag = new QActionGroup(this);
  ag->addAction(this->actionInsertPolyline);

  // Check two actions 
  this->actionInsertPolyline->setChecked(true);
  this->actionShowVoronoi->setChecked(true);
  this->actionShowConstraints->setChecked(true);

  //
  // Setup the scene and the view
  //
  scene.setItemIndexMethod(QGraphicsScene::NoIndex);
  scene.setSceneRect(-100, -100, 100, 100);
  this->graphicsView->setScene(&scene);
  this->graphicsView->setMouseTracking(true);

  // Turn the vertical axis upside down
  this->graphicsView->scale(1, -1);
                                                      
  // The navigation adds zooming and translation functionality to the
  // QGraphicsView
  this->addNavigation(this->graphicsView);

  this->setupStatusBar();
  this->setupOptionsMenu();
  this->addAboutDemo(":/cgal/help/about_Segment_voronoi_2.html");
  this->addAboutCGAL();

  this->addRecentFiles(this->menuFile, this->actionQuit);
  connect(this, SIGNAL(openRecentFile(QString)),
	  this, SLOT(open(QString)));

  if (has_file_argument) {
    open(file_to_open);
    statusBar()->showMessage(file_to_open, 0);
  }
}


void
MainWindow::processInput(CGAL::Object o)
{

  std::list<Point_2> points;
  if(CGAL::assign(points, o)){
    if(points.size() == 1) {
      svd.insert(points.front());
      std::cout << "p " << points.front() << std::endl;
    }
    else {
      /*
      std::cout.precision(12);
      std::cout << points.size() << std::endl;
      for( std::list<Point_2>::iterator it =  points.begin(); it != points.end(); ++it){
	std::cout << *it << std::endl;
      }
      */
      insert_polyline(points.begin(), points.end());
    }
  }


  emit(changed());
}


/* 
 *  Qt Automatic Connections
 *  http://doc.trolltech.com/4.4/designer-using-a-component.html#automatic-connections
 * 
 *  setupUi(this) generates connections to the slots named
 *  "on_<action_name>_<signal_name>"
 */
void
MainWindow::on_actionInsertPolyline_toggled(bool checked)
{
  if(checked){
    scene.installEventFilter(pi);
  } else {
    scene.removeEventFilter(pi);
  }
}



void
MainWindow::on_actionClear_triggered()
{
  svd.clear();
  emit(changed());
}


void 
MainWindow::open(QString fileName)
{
  if(! fileName.isEmpty()){
    if(fileName.endsWith(".plg")){
      loadPolygonConstraints(fileName);
      this->addToRecentFiles(fileName);
    } else if(fileName.endsWith(".edg")){
      loadEdgConstraints(fileName);
      this->addToRecentFiles(fileName);
    } else if(fileName.endsWith(".pts")){
      loadPoints(fileName);
      this->addToRecentFiles(fileName);
    } else if(fileName.endsWith(".pin")){
      loadPointsInput(fileName);
      this->addToRecentFiles(fileName);
    } else if(fileName.endsWith(".cin")){
      loadSitesInput(fileName);
      this->addToRecentFiles(fileName);
    }    
  }
}

void
MainWindow::on_actionLoadSegments_triggered()
{
  QString fileName = 
    QFileDialog::getOpenFileName(this,
                                 tr("Open Constraint File"),
                                 ".",
                                 tr(
                                    "Cin  files (*.cin)\n"
                                    "Pin  files (*.pin)\n" 
                                    "Pts  files (*.pts)\n"
                                    "Edge files (*.edg)\n"
                                    "Poly files (*.plg)"
                                                          ));
  open(fileName);
}

void
MainWindow::loadPolygonConstraints(QString fileName)
{
  K::Point_2 p,q, first;
  SVD::Vertex_handle vp, vq, vfirst;
  std::ifstream ifs(qPrintable(fileName));
  int n;
  while(ifs >> n){
    CGAL_assertion( n > 0 );
    ifs >> first;
    p = first;
    vfirst = vp = svd.insert(p);
    n--;
    while(n--){
      ifs >> q;
      vq = svd.insert(q, vp);
      svd.insert(vp,vq);
      p = q;
      vp = vq;
    }
    if (vfirst != vp) {
      svd.insert(vp, vfirst);
    }
  }
  
  
  emit(changed());
  actionRecenter->trigger();
}


void
MainWindow::loadEdgConstraints(QString fileName)
{
  // wait cursor
  QApplication::setOverrideCursor(Qt::WaitCursor);
  CGAL::Timer tim;
  tim.start();
  std::ifstream ifs(qPrintable(fileName));
  bool first=true;
  int n;
  ifs >> n;
  
  K::Point_2 p,q, qold(0,0); // Initialize qold, as otherwise some g++ issue a unjustified warning

  SVD::Vertex_handle vp, vq, vqold;
  while(ifs >> p) {
    ifs >> q;
    if(p == q){
      continue;
    }
    if((!first) && (p == qold)){
      vp = vqold;
    } else {
      vp = svd.insert(p);
    }
    vq = svd.insert(q, vp);
    svd.insert(vp,vq);
    qold = q;
    vqold = vq;
    first = false;
  }


  tim.stop();
  statusBar()->showMessage(QString("Insertion took %1 seconds").arg(tim.time()), 2000);
  // default cursor
  QApplication::restoreOverrideCursor();
  emit(changed());
  actionRecenter->trigger();
}

void
MainWindow::loadPoints(QString fileName)
{
  K::Point_2 p;
  std::ifstream ifs(qPrintable(fileName));
  int n;
  while(ifs >> n){
    while(n--){
      ifs >> p;
      svd.insert(p);
    }
  }
  
  emit(changed());
  actionRecenter->trigger();
}

void
MainWindow::loadPointsInput(QString fileName)
{
  K::Point_2 p;
  std::ifstream ifs(qPrintable(fileName));
  while (ifs >> p) {
    svd.insert(p);
  }
  
  emit(changed());
  actionRecenter->trigger();
}

void
MainWindow::loadSitesInput(QString fileName)
{
  SVD::Site_2 site;
  std::ifstream ifs(qPrintable(fileName));
  while (ifs >> site) {
    svd.insert(site);

    CGAL_SDG_DEBUG( svd.file_output_verbose(std::cout); ) ;
  }
  
  emit(changed());
  actionRecenter->trigger();
}




void
MainWindow::on_actionRecenter_triggered()
{
  this->graphicsView->setSceneRect(sdggi->boundingRect());
  this->graphicsView->fitInView(sdggi->boundingRect(), Qt::KeepAspectRatio);  
}


void
MainWindow::on_actionSaveConstraints_triggered()
{
  QString fileName = QFileDialog::getSaveFileName(this,
						  tr("Save Constraints"),
						  ".",
						  tr("Poly files (*.poly)\n"
						     "Edge files (*.edg)"));
  if(! fileName.isEmpty()){
    saveConstraints(fileName);
  }
}


void
MainWindow::saveConstraints(QString /*fileName*/)
{
  QMessageBox::warning(this,
                       tr("saveConstraints"),
                       tr("Not implemented!"));
}





#include "Segment_voronoi_2.moc"
#include <CGAL/Qt/resources.h>


int main(int argc, char **argv)
{
  QApplication app(argc, argv);

  app.setOrganizationDomain("geometryfactory.com");
  app.setOrganizationName("GeometryFactory");
  app.setApplicationName("Segment Voronoi 2 demo");

  // Import resources from libCGALQt4.
  // See http://doc.trolltech.com/4.4/qdir.html#Q_INIT_RESOURCE
  CGAL_QT4_INIT_RESOURCES;

  if (argc == 2) {
    has_file_argument = true;
    file_to_open = argv[1];
  } else {
    has_file_argument = false;
  }

  MainWindow mainWindow;
  mainWindow.show();


  return app.exec();
}
