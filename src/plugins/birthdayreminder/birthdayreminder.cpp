#include "birthdayreminder.h"

#define NOTIFY_WITHIN_DAYS 7
#define NOTIFY_TIMEOUT     90000

BirthdayReminder::BirthdayReminder()
{
	FAvatars = NULL;
	FVCardPlugin = NULL;
	FRosterPlugin = NULL;
	FPresencePlugin = NULL;
	FRostersModel = NULL;
	FNotifications = NULL;
	FRostersViewPlugin = NULL;
	FMessageProcessor = NULL;

	FNotifyTimer.setSingleShot(false);
	FNotifyTimer.setInterval(NOTIFY_TIMEOUT);
	connect(&FNotifyTimer,SIGNAL(timeout()),SLOT(onShowNotificationTimer()));
}

BirthdayReminder::~BirthdayReminder()
{

}

void BirthdayReminder::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Birthday Reminder");
	APluginInfo->description = tr("Reminds about birthdays of your friends");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(VCARD_UUID);
}

bool BirthdayReminder::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);

	IPlugin *plugin = APluginManager->pluginInterface("IVCardPlugin").value(0,NULL);
	if (plugin)
	{
		FVCardPlugin = qobject_cast<IVCardPlugin *>(plugin->instance());
		if (FVCardPlugin)
		{
			connect(FVCardPlugin->instance(),SIGNAL(vcardReceived(const Jid &)),SLOT(onVCardReceived(const Jid &)));
		}
	}

	plugin = APluginManager->pluginInterface("IAvatars").value(0,NULL);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersViewPlugin").value(0,NULL);
	if (plugin)
	{
		FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());
		if (FRostersViewPlugin)
		{
			connect(FRostersViewPlugin->rostersView()->instance(),SIGNAL(labelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)),
				SLOT(onRosterLabelToolTips(IRosterIndex *, int, QMultiMap<int,QString> &)));
		}
	}

	plugin = APluginManager->pluginInterface("IRosterPlugin").value(0,NULL);
	if (plugin)
	{
		FRosterPlugin = qobject_cast<IRosterPlugin *>(plugin->instance());
		if (FRosterPlugin)
		{
			connect(FRosterPlugin->instance(),SIGNAL(rosterItemReceived(IRoster *, const IRosterItem &)),
				SLOT(onRosterItemReceived(IRoster *, const IRosterItem &)));
		}
	}

	plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
	if (plugin)
	{
		FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IRostersModel").value(0,NULL);
	if (plugin)
	{
		FRostersModel = qobject_cast<IRostersModel *>(plugin->instance());
		if (FRostersModel)
		{
			connect(FRostersModel->instance(),SIGNAL(indexInserted(IRosterIndex *)),SLOT(onRosterIndexInserted(IRosterIndex *)));
		}
	}

	plugin = APluginManager->pluginInterface("IMessageProcessor").value(0,NULL);
	if (plugin)
	{
		FMessageProcessor = qobject_cast<IMessageProcessor *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("INotifications").value(0,NULL);
	if (plugin)
	{
		FNotifications = qobject_cast<INotifications *>(plugin->instance());
		if (FNotifications)
		{
			connect(FNotifications->instance(),SIGNAL(notificationActivated(int)), SLOT(onNotificationActivated(int)));
			connect(FNotifications->instance(),SIGNAL(notificationRemoved(int)), SLOT(onNotificationRemoved(int)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return FVCardPlugin!=NULL;
}

bool BirthdayReminder::initObjects()
{
	if (FNotifications)
	{
		uchar kindMask = INotification::PopupWindow|INotification::PlaySound;
		FNotifications->registerNotificationType(NNT_BIRTHDAY,OWO_NOTIFICATIONS_BIRTHDAY,tr("Birthdays"),kindMask,kindMask);
	}
	if (FRostersViewPlugin)
	{
		FBirthdayLabelId = FRostersViewPlugin->rostersView()->createIndexLabel(RLO_BIRTHDAY_NOTIFY,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_BIRTHDAY_NOTIFY));
	}
	return true;
}

bool BirthdayReminder::startPlugin()
{
	FNotifyTimer.start();
	return true;
}

QDate BirthdayReminder::contactBithday(const Jid &AContactJid) const
{
	return FBirthdays.value(AContactJid.bare());
}

int BirthdayReminder::contactBithdayDaysLeft(const Jid &AContactJid) const
{
	QDate birthday = contactBithday(AContactJid);
	if (birthday.isValid())
	{
		QDate curDate = QDate::currentDate();
		if (curDate.month()<birthday.month() || (curDate.month()==birthday.month() && curDate.day()<=birthday.day()))
			birthday.setDate(curDate.year(),birthday.month(),birthday.day());
		else
			birthday.setDate(curDate.year()+1,birthday.month(),birthday.day());
		return curDate.daysTo(birthday);
	}
	return -1;
}

Jid BirthdayReminder::findContactStream(const Jid &AContactJid) const
{
	if (FRostersModel && FRosterPlugin)
	{
		foreach(Jid streamJid, FRostersModel->streams())
		{
			IRoster *roster = FRosterPlugin->getRoster(streamJid);
			if (roster && roster->rosterItem(AContactJid).isValid)
				return streamJid;
		}
	}
	return Jid::null;
}

void BirthdayReminder::updateBirthdaysStates()
{
	if (FNotifyDate != QDate::currentDate())
	{
		FNotifiedContacts.clear();
		FNotifyDate = QDate::currentDate();

		foreach(Jid contactJid, FBirthdays.keys()) {
			updateBirthdayState(contactJid); }
	}
}

bool BirthdayReminder::updateBirthdayState(const Jid &AContactJid)
{
	bool notify = false;
	int daysLeft = contactBithdayDaysLeft(AContactJid);

	bool isStateChanged = false;
	if (daysLeft>=0 && daysLeft<=NOTIFY_WITHIN_DAYS)
	{
		notify = true;
		isStateChanged = !FUpcomingBirthdays.contains(AContactJid);
		FUpcomingBirthdays.insert(AContactJid,daysLeft);
	}
	else
	{
		isStateChanged = FUpcomingBirthdays.contains(AContactJid);
		FUpcomingBirthdays.remove(AContactJid);
	}

	if (isStateChanged && FRostersViewPlugin && FRostersModel)
	{
		QMultiMap<int, QVariant> findData;
		findData.insert(RDR_TYPE,RIT_CONTACT);
		findData.insert(RDR_PREP_BARE_JID,AContactJid.pBare());
		foreach (IRosterIndex *index, FRostersModel->rootIndex()->findChilds(findData,true))
			FRostersViewPlugin->rostersView()->insertIndexLabel(FBirthdayLabelId,index);
	}

	return notify;
}

void BirthdayReminder::setContactBithday(const Jid &AContactJid, const QDate &ABirthday)
{
	Jid contactJid = AContactJid.bare();
	if (FBirthdays.value(contactJid) != ABirthday)
	{
		if (ABirthday.isValid())
			FBirthdays.insert(contactJid,ABirthday);
		else
			FBirthdays.remove(contactJid);
		updateBirthdayState(contactJid);
	}
}

void BirthdayReminder::onShowNotificationTimer()
{
	if (FNotifications && FNotifications->notifications().isEmpty())
	{
		INotification notify;
		notify.kinds = FNotifications->notificationKinds(NNT_BIRTHDAY);
		if ((notify.kinds & (INotification::PopupWindow|INotification::PlaySound))>0)
		{
			updateBirthdaysStates();
			notify.type = NNT_BIRTHDAY;
			QSet<Jid> notifyList = FUpcomingBirthdays.keys().toSet() - FNotifiedContacts.toSet();
			foreach(Jid contactJid, notifyList)
			{
				Jid streamJid = findContactStream(contactJid);

				notify.data.insert(NDR_ICON,IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_BIRTHDAY_NOTIFY));
				notify.data.insert(NDR_POPUP_CAPTION,tr("Birthday remind"));
				notify.data.insert(NDR_POPUP_TITLE,FNotifications->contactName(streamJid,contactJid));
				notify.data.insert(NDR_POPUP_IMAGE,FNotifications->contactAvatar(contactJid));

				QDate	birthday = contactBithday(contactJid);
				int daysLeft = FUpcomingBirthdays.value(contactJid);
				QString text = daysLeft>0 ? tr("Birthday in %n day(s),<br> %1","",daysLeft).arg(birthday.toString(Qt::SystemLocaleLongDate)) : tr("Birthday today!");
				notify.data.insert(NDR_POPUP_HTML,text);

				if (daysLeft==NOTIFY_WITHIN_DAYS || daysLeft==NOTIFY_WITHIN_DAYS/2 || daysLeft==0)
					notify.data.insert(NDR_POPUP_TIMEOUT,0);

				FNotifiedContacts.append(contactJid);
				FNotifies.insert(FNotifications->appendNotification(notify),contactJid);
			}
		}
	}
}

void BirthdayReminder::onNotificationActivated(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		if (FMessageProcessor)
		{
			Jid contactJid = FNotifies.value(ANotifyId);
			Jid streamJid = findContactStream(contactJid);
			IPresence *presence = FPresencePlugin!=NULL ? FPresencePlugin->getPresence(streamJid) : NULL;
			QList<IPresenceItem> presences = presence!=NULL ? presence->presenceItems(contactJid) : QList<IPresenceItem>();
			FMessageProcessor->createMessageWindow(streamJid, !presences.isEmpty() ? presences.first().itemJid : contactJid, Message::Chat, IMessageHandler::SM_SHOW);
		}
		FNotifications->removeNotification(ANotifyId);
	}
}

void BirthdayReminder::onNotificationRemoved(int ANotifyId)
{
	if (FNotifies.contains(ANotifyId))
	{
		FNotifies.remove(ANotifyId);
	}
}

void BirthdayReminder::onRosterIndexInserted(IRosterIndex *AIndex)
{
	if (FRostersViewPlugin && AIndex->type() == RIT_CONTACT)
	{
		if (FUpcomingBirthdays.contains(AIndex->data(RDR_PREP_BARE_JID).toString()))
			FRostersViewPlugin->rostersView()->insertIndexLabel(FBirthdayLabelId,AIndex);
	}
}

void BirthdayReminder::onRosterLabelToolTips(IRosterIndex *AIndex, int ALabelId, QMultiMap<int,QString> &AToolTips)
{
	if (ALabelId==RLID_DISPLAY || ALabelId==FBirthdayLabelId)
	{
		Jid contactJid = AIndex->data(RDR_PREP_BARE_JID).toString();
		int daysLeft = contactBithdayDaysLeft(contactJid);
		if (daysLeft>=0 && daysLeft<=NOTIFY_WITHIN_DAYS)
		{
			if (ALabelId == FBirthdayLabelId)
			{
				QDate birthday = contactBithday(contactJid);
				QString tip = tr("%1 marks %n years","",QDate::currentDate().year() - birthday.year()).arg(QDate::currentDate().addDays(daysLeft).toString(Qt::DefaultLocaleLongDate));
				AToolTips.insert(RTTO_BIRTHDAY_NOTIFY,tip);
			}
			QString tip = daysLeft>0 ? tr("Birthday in %n day(s)!","",daysLeft) : tr("Birthday today!");
			AToolTips.insert(RTTO_BIRTHDAY_NOTIFY,tip);
		}
	}
}

void BirthdayReminder::onVCardReceived(const Jid &AContactJid)
{
	if (findContactStream(AContactJid).isValid())
	{
		IVCard *vcard = FVCardPlugin->vcard(AContactJid);
		setContactBithday(AContactJid,DateTime(vcard->value(VVN_BIRTHDAY)).dateTime().date());
		vcard->unlock();
	}
}

void BirthdayReminder::onRosterItemReceived(IRoster *ARoster, const IRosterItem &AItem)
{
	Q_UNUSED(ARoster);
	if (FVCardPlugin && FVCardPlugin->hasVCard(AItem.itemJid))
	{
		IVCard *vcard = FVCardPlugin->vcard(AItem.itemJid);
		setContactBithday(AItem.itemJid,DateTime(vcard->value(VVN_BIRTHDAY)).dateTime().date());
		vcard->unlock();
	}
}

void BirthdayReminder::onOptionsOpened()
{
	FNotifyDate = Options::fileValue("birthdays.notify.date").toDate();
	QStringList notified = Options::fileValue("birthdays.notify.notified").toStringList();

	FNotifiedContacts.clear();
	foreach(QString contactJid, notified)
		FNotifiedContacts.append(contactJid);

	updateBirthdaysStates();
}

void BirthdayReminder::onOptionsClosed()
{
	QStringList notified;
	foreach (Jid contactJid, FNotifiedContacts)
		notified.append(contactJid.bare());

	Options::setFileValue(FNotifyDate,"birthdays.notify.date");
	Options::setFileValue(notified,"birthdays.notify.notified");
}

Q_EXPORT_PLUGIN2(plg_birthdayreminder, BirthdayReminder)
