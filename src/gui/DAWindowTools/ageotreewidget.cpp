#include "ageotreewidget.h"
#include "ageobasedelegate.h"
#include "ageoobjectdelegate.h"
#include "amonitordelegate.h"
#include "agridelementdelegate.h"
#include "ageoslabdelegate.h"
#include "ageoobject.h"
#include "ageoshape.h"
#include "atypegeoobject.h"
#include "ashapehelpdialog.h"
#include "arootlineconfigurator.h"
#include "aslablistwidget.h"
#include "slabdelegate.h"
#include "aslab.h"
#include "asandwich.h"
#include "agridelementdialog.h"
#include "amonitordelegateform.h"
#include "amessage.h"
#include "ageoconsts.h"

#include <QDebug>
#include <QDropEvent>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QScrollArea>
#include <QApplication>
#include <QPainter>
#include <QClipboard>
#include <QShortcut>

#include "TMath.h"
#include "TGeoShape.h"

AGeoTreeWidget::AGeoTreeWidget(ASandwich *Sandwich) : Sandwich(Sandwich)
{
  World = Sandwich->World;

  connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(onItemSelectionChanged()));
  //setHeaderLabels(QStringList() << "Tree of geometry objects: use context menu and drag-and-drop");
  setHeaderHidden(true);

  setAcceptDrops(true);
  setDragEnabled(true);
  setDragDropMode(QAbstractItemView::InternalMove);
  setSelectionMode(QAbstractItemView::ExtendedSelection);
  setDropIndicatorShown(false);
  //setIndentation(20);
  setContentsMargins(0,0,0,0);
  setFrameStyle(QFrame::NoFrame);
  setIconSize(QSize(20,20));

  QString dir = ":/images/";
  Lock.load(dir+"lock.png");
  GroupStart.load(dir+"TopGr.png");
  GroupMid.load(dir+"MidGr.png");
  GroupEnd.load(dir+"BotGr.png");
  StackStart.load(dir+"TopSt.png");
  StackMid.load(dir+"MidSt.png");
  StackEnd.load(dir+"BotSt.png");

  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, SIGNAL(customContextMenuRequested(const QPoint &)), this, SLOT(customMenuRequested(const QPoint &)));

  BackgroundColor = QColor(240,240,240);
  fSpecialGeoViewMode = false;

  EditWidget = new AGeoWidget(Sandwich->World, this);
  connect(this, SIGNAL(ObjectSelectionChanged(QString)), EditWidget, SLOT(onObjectSelectionChanged(QString)));
  connect(this, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(onItemClicked()));
  connect(EditWidget, &AGeoWidget::showMonitor, this, &AGeoTreeWidget::RequestShowMonitor);
  connect(EditWidget, &AGeoWidget::requestBuildScript, this, &AGeoTreeWidget::objectToScript);

  QString style;
  style = "QTreeView::branch:has-siblings:!adjoins-item {"
          "border-image: url(:/images/tw-vline.png) 0; }"
  "QTreeView::branch:has-siblings:adjoins-item {"
      "border-image: url(:/images/tw-branch-more.png) 0; }"
  "QTreeView::branch:!has-children:!has-siblings:adjoins-item {"
      "border-image: url(:/images/tw-branch-end.png) 0; }"
  "QTreeView::branch:has-children:!has-siblings:closed,"
  "QTreeView::branch:closed:has-children:has-siblings {"
          "border-image: none;"
          "image: url(:/images/tw-branch-closed.png);}"
  "QTreeView::branch:open:has-children:!has-siblings,"
  "QTreeView::branch:open:has-children:has-siblings  {"
          "border-image: none;"
          "image: url(:/images/tw-branch-open.png);}";
  setStyleSheet(style);

  QShortcut* Del = new QShortcut(Qt::Key_Backspace, this);
  connect(Del, &QShortcut::activated, this, &AGeoTreeWidget::onRemoveTriggered);

  QShortcut* DelRec = new QShortcut(QKeySequence(QKeySequence::Delete), this);
  connect(DelRec, &QShortcut::activated, this, &AGeoTreeWidget::onRemoveRecursiveTriggered);
}

void AGeoTreeWidget::SelectObjects(QStringList ObjectNames)
{
   clearSelection();
   //qDebug() << "Request select the following objects:"<<ObjectNames;

   for (int i=0; i<ObjectNames.size(); i++)
     {
       QList<QTreeWidgetItem*> list = findItems(ObjectNames.at(i), Qt::MatchExactly | Qt::MatchRecursive);
       if (!list.isEmpty())
         {
            //qDebug() << "Attempting to focus:"<<list.first()->text(0);
            list.first()->setSelected(true);
         }
     }
}

void AGeoTreeWidget::UpdateGui(QString selected)
{
  if (!World) return;

  //qDebug() << "Update, selected = "<<selected;
  if (selected.isEmpty() && currentItem())
  {
      //qDebug() << currentItem()->text(0);
      selected = currentItem()->text(0);
  }
  clear();

  //World
  QTreeWidgetItem *w = new QTreeWidgetItem(this);
  w->setText(0, "World");
  QFont f = w->font(0);
  f.setBold(true);
  w->setFont(0, f);
  w->setSizeHint(0, QSize(50, 20));
  w->setFlags(w->flags() & ~Qt::ItemIsDragEnabled);// & ~Qt::ItemIsSelectable);
  //w->setBackgroundColor(0, BackgroundColor);
  //qDebug() << "New world WidgetItem created";

  populateTreeWidget(w, World);
  //expandAll(); // less blunt later - e.g. on exapand remember the status in AGeoObject
  if (topLevelItemCount()>0)
    updateExpandState(this->topLevelItem(0));

  if (!selected.isEmpty())
  {
      //qDebug() << "Selection:"<<selected;
      QList<QTreeWidgetItem*> list = findItems(selected, Qt::MatchExactly | Qt::MatchRecursive);
      //qDebug() << list.size();
        if (!list.isEmpty())
        {
           //qDebug() << "Attempting to focus:"<<list.first()->text(0);
           list.first()->setSelected(true);
           setCurrentItem(list.first());
        }
  }
}

void AGeoTreeWidget::onGridReshapeRequested(QString objName)
{
    AGeoObject* obj = World->findObjectByName(objName);
    if (!obj) return;
    if (!obj->ObjectType->isGrid()) return;    
    if (!obj->getGridElement()) return;
    ATypeGridElementObject* GE = static_cast<ATypeGridElementObject*>(obj->getGridElement()->ObjectType);

    AGridElementDialog* d = new AGridElementDialog(Sandwich->Materials, this);
    switch (GE->shape)
     {
      case 0: d->setValues(0, GE->size1, GE->size2, obj->getGridElement()->Shape->getHeight()-0.001); break;
      case 1: d->setValues(1, GE->size1, GE->size2, obj->getGridElement()->Shape->getHeight()-0.001); break;
      case 2:
      {
        AGeoPgon* pg = dynamic_cast<AGeoPgon*>(obj->getGridElement()->Shape);
        if (pg)
          d->setValues(2, GE->size1, GE->size2, obj->getGridElement()->Shape->getHeight()-0.001);
        break;
      }
    }

    //setting materials
    d->setBulkMaterial(obj->Material);
    if (!obj->HostedObjects.isEmpty())
        if (!obj->HostedObjects.first()->HostedObjects.isEmpty())
        {
            int wireMat = obj->HostedObjects.first()->HostedObjects.first()->Material;
            d->setWireMaterial(wireMat);
        }

    int res = d->exec();

    if (res != 0)
    {
        //qDebug() << "Accepted!";
        switch (d->shape())
        {
        case 0: Sandwich->shapeGrid(obj, 0, d->pitch(), d->length(), d->diameter(), d->wireMaterial()); break;
        case 1: Sandwich->shapeGrid(obj, 1, d->pitchX(), d->pitchY(), d->diameter(), d->wireMaterial()); break;
        case 2: Sandwich->shapeGrid(obj, 2, d->outer(), d->inner(), d->height(), d->wireMaterial()); break;
        default:
            qWarning() << "Unknown grid type!";
        }

        obj->Material = d->bulkMaterial();

        emit RequestRebuildDetector();
        UpdateGui(objName);
    }
    //else qDebug() << "Rejected!";
    delete d;
}

void AGeoTreeWidget::populateTreeWidget(QTreeWidgetItem* parent, AGeoObject *Container, bool fDisabled)
{  
  for (int i=0; i<Container->HostedObjects.size(); i++)
    {
      AGeoObject *obj = Container->HostedObjects[i];
      QString SubName = obj->Name;      
      QTreeWidgetItem *item = new QTreeWidgetItem(parent);

      bool fDisabledLocal = fDisabled || !obj->fActive;
      if (fDisabledLocal) item->setForeground(0, QBrush(Qt::red));

      item->setText(0, SubName);
      item->setSizeHint(0, QSize(50, 20));

      if (obj->ObjectType->isHandlingStatic())
        { //this is one of the slabs or World
          item->setFlags(item->flags() & ~Qt::ItemIsDragEnabled);// & ~Qt::ItemIsSelectable);
          QFont f = item->font(0);
          f.setBold(true);
          item->setFont(0, f);
        }
      else if (obj->ObjectType->isHandlingSet() || obj->ObjectType->isArray() || obj->ObjectType->isGridElement())
        { //group or stack or array or gridElement
          QFont f = item->font(0);
          f.setItalic(true);
          item->setFont(0, f);
          updateIcon(item, obj);
          item->setBackgroundColor(0, BackgroundColor);
        }      
      else
        {
          updateIcon(item, obj);
          item->setBackgroundColor(0, BackgroundColor);
        }      

      populateTreeWidget(item, obj, fDisabledLocal);
  }
}

void AGeoTreeWidget::updateExpandState(QTreeWidgetItem *item)
{
    AGeoObject* obj = World->findObjectByName(item->text(0));
    if (obj)
    {
        if (obj->fExpanded)
        {
            expandItem(item);
            for (int i=0; i<item->childCount(); i++)
                updateExpandState(item->child(i));
        }
    }
}

void AGeoTreeWidget::dropEvent(QDropEvent* event)
{
    if (previousHoverItem)
    {
        previousHoverItem->setBackgroundColor(0, Qt::white);
        previousHoverItem = nullptr;
    }

    QList<QTreeWidgetItem*> selected = selectedItems();

    QTreeWidgetItem * itemTo = this->itemAt(event->pos());
    if (!itemTo)
    {
        qDebug() << "No item on drop position - rejected!";
        event->ignore();
        return;
    }
    QString DraggedTo = itemTo->text(0);
    AGeoObject * objTo = World->findObjectByName(DraggedTo);
    if (!objTo)
    {
        qWarning() << "Error: objTo not found!";
        event->ignore();
        return;
    }

    QStringList selNames;

    //if keyboard modifier is on, rearrange objects instead of change to new container
    const Qt::KeyboardModifiers mod = event->keyboardModifiers();
    if (mod == Qt::ALT || mod == Qt::CTRL || mod == Qt::SHIFT)
    {
        //qDebug() << "Rearrange order event triggered";
        if (selected.size() != 1)
        {
            //qDebug() << "Only one item should be selected to use rearrangment!";
            event->ignore();
            return;
        }
        QTreeWidgetItem * DraggedItem = this->selectedItems().first();
        if (!DraggedItem)
        {
            //qDebug() << "Drag source item invalid, ignore";
            event->ignore();
            return;
        }
        const QString DraggedName = DraggedItem->text(0);
        selNames << DraggedName;

        AGeoObject* obj = World->findObjectByName(DraggedName);
        if (obj)
        {
            //bool fAfter = (event->pos().y() > visualItemRect(itemTo).center().y());
            bool fAfter = (dropIndicatorPosition() == QAbstractItemView::BelowItem);
            obj->repositionInHosted(objTo, fAfter);
        }

        if (obj && obj->Container && obj->Container->ObjectType->isStack())
            obj->updateStack();
    }
    else
    {
      // Normal drag n drop
      if (objTo->ObjectType->isGrid())
      {
          event->ignore();
          QMessageBox::information(this, "", "Grid cannot contain anything but the grid element!");
          return;
      }
      if (objTo->isCompositeMemeber())
      {
          event->ignore();
          QMessageBox::information(this, "", "Cannot move objects to composite logical objects!");
          return;
      }
      if (objTo->ObjectType->isMonitor())
      {
          event->ignore();
          QMessageBox::information(this, "", "Monitors cannot host objects!");
          return;
      }

      //check if any of the items are composite members and are in use
      // possible optimisation: make search only once, -> container with AGeoObjects?
      for (int i=0; i<selected.size(); i++)
      {
          QTreeWidgetItem* DraggedItem = this->selectedItems().at(i);
          if (!DraggedItem)
            {
              qDebug() << "Drag source item invalid, ignore";
              event->ignore();
              return;
            }

          QString DraggedName = DraggedItem->text(0);
          AGeoObject* obj = World->findObjectByName(DraggedName);
          if (!obj)
          {
              qWarning() << "Error: obj not found!";
              event->ignore();
              return;
          }
          if (objTo->ObjectType->isArray() && obj->ObjectType->isArray())
          {
              event->ignore();
              QMessageBox::information(this, "", "Cannot move array directly inside another array!");
              return;
          }
          if (obj->isInUseByComposite())
          {
              event->ignore();
              QMessageBox::information(this, "", "Cannot move objects: Contains object(s) used in a composite object generation");
              return;
          }
          if (objTo->ObjectType->isCompositeContainer() && !obj->ObjectType->isSingle())
          {
              event->ignore();
              QMessageBox::information(this, "", "Can insert only elementary objects to the list of composite members!");
              return;
          }
          if (objTo->ObjectType->isHandlingSet() && !obj->ObjectType->isSingle())
          {
              event->ignore();
              QMessageBox::information(this, "", "Can insert only elementary objects to sets!");
              return;
          }
      }

      for (int i=selected.size()-1; i>-1; i--)
        {
          QTreeWidgetItem* DraggedItem = this->selectedItems().at(i);
          QString DraggedName = DraggedItem->text(0);
          selNames << DraggedName;
          //qDebug() << "Draggin item:"<< DraggedName;

          AGeoObject* obj = World->findObjectByName(DraggedName);
          AGeoObject* objFormerContainer = obj->Container;
          //qDebug() << "Dragging"<<obj->Name<<"to"<<objTo->Name<<"from"<<objFormerContainer->Name;
          bool ok = obj->migrateTo(objTo);
          if (!ok)
          {
              qWarning() << "Object migration failed: cannot migrate down the chain (or to itself)!";
              event->ignore();
              return;
          }

          if (objTo->ObjectType->isStack())
            {
              //qDebug() << "updating stack of this object";
              obj->updateStack();
            }
          if (objFormerContainer && objFormerContainer->ObjectType->isStack())
            {
              //qDebug() << "updating stack of the former container";
              if (objFormerContainer->HostedObjects.size()>0)
                objFormerContainer->HostedObjects.first()->updateStack();
            }
        }
    }

    //qDebug() << "Drag completed, updating gui";
    UpdateGui();
    emit RequestRebuildDetector();

    // Restore selection
    for (int i=0; i<selNames.size(); i++)
    {
        QList<QTreeWidgetItem*> list = findItems(selNames.at(i), Qt::MatchExactly | Qt::MatchRecursive);
        if (!list.isEmpty()) list.first()->setSelected(true);
    }

    event->ignore();
    return;
}

void AGeoTreeWidget::dragEnterEvent(QDragEnterEvent *event)
{
    previousHoverItem = nullptr;
    //qDebug() << "Drag enter. Selection size:"<< selectedItems().size();
    //attempt to drag items contaning locked objects should be canceled!

    const int numItems = selectedItems().size();
    for (int iItem = 0; iItem < numItems; iItem++)
    {
        QTreeWidgetItem * DraggedItem = selectedItems().at(iItem);
        QString DraggedName = DraggedItem->text(0);
        //qDebug() << "Draggin item:"<< DraggedName;
        AGeoObject * obj = World->findObjectByName(DraggedName);
        if (obj->fLocked || obj->isContainsLocked() || obj->ObjectType->isGridElement() || obj->ObjectType->isCompositeContainer())
        {
            qDebug() << "Drag forbidden for one of the items!";
            event->ignore();
            return;
        }
    }

    // Drop and mouseRelease are not fired if drop on the same item as started -> teal highlight is not removed
    // Clumsy fix - do not show teal highlight if the item is the same
    movingItem = ( numItems > 0 ? selectedItems().at(0)
                                : nullptr);

    event->accept();
}

void AGeoTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidget::dragMoveEvent(event);

    const Qt::KeyboardModifiers mod = event->keyboardModifiers();
    bool bRearrange = (mod == Qt::ALT || mod == Qt::CTRL || mod == Qt::SHIFT);

    setDropIndicatorShown(bRearrange);

    if (previousHoverItem)
    {
        previousHoverItem->setBackgroundColor(0, Qt::white);
        previousHoverItem = nullptr;
    }
    if (!bRearrange)
    {
        QTreeWidgetItem * itemOver = this->itemAt(event->pos());
        if (itemOver && itemOver != movingItem)
        {
            itemOver->setBackgroundColor(0, Qt::cyan);
            previousHoverItem = itemOver;
        }
    }
}

void AGeoTreeWidget::onItemSelectionChanged()
{
  QList<QTreeWidgetItem*> sel = selectedItems();

  if (sel.size() == 0)
    {
      emit ObjectSelectionChanged("");
      return;
    }
  if (sel.size() == 1)
    {
      QString name = sel.first()->text(0);
      emit ObjectSelectionChanged(name);
      return;
    }

  //multiple selected

  //allow only selection of objects of the same container!
  //static objects cannot be mixed with others
  QTreeWidgetItem* FirstParent = sel.first()->parent();
  for (int i=1; i<sel.size(); i++)
    {
      if (sel.at(i)->parent() != FirstParent)
        {
          //qDebug() << "Cannot select together items from different containers!";
          sel.at(i)->setSelected(false);
          return; // will retrigger anyway
        }
      if (sel.at(i)->font(0).bold())
        {
          //qDebug() << "Cannot select together different slabs or    world and slab(s)";
          sel.at(i)->setSelected(false);
          return; // will retrigger anyway
        }
    }

  //with multiple selected do not show EditWidget
  emit ObjectSelectionChanged("");
}

QAction* Action(QMenu& Menu, QString Text)
{
  QAction* s = Menu.addAction(Text);
  s->setEnabled(false);
  return s;
}

void AGeoTreeWidget::customMenuRequested(const QPoint &pos)
{  
  QMenu menu;

  QAction* showA = Action(menu, "Show - highlight in geometry");
  QAction* showAdown = Action(menu, "Show - this object with content");
  QAction* showAonly = Action(menu, "Show - only this object");
  QAction* lineA = Action(menu, "Change line color/width/style");

  menu.addSeparator();

  QAction* enableDisableA = Action(menu, "Enable/Disable");

  menu.addSeparator();

  QMenu * addObjMenu = menu.addMenu("Add object");
  //QFont f = addObjMenu->font();
  //f.setBold(true);
  //addObjMenu->setFont(f);
    QAction* newBox  = addObjMenu->addAction("Box");
    QMenu * addTubeMenu = addObjMenu->addMenu("Tube");
        QAction* newTube =        addTubeMenu->addAction("Tube");
        QAction* newTubeSegment = addTubeMenu->addAction("Tube segment");
        QAction* newTubeSegCut =  addTubeMenu->addAction("Tube segment cut");
        QAction* newTubeElli =    addTubeMenu->addAction("Elliptical tube");
    QMenu * addTrapMenu = addObjMenu->addMenu("Trapezoid");
        QAction* newTrapSim =     addTrapMenu->addAction("Trapezoid simplified");
        QAction* newTrap    =     addTrapMenu->addAction("Trapezoid");
    QAction* newPcon = addObjMenu->addAction("Polycone");
    QMenu * addPgonMenu = addObjMenu->addMenu("Polygon");
        QAction* newPgonSim =     addPgonMenu->addAction("Polygon simplified");
        QAction* newPgon    =     addPgonMenu->addAction("Polygon");
    QAction* newPara = addObjMenu->addAction("Parallelepiped");
    QAction* newSphere = addObjMenu->addAction("Sphere");
    QMenu * addConeMenu = addObjMenu->addMenu("Cone");
        QAction* newCone =        addConeMenu->addAction("Cone");
        QAction* newConeSeg =     addConeMenu->addAction("Cone segment");
    QAction* newTor = addObjMenu->addAction("Torus");
    QAction* newParabol = addObjMenu->addAction("Paraboloid");
    QAction* newArb8 = addObjMenu->addAction("Arb8");

  QAction* newArrayA  = Action(menu, "Add array");
  QAction* newCompositeA  = Action(menu, "Add composite object");
  QAction* newGridA = Action(menu, "Add optical grid");
  QAction* newMonitorA = Action(menu, "Add monitor");

  menu.addSeparator();

  QAction* copyA = Action(menu, "Duplicate this object");

  menu.addSeparator();

  QAction* removeThisAndHostedA = Action(menu, "Remove object and content");
  removeThisAndHostedA->setShortcut(QKeySequence(QKeySequence::Delete));
  QAction* removeA = Action(menu, "Remove object, keep its content");
  removeA->setShortcut(Qt::Key_Backspace);
  QAction* removeHostedA = Action(menu, "Remove all objects inside");

  menu.addSeparator();

  QAction* lockA = Action(menu, "Lock");
  QAction* lockallA = Action(menu, "Lock objects inside");

  menu.addSeparator();

  QAction* unlockA = Action(menu, "Unlock");
  QAction* unlockallA = Action(menu, "Unlock objects inside");

  menu.addSeparator();

  QAction* groupA = Action(menu, "Group");
  QAction* stackA = Action(menu, "Form a stack");

  menu.addSeparator();

  QAction* addUpperLGA = Action(menu, "Define compound lightguide (top)");
  QAction* addLoweLGA = Action(menu, "Define compound lightguide (bottom)");


  // enable actions according to selection
  QList<QTreeWidgetItem*> selected = selectedItems();

  addUpperLGA->setEnabled(!World->containsUpperLightGuide());
  addLoweLGA->setEnabled(!World->containsLowerLightGuide());

  QString objName;
  if      (selected.size() == 0) objName = "World"; // no object selected
  else if (selected.size() == 1)
  { //menu triggered with only one selected item
      objName = selected.first()->text(0);
      AGeoObject* obj = World->findObjectByName(objName);
      if (!obj) return;
      const ATypeGeoObject& ObjectType = *obj->ObjectType;

      bool fNotGridNotMonitor = !ObjectType.isGrid() && !ObjectType.isMonitor();

      //newA->setEnabled(fNotGridNotMonitor);
      addObjMenu->setEnabled(fNotGridNotMonitor);
      enableDisableA->setEnabled(true);      
      enableDisableA->setText( (obj->isDisabled() ? "Enable object" : "Disable object" ) );
      if (obj->getSlabModel())
          if (obj->getSlabModel()->fCenter) enableDisableA->setEnabled(false);

      newCompositeA->setEnabled(fNotGridNotMonitor);
      newArrayA->setEnabled(fNotGridNotMonitor && !ObjectType.isArray());
      newMonitorA->setEnabled(fNotGridNotMonitor && !ObjectType.isArray());
      newGridA->setEnabled(fNotGridNotMonitor);
      copyA->setEnabled( ObjectType.isSingle() || ObjectType.isSlab() || ObjectType.isMonitor());  //supported so far only Single, Slab and Monitor
      removeHostedA->setEnabled(fNotGridNotMonitor);
      //removeThisAndHostedA->setEnabled(fNotGridNotMonitor);
      removeThisAndHostedA->setEnabled(!ObjectType.isWorld());
      removeA->setEnabled(!ObjectType.isWorld());
      lockA->setEnabled(!ObjectType.isHandlingStatic() || ObjectType.isLightguide());
      unlockA->setEnabled(true);
      lockallA->setEnabled(true);
      unlockallA->setEnabled(true);
      lineA->setEnabled(true);
      showA->setEnabled(true);
      showAonly->setEnabled(true);
      showAdown->setEnabled(true);
  }
  else if (!selected.first()->font(0).bold())
  { //menu triggered with several items selected, and they are not slabs
      removeA->setEnabled(true); //world cannot be in selection with anything else anyway
      lockA->setEnabled(true);
      unlockA->setEnabled(true);
      groupA->setEnabled(true);
      stackA->setEnabled(true);
  }

  QAction* SelectedAction = menu.exec(mapToGlobal(pos));
  if (!SelectedAction) return; //nothing was selected

  // -- EXECUTE SELECTED ACTION --
  if (SelectedAction == showA)  // SHOW OBJECT
     ShowObject(objName);
  else if (SelectedAction == showAonly)
     ShowObjectOnly(objName);
  else if (SelectedAction == showAdown)
     ShowObjectRecursive(objName);
  else if (SelectedAction == lineA) // SET LINE ATTRIBUTES
     SetLineAttributes(objName);
  else if (SelectedAction == enableDisableA)
     menuActionEnableDisable(objName);
  // ADD NEW OBJECT
  else if (SelectedAction == newBox)         menuActionAddNewObject(objName, new AGeoBox());
  else if (SelectedAction == newTube)        menuActionAddNewObject(objName, new AGeoTube());
  else if (SelectedAction == newTubeSegment) menuActionAddNewObject(objName, new AGeoTubeSeg());
  else if (SelectedAction == newTubeSegCut)  menuActionAddNewObject(objName, new AGeoCtub());
  else if (SelectedAction == newTubeElli)    menuActionAddNewObject(objName, new AGeoEltu());
  else if (SelectedAction == newTrapSim)     menuActionAddNewObject(objName, new AGeoTrd1());
  else if (SelectedAction == newTrap)        menuActionAddNewObject(objName, new AGeoTrd2());
  else if (SelectedAction == newPcon)        menuActionAddNewObject(objName, new AGeoPcon());
  else if (SelectedAction == newPgonSim)     menuActionAddNewObject(objName, new AGeoPolygon());
  else if (SelectedAction == newPgon)        menuActionAddNewObject(objName, new AGeoPgon());
  else if (SelectedAction == newPara)        menuActionAddNewObject(objName, new AGeoPara());
  else if (SelectedAction == newSphere)      menuActionAddNewObject(objName, new AGeoSphere());
  else if (SelectedAction == newCone)        menuActionAddNewObject(objName, new AGeoCone());
  else if (SelectedAction == newConeSeg)     menuActionAddNewObject(objName, new AGeoConeSeg());
  else if (SelectedAction == newTor)         menuActionAddNewObject(objName, new AGeoTorus());
  else if (SelectedAction == newParabol)     menuActionAddNewObject(objName, new AGeoParaboloid());
  else if (SelectedAction == newArb8)        menuActionAddNewObject(objName, new AGeoArb8());
  //ADD NEW COMPOSITE
  else if (SelectedAction == newCompositeA)
     menuActionAddNewComposite(objName);
  else if (SelectedAction == newArrayA) //ADD NEW COMPOSITE
     menuActionAddNewArray(objName);
  else if (SelectedAction == newGridA) //ADD NEW GRID
     menuActionAddNewGrid(objName);
  else if (SelectedAction == newMonitorA) //ADD NEW MONITOR
     menuActionAddNewMonitor(objName);
  else if (SelectedAction == addUpperLGA || SelectedAction == addLoweLGA) // ADD LIGHTGUIDE
     addLightguide(SelectedAction == addUpperLGA);
  else if (SelectedAction == copyA) // COPY OBJECT
     menuActionCopyObject(objName);
  else if (SelectedAction == groupA || SelectedAction == stackA) //GROUP & STACK
    {
      int option = (SelectedAction == groupA) ? 0 : 1;
      formSet(selected, option);
    }
  else if (SelectedAction == lockA) // LOCK
     menuActionLock();
  else if (SelectedAction == unlockA) // UNLOCK
     menuActionUnlock();
  else if (SelectedAction == lockallA) // LOCK OBJECTS INSIDE
     menuActionLockAllInside(objName);
  else if (SelectedAction == unlockallA)
     menuActionUnlockAllInside(objName);
  else if (SelectedAction == removeA) // REMOVE
     menuActionRemove();
  else if (SelectedAction == removeThisAndHostedA) // REMOVE RECURSIVLY
     menuActionRemoveRecursively(objName);
  else if (SelectedAction == removeHostedA) // REMOVE HOSTED
      menuActionRemoveHostedObjects(objName);
}

void AGeoTreeWidget::onItemClicked()
{
    if (fSpecialGeoViewMode)
      {
          fSpecialGeoViewMode = false;
          emit RequestNormalDetectorDraw();
      }
}

void AGeoTreeWidget::onItemExpanded(QTreeWidgetItem *item)
{
    AGeoObject* obj = World->findObjectByName(item->text(0));
    if (obj) obj->fExpanded = true;
}

void AGeoTreeWidget::onItemCollapsed(QTreeWidgetItem *item)
{
    AGeoObject* obj = World->findObjectByName(item->text(0));
    if (obj) obj->fExpanded = false;
}

void AGeoTreeWidget::onRemoveTriggered()
{
    menuActionRemove();
}

void AGeoTreeWidget::onRemoveRecursiveTriggered()
{
    QList<QTreeWidgetItem*> selected = selectedItems();
    if (selected.isEmpty()) return;

    if (selected.size() > 1)
    {
        message("This action can be used when only ONE volume is selected", this);
        return;
    }

    menuActionRemoveRecursively(selected.first()->text(0));
}

void AGeoTreeWidget::menuActionRemove()
{
  QList<QTreeWidgetItem*> selected = selectedItems();
  if (selected.isEmpty()) return;

  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle("Locked objects are NOT removed!");
  QString str;// = "\nThis command, executed on a slab or lightguide removes it too!";
  if (selected.size() == 1)  str = "Remove "+selected.first()->text(0)+"?";
  else str = "Remove selected objects?";
  str += "                                             ";
  msgBox.setText(str);
  QPushButton *remove = msgBox.addButton(QMessageBox::Yes);
  QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
  msgBox.setDefaultButton(cancel);

  msgBox.exec();

  if (msgBox.clickedButton() == remove)
    {
      //emit ObjectSelectionChanged("");
      for (int i=0; i<selected.size(); i++)
        {
          QString ObjectName = selected.at(i)->text(0);
          AGeoObject* obj = World->findObjectByName(ObjectName);
          if (obj)
          {
              if (obj->isInUseByComposite()) continue;
              obj->suicide(true);
          }
        }
      UpdateGui();
      emit RequestRebuildDetector();
    }
}

void AGeoTreeWidget::menuActionRemoveRecursively(QString ObjectName)
{
    AGeoObject * obj = World->findObjectByName(ObjectName);
    if (!obj)
    {
        qWarning() << "Error: object" << ObjectName << "not found in the geometry!";
        return;
    }

    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setWindowTitle("Locked objects are NOT removed!");

    QString str;
    if (obj->ObjectType->isSlab())
        str = "Remove all objects hosted inside " + ObjectName + " slab?";
    else if (obj->ObjectType->isWorld())
        str = "Remove all non-slab objects from the geometry?";
    else
        str = "Remove " + ObjectName + " and all objects hosted inside?";

    msgBox.setText(str);
    QPushButton *remove = msgBox.addButton(QMessageBox::Yes);
    QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
    msgBox.setDefaultButton(cancel);

    msgBox.exec();

    if (msgBox.clickedButton() == remove)
    {
        //emit ObjectSelectionChanged("");
        obj->recursiveSuicide();
        //UpdateGui();
        emit RequestRebuildDetector();
    }
}

void AGeoTreeWidget::menuActionRemoveHostedObjects(QString ObjectName)
{
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Question);
  msgBox.setWindowTitle("Locked objects will NOT be deleted!");
  msgBox.setText("Delete objects hosted inside " + ObjectName + "?\nSlabs and lightguides are NOT removed.");
  QPushButton *remove = msgBox.addButton(QMessageBox::Yes);
  QPushButton *cancel = msgBox.addButton(QMessageBox::Cancel);
  msgBox.setDefaultButton(cancel);

  msgBox.exec();

  if (msgBox.clickedButton() == remove)
    {
      //emit ObjectSelectionChanged("");
      AGeoObject* obj = World->findObjectByName(ObjectName);
      if (obj)
        for (int i=obj->HostedObjects.size()-1; i>-1; i--)
          obj->HostedObjects[i]->recursiveSuicide();
      UpdateGui();
      emit RequestRebuildDetector();
    }
}

void AGeoTreeWidget::menuActionUnlockAllInside(QString ObjectName)
{
  int ret = QMessageBox::question(this, "",
                                 "Unlock all objects inside "+ObjectName+"?",
                                 QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
  if (ret == QMessageBox::Yes)
    {
      AGeoObject* obj = World->findObjectByName(ObjectName);
      if (obj)
        {
          for (int i=0; i<obj->HostedObjects.size(); i++)
            obj->HostedObjects[i]->unlockAllInside();
          UpdateGui();
        }
    }
}

void AGeoTreeWidget::menuActionLockAllInside(QString ObjectName)
{
  int ret = QMessageBox::question(this, "",
                                 "Lock all objects inside "+ObjectName+"?",
                                 QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel);
  if (ret == QMessageBox::Yes)
    {
      AGeoObject* obj = World->findObjectByName(ObjectName);
      if (obj)
        {
          obj->lockRecursively();
          obj->lockUpTheChain();
          UpdateGui();
        }
    }
}

void AGeoTreeWidget::menuActionUnlock()
{
  bool fContainsLocked = false;
  QList<QTreeWidgetItem*> selected = selectedItems();
  for (int i=0; i<selected.size(); i++)
    {
      QString Object = selected.at(i)->text(0);
      AGeoObject* obj = World->findObjectByName(Object);
      fContainsLocked = obj->isContainsLocked();
      if (fContainsLocked) break;
    }

  if (fContainsLocked) QMessageBox::information(this, "", "Cannot unlock selected object(s): some of the objects inside are locked!");
  else
    {
      for (int i=0; i<selected.size(); i++)
        {
          QString Object = selected.at(i)->text(0);
          AGeoObject* obj = World->findObjectByName(Object);
          if (obj) obj->fLocked = false;
        }
      if (selected.size() == 1)
          UpdateGui(selected.first()->text(0));
      else UpdateGui();
    }
}

void AGeoTreeWidget::menuActionLock()
{
  QList<QTreeWidgetItem*> selected = selectedItems();
  for (int i=0; i<selected.size(); i++)
    {
      QString Object = selected.at(i)->text(0);
      AGeoObject* obj = World->findObjectByName(Object);
      if (obj) obj->lockUpTheChain();
    }
  if (selected.size() == 1) UpdateGui(selected.first()->text(0));
  else UpdateGui();
}

void AGeoTreeWidget::menuActionCopyObject(QString ObjToCopyName)
{
  AGeoObject* ObjToCopy = World->findObjectByName(ObjToCopyName);
  if (!ObjToCopy || ObjToCopy->ObjectType->isWorld()) return;

  if ( !(ObjToCopy->ObjectType->isSingle() || ObjToCopy->ObjectType->isSlab() || ObjToCopy->ObjectType->isMonitor()) ) return; //supported so far only Single and Slab

  if (ObjToCopy->ObjectType->isSlab())
  {
    ATypeSlabObject* slab = static_cast<ATypeSlabObject*>(ObjToCopy->ObjectType);
    ObjToCopy->UpdateFromSlabModel(slab->SlabModel);
  }

  AGeoObject* newObj = new AGeoObject(ObjToCopy);
  if (ObjToCopy->ObjectType->isMonitor())
  {
      while (World->isNameExists(newObj->Name))
        newObj->Name = AGeoObject::GenerateRandomMonitorName();
      delete newObj->ObjectType;
      ATypeMonitorObject* mt = new ATypeMonitorObject();
      mt->config = static_cast<ATypeMonitorObject*>(ObjToCopy->ObjectType)->config;
      newObj->ObjectType = mt;
  }
  else
  {
      while (World->isNameExists(newObj->Name))
        newObj->Name = AGeoObject::GenerateRandomObjectName();
  }

  AGeoObject* container = ObjToCopy->Container;
  if (!container) container = World;
  container->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!

  QString name = newObj->Name;
  UpdateGui(name);
  emit RequestRebuildDetector();
  emit RequestHighlightObject(name);
  UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewObject(QString ContainerName, AGeoShape * shape)
{
  AGeoObject* ContObj = World->findObjectByName(ContainerName);
  if (!ContObj) return;

  AGeoObject* newObj = new AGeoObject();
  while (World->isNameExists(newObj->Name))
    newObj->Name = AGeoObject::GenerateRandomObjectName();

  delete newObj->Shape;
  newObj->Shape = shape;

  newObj->color = 1;
  ContObj->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!
  QString name = newObj->Name;
  UpdateGui(name);
  emit RequestRebuildDetector();
  UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewArray(QString ContainerName)
{
  AGeoObject* ContObj = World->findObjectByName(ContainerName);
  if (!ContObj) return;

  AGeoObject* newObj = new AGeoObject();
  do
    {
      newObj->Name = AGeoObject::GenerateRandomArrayName();
    }
  while (World->isNameExists(newObj->Name));

  delete newObj->ObjectType;
  newObj->ObjectType = new ATypeArrayObject();

  newObj->color = 1;
  ContObj->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!
  QString name = newObj->Name;

  //element inside
  AGeoObject* elObj = new AGeoObject();
  while (World->isNameExists(elObj->Name))
    elObj->Name = AGeoObject::GenerateRandomObjectName();
  elObj->color = 1;
  newObj->addObjectFirst(elObj);

  UpdateGui(name);
  emit RequestRebuildDetector();
  UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewGrid(QString ContainerName)
{
  AGeoObject* ContObj = World->findObjectByName(ContainerName);
  if (!ContObj) return;

  AGeoObject* newObj = new AGeoObject();
  do newObj->Name = AGeoObject::GenerateRandomGridName();
  while (World->isNameExists(newObj->Name));
  if (newObj->Shape) delete newObj->Shape;
  newObj->Shape = new AGeoBox(50, 50, 0.501);
  newObj->Material = ContObj->Material;

  newObj->color = 1;
  ContObj->addObjectFirst(newObj);
  Sandwich->convertObjToGrid(newObj);

  QString name = newObj->Name;
  UpdateGui(name);
  emit RequestRebuildDetector();
  UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewMonitor(QString ContainerName)
{
    AGeoObject* ContObj = World->findObjectByName(ContainerName);
    if (!ContObj) return;

    AGeoObject* newObj = new AGeoObject();
    do newObj->Name = AGeoObject::GenerateRandomMonitorName();
    while (World->isNameExists(newObj->Name));

    newObj->Material = ContObj->Material;

    delete newObj->ObjectType;
    newObj->ObjectType = new ATypeMonitorObject();

    newObj->updateMonitorShape();

    newObj->color = 1;
    ContObj->addObjectFirst(newObj);

    QString name = newObj->Name;
    //UpdateGui(name);
    emit RequestRebuildDetector();
    UpdateGui(name);
}

void AGeoTreeWidget::menuActionAddNewComposite(QString ContainerName)
{
  AGeoObject* ContObj = World->findObjectByName(ContainerName);
  if (!ContObj) return;

  AGeoObject* newObj = new AGeoObject();
  do newObj->Name = AGeoObject::GenerateRandomCompositeName();
  while (World->isNameExists(newObj->Name));

  newObj->color = 1;
  ContObj->addObjectFirst(newObj);  //inserts to the first position in the list of HostedObjects!

  Sandwich->convertObjToComposite(newObj);

  QString name = newObj->Name;
  UpdateGui(name);
  emit RequestRebuildDetector();
  UpdateGui(name);
}

void AGeoTreeWidget::SetLineAttributes(QString ObjectName)
{
    AGeoObject* obj = World->findObjectByName(ObjectName);
    if (!obj) return;

    ARootLineConfigurator* rlc = new ARootLineConfigurator(&obj->color, &obj->width, &obj->style, this);
    int res = rlc->exec();
    if (res != 0)
    {
        if (obj->ObjectType->isSlab())
        {
            obj->getSlabModel()->color = obj->color;
            obj->getSlabModel()->width = obj->width;
            obj->getSlabModel()->style = obj->style;
        }
        if (obj->ObjectType->isArray() || obj->ObjectType->isHandlingSet())
        {
            QVector<AGeoObject*> vec;
            obj->collectContainingObjects(vec);
            for (AGeoObject * co : vec)
            {
                co->color = obj->color;
                co->width = obj->width;
                co->style = obj->style;
            }
        }
        emit RequestRebuildDetector();
        //UpdateGui(ObjectName);
    }
}

void AGeoTreeWidget::ShowObject(QString ObjectName)
{
  AGeoObject* obj = World->findObjectByName(ObjectName);
  if (obj)
  {
      fSpecialGeoViewMode = true;
      emit RequestHighlightObject(ObjectName);
      UpdateGui(ObjectName);
  }
}

void AGeoTreeWidget::ShowObjectRecursive(QString ObjectName)
{
    AGeoObject* obj = World->findObjectByName(ObjectName);
    if (obj)
    {
        fSpecialGeoViewMode = true;
        emit RequestShowObjectRecursive(ObjectName);
        UpdateGui(ObjectName);
    }
}

void AGeoTreeWidget::ShowObjectOnly(QString ObjectName)
{
    fSpecialGeoViewMode = true;
    AGeoObject* obj = World->findObjectByName(ObjectName);
    TGeoShape* sh = obj->Shape->createGeoShape();
    sh->Draw();
}

void AGeoTreeWidget::menuActionEnableDisable(QString ObjectName)
{
    AGeoObject* obj = World->findObjectByName(ObjectName);
    if (obj)
    {
        if (obj->isDisabled()) obj->enableUp();
        else
        {
            obj->fActive = false;
            if (obj->ObjectType->isSlab()) obj->getSlabModel()->fActive = false;
        }

        obj->fExpanded = obj->fActive;

        QString name = obj->Name;
        emit RequestRebuildDetector();
        UpdateGui(name);
    }
}

void AGeoTreeWidget::formSet(QList<QTreeWidgetItem*> selected, int option) //option 0->group, option 1->stack
{
  //creating a set
  AGeoObject* grObj = new AGeoObject();

  delete grObj->ObjectType;
  if (option == 0)
      grObj->ObjectType = new ATypeGroupContainerObject();
  else
      grObj->ObjectType = new ATypeStackContainerObject();

  do
    grObj->Name = (option==0) ? AGeoObject::GenerateRandomGroupName() : AGeoObject::GenerateRandomStackName();
  while (World->isNameExists(grObj->Name));
  //qDebug() << "--Created new set object:"<<grObj->Name;

  QString FirstName = selected.first()->text(0); //selected objects always have the same container
  AGeoObject* firstObj = World->findObjectByName(FirstName);
  if (!firstObj) return;
  AGeoObject* contObj = firstObj->Container;
  grObj->Container = contObj;
  //qDebug() << "--Group will be hosted by:"<<grObj->Container->Name;

  for (int i=0; i<selected.size(); i++)
    {
      QString name = selected.at(i)->text(0);
      AGeoObject* obj = World->findObjectByName(name);
      if (!obj) continue;
      contObj->HostedObjects.removeOne(obj);
      obj->Container = grObj;
      grObj->HostedObjects.append(obj);
    }
  contObj->HostedObjects.insert(0, grObj);

  QString name = grObj->Name;
  if (option == 1)
  {
    firstObj->updateStack();
    emit RequestRebuildDetector();
  }
  //qDebug() << "--Done! updating gui";
  UpdateGui(name);
}

void AGeoTreeWidget::addLightguide(bool upper)
{
    AGeoObject* obj = Sandwich->addLightguide(upper);

    //qDebug() << "Done, rebuidling detector and refreshing gui";
    QString name = obj->Name;
    UpdateGui(name);
    qApp->processEvents();
    emit RequestRebuildDetector();
    UpdateGui(name);
}

QImage createImageWithOverlay(const QImage& base, const QImage& overlay)
{
    QImage imageWithOverlay = QImage(overlay.size(), QImage::Format_ARGB32_Premultiplied);
    QPainter painter(&imageWithOverlay);

    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(imageWithOverlay.rect(), Qt::transparent);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, base);

    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawImage(0, 0, overlay);

    painter.end();

    return imageWithOverlay;
}

void AGeoTreeWidget::updateIcon(QTreeWidgetItem* item, AGeoObject *obj)
{  
  if (!obj || !item) return;

  QImage image;

  AGeoObject* cont = obj->Container;
  if (cont && !cont->HostedObjects.isEmpty())
  {
      if (cont->ObjectType->isGroup())
      {
          if (obj == cont->HostedObjects.first())
              image = GroupStart;
          else if (obj == cont->HostedObjects.last())
              image = GroupEnd;
          else
              image = GroupMid;
      }
      else if (cont->ObjectType->isStack())
      {
          if (obj == cont->HostedObjects.first())
              image = StackStart;
          else if (obj == cont->HostedObjects.last())
              image = StackEnd;
          else
              image = StackMid;
      }
  }

  if (obj->fLocked)
    {
      if (image.isNull())
        image = Lock;
      else
        image = createImageWithOverlay(Lock, image);
    }

  QIcon icon = QIcon(QPixmap::fromImage(image));
  item->setIcon(0, icon); 
}

void AGeoTreeWidget::objectMembersToScript(AGeoObject* Master, QString &script, int ident, bool bExpandMaterial, bool bRecursive)
{
    for (AGeoObject* obj : Master->HostedObjects)
        objectToScript(obj, script, ident, bExpandMaterial, bRecursive);
}

void AGeoTreeWidget::objectToScript(AGeoObject *obj, QString &script, int ident, bool bExpandMaterial, bool bRecursive)
{
    const QString Starter = "\n" + QString(" ").repeated(ident);

    if (obj->ObjectType->isLogical())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_basicObject(obj, bExpandMaterial);
    }
    else if (obj->ObjectType->isCompositeContainer())
    {
         //nothing to do
    }
    else if (obj->ObjectType->isSlab() || obj->ObjectType->isSingle() )
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_basicObject(obj, bExpandMaterial);
        script += "\n" + QString(" ").repeated(ident)+ makeLinePropertiesString(obj);
        if (obj->ObjectType->isLightguide())
        {
            script += "\n";
            script += "\n" + QString(" ").repeated(ident)+ "//=== Lightguide object is not supported! ===";
            script += "\n";
        }
        if (bRecursive) objectMembersToScript(obj, script, ident + 2, bExpandMaterial, bRecursive);
    }
    else if (obj->ObjectType->isComposite())
    {
        script += "\n" + QString(" ").repeated(ident) + "//-->-- logical volumes for " + obj->Name;
        objectMembersToScript(obj->getContainerWithLogical(), script, ident + 4, bExpandMaterial, bRecursive);
        script += "\n" + QString(" ").repeated(ident) + "//--<-- logical volumes end for " + obj->Name;

        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_basicObject(obj, bExpandMaterial);
        script += "\n" + QString(" ").repeated(ident)+ makeLinePropertiesString(obj);
        if (bRecursive) objectMembersToScript(obj, script, ident + 2, bExpandMaterial, bRecursive);
    }
    else if (obj->ObjectType->isArray())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_arrayObject(obj);
        script += "\n" + QString(" ").repeated(ident)+ "//-->-- array elements for " + obj->Name;
        objectMembersToScript(obj, script, ident + 2, bExpandMaterial, bRecursive);
        script += "\n" + QString(" ").repeated(ident)+ "//--<-- array elements end for " + obj->Name;
    }
    else if (obj->ObjectType->isMonitor())
    {
        script += Starter + makeScriptString_monitorBaseObject(obj);
        script += Starter + makeScriptString_monitorConfig(obj);
    }
    else if (obj->ObjectType->isStack())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_stackObjectStart(obj);
        script += "\n" + QString(" ").repeated(ident)+ "//-->-- stack elements for " + obj->Name;
        script += "\n" + QString(" ").repeated(ident)+ "// Values of x, y, z only matter for the stack element, refered to at InitializeStack below";
        script += "\n" + QString(" ").repeated(ident)+ "// For the rest of elements they are calculated automatically";
        objectMembersToScript(obj, script, ident + 2, bExpandMaterial, bRecursive);
        script += "\n" + QString(" ").repeated(ident)+ "//--<-- stack elements end for " + obj->Name;
        if (!obj->HostedObjects.isEmpty())
            script += "\n" + QString(" ").repeated(ident)+ makeScriptString_stackObjectEnd(obj);
    }
    else if (obj->ObjectType->isGroup())
    {
        script += "\n" + QString(" ").repeated(ident)+ makeScriptString_groupObjectStart(obj);
        script += "\n" + QString(" ").repeated(ident)+ "//-->-- group elements for " + obj->Name;
        objectMembersToScript(obj, script, ident + 2, bExpandMaterial, bRecursive);
        script += "\n" + QString(" ").repeated(ident)+ "//--<-- group elements end for " + obj->Name;
    }
    else if (obj->ObjectType->isGrid())
    {
        script += "\n";
        script += "\n" + QString(" ").repeated(ident)+ "//=== Optical grid object is not supported! Make a request to the developers ===";
        script += "\n";
    }
}

void AGeoTreeWidget::rebuildDetetctorAndRestoreCurrentDelegate()
{
    const QString CurrentObjName = ( EditWidget->getCurrentObject() ? EditWidget->getCurrentObject()->Name : "" );
    emit RequestRebuildDetector();
    UpdateGui(CurrentObjName);
}

const QString AGeoTreeWidget::makeScriptString_basicObject(AGeoObject* obj, bool bExpandMaterials) const
{
    QVector<QString> posStrs; posStrs.reserve(3);
    QVector<QString> oriStrs; oriStrs.reserve(3);
    for (int i = 0; i < 3; i++)
    {
        posStrs << ( obj->PositionStr[i].isEmpty() ? QString::number(obj->Position[i]) : obj->PositionStr[i] );
        oriStrs << ( obj->OrientationStr[i].isEmpty() ? QString::number(obj->Orientation[i]) : obj->OrientationStr[i] );
    }

    QString str = QString("geo.TGeo( ") +
            "'" + obj->Name + "', " +
            "'" + obj->Shape->getGenerationString(true) + "', " +
            (bExpandMaterials && obj->Material < Sandwich->GetMaterials().size() ?
                 Sandwich->GetMaterials().at(obj->Material) + "_mat" : QString::number(obj->Material)) + ", "
            "'" + obj->Container->Name + "',   "+
            posStrs[0] + ", " +
            posStrs[1] + ", " +
            posStrs[2] + ",   " +
            oriStrs[0] + ", " +
            oriStrs[1] + ", " +
            oriStrs[2] + " )";

    AGeoConsts::getConstInstance().formulaToJavaScript(str);
    return str;
}

QString AGeoTreeWidget::makeScriptString_arrayObject(AGeoObject *obj)
{
    ATypeArrayObject* a = dynamic_cast<ATypeArrayObject*>(obj->ObjectType);
    if (!a)
    {
        qWarning() << "It is not an array!";
        return "Error accessing object as array!";
    }

    return  QString("geo.Array( ") +
            "'" + obj->Name + "', " +
            QString::number(a->numX) + ", " +
            QString::number(a->numY) + ", " +
            QString::number(a->numZ) + ",   " +
            QString::number(a->stepX) + ", " +
            QString::number(a->stepY) + ", " +
            QString::number(a->stepZ) + ", " +
            "'" + obj->Container->Name + "',   " +
            QString::number(obj->Position[0]) + ", " +
            QString::number(obj->Position[1]) + ", " +
            QString::number(obj->Position[2]) + ",   " +
            QString::number(obj->Orientation[2]) + " )";
}

const QString AGeoTreeWidget::makeScriptString_monitorBaseObject(const AGeoObject * obj) const
{
    ATypeMonitorObject * m = dynamic_cast<ATypeMonitorObject*>(obj->ObjectType);
    if (!m)
    {
        qWarning() << "It is not a monitor!";
        return "Error accessing monitor!";
    }
    const AMonitorConfig & c = m->config;

    // geo.Monitor( name,  shape,  size1,  size2,  container,  x,  y,  z,  phi,  theta,  psi,  SensitiveTop,  SensitiveBottom,  StopsTraking )
    return QString("geo.Monitor( %1, %2,  %3, %4,  %5,   %6, %7, %8,   %9, %10, %11,   %12, %13,   %14 )")
            .arg("'" + obj->Name + "'")
            .arg(c.shape)
            .arg(2.0*c.size1)
            .arg(2.0*c.size2)
            .arg("'" + obj->Container->Name + "'")
            .arg(obj->Position[0])
            .arg(obj->Position[1])
            .arg(obj->Position[2])
            .arg(obj->Orientation[0])
            .arg(obj->Orientation[1])
            .arg(obj->Orientation[2])
            .arg(c.bUpper ? "true" : "false")
            .arg(c.bLower ? "true" : "false")
            .arg(c.bStopTracking ? "true" : "false");
}

const QString AGeoTreeWidget::makeScriptString_monitorConfig(const AGeoObject *obj) const
{
    ATypeMonitorObject * m = dynamic_cast<ATypeMonitorObject*>(obj->ObjectType);
    if (!m)
    {
        qWarning() << "It is not a monitor!";
        return "Error accessing monitor!";
    }
    const AMonitorConfig & c = m->config;

    if (c.PhotonOrParticle == 0)
    {
        //geo.Monitor_ConfigureForPhotons( MonitorName,  Position,  Time,  Angle,  Wave )
        return QString("geo.Monitor_ConfigureForPhotons( %1,  [%2, %3],  [%4, %5, %6],  [%7, %8, %9],  [%10, %11, %12] )")
                .arg("'" + obj->Name + "'")
                .arg(c.xbins)
                .arg(c.ybins)
                .arg(c.timeBins)
                .arg(c.timeFrom)
                .arg(c.timeTo)
                .arg(c.angleBins)
                .arg(c.angleFrom)
                .arg(c.angleTo)
                .arg(c.waveBins)
                .arg(c.waveFrom)
                .arg(c.waveTo);
    }
    else
    {
        //geo.Monitor_ConfigureForParticles( MonitorName,  ParticleIndex,  Both_Primary_Secondary,  Both_Direct_Indirect,  Position,  Time,  Angle,  Energy )
        return QString("geo.Monitor_ConfigureForParticles( %1,  %2,  %3,  %4,   [%5, %6],  [%7, %8, %9],  [%10, %11, %12],  [%13, %14, %15, %16] )")
                .arg("'" + obj->Name + "'")
                .arg(c.ParticleIndex)
                .arg(c.bPrimary && c.bSecondary ? 0 : (c.bPrimary ? 1 : 2))
                .arg(c.bDirect  && c.bIndirect  ? 0 : (c.bDirect  ? 1 : 2))
                .arg(c.xbins)
                .arg(c.ybins)
                .arg(c.timeBins)
                .arg(c.timeFrom)
                .arg(c.timeTo)
                .arg(c.angleBins)
                .arg(c.angleFrom)
                .arg(c.angleTo)
                .arg(c.energyBins)
                .arg(c.energyFrom)
                .arg(c.energyTo)
                .arg(c.energyUnitsInHist);
    }
}

QString AGeoTreeWidget::makeScriptString_stackObjectStart(AGeoObject *obj)
{
    return  QString("geo.MakeStack(") +
            "'" + obj->Name + "', " +
            "'" + obj->Container->Name + "' )";
}

QString AGeoTreeWidget::makeScriptString_groupObjectStart(AGeoObject *obj)
{
    return  QString("geo.MakeGroup(") +
            "'" + obj->Name + "', " +
            "'" + obj->Container->Name + "' )";
}

QString AGeoTreeWidget::makeScriptString_stackObjectEnd(AGeoObject *obj)
{
    return QString("geo.InitializeStack( ") +
           "'" + obj->Name + "',  " +
           "'" + obj->HostedObjects.first()->Name + "' )";
}

QString AGeoTreeWidget::makeLinePropertiesString(AGeoObject *obj)
{
    return "geo.SetLine( '" +
            obj->Name +
            "',  " +
            QString::number(obj->color) + ",  " +
            QString::number(obj->width) + ",  " +
            QString::number(obj->style) + " )";
}

// ================== EDIT WIDGET ===================

AGeoWidget::AGeoWidget(AGeoObject *World, AGeoTreeWidget *tw) :
  World(World), tw(tw)
{
  lMain = new QVBoxLayout(this);
  lMain->setContentsMargins(2,2,2,5);
  this->setLayout(lMain);

  //Scroll area in middle
  QScrollArea* sa = new QScrollArea(this);
  sa->setFrameShape(QFrame::Box);//NoFrame);
  sa->setContentsMargins(2,2,2,2);
  sa->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
  sa->setWidgetResizable(true);
  sa->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  QWidget* scrollAreaWidgetContents = new QWidget();
  scrollAreaWidgetContents->setGeometry(QRect(0, 0, 350, 200));

  ObjectLayout = new QVBoxLayout(scrollAreaWidgetContents);
  ObjectLayout->setContentsMargins(0,0,0,0);

  sa->setWidget(scrollAreaWidgetContents);
  lMain->addWidget(sa);

  frBottom = new QFrame();
  frBottom->setFrameShape(QFrame::StyledPanel);
  frBottom->setMinimumHeight(38);
  frBottom->setMaximumHeight(38);
  QPalette palette = frBottom->palette();
  palette.setColor( backgroundRole(), QColor( 255, 255, 255 ) );
  frBottom->setPalette( palette );
  frBottom->setAutoFillBackground( true );
  QHBoxLayout* lb = new QHBoxLayout();
  lb->setContentsMargins(0,0,0,0);
  frBottom->setLayout(lb);
    pbConfirm = new QPushButton("Confirm changes");    
    pbConfirm->setMinimumHeight(25);
    connect(pbConfirm, SIGNAL(clicked()), this, SLOT(onConfirmPressed()));
    pbConfirm->setMaximumWidth(150);
    lb->addWidget(pbConfirm);
    pbCancel = new QPushButton("Cancel changes");
    connect(pbCancel, SIGNAL(clicked()), this, SLOT(onCancelPressed()));
    pbCancel->setMaximumWidth(150);
    pbCancel->setMinimumHeight(25);
    lb->addWidget(pbCancel);
  lMain->addWidget(frBottom);

  pbConfirm->setEnabled(false);
  pbCancel->setEnabled(false);

  fIgnoreSignals = false;
}

void AGeoWidget::ClearGui()
{
    //qDebug() << "Object widget clear triggered!";
    fIgnoreSignals = true;

    while(ObjectLayout->count() > 0)
    {
        QLayoutItem* item = ObjectLayout->takeAt(0);
        if (item->widget())
            delete item->widget();
        delete item;
    }

    delete GeoDelegate; GeoDelegate = nullptr;

    fIgnoreSignals = false;

    //if update triggered during editing
    exitEditingMode();
}

void AGeoWidget::UpdateGui()
{  
    //qDebug() << "UpdateGui triggered";
    ClearGui(); //deletes Delegate!

    if (!CurrentObject) return;

    pbConfirm->setEnabled(true);
    pbCancel->setEnabled(true);

    //AGeoObject* contObj = CurrentObject->Container;
    //if (!contObj) return; //true only for World

    if (CurrentObject->ObjectType->isWorld())
        GeoDelegate = new AWorldDelegate(tw->Sandwich->Materials, this);
    else if (CurrentObject->ObjectType->isSlab())        // SLAB or LIGHTGUIDE
        GeoDelegate = createAndAddSlabDelegate();
    else if (CurrentObject->ObjectType->isGridElement())
        GeoDelegate = createAndAddGridElementDelegate();
    else if (CurrentObject->ObjectType->isMonitor())
        GeoDelegate = createAndAddMonitorDelegate();
    else
        GeoDelegate = createAndAddGeoObjectDelegate();

    GeoDelegate->Update(CurrentObject);

    GeoDelegate->Widget->setEnabled(!CurrentObject->fLocked);
    connect(GeoDelegate, &AGeoBaseDelegate::ContentChanged,             this, &AGeoWidget::onStartEditing);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestChangeVisAttributes, this, &AGeoWidget::onRequestSetVisAttributes);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestShow,                this, &AGeoWidget::onRequestShowCurrentObject);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestScriptToClipboard,   this, &AGeoWidget::onRequestScriptLineToClipboard);
    connect(GeoDelegate, &AGeoBaseDelegate::RequestScriptRecursiveToClipboard,   this, &AGeoWidget::onRequestScriptRecursiveToClipboard);

    ObjectLayout->addStretch();
    ObjectLayout->addWidget(GeoDelegate->Widget);
    ObjectLayout->addStretch();
}

AGeoBaseDelegate * AGeoWidget::createAndAddGeoObjectDelegate()
{
    AGeoObjectDelegate * Del;

    AGeoScaledShape * scaled = dynamic_cast<AGeoScaledShape*>(CurrentObject->Shape);
    const QString shape = (scaled ? scaled->getBaseShapeType() : CurrentObject->Shape->getShapeType());

    if (CurrentObject->ObjectType->isArray())
        Del = new AGeoArrayDelegate(tw->Sandwich->Materials, this);
    else if (CurrentObject->ObjectType->isHandlingSet())
        Del = new AGeoSetDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoBBox")
        Del = new AGeoBoxDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoTube")
        Del = new AGeoTubeDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoTubeSeg")
        Del = new AGeoTubeSegDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoCtub")
        Del = new AGeoTubeSegCutDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoEltu")
        Del = new AGeoElTubeDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoPara")
        Del = new AGeoParaDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoSphere")
        Del = new AGeoSphereDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoTrd1")
        Del = new AGeoTrapXDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoTrd2")
        Del = new AGeoTrapXYDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoCone")
        Del = new AGeoConeDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoConeSeg")
        Del = new AGeoConeSegDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoParaboloid")
        Del = new AGeoParaboloidDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoTorus")
        Del = new AGeoTorusDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoPolygon")
        Del = new AGeoPolygonDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoPcon")
        Del = new AGeoPconDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoPgon")
        Del = new AGeoPgonDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoCompositeShape")
        Del = new AGeoCompositeDelegate(tw->Sandwich->Materials, this);
    else if (shape == "TGeoArb8")
        Del = new AGeoArb8Delegate(tw->Sandwich->Materials, this);
    else
        Del = new AGeoObjectDelegate(tw->Sandwich->Materials, this);

    connect(Del, &AGeoObjectDelegate::RequestChangeShape,     this, &AGeoWidget::onRequestChangeShape);

    return Del;
}

AGeoBaseDelegate * AGeoWidget::createAndAddSlabDelegate()
{
    AGeoBaseDelegate * Del;

    ASlabModel * SlabModel = (static_cast<ATypeSlabObject*>(CurrentObject->ObjectType))->SlabModel;
    switch (SlabModel->XYrecord.shape)
    {
    default: qWarning() << "Unknown slab shape, assuming rectangular";
    case 0:
    {
        AGeoObjectDelegate * del = new AGeoSlabDelegate_Box(tw->Sandwich->Materials, static_cast<int>(tw->Sandwich->SandwichState), this);
        connect(del, &AGeoObjectDelegate::RequestChangeSlabShape, this, &AGeoWidget::onRequestChangeSlabShape);
        Del = del; // temporary!
        break;
    }
    case 1:
    {
        AGeoObjectDelegate * del = new AGeoSlabDelegate_Tube(tw->Sandwich->Materials, static_cast<int>(tw->Sandwich->SandwichState), this);
        connect(del, &AGeoObjectDelegate::RequestChangeSlabShape, this, &AGeoWidget::onRequestChangeSlabShape);
        Del = del; // temporary!
        break;
    }
    case 2:
    {
        AGeoObjectDelegate * del = new AGeoSlabDelegate_Poly(tw->Sandwich->Materials, static_cast<int>(tw->Sandwich->SandwichState), this);
        connect(del, &AGeoObjectDelegate::RequestChangeSlabShape, this, &AGeoWidget::onRequestChangeSlabShape);
        Del = del; // temporary!
        break;
    }
    }

    //Del = new AGeoSlabDelegate(tw->Sandwich->Materials, static_cast<int>(tw->Sandwich->SandwichState), this);


    return Del;
}

AGeoBaseDelegate * AGeoWidget::createAndAddGridElementDelegate()
{
    AGridElementDelegate * Del = new AGridElementDelegate(this);
    connect(Del, &AGridElementDelegate::RequestReshapeGrid, tw, &AGeoTreeWidget::onGridReshapeRequested);
    return Del;
}

AGeoBaseDelegate *AGeoWidget::createAndAddMonitorDelegate()
{
    QStringList particles;
    emit tw->RequestListOfParticles(particles);
    AMonitorDelegate* Del = new AMonitorDelegate(particles, this);
    connect(Del, &AMonitorDelegate::requestShowSensitiveFaces, this, &AGeoWidget::onMonitorRequestsShowSensitiveDirection);
    return Del;
}

void AGeoWidget::onObjectSelectionChanged(const QString SelectedObject)
{  
    CurrentObject = nullptr;
    //qDebug() << "Object selection changed! ->" << SelectedObject;
    ClearGui();

    AGeoObject* obj = World->findObjectByName(SelectedObject);
    if (!obj) return;

    //if (obj->ObjectType->isWorld()) return;

    CurrentObject = obj;
    //qDebug() << "New current object:"<<CurrentObject->Name;
    UpdateGui();
    //qDebug() << "OnObjectSelection procedure completed";
}

void AGeoWidget::onStartEditing()
{
  //qDebug() << "Start editing";
  if (fIgnoreSignals) return;
  if (!CurrentObject) return;

  if (!fEditingMode)
  {
      fEditingMode = true;
      tw->setEnabled(false);
      QFont f = pbConfirm->font();
      f.setBold(true);
      pbConfirm->setFont(f);
      pbConfirm->setStyleSheet("QPushButton {color: red;}");
  }
}

void AGeoWidget::onRequestChangeShape(AGeoShape * NewShape)
{
    if (!GeoDelegate) return;
    if (!CurrentObject) return;
    if (!NewShape) return;

    delete CurrentObject->Shape;
    CurrentObject->Shape = NewShape;
    if (!CurrentObject->ObjectType->isGrid()) CurrentObject->removeCompositeStructure();
    UpdateGui();
    onConfirmPressed();
}

void AGeoWidget::onRequestChangeSlabShape(int NewShape)
{
    if (!GeoDelegate) return;
    if (!CurrentObject) return;
    if (NewShape < 0 || NewShape > 2) return;
    if (!CurrentObject->ObjectType->isSlab()) return;

    ASlabModel * SlabModel = (static_cast<ATypeSlabObject*>(CurrentObject->ObjectType))->SlabModel;
    SlabModel->XYrecord.shape = NewShape;

    exitEditingMode();
    QString name = CurrentObject->Name;
    emit tw->RequestRebuildDetector();
    tw->UpdateGui(name);
}

void AGeoWidget::onRequestShowCurrentObject()
{
    if (!CurrentObject) return;

    QString name = CurrentObject->Name;
    emit tw->RequestHighlightObject(name);
    tw->UpdateGui(name);
}

void AGeoWidget::onRequestScriptLineToClipboard()
{
    if (!CurrentObject) return;

    QString script;
    bool bNotRecursive = (CurrentObject->ObjectType->isSlab() || CurrentObject->ObjectType->isSingle() || CurrentObject->ObjectType->isComposite());
    emit requestBuildScript(CurrentObject, script, 0, false, !bNotRecursive);

    qDebug() << script;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(script);
}

void AGeoWidget::onRequestScriptRecursiveToClipboard()
{
    if (!CurrentObject) return;

    QString script;
    emit requestBuildScript(CurrentObject, script, 0, false, true);

    qDebug() << script;

    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(script);
}

void AGeoWidget::onRequestSetVisAttributes()
{
    if (!CurrentObject) return;

    tw->SetLineAttributes(CurrentObject->Name);
}

void AGeoWidget::onMonitorRequestsShowSensitiveDirection()
{
    emit showMonitor(CurrentObject);
}

void AGeoWidget::exitEditingMode()
{
    fEditingMode = false;
    tw->setEnabled(true);
    QFont f = pbConfirm->font();
    f.setBold(false);
    pbConfirm->setFont(f);
    pbConfirm->setStyleSheet("QPushButton {color: black;}");
    pbConfirm->setEnabled(false);
    pbCancel->setEnabled(false);
}

void AGeoWidget::onConfirmPressed()
{
    if (!GeoDelegate)
    {
        qWarning() << "|||---Confirm triggered without active Delegate!";
        exitEditingMode();
        tw->UpdateGui();
        return;
    }

    //    qDebug() << "Validating update data for object" << CurrentObject->Name;
    bool ok = checkDelegateValidity();
    if (!ok) return;
    GeoDelegate->finalizeLocalParameters();
    ok = GeoDelegate->updateObject(CurrentObject);
    if (!ok) return;

    /*
    AWorldDelegate * del = dynamic_cast<AWorldDelegate*>(GeoDelegate);
    if (del)
    {
        AGeoBox * box = static_cast<AGeoBox*>(World->Shape);
        double WorldSizeXY = box->dx;
        double WorldSizeZ  = box->dz;
        ATypeWorldObject * typeWorld = static_cast<ATypeWorldObject *>(World->ObjectType);
        bool fWorldSizeFixed = typeWorld->bFixedSize;
        emit tw->RequestUpdateWorldSize(WorldSizeXY, WorldSizeZ, fWorldSizeFixed);
    }
    */

    exitEditingMode();
    QString name = CurrentObject->Name;
    //tw->UpdateGui(name);
    emit tw->RequestRebuildDetector();
    tw->UpdateGui(name);
}

bool AGeoWidget::checkDelegateValidity()
{
    const QString newName = GeoDelegate->getName();
    if (newName != CurrentObject->Name && World->isNameExists(newName))
    {
        QMessageBox::warning(this, "", QString("%1 name already exists").arg(newName));
        return false;
    }
    return true;
    //return GeoDelegate->isValid(CurrentObject);
}

void AGeoWidget::onCancelPressed()
{
  exitEditingMode();
  tw->UpdateGui( (CurrentObject) ? CurrentObject->Name : "" );
}

