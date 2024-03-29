#ifndef AGEOTREEWIDGET_H
#define AGEOTREEWIDGET_H

#include <QObject>
#include <QString>
#include <QTreeWidget>
#include <QImage>
#include <QColor>
#include <QList>

class AGeoObject;
class AGeoShape;
class AGeoWidget;
class ASandwich;
class QVBoxLayout;
class QPushButton;
class AGeoBaseDelegate;
class QPoint;
class QTreeWidgetItem;
class QStringList;

class AGeoTreeWidget : public QTreeWidget
{
  Q_OBJECT

public:
  AGeoTreeWidget(ASandwich * Sandwich);

  AGeoWidget * GetEditWidget() {return EditWidget;}
  void         SetLineAttributes(AGeoObject * obj);
  void         SelectObjects(QStringList ObjectNames);
  void         AddLightguide(bool bUpper);

  QTreeWidget * twPrototypes = nullptr;
  QString LastShownObjectName;

public slots:
  void UpdateGui(QString ObjectName = "");
  void onGridReshapeRequested(QString objName);
  void objectMembersToScript(AGeoObject *Master, QString &script, int ident, bool bExpandMaterial, bool bRecursive, bool usePython);
  void objectToScript(AGeoObject *obj, QString &script, int ident, bool bExpandMaterial, bool bRecursive, bool usePython);
  void commonSlabToScript(QString &script, const QString &identStr);
  void rebuildDetectorAndRestoreCurrentDelegate();  // used by geoConst widget
  void onRequestShowPrototype(QString ProtoName);
  void onRequestIsValidPrototypeName(const QString & ProtoName, bool & bResult) const;

private slots:
  void onItemSelectionChanged();
  void onProtoItemSelectionChanged();
  void customMenuRequested(const QPoint &pos);      // ==== World tree CONTEXT MENU ====
  void customProtoMenuRequested(const QPoint &pos); // ---- Proto tree CONTEXT MENU ----
  void onItemClicked();
  void onProtoItemClicked();

  void onItemExpanded(QTreeWidgetItem * item);
  void onItemCollapsed(QTreeWidgetItem * item);
  void onPrototypeItemExpanded(QTreeWidgetItem * item);
  void onPrototypeItemCollapsed(QTreeWidgetItem * item);

  void onRemoveTriggered();
  void onRemoveRecursiveTriggered();

protected:
  void dropEvent(QDropEvent *event);
  void dragEnterEvent(QDragEnterEvent* event);
  void dragMoveEvent(QDragMoveEvent* event);

private:
  ASandwich  * Sandwich   = nullptr;
  AGeoObject * World      = nullptr;
  AGeoObject * Prototypes = nullptr;

  AGeoWidget * EditWidget = nullptr;

  QTreeWidgetItem * topItemPrototypes = nullptr;
  bool bWorldTreeSelected = true;

  //base images for icons
  QImage Lock;
  //QImage GroupStart, GroupMid, GroupEnd;
  QImage StackStart, StackMid, StackEnd;

  //QColor BackgroundColor = QColor(240,240,240);
  bool   fSpecialGeoViewMode = false;

  QTreeWidgetItem * previousHoverItem = nullptr;
  const QTreeWidgetItem * movingItem  = nullptr;  // used only to prevent highlight of item under the moving one if it is the same as target

  void loadImages();
  void configureStyle(QTreeWidget * wid);
  void populateTreeWidget(QTreeWidgetItem *parent, AGeoObject *Container, bool fDisabled = false);
  void updateExpandState(QTreeWidgetItem * item, bool bPrototypes); //recursive!
  void updateIcon(QTreeWidgetItem *item, AGeoObject *obj);
  void menuActionFormStack(QList<QTreeWidgetItem *> selected);
  void markAsStackRefVolume(AGeoObject * obj);
  void createPrototypeTreeWidget();
  void updatePrototypeTreeGui();

  void menuActionAddNewObject(AGeoObject * ContObj, AGeoShape * shape);
  void menuActionCloneObject(AGeoObject * obj);
  void ShowObject(AGeoObject * obj);
  void ShowObjectRecursive(AGeoObject * obj);
  void ShowObjectOnly(AGeoObject * obj);
  void ShowAllInstances(AGeoObject * obj);
  void menuActionEnableDisable(AGeoObject * obj);
  void menuActionRemoveKeepContent();
  void menuActionRemoveHostedObjects(AGeoObject * obj);
  void menuActionRemoveWithContent(QTreeWidget * treeWidget);
  void menuActionAddNewComposite(AGeoObject * ContObj);
  void menuActionAddNewArray(AGeoObject * ContObj);
  void menuActionAddNewGrid(AGeoObject * ContObj);
  void menuActionAddNewMonitor(AGeoObject * ContObj);
  void menuActionAddInstance(AGeoObject * ContObj, const QString & PrototypeName);
  void menuActionMakeItPrototype(const QList<QTreeWidgetItem *> & selected);
  void menuActionMoveProtoToWorld(AGeoObject * obj);

  QString makeScriptString_basicObject(AGeoObject *obj, bool bExpandMaterials, bool usePython) const;
  QString makeScriptString_slab(AGeoObject *obj, bool bExpandMaterials, int ident) const;
  QString makeScriptString_setCenterSlab(AGeoObject *obj) const;
  QString makeScriptString_arrayObject(AGeoObject *obj) const;
  QString makeScriptString_instanceObject(AGeoObject *obj, bool usePython) const;
  QString makeScriptString_prototypeObject(AGeoObject *obj) const;
  QString makeScriptString_monitorBaseObject(const AGeoObject *obj) const;
  QString makeScriptString_monitorConfig(const AGeoObject *obj) const;
  QString makeScriptString_stackObjectStart(AGeoObject *obj) const;
  QString makeScriptString_groupObjectStart(AGeoObject *obj) const;
  QString makeScriptString_stackObjectEnd(AGeoObject *obj) const;
  QString makeLinePropertiesString(AGeoObject *obj) const;
  QString makeScriptString_DisabledObject(AGeoObject *obj) const;

signals:
  void ObjectSelectionChanged(QString);
  void ProtoObjectSelectionChanged(QString);
  void RequestRebuildDetector();
  void RequestHighlightObject(QString name);
  void RequestFocusObject(QString name);
  void RequestShowObjectRecursive(QString name);
  void RequestShowAllInstances(QString name);
  void RequestNormalDetectorDraw();
  void RequestListOfParticles(QStringList & definedParticles);
  void RequestShowMonitor(const AGeoObject * mon);
  void RequestShowPrototypeList();
};


class AGeoWidget : public QWidget
{
  Q_OBJECT

public:
  AGeoWidget(ASandwich * Sandwich, AGeoTreeWidget * tw);
  //destructor does not delete Widget - it is handled by the layout

  void ClearGui();
  void UpdateGui();

  QString getCurrentObjectName() const;

private:
  ASandwich        * Sandwich = nullptr;
  AGeoTreeWidget   * tw       = nullptr;

  AGeoObject       * CurrentObject = nullptr;
  AGeoBaseDelegate * GeoDelegate = nullptr;

  QVBoxLayout * lMain;
  QVBoxLayout * ObjectLayout;
  QFrame      * frBottom;
  QPushButton * pbConfirm;
  QPushButton * pbCancel;

  bool fIgnoreSignals = true;
  bool fEditingMode   = false;

public slots:
  void onObjectSelectionChanged(QString SelectedObject); //starts GUI update
  void onStartEditing();
  void onRequestChangeShape(AGeoShape * NewShape);
  void onRequestChangeSlabShape(int NewShape);
  void onMonitorRequestsShowSensitiveDirection();

  void onRequestShowCurrentObject();
  void onRequestScriptLineToClipboard();
  void onRequestScriptRecursiveToClipboard();
  void onRequestSetVisAttributes();

  void onConfirmPressed(); // CONFIRM BUTTON PRESSED: performing copy from delegate to object
  void onCancelPressed();

private:
  void exitEditingMode();
  void updateInstancesOnProtoNameChange(QString oldName, QString newName);
  AGeoBaseDelegate * createAndAddSlabDelegate();
  AGeoBaseDelegate * createAndAddGeoObjectDelegate();
  AGeoBaseDelegate * createAndAddGridElementDelegate();
  AGeoBaseDelegate * createAndAddMonitorDelegate();

signals:
  void showMonitor(const AGeoObject* mon);
  void requestBuildScript(AGeoObject *obj, QString &script, int ident, bool bExpandMaterial, bool bRecursive, bool usePython);
  void requestEnableGeoConstWidget(bool);
};

#endif // AGEOTREEWIDGET_H
