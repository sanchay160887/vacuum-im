#ifndef ROSTERSVIEW_H
#define ROSTERSVIEW_H

#include <QTimer>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/actiongroups.h>
#include <definitions/optionvalues.h>
#include <definitions/shortcuts.h>
#include <definitions/rostertooltiporders.h>
#include <definitions/rosterindextyperole.h>
#include <definitions/rosterdragdropmimetypes.h>
#include <definitions/rosterdataholderorders.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imainwindow.h>
#include <utils/options.h>
#include <utils/shortcuts.h>
#include <utils/iconstorage.h>

class RostersView :
	public QTreeView,
	public IMainTabPage,
	public IRostersView,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IMainTabPage IRostersView IRosterDataHolder);
public:
	RostersView(QWidget *AParent = NULL);
	~RostersView();
	virtual QTreeView *instance() { return this; }
	//IMainTabPage
	virtual QIcon tabPageIcon() const;
	virtual QString tabPageCaption() const;
	virtual QString tabPageToolTip() const;
	//IRosterDataHolder
	virtual int rosterDataOrder() const;
	virtual QList<int> rosterDataRoles() const;
	virtual QList<int> rosterDataTypes() const;
	virtual QVariant rosterData(const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(IRosterIndex *AIndex, int ARole, const QVariant &AValue);
	//IRostersView
	virtual IRostersModel *rostersModel() const;
	virtual void setRostersModel(IRostersModel *AModel);
	virtual bool repaintRosterIndex(IRosterIndex *AIndex);
	virtual void expandIndexParents(IRosterIndex *AIndex);
	virtual void expandIndexParents(const QModelIndex &AIndex);
	virtual bool editRosterIndex(int ADataRole, IRosterIndex *AIndex);
	virtual bool hasMultiSelection() const;
	virtual QList<IRosterIndex *> selectedRosterIndexes() const;
	virtual void selectRosterIndex(IRosterIndex *AIndex);
	virtual QMap<int, QStringList> indexesRolesMap(const QList<IRosterIndex *> &AIndexes, const QList<int> &ARoles, int AUniqueRole=-1) const;
	//--ProxyModels
	virtual void insertProxyModel(QAbstractProxyModel *AProxyModel, int AOrder);
	virtual QList<QAbstractProxyModel *> proxyModels() const;
	virtual void removeProxyModel(QAbstractProxyModel *AProxyModel);
	virtual QModelIndex mapToModel(const QModelIndex &AProxyIndex) const;
	virtual QModelIndex mapFromModel(const QModelIndex &AModelIndex) const;
	virtual QModelIndex mapToProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AModelIndex) const;
	virtual QModelIndex mapFromProxy(QAbstractProxyModel *AProxyModel, const QModelIndex &AProxyIndex) const;
	//--IndexLabel
	virtual quint32 registerLabel(const AdvancedDelegateItem &ALabel);
	virtual void updateLabel(quint32 ALabelId, const AdvancedDelegateItem &ALabel);
	virtual void insertLabel(quint32 ALabelId, IRosterIndex *AIndex);
	virtual void removeLabel(quint32 ALabelId, IRosterIndex *AIndex);
	virtual void destroyLabel(quint32 ALabelId);
	virtual quint32 labelAt(const QPoint &APoint, const QModelIndex &AIndex) const;
	virtual QRect labelRect(quint32 ALabeld, const QModelIndex &AIndex) const;
	//--IndexNotify
	virtual int activeNotify(IRosterIndex *AIndex) const;
	virtual QList<int> notifyQueue(IRosterIndex *Index) const;
	virtual IRostersNotify notifyById(int ANotifyId) const;
	virtual QList<IRosterIndex *> notifyIndexes(int ANotifyId) const;
	virtual int insertNotify(const IRostersNotify &ANotify, const QList<IRosterIndex *> &AIndexes);
	virtual void activateNotify(int ANotifyId);
	virtual void removeNotify(int ANotifyId);
	//--ClickHookers
	virtual void insertClickHooker(int AOrder, IRostersClickHooker *AHooker);
	virtual void removeClickHooker(int AOrder, IRostersClickHooker *AHooker);
	//--KeyHookers
	virtual void insertKeyHooker(int AOrder, IRostersKeyHooker *AHooker);
	virtual void removeKeyHooker(int AOrder, IRostersKeyHooker *AHooker);
	//--DragDrop
	virtual void insertDragDropHandler(IRostersDragDropHandler *AHandler);
	virtual void removeDragDropHandler(IRostersDragDropHandler *AHandler);
	//--EditHandlers
	virtual void insertEditHandler(int AOrder, IRostersEditHandler *AHandler);
	virtual void removeEditHandler(int AOrder, IRostersEditHandler *AHandler);
	//--ContextMenu
	virtual void contextMenuForIndex(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	//--ClipboardMenu
	virtual void clipboardMenuForIndex(const QList<IRosterIndex *> &AIndexes, Menu *AMenu);
signals:
	//IMainTabPage
	void tabPageChanged();
	void tabPageDestroyed();
	//IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex = NULL, int ARole = 0);
	//IRostersView
	void modelAboutToBeSeted(IRostersModel *AModel);
	void modelSeted(IRostersModel *AModel);
	void proxyModelAboutToBeInserted(QAbstractProxyModel *AProxyModel, int AOrder);
	void proxyModelInserted(QAbstractProxyModel *AProxyModel);
	void proxyModelAboutToBeRemoved(QAbstractProxyModel *AProxyModel);
	void proxyModelRemoved(QAbstractProxyModel *AProxyModel);
	void viewModelAboutToBeChanged(QAbstractItemModel *AModel);
	void viewModelChanged(QAbstractItemModel *AModel);
	void indexMultiSelection(const QList<IRosterIndex *> &ASelected, bool &AAccepted);
	void indexClipboardMenu(const QList<IRosterIndex *> &AIndexes, Menu *AMenu);
	void indexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void indexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMultiMap<int,QString> &AToolTips);
	void indexClicked(IRosterIndex *AIndex, quint32 ALabelId);
	void indexDoubleClicked(IRosterIndex *AIndex, quint32 ALabelId);
	void notifyInserted(int ANotifyId);
	void notifyActivated(int ANotifyId);
	void notifyRemoved(int ANotifyId);
protected:
	QStyleOptionViewItemV4 indexOption(const QModelIndex &AIndex) const;
	void appendBlinkItem(int ALabelId, int ANotifyId);
	void removeBlinkItem(int ALabelId, int ANotifyId);
	QString intId2StringId(int AIntId) const;
	void removeLabels();
	void setDropIndicatorRect(const QRect &ARect);
	IRostersEditHandler *findEditHandler(int ADataRole, const QModelIndex &AIndex) const;
protected:
	//QTreeView
	void drawBranches(QPainter *APainter, const QRect &ARect, const QModelIndex &AIndex) const;
	//QAbstractItemView
	bool viewportEvent(QEvent *AEvent);
	void resizeEvent(QResizeEvent *AEvent);
	bool edit(const QModelIndex &AIndex, EditTrigger ATrigger, QEvent *AEvent);
	//QWidget
	void paintEvent(QPaintEvent *AEvent);
	void contextMenuEvent(QContextMenuEvent *AEvent);
	void mouseDoubleClickEvent(QMouseEvent *AEvent);
	void mousePressEvent(QMouseEvent *AEvent);
	void mouseMoveEvent(QMouseEvent *AEvent);
	void mouseReleaseEvent(QMouseEvent *AEvent);
	void keyPressEvent(QKeyEvent *AEvent);
	void keyReleaseEvent(QKeyEvent *AEvent);
	void dropEvent(QDropEvent *AEvent);
	void dragEnterEvent(QDragEnterEvent *AEvent);
	void dragMoveEvent(QDragMoveEvent *AEvent);
	void dragLeaveEvent(QDragLeaveEvent *AEvent);
protected slots:
	//QAbstractItemView
	void closeEditor(QWidget *AEditor, QAbstractItemDelegate::EndEditHint AHint);
protected slots:
	void onRosterIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRosterIndexToolTips(IRosterIndex *AIndex, quint32 ALabelId, QMultiMap<int,QString> &AToolTips);
	void onSelectionChanged(const QItemSelection &ASelected, const QItemSelection &ADeselected);
	void onCopyToClipboardActionTriggered(bool);
	void onIndexDestroyed(IRosterIndex *AIndex);
	void onRemoveIndexNotifyTimeout();
	void onUpdateIndexNotifyTimeout();
	void onUpdateBlinkLabels();
	void onDragExpandTimer();
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
private:
	IRostersModel *FRostersModel;
private:
	quint32 FPressedLabel;
	QPoint FPressedPos;
	QModelIndex FPressedIndex;
private:
	QSet<int> FBlinkNotifies;
	QSet<quint32> FBlinkLabels;
private:
	QMap<quint32, AdvancedDelegateItem> FLabelItems;
	QMultiMap<IRosterIndex *, quint32> FIndexLabels;
private:
	QMap<QTimer *, int> FNotifyTimer;
	QSet<IRosterIndex *> FNotifyUpdates;
	QMap<int, IRostersNotify> FNotifyItems;
	QMap<IRosterIndex *, int> FActiveNotifies;
	QMultiMap<IRosterIndex *, int> FIndexNotifies;
private:
	QMultiMap<int, IRostersKeyHooker *> FKeyHookers;
	QMultiMap<int, IRostersClickHooker *> FClickHookers;
	QMultiMap<int, IRostersEditHandler *> FEditHandlers;
private:
	AdvancedItemDelegate *FAdvancedItemDelegate;
	QMultiMap<int, QAbstractProxyModel *> FProxyModels;
private:
	bool FStartDragFailed;
	QTimer FDragExpandTimer;
	QRect FDropIndicatorRect;
	QList<IRostersDragDropHandler *> FDragDropHandlers;
	QList<IRostersDragDropHandler *> FActiveDragHandlers;
};

#endif // ROSTERSVIEW_H
