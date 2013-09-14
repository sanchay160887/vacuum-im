#ifndef VCARDPLUGIN_H
#define VCARDPLUGIN_H

#include <QTimer>
#include <QObjectCleanupHandler>
#include <definitions/namespaces.h>
#include <definitions/actiongroups.h>
#include <definitions/rosterindexkinds.h>
#include <definitions/rosterindexroles.h>
#include <definitions/multiuserdataroles.h>
#include <definitions/resources.h>
#include <definitions/menuicons.h>
#include <definitions/shortcuts.h>
#include <definitions/vcardvaluenames.h>
#include <definitions/xmppurihandlerorders.h>
#include <definitions/toolbargroups.h>
#include <definitions/rosterdataholderorders.h>
#include <interfaces/ipluginmanager.h>
#include <interfaces/ivcard.h>
#include <interfaces/iroster.h>
#include <interfaces/ixmppstreams.h>
#include <interfaces/irostersview.h>
#include <interfaces/irostersmodel.h>
#include <interfaces/imultiuserchat.h>
#include <interfaces/istanzaprocessor.h>
#include <interfaces/iservicediscovery.h>
#include <interfaces/ixmppuriqueries.h>
#include <interfaces/imessagewidgets.h>
#include <interfaces/irostersearch.h>
#include <utils/stanza.h>
#include <utils/action.h>
#include <utils/shortcuts.h>
#include <utils/xmpperror.h>
#include <utils/textmanager.h>
#include <utils/widgetmanager.h>
#include "vcard.h"
#include "vcarddialog.h"

struct VCardItem {
	VCardItem() {
		vcard = NULL;
		locks = 0;
	}
	VCard *vcard;
	int locks;
};

class VCardPlugin :
	public QObject,
	public IPlugin,
	public IVCardPlugin,
	public IStanzaRequestOwner,
	public IXmppUriHandler,
	public IRosterDataHolder
{
	Q_OBJECT;
	Q_INTERFACES(IPlugin IVCardPlugin IRosterDataHolder IStanzaRequestOwner IXmppUriHandler);
	friend class VCard;
public:
	VCardPlugin();
	~VCardPlugin();
	//IPlugin
	virtual QObject *instance() { return this; }
	virtual QUuid pluginUuid() const { return VCARD_UUID; }
	virtual void pluginInfo(IPluginInfo *APluginInfo);
	virtual bool initConnections(IPluginManager *APluginManager, int &AInitOrder);
	virtual bool initObjects();
	virtual bool initSettings() { return true; }
	virtual bool startPlugin()  { return true; }
	//IRosterDataHolder
	virtual QList<int> rosterDataRoles(int AOrder) const;
	virtual QVariant rosterData(int AOrder, const IRosterIndex *AIndex, int ARole) const;
	virtual bool setRosterData(int AOrder, const QVariant &AValue, IRosterIndex *AIndex, int ARole);
	//IStanzaRequestOwner
	virtual void stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza);
	//IXmppUriHandler
	virtual bool xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams);
	//IVCardPlugin
	virtual QString vcardFileName(const Jid &AContactJid) const;
	virtual bool hasVCard(const Jid &AContactJid) const;
	virtual IVCard *getVCard(const Jid &AContactJid);
	virtual bool requestVCard(const Jid &AStreamJid, const Jid &AContactJid);
	virtual bool publishVCard(IVCard *AVCard, const Jid &AStreamJid);
	virtual void showVCardDialog(const Jid &AStreamJid, const Jid &AContactJid);
signals:
	void vcardReceived(const Jid &AContactJid);
	void vcardPublished(const Jid &AContactJid);
	void vcardError(const Jid &AContactJid, const XmppError &AError);
	// IRosterDataHolder
	void rosterDataChanged(IRosterIndex *AIndex, int ARole);
protected:
	void registerDiscoFeatures();
	void unlockVCard(const Jid &AContactJid);
	void saveVCardFile(const Jid &AContactJid, const QDomElement &AElem) const;
	void removeEmptyChildElements(QDomElement &AElem) const;
	void insertMessageToolBarAction(IMessageToolBarWidget *AWidget);
	QList<Action *> createClipboardActions(const QSet<QString> &AStrings, QObject *AParent) const;
protected slots:
	void onCopyToClipboardActionTriggered(bool);
	void onShortcutActivated(const QString &AId, QWidget *AWidget);
	void onRostersViewIndexContextMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onRostersViewIndexClipboardMenu(const QList<IRosterIndex *> &AIndexes, quint32 ALabelId, Menu *AMenu);
	void onMultiUserContextMenu(IMultiUserChatWindow *AWindow, IMultiUser *AUser, Menu *AMenu);
	void onShowVCardDialogByAction(bool);
	void onShowVCardDialogByMessageWindowAction(bool);
	void onVCardDialogDestroyed(QObject *ADialog);
	void onXmppStreamRemoved(IXmppStream *AXmppStream);
	void onMessageNormalWindowCreated(IMessageNormalWindow *AWindow);
	void onMessageChatWindowCreated(IMessageChatWindow *AWindow);
protected slots:
	void onUpdateTimerTimeout();
	void onRosterOpened(IRoster *ARoster);
	void onRosterClosed(IRoster *ARoster);
	void onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem, const IRosterItem &ABefore);
private:
	IPluginManager *FPluginManager;
	IXmppStreams *FXmppStreams;
	IRosterPlugin *FRosterPlugin;
	IRostersModel *FRostersModel;
	IRostersView *FRostersView;
	IRostersViewPlugin *FRostersViewPlugin;
	IStanzaProcessor *FStanzaProcessor;
	IMultiUserChatPlugin *FMultiUserChatPlugin;
	IServiceDiscovery *FDiscovery;
	IXmppUriQueries *FXmppUriQueries;
	IMessageWidgets *FMessageWidgets;
	IRosterSearch *FRosterSearch;
private:
	QTimer FUpdateTimer;
	QMap<Jid,VCardItem> FVCards;
	QMultiMap<Jid,Jid> FUpdateQueue;
	QMap<QString,Jid> FVCardRequestId;
	QMap<QString,Jid> FVCardPublishId;
	QMap<QString,Stanza> FVCardPublishStanza;
	QMap<Jid,VCardDialog *> FVCardDialogs;
	mutable QHash<Jid,QStringList> FSearchStrings;
};

#endif // VCARDPLUGIN_H
