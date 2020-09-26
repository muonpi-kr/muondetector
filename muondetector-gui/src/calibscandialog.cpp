#include "calibscandialog.h"
#include "ui_calibscandialog.h"
#include <muondetector_structs.h>

#define calVoltMin 0.3
#define calVoltMax 2.5

#define BIAS_SCAN_INCREMENT 0.025  // scan step in Volts

inline static double sqr(double x) {
        return x*x;
}

/*
 linear regression algorithm
 taken from:
   http://stackoverflow.com/questions/5083465/fast-efficient-least-squares-fit-algorithm-in-c
 parameters:
  n = number of data points
  xarray,yarray  = arrays of data
  *offs = output intercept
  *slope  = output slope
 */
bool calcLinearCoefficients( const QVector<QPointF>& points,
        /*int n, double *xarray, double *yarray,
                */double *offs, double* slope)
{
   int n=points.size();
   if (n<3) return false;

   double   sumx = 0.0;                        /* sum of x                      */
   double   sumx2 = 0.0;                       /* sum of x**2                   */
   double   sumxy = 0.0;                       /* sum of x * y                  */
   double   sumy = 0.0;                        /* sum of y                      */
   double   sumy2 = 0.0;                       /* sum of y**2                   */

//   int ix=0;
//   double offsx=xarray[ix];
//   double offsy=yarray[ix];
   double offsx=0;
   double offsy=0;
//    long long int offsy=0;

   for (int i=0; i<n; i++) {
          sumx  += points[i].x()-offsx;
          sumx2 += sqr(points[i].x()-offsx);
          sumxy += (points[i].x()-offsx) * (points[i].y()-offsy);
          sumy  += (points[i].y()-offsy);
          sumy2 += sqr(points[i].y()-offsy);
   }


   double denom = (n * sumx2 - sqr(sumx));
   if (denom == 0) {
       // singular matrix. can't solve the problem.
       *slope = 0;
       *offs = 0;
//       if (r) *r = 0;
       return false;
   }

   double m = (n * sumxy  -  sumx * sumy) / denom;
   double b = (sumy * sumx2  -  sumx * sumxy) / denom;

   *slope=m;
   *offs=b+offsy;
   return true;
//    *offs=b;
//   printf("offsI=%lld  offsF=%f\n", offsy, b);

}


CalibScanDialog::CalibScanDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CalibScanDialog)
{
    ui->setupUi(this);
}

CalibScanDialog::~CalibScanDialog()
{
    delete ui;
}

void CalibScanDialog::onCalibReceived(bool valid, bool eepromValid, quint64 id, const QVector<CalibStruct> &calibList)
{
    if (!eepromValid) return;

    fCalibList.clear();
    for (int i=0; i<calibList.size(); i++)
    {
        fCalibList.push_back(calibList[i]);
    }

    int ver = getCalibParameter("VERSION").toInt();
    double rsense = 0.1*getCalibParameter("RSENSE").toInt();
    double vdiv = 0.01*getCalibParameter("VDIV").toInt();
    int featureFlags = getCalibParameter("FEATURE_FLAGS").toInt();

        fSlope1 = getCalibParameter("COEFF1").toDouble();
        fOffs1 = getCalibParameter("COEFF0").toDouble();
        fSlope2 = getCalibParameter("COEFF3").toDouble();
        fOffs2 = getCalibParameter("COEFF2").toDouble();
}

void CalibScanDialog::onAdcSampleReceived(uint8_t channel, float value)
{
	double ubias = 0.;
    if (channel>=2) {
        QString result = getCalibParameter("VDIV");
        if (result!="") {
            double vdiv = result.toDouble(NULL);
            ubias = vdiv*value/100.;
//            ui->biasVoltageLineEdit->setText(QString::number(ubias));
        }
    }
    if (channel == 3) {
        //ui->biasAdcLineEdit->setText(QString::number(value));
        //ui->biasVoltageLineEdit->setText(QString::number(ubias));
/*
        if (fCalibRunning) {
            if (fCurrBias>calVoltMax) { on_doBiasCalibPushButton_clicked(); return; }
            QPointF p(fCurrBias, ubias);
            fCurrBias+=BIAS_SCAN_INCREMENT;
            emit setBiasDacVoltage(fCurrBias);
            QThread::msleep(100);
            fPoints2.push_back(p);
            ui->biasVoltageCalibPlot->curve("curve2").setSamples(fPoints2);

            double vdiv=getCalibParameter("VDIV").toDouble()*0.01;
            double rsense = getCalibParameter("RSENSE").toDouble()*0.1/1000.; // RSense in MOhm
            QPointF p2(ubias,vdiv*(fLastRSenseHiVoltage-value)/rsense);
            fPoints3.push_back(p2);
            ui->biasCurrentCalibPlot->curve("curve3").setSamples(fPoints3);

            ui->biasVoltageCalibPlot->replot();
            ui->biasCurrentCalibPlot->replot();
        }
*/
        fLastRSenseLoVoltage = value;
    } else if (channel == 2) {
/*
        if (fCalibRunning) {
            QPointF p(fCurrBias, ubias);
            fPoints1.push_back(p);
            ui->biasVoltageCalibPlot->curve("curve1").setSamples(fPoints1);
            ui->biasVoltageCalibPlot->replot();
            doFit();
        }
*/
        fLastRSenseHiVoltage = value;
    }
}


void CalibScanDialog::setCalibParameter(const QString &name, const QString &value)
{
    if (!fCalibList.empty()) {
        auto result = std::find_if(fCalibList.begin(), fCalibList.end(), [&name](const CalibStruct& s){ return s.name==name.toStdString(); } );
        if (result != fCalibList.end()) {
            result->value=value.toStdString();
        }
    }
}

QString CalibScanDialog::getCalibParameter(const QString &name)
{
    if (!fCalibList.empty()) {
        auto result = std::find_if(fCalibList.begin(), fCalibList.end(), [&name](const CalibStruct& s){ return s.name==name.toStdString(); } );
        if (result != fCalibList.end()) {
            return QString::fromStdString(result->value);
        }
    }
    return "";
}
