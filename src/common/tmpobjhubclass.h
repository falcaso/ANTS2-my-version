#ifndef TMPOBJHUBCLASS_H
#define TMPOBJHUBCLASS_H

#include <QList>
#include <QString>
#include <QVector>
#include <QObject>

class TObject;
class TrackHolderClass;
class TH1D;

class RootDrawObj
{
  public:
    TObject* Obj;
    QString name; // it is also the title
    QString type; // object type (e.g. TH1D)

    QString Xtitle, Ytitle, Ztitle;
    int MarkerColor, MarkerStyle, MarkerSize, LineColor, LineStyle, LineWidth;

    RootDrawObj();
    ~RootDrawObj();
};

class ScriptDrawCollection
{
public:
   QList<RootDrawObj> List;

   int findIndexOf(QString name); //returns -1 if not found
   bool remove(QString name);
   void append(TObject* obj, QString name, QString type);
   void clear() {List.clear();}
   void removeAllHists();
   void removeAllGraphs();
};


//=================================================================
class TmpObjHubClass : public QObject
{
    Q_OBJECT
public:
  TmpObjHubClass();
  ~TmpObjHubClass();

  ScriptDrawCollection ScriptDrawObjects;

  double PreEnAdd, PreEnMulti;

  QVector<TrackHolderClass*> TrackInfo;
  void ClearTracks();

  QVector<TH1D*> PeakHists;
  void ClearTmpHistsPeaks();
  QVector< QVector<double> > FoundPeaks;
  QVector<double> ChPerPhEl_Peaks;

  QVector<TH1D*> SigmaHists;
  void ClearTmpHistsSigma2();
  QVector<double> ChPerPhEl_Sigma2;

public slots:
  void Clear();

};

#endif // TMPOBJHUBCLASS_H
