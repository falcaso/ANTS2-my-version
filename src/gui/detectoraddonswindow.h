#ifndef DETECTORADDONSWINDOW_H
#define DETECTORADDONSWINDOW_H

#include "aguiwindow.h"

class MainWindow;
class DetectorClass;
class AGeo_SI;
class AGeoTreeWidget;
class AGeoObject;

namespace Ui {
  class DetectorAddOnsWindow;
}

class DetectorAddOnsWindow : public AGuiWindow
{
  Q_OBJECT
  
public:
  explicit DetectorAddOnsWindow(QWidget * parent, MainWindow * MW, DetectorClass* detector);
  ~DetectorAddOnsWindow();

  void UpdateGUI(); //update gui controls
  void SetTab(int tab);
  void UpdateDummyPMindication();
  void HighlightVolume(const QString & VolName);

  AGeo_SI        * AddObjScriptInterface = nullptr;  // if created -> owned by the script manager
  AGeoTreeWidget * twGeo   = nullptr;                  // WorldTree widget

private slots:
  void onReconstructDetectorRequest();
  void onGeoConstEditingFinished(int index, QString newValue);
  void onGeoConstExpressionEditingFinished(int index, QString newValue);
  void onGeoConstEscapePressed(int index);
  void onRequestShowPrototypeList();
  void updateMenuIndication();
  void onSandwichRebuild();

  void on_tabwConstants_customContextMenuRequested(const QPoint &pos);
  void on_pbConvertToDummies_clicked();
  void on_sbDummyPMindex_valueChanged(int arg1);
  void on_pbDeleteDummy_clicked();
  void on_pbConvertDummy_clicked();
  void on_pbUpdateDummy_clicked();
  void on_pbCreateNewDummy_clicked();
  void on_sbDummyType_valueChanged(int arg1);
  void on_pbLoadDummyPMs_clicked();
  void on_pbConvertAllToPMs_clicked();
//void on_pbUseScriptToAddObj_clicked();
  void on_pbSaveTGeo_clicked();
  void on_pbLoadTGeo_clicked();
  void on_pbBackToSandwich_clicked();
  void on_pbRootWeb_clicked();
  void on_pbCheckGeometry_clicked();
  void on_cbAutoCheck_clicked(bool checked);
  void on_pbRunTestParticle_clicked();
  void on_cbAutoCheck_stateChanged(int arg1);
  void on_pmParseInGeometryFromGDML_clicked();
  void on_tabwConstants_cellChanged(int row, int column);
  void on_actionUndo_triggered();
  void on_actionRedo_triggered();
  void on_actionHow_to_use_drag_and_drop_triggered();
  void on_actionTo_JavaScript_triggered();
  void on_actionTo_Python_triggered();
  void on_cbShowPrototypes_toggled(bool checked);

  void on_actionAdd_top_lightguide_triggered();
  void on_actionAdd_bottom_lightguide_triggered();

private:
  Ui::DetectorAddOnsWindow *ui;
  MainWindow * MW;
  DetectorClass * Detector;

  QString ObjectScriptTarget;

  bool bGeoConstsWidgetUpdateInProgress = false;

  void    ConvertDummyToPM(int idpm);
  bool    GDMLtoTGeo(const QString &fileName);
  QString loadGDML(const QString &fileName, QString &gdml);  //returns error string - empty if OK
  void    loadDummyPMs(const QString &DFile);
  void    updateGeoConstsIndication();
  QString createScript(QString &script, bool usePython);

protected:
  void resizeEvent(QResizeEvent *event);

public slots:
  void UpdateGeoTree(QString name = "");
  void ShowTab(int tab);
  void AddObjScriptSuccess();
  void ReportScriptError(QString ErrorMessage);
  void ShowObject(QString name = "");
  void FocusVolume(QString name);
  void ShowObjectRecursive(QString name);
  void ShowAllInstances(QString name);
  void OnrequestShowMonitor(const AGeoObject* mon);
  void onRequestEnableGeoConstWidget(bool flag);

signals:
  void requestDelayedRebuildAndRestoreDelegate();

};

#include <QLineEdit>
class ALineEditWithEscape : public QLineEdit
{
    Q_OBJECT
public:
    ALineEditWithEscape(const QString & text, QWidget * parent) : QLineEdit(text, parent){}

protected:
    void keyPressEvent(QKeyEvent * event);

signals:
    void escapePressed();
};

#endif // DETECTORADDONSWINDOW_H
