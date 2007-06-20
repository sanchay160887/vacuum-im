#ifndef ROSTERSVIEW_H
#define ROSTERSVIEW_H

#include <QContextMenuEvent>
#include <QPainter>
#include <QTimer>
#include "../../definations/tooltiporders.h"
#include "../../definations/rosterlabelorders.h"
#include "../../interfaces/irostersview.h"
#include "../../interfaces/irostersmodel.h"
#include "../../interfaces/isettings.h"
#include "indexdataholder.h"
#include "rosterindexdelegate.h" 


class RostersView : 
  virtual public QTreeView,
  public IRostersView
{
  Q_OBJECT;
  Q_INTERFACES(IRostersView);

public:
  RostersView(QWidget *AParent = NULL);
  ~RostersView();

  virtual QObject *instance() { return this; }

  //IRostersView
  virtual void setModel(IRostersModel *AModel); 
  virtual IRostersModel *rostersModel() const { return FRostersModel; }
  virtual IRosterIndexDataHolder *defaultDataHolder() const { return FIndexDataHolder; }
  //--ProxyModels
  virtual void addProxyModel(QAbstractProxyModel *AProxyModel);
  virtual QList<QAbstractProxyModel *> proxyModels() const { return FProxyModels; }
  virtual QAbstractProxyModel *lastProxyModel() const { return FProxyModels.value(FProxyModels.count()-1,NULL); }
  virtual void removeProxyModel(QAbstractProxyModel *AProxyModel);
  virtual QModelIndex mapToModel(const QModelIndex &AProxyIndex);
  virtual QModelIndex mapFromModel(const QModelIndex &AModelIndex);
  virtual QModelIndex mapToProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AModelIndex);
  virtual QModelIndex mapFromProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AProxyIndex);
  //--IndexLabel
  virtual int createIndexLabel(int AOrder, const QVariant &ALabel, int AFlags = 0);
  virtual void updateIndexLabel(int ALabelId, const QVariant &ALabel, int AFlags = 0);
  virtual void insertIndexLabel(int ALabelId, IRosterIndex *AIndex);
  virtual void removeIndexLabel(int ALabelId, IRosterIndex *AIndex);
  virtual void destroyIndexLabel(int ALabelId);
  virtual int labelAt(const QPoint &APoint, const QModelIndex &AIndex) const;
  virtual QRect labelRect(int ALabeld, const QModelIndex &AIndex) const;
  //--ClickHookers
  virtual int createClickHooker(IRostersClickHooker *AHooker, int APriority, bool AAutoRemove = false);
  virtual void insertClickHooker(int AHookerId, IRosterIndex *AIndex);
  virtual void removeClickHooker(int AHookerId, IRosterIndex *AIndex);
  virtual void destroyClickHooker(int AHookerId);
signals:
  virtual void modelAboutToBeSeted(IRostersModel *);
  virtual void modelSeted(IRostersModel *);
  virtual void proxyModelAboutToBeAdded(QAbstractProxyModel *);
  virtual void proxyModelAdded(QAbstractProxyModel *);
  virtual void proxyModelAboutToBeRemoved(QAbstractProxyModel *);
  virtual void proxyModelRemoved(QAbstractProxyModel *);
  virtual void contextMenu(IRosterIndex *AIndex, Menu *AMenu);
  virtual void toolTips(IRosterIndex *AIndex, QMultiMap<int,QString> &AToolTips);
  virtual void labelContextMenu(IRosterIndex *AIndex, int ALabelId, Menu *AMenu);
  virtual void labelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips);
  virtual void labelClicked(IRosterIndex *AIndex, int ALabelId);
  virtual void labelDoubleClicked(IRosterIndex *AIndex, int ALabelId, bool &AAccepted);
protected:
  void drawBranches(QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const;
  void contextMenuEvent(QContextMenuEvent *AEvent);
  QStyleOptionViewItemV2 indexOption(const QModelIndex &AIndex) const;
  void appendBlinkLabel(int ALabelId);
  void removeBlinkLabel(int ALabelId);
  //QAbstractItemView
  virtual bool viewportEvent(QEvent *AEvent);
  virtual void mouseDoubleClickEvent(QMouseEvent *AEvent);
  virtual void mousePressEvent(QMouseEvent *AEvent);
  virtual void mouseReleaseEvent(QMouseEvent *AEvent);
protected slots:
  void onIndexCreated(IRosterIndex *AIndex, IRosterIndex *AParent);
  void onIndexRemoved(IRosterIndex *AIndex);
  void onBlinkTimer();
private:
  IRostersModel *FRostersModel;
private:
  Menu *FContextMenu;
private:
  int FLabelId;
  QTimer FBlinkTimer;
  QSet<int> FBlinkLabels;
  bool FBlinkShow;
  int FPressedLabel;
  IRosterIndex *FPressedIndex;
  QHash<int, QVariant> FIndexLabels;
  QHash<int, int /*Order*/> FIndexLabelOrders;
  QHash<int, int /*Flags*/> FIndexLabelFlags;
  QHash<int, QSet<IRosterIndex *> > FIndexLabelIndexes;
private:
  int FHookerId;
  struct ClickHookerItem
  {
    int hookerId;
    IRostersClickHooker *hooker;
    QSet<IRosterIndex *> indexes;
    int priority;
    bool autoRemove;
  };
  QList<ClickHookerItem *> FClickHookerItems;
private:
  IndexDataHolder *FIndexDataHolder;
  RosterIndexDelegate *FRosterIndexDelegate;
  QList<QAbstractProxyModel *> FProxyModels;
};

#endif // ROSTERSVIEW_H
