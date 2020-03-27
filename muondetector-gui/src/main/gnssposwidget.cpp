#include "gnssposwidget.h"
#include "ui_gnssposwidget.h"
#include <QPainter>
#include <QPixmap>
#include <muondetector_structs.h>
#include <cmath>
#include <QMenu>
#include <QFileDialog>
#include <QTextStream>
#include <QResizeEvent>

using namespace std;

constexpr double pi() { return acos(-1); }
constexpr double sqrt2() {return sqrt(2.); }
const int MAX_SAT_TRACK_ENTRIES = 10000;
static const QList<QColor> GNSS_COLORS = { Qt::darkGreen, Qt::darkYellow, Qt::blue, Qt::magenta, Qt::gray, Qt::cyan, Qt::red, Qt::black };
static int alphaFromCnr(int cnr) {
    int alpha=cnr*255/40;
    if (alpha>255) alpha=255;
    return alpha;
}

GnssPosWidget::GnssPosWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::GnssPosWidget)
{
    ui->setupUi(this);
    ui->satLabel->setText(QString::number(ui->satLabel->width())+"x"+QString::number(ui->satLabel->height()));
    connect(ui->receivedSatsCheckBox, &QCheckBox::toggled, this, &GnssPosWidget::replot);
    connect(ui->satLabelsCheckBox, &QCheckBox::toggled, this, &GnssPosWidget::replot);
    connect(ui->tracksCheckBox, &QCheckBox::toggled, this, &GnssPosWidget::replot);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this,SIGNAL(customContextMenuRequested(const QPoint &  )),this,SLOT(popUpMenu(const QPoint &)));
    connect(ui->cartPolarCheckBox, &QCheckBox::toggled, this, [this](bool checked) { resizeEvent(nullptr); } );
}

GnssPosWidget::~GnssPosWidget()
{
    delete ui;
}

void GnssPosWidget::resizeEvent(QResizeEvent *event)
{
    int w=ui->hostWidget->width();
    int h=ui->hostWidget->height();
    if (ui->cartPolarCheckBox->isChecked()) {
        if (h<w) w=h;
        else h=w;
    } else {
        // no constraints
    }
    w-=10; h-=10;
    if (w<10) w=10;
    if (h<10) h=10;
    ui->satLabel->resize(w,h);
    
    replot();
    //ui->satLabel->setText(QString::number(ui->satLabel->width())+"x"+QString::number(ui->satLabel->height()));
}

QPointF GnssPosWidget::polar2cartUnity(const QPointF& pol) {
    double magn=(90.-pol.y())/180.;
    double xpos = magn*sin(pi()*pol.x()/180.);
    double ypos = -magn*cos(pi()*pol.x()/180.);
    return QPointF(xpos,ypos);
}

void GnssPosWidget::drawPolarPixMap(QPixmap& pm) {
    
    QVector<GnssSatellite> newlist;
    QString str;
    QColor color;

    const int satPosPixmapSize = pm.width();
    const QPointF originOffset(satPosPixmapSize/2., satPosPixmapSize/2.);

    //    pixmap.fill(QColor("transparent"));
    pm.fill(Qt::white);
    QPainter satPosPainter(&pm);
    satPosPainter.setPen(QPen(Qt::black));
    satPosPainter.drawEllipse(QPoint(satPosPixmapSize/2,satPosPixmapSize/2),satPosPixmapSize/6,satPosPixmapSize/6);
    satPosPainter.drawEllipse(QPoint(satPosPixmapSize/2,satPosPixmapSize/2),satPosPixmapSize/3,satPosPixmapSize/3);
    satPosPainter.drawEllipse(QPoint(satPosPixmapSize/2,satPosPixmapSize/2),satPosPixmapSize/2,satPosPixmapSize/2);
    satPosPainter.drawLine(QPoint(satPosPixmapSize/2,0),QPoint(satPosPixmapSize/2,satPosPixmapSize));
    satPosPainter.drawLine(QPoint(0,satPosPixmapSize/2),QPoint(satPosPixmapSize,satPosPixmapSize/2));
    satPosPainter.drawText(satPosPixmapSize/2+2,3,18,18,Qt::AlignHCenter,"N");
    satPosPainter.drawText(satPosPixmapSize/2+2,satPosPixmapSize-19,18,18,Qt::AlignHCenter,"S");
    satPosPainter.drawText(4,satPosPixmapSize/2-19,18,18,Qt::AlignHCenter,"W");
    satPosPainter.drawText(satPosPixmapSize-19,satPosPixmapSize/2-19,18,18,Qt::AlignHCenter,"E");

    QFont font = satPosPainter.font();
    font.setPointSize(font.pointSize()-2);
    satPosPainter.setFont(font);
    satPosPainter.drawText(satPosPixmapSize/2-14,satPosPixmapSize-12,18,18,Qt::AlignHCenter,"0°");
    satPosPainter.drawText(satPosPixmapSize/2-16,satPosPixmapSize*5/6-12,18,18,Qt::AlignHCenter,"30°");
    satPosPainter.drawText(satPosPixmapSize/2-16,satPosPixmapSize*2/3-12,18,18,Qt::AlignHCenter,"60°");


    int nrGoodSats = 0;
    for (auto it=fCurrentSatlist.begin(); it!=fCurrentSatlist.end(); it++)
        if (it->fCnr>0) nrGoodSats++;

    for (int i=0; i<fCurrentSatlist.size(); i++)
    {
        if (ui->receivedSatsCheckBox->isChecked()) {
            if (fCurrentSatlist[i].fCnr>0) newlist.push_back(fCurrentSatlist[i]);
        } else newlist.push_back(fCurrentSatlist[i]);
        if (fCurrentSatlist[i].fElev<=90. && fCurrentSatlist[i].fElev>=-90.) {
            if (ui->receivedSatsCheckBox->isChecked() && fCurrentSatlist[i].fCnr==0) continue;
            QPointF currPos(fCurrentSatlist[i].fAzim, fCurrentSatlist[i].fElev);
            QPointF currPoint = polar2cartUnity(currPos);

            QColor satColor=Qt::white;
            QColor fillColor=Qt::white;
            satColor = GNSS_COLORS[fCurrentSatlist[i].fGnssId];
            satPosPainter.setPen(satColor);
            fillColor=satColor;
//            int alpha = fCurrentSatlist[i].fCnr*255/40;
            int alpha = alphaFromCnr(fCurrentSatlist[i].fCnr);
            //if (alpha>255) alpha=255;
            fillColor.setAlpha(alpha);
            int satId = fCurrentSatlist[i].fGnssId*1000 + fCurrentSatlist[i].fSatId;
//            QPointF currPoint(xpos+satPosPixmapSize/2,ypos+satPosPixmapSize/2);
            QPointF lastPoint(-2,-2);
            int lastCnr = 255;
            if (satTracks[satId].size()) {
                lastPoint=satTracks[satId].last().posCart;
                lastCnr = satTracks[satId].last().cnr;
            }
            if (lastPoint!=currPoint || lastCnr!=fCurrentSatlist[i].fCnr)
            {
                SatHistoryPoint p;
                p.posCart=currPoint;
                p.color=fillColor;
                p.posPolar=QPoint(currPos.x(), currPos.y());
                p.gnssId=fCurrentSatlist[i].fGnssId;
                p.satId=fCurrentSatlist[i].fSatId;
                p.cnr=fCurrentSatlist[i].fCnr;
                p.time=QDateTime::currentDateTimeUtc();
                satTracks[satId].push_back(p);
            }
            satPosPainter.setPen(satColor);
            satPosPainter.setBrush(fillColor);
/*
            if (fCurrentSatlist[i].fCnr>40) satPosPainter.setBrush(Qt::darkGreen);
            else if (fCurrentSatlist[i].fCnr>30) satPosPainter.setBrush(Qt::green);
            else if (fCurrentSatlist[i].fCnr>20) satPosPainter.setBrush(Qt::yellow);
            else if (fCurrentSatlist[i].fCnr>10) satPosPainter.setBrush(QColor(255,100,0));
            else if (fCurrentSatlist[i].fCnr>0) satPosPainter.setBrush(Qt::red);
            else satPosPainter.setBrush(QColor("transparent"));
*/
            currPoint*=satPosPixmapSize;
            currPoint+=originOffset;

            float satsize=ui->satSizeSpinBox->value();
            satPosPainter.drawEllipse(currPoint,satsize/2.,satsize/2.);
            if (fCurrentSatlist[i].fUsed) satPosPainter.drawEllipse(currPoint,satsize/2.+0.5,satsize/2.+0.5);
            currPoint.rx()+=3;
            if (ui->satLabelsCheckBox->isChecked()) satPosPainter.drawText(currPoint, QString::number(fCurrentSatlist[i].fSatId));
        }
    }

    // draw the sat tracks
    if (ui->tracksCheckBox->isChecked()) {
        foreach (QVector<SatHistoryPoint> sat, satTracks) {
            for (int i=0; i<sat.size(); i++) {
                satPosPainter.setPen(sat[i].color);
                QPointF p(sat[i].posCart);
                p*=satPosPixmapSize;
                p+=originOffset;
                //satPosPainter.setBrush(satTracks[satId][i].color);
                satPosPainter.drawPoint(p);
            }
        }
    }
    
}

void GnssPosWidget::drawCartesianPixMap(QPixmap& pm) {
    
    QVector<GnssSatellite> newlist;
    QString str;
    QColor color;

    const QPointF originOffset(0., pm.height()*0.9);

    //    pixmap.fill(QColor("transparent"));
    pm.fill(Qt::white);
    QPainter satPosPainter(&pm);
    satPosPainter.setPen(QPen(Qt::black));
    satPosPainter.drawLine(originOffset,originOffset+QPointF(pm.width(),0));
    satPosPainter.drawLine(0.33*originOffset,0.33*originOffset+QPointF(pm.width(),0));
    satPosPainter.drawLine(0.67*originOffset,0.67*originOffset+QPointF(pm.width(),0));
    satPosPainter.drawLine(QPoint(pm.width()/2,0),originOffset+QPointF(pm.width()/2,0));
    
    satPosPainter.drawText(pm.width()/2-3,originOffset.y()+6,18,18,Qt::AlignHCenter,"S");
    satPosPainter.drawText(2,originOffset.y()+6,18,18,Qt::AlignHCenter,"N");
    satPosPainter.drawText(pm.width()-18,originOffset.y()+6,18,18,Qt::AlignHCenter,"N");
    satPosPainter.drawText(pm.width()/4-3,originOffset.y()+6,18,18,Qt::AlignHCenter,"E");
    satPosPainter.drawText(3*pm.width()/4-3,originOffset.y()+6,18,18,Qt::AlignHCenter,"W");

    QFont font = satPosPainter.font();
    font.setPointSize(font.pointSize()-2);
    satPosPainter.setFont(font);
    satPosPainter.drawText(pm.width()/2,originOffset.y()-10,18,18,Qt::AlignHCenter,"0°");
    satPosPainter.drawText(pm.width()/2,0.33*originOffset.y()+3,18,18,Qt::AlignHCenter,"60°");
    satPosPainter.drawText(pm.width()/2,0.67*originOffset.y()+3,18,18,Qt::AlignHCenter,"30°");
//    satPosPainter.drawText(satPosPixmapSize/2-16,satPosPixmapSize*5/6-12,18,18,Qt::AlignHCenter,"30°");
//    satPosPainter.drawText(satPosPixmapSize/2-16,satPosPixmapSize*2/3-12,18,18,Qt::AlignHCenter,"60°");


    int nrGoodSats = 0;
    for (auto it=fCurrentSatlist.begin(); it!=fCurrentSatlist.end(); it++)
        if (it->fCnr>0) nrGoodSats++;

    for (int i=0; i<fCurrentSatlist.size(); i++)
    {
        if (ui->receivedSatsCheckBox->isChecked()) {
            if (fCurrentSatlist[i].fCnr>0) newlist.push_back(fCurrentSatlist[i]);
        } else newlist.push_back(fCurrentSatlist[i]);
        if (fCurrentSatlist[i].fElev<=90. && fCurrentSatlist[i].fElev>=-90.) {
            if (ui->receivedSatsCheckBox->isChecked() && fCurrentSatlist[i].fCnr==0) continue;
            QPointF currPos(fCurrentSatlist[i].fAzim, fCurrentSatlist[i].fElev);
            QPointF currPoint = polar2cartUnity(currPos);

            QColor satColor=Qt::white;
            QColor fillColor=Qt::white;
            satColor = GNSS_COLORS[fCurrentSatlist[i].fGnssId];
            satPosPainter.setPen(satColor);
            fillColor=satColor;
            int alpha = fCurrentSatlist[i].fCnr*255/40;
            if (alpha>255) alpha=255;
            fillColor.setAlpha(alpha);
            int satId = fCurrentSatlist[i].fGnssId*1000 + fCurrentSatlist[i].fSatId;
//            QPointF currPoint(xpos+satPosPixmapSize/2,ypos+satPosPixmapSize/2);
            QPointF lastPoint(-2,-2);
            int lastCnr = 255;
            if (satTracks[satId].size()) {
                lastPoint=satTracks[satId].last().posCart;
                lastCnr = satTracks[satId].last().cnr;
            }
            if (lastPoint!=currPoint || lastCnr!=fCurrentSatlist[i].fCnr)
            {
                SatHistoryPoint p;
                p.posCart=currPoint;
                p.color=fillColor;
                p.posPolar=QPoint(currPos.x(), currPos.y());
                p.gnssId=fCurrentSatlist[i].fGnssId;
                p.satId=fCurrentSatlist[i].fSatId;
                p.cnr=fCurrentSatlist[i].fCnr;
                p.time=QDateTime::currentDateTimeUtc();
                satTracks[satId].push_back(p);
            }
            satPosPainter.setPen(satColor);
            satPosPainter.setBrush(fillColor);
            
            currPos=QPointF(currPos.x()/360.*pm.width(), -currPos.y()/90.*pm.height()*0.9+pm.height()*0.9);
            
            float satsize=ui->satSizeSpinBox->value();
            satPosPainter.drawEllipse(currPos,satsize/2.,satsize/2.);
            if (fCurrentSatlist[i].fUsed) satPosPainter.drawEllipse(currPos,satsize/2.+0.5,satsize/2.+0.5);
            currPoint.rx()+=4.5;
            if (ui->satLabelsCheckBox->isChecked()) satPosPainter.drawText(currPos, QString::number(fCurrentSatlist[i].fSatId));
        }
    }

    // draw the sat tracks
    if (ui->tracksCheckBox->isChecked()) {
        foreach (QVector<SatHistoryPoint> sat, satTracks) {
            for (int i=0; i<sat.size(); i++) {
                satPosPainter.setPen(sat[i].color);
                QPointF p(sat[i].posPolar);
                p=QPointF(p.x()/360.*pm.width(), -p.y()/90.*pm.height()*0.9+pm.height()*0.9);
                //satPosPainter.drawPoint(p);
                satPosPainter.setPen(QColor("transparent"));
                satPosPainter.setBrush(sat[i].color);
                int w=pm.width()/360+1;
                int h=pm.height()*0.9/90+1;
                satPosPainter.drawRect(p.x()-w/2,p.y()-h/2,w,h);
            }
        }
    }
    
}

void GnssPosWidget::replot() {

    const int w=ui->satLabel->width();
    const int h=ui->satLabel->height();

    QPixmap satPosPixmap(w,h);
    if (ui->cartPolarCheckBox->isChecked()){
        drawPolarPixMap(satPosPixmap);
    } else {
        drawCartesianPixMap(satPosPixmap);
    }

    ui->satLabel->setPixmap(satPosPixmap);
}

void GnssPosWidget::onSatsReceived(const QVector<GnssSatellite> &satlist)
{
    fCurrentSatlist = satlist;
    replot();
}

void GnssPosWidget::on_satSizeSpinBox_valueChanged(int arg1)
{
    replot();
}

void GnssPosWidget::onUiEnabledStateChange(bool connected)
{
    if (!connected) {
        QVector<GnssSatellite> emptylist;
//        onSatsReceived(emptylist);
        fCurrentSatlist.clear();
    }
    this->setEnabled(connected);

}

void GnssPosWidget::popUpMenu(const QPoint & pos)
{
    QMenu contextMenu(tr("Context menu"), this);

    QAction action2("&Clear Tracks", this);
    connect(&action2, &QAction::triggered, this,  [this](bool checked){ satTracks.clear() ; this->replot(); });
    contextMenu.addAction(&action2);

    contextMenu.addSeparator();

    QAction action3("&Export", this);
    connect(&action3, &QAction::triggered, this, &GnssPosWidget::exportToFile );
    contextMenu.addAction(&action3);

    contextMenu.exec(mapToGlobal(pos));
}

void GnssPosWidget::exportToFile() {
    const int pixmapSize = 512;
    QPixmap satPosPixmap;
    if (ui->cartPolarCheckBox->isChecked()){
        satPosPixmap = QPixmap(pixmapSize, pixmapSize);
        drawPolarPixMap(satPosPixmap);
    } else {
        satPosPixmap = QPixmap(sqrt2()*pixmapSize, pixmapSize);
        drawCartesianPixMap(satPosPixmap);
    }

    QString types(	"JPEG file (*.jpeg);;"				// Set up the possible graphics formats
            "Portable Network Graphics file (*.png);;"
            "Bitmap file (*.bmp);;"
            "ASCII raw data (*.txt)");
    QString filter;							// Type of filter
    QString jpegExt=".jpeg", pngExt=".png", tifExt=".tif", bmpExt=".bmp", tif2Ext="tiff";		// Suffix for the files
    QString txtExt=".txt";
    QString suggestedName="";
    QString fn = QFileDialog::getSaveFileName(this,tr("Export Pixmap"),
                                                  suggestedName,types,&filter);

    if ( !fn.isEmpty() ) {						// If filename is not null
        if (fn.contains(jpegExt)) {				// Remove file extension if already there
            fn.remove(jpegExt);
        } else if (fn.contains(pngExt)) {
            fn.remove(pngExt);
        } else if (fn.contains(bmpExt)) {
            fn.remove(bmpExt);
        } else if (fn.contains(txtExt)) {
            fn.remove(txtExt);
        }

        if (filter.contains(jpegExt)) {				// OR, Test to see if jpeg and save
            fn+=jpegExt;
            satPosPixmap.save( fn, "JPEG" );
        }
        else if (filter.contains(pngExt)) {			// OR, Test to see if png and save
            fn+=pngExt;
            satPosPixmap.save( fn, "PNG" );
        }
        else if (filter.contains(bmpExt)) {			// OR, Test to see if bmp and save
            fn+=bmpExt;
            satPosPixmap.save( fn, "BMP" );
        }
        else if (filter.contains(txtExt)) {
            fn+=txtExt;
            // export sat history in asci raw data format
            QFile file(fn);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
            QTextStream out(&file);
            //out.setFieldWidth(20);
            out<<"# GNSS sat history from  xxx to yyy\n";
            out<<"# <ISO8601-datetime> <gnss-id> <gnss-id-string> <sat-id> <azimuth> <elevation> <cnr>\n";

            foreach (QVector<SatHistoryPoint> satvector, satTracks) {
                foreach (SatHistoryPoint p, satvector) {
                //for (int i=0; i<sat.size(); i++) {
                    //QPointF p(sat[i].pos);
                    out << p.time.toString(Qt::ISODate) << " "
                        << p.gnssId << " "
                        << GNSS_ID_STRING[p.gnssId] << " "
                        << p.satId << " "
                        << p.posPolar.x() << " "
                        << p.posPolar.y() << " "
                        << p.cnr <<"\n";
                }
            }
        }
    }
}
