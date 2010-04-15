#ifndef FILETRANSFER_H
#define FILETRANSFER_H

#include <QDropEvent>
#include <QDragMoveEvent>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <definations/namespaces.h>
#include <definations/fshandlerorders.h>
#include <definations/rosterindextyperole.h>
#include <definations/discofeaturehandlerorders.h>
#include <definations/notificationdataroles.h>
#include <definations/menuicons.h>
#include <definations/soundfiles.h>
#include <definations/resources.h>
#include <definations/toolbargroups.h>
#include <definations/optionvalues.h>
#include <definations/optionnodes.h>
#include <definations/optionnodeorders.h>
#include <definations/optionwidgetorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ifiletransfer.h>
#include <interfaces/ifilestreamsmanager.h>
#include <interfaces/idatastreamsmanager.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/iroster.h>
#include <interfaces/inotifications.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/irostersview.h>
#include <interfaces/ioptionsmanager.h>
#include <utils/jid.h>
#include <utils/action.h>
#include <utils/stanza.h>
#include <utils/options.h>
#include <utils/datetime.h>
#include <utils/iconstorage.h>
#include <utils/widgetmanager.h>
#include "streamdialog.h"

class FileTransfer : 
  public QObject,
  public IPlugin,
  public IFileTransfer,
  public IOptionsHolder,
  public IDiscoFeatureHandler,
  public IRostersDragDropHandler,
  public IViewDropHandler,
  public IFileStreamsHandler
{
  Q_OBJECT;
  Q_INTERFACES(IPlugin IFileTransfer IOptionsHolder IDiscoFeatureHandler  IRostersDragDropHandler IViewDropHandler IFileStreamsHandler);
public:
  FileTransfer();
  ~FileTransfer();
  //IPlugin
  virtual QObject *instance() { return this; }
  virtual QUuid pluginUuid() const { return FILETRANSFER_UUID; }
  virtual void pluginInfo(IPluginInfo *APluginInfo);
  virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
  virtual bool initObjects();
  virtual bool initSettings();
  virtual bool startPlugin() { return true; }
  //IOptionsHolder
  virtual IOptionsWidget *optionsWidget(const QString &ANodeId, int &AOrder, QWidget *AParent);
  //IDiscoFeatureHandler
  virtual bool execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo);
  virtual Action *createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent);
  //IRostersDragDropHandler
  virtual Qt::DropActions rosterDragStart(const QMouseEvent *AEvent, const QModelIndex &AIndex, QDrag *ADrag);
  virtual bool rosterDragEnter(const QDragEnterEvent *AEvent);
  virtual bool rosterDragMove(const QDragMoveEvent *AEvent, const QModelIndex &AHover);
  virtual void rosterDragLeave(const QDragLeaveEvent *AEvent);
  virtual bool rosterDropAction(const QDropEvent *AEvent, const QModelIndex &AIndex, Menu *AMenu);
  //IViewDropHandler
  virtual bool viewDragEnter(IViewWidget *AWidget, const QDragEnterEvent *AEvent);
  virtual bool viewDragMove(IViewWidget *AWidget, const QDragMoveEvent *AEvent);
  virtual void viewDragLeave(IViewWidget *AWidget, const QDragLeaveEvent *AEvent);
  virtual bool viewDropAction(IViewWidget *AWidget, const QDropEvent *AEvent, Menu *AMenu);
  //IFileTransferHandler
  virtual bool fileStreamRequest(int AOrder, const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods);
  virtual bool fileStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethodNS);
  virtual bool fileStreamShowDialog(const QString &AStreamId);
  //IFileTransfer
  virtual bool isSupported(const Jid &AStreamJid, const Jid &AContactJid) const;
  virtual IFileStream *sendFile(const Jid &AStreamJid, const Jid &AContactJid, const QString &AFileName = QString::null, const QString &AFileDesc = QString::null);
protected:
  void registerDiscoFeatures();
  void notifyStream(IFileStream *AStream, bool ANewStream = false);
  void autoStartStream(IFileStream *AStream);
  void insertToolBarAction(IToolBarWidget *AWidget);
  void removeToolBarAction(IToolBarWidget *AWidget);
  QList<IToolBarWidget *> findToolBarWidgets(const Jid &AContactJid) const;
  StreamDialog *createStreamDialog(IFileStream *ASession);
  IFileStream *createStream(const QString &AStreamId, const Jid &AStreamJid, const Jid &AContactJid, IFileStream::StreamKind AStreamKind);
protected slots:
  void onStreamStateChanged();
  void onStreamDestroyed();
  void onStreamDialogDestroyed();
  void onShowSendFileDialogByAction(bool);
  void onNotificationActivated(int ANotifyId);
  void onNotificationRemoved(int ANotifyId);
  void onDiscoInfoReceived(const IDiscoInfo &AInfo);
  void onDiscoInfoRemoved(const IDiscoInfo &AInfo);
  void onToolBarWidgetCreated(IToolBarWidget *AWidget);
  void onEditWidgetContactJidChanged(const Jid &ABefour);
  void onToolBarWidgetDestroyed(QObject *AObject);
private:
  IRosterPlugin *FRosterPlugin;
  IServiceDiscovery *FDiscovery;
  INotifications *FNotifications;
  IDataStreamsManager *FDataManager;
  IFileStreamsManager *FFileManager;
  IMessageWidgets *FMessageWidgets;
  IOptionsManager *FOptionsManager;
  IRostersViewPlugin *FRostersViewPlugin;
private:
  QMap<QString, int> FStreamNotify;
  QMap<QString, StreamDialog *> FStreamDialog;
  QMap<IToolBarWidget *, Action *> FToolBarActions;
};

#endif // FILETRANSFER_H
