#include "accountmanager.h"

#include <QSet>
#include <QIcon>
#include <QTime>

#define OPTIONS_NODE_ACCOUNTS "Accounts"
#define MAINMENU_ACTION_GROUP_OPTIONS 700
#define SYSTEM_ICONSETFILE "system/common.jisp"

AccountManager::AccountManager()
{
  FPluginManager = NULL;
  FSettingsPlugin = NULL;
  FSettings = NULL;
  FMainWindowPlugin = NULL;
  FRostersViewPlugin = NULL;
  FAccountsSetup = NULL;
  srand(QTime::currentTime().msec());
}

AccountManager::~AccountManager()
{

}

//IPlugin
void AccountManager::pluginInfo(PluginInfo *APluginInfo)
{
  APluginInfo->author = tr("Potapov S.A. aka Lion");
  APluginInfo->description = tr("Creating and removing accounts");
  APluginInfo->homePage = "http://jrudevels.org";
  APluginInfo->name = "Account manager";
  APluginInfo->uid = ACCOUNTMANAGER_UUID;
  APluginInfo->version = "0.1";
  APluginInfo->dependences.append("{8074A197-3B77-4bb0-9BD3-6F06D5CB8D15}"); //IXmppStreams  
  APluginInfo->dependences.append("{6030FCB2-9F1E-4ea2-BE2B-B66EBE0C4367}"); //ISettings  
}

bool AccountManager::initPlugin(IPluginManager *APluginManager)
{
  FPluginManager = APluginManager;

  IPlugin *plugin = FPluginManager->getPlugins("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
  }

  plugin = FPluginManager->getPlugins("ISettingsPlugin").value(0,NULL);
  if (plugin)
  {
    FSettingsPlugin = qobject_cast<ISettingsPlugin *>(plugin->instance());
    if (FSettingsPlugin)
    {
      FSettings = FSettingsPlugin->openSettings(ACCOUNTMANAGER_UUID,this);
      if (FSettings)
      {
        connect(FSettings->instance(),SIGNAL(opened()),SLOT(onSettingsOpened()));
        connect(FSettings->instance(),SIGNAL(closed()),SLOT(onSettingsClosed()));
      }
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogAccepted()),SLOT(onOptionsDialogAccepted()));
      connect(FSettingsPlugin->instance(),SIGNAL(optionsDialogRejected()),SLOT(onOptionsDialogRejected()));
   }
  }

  plugin = APluginManager->getPlugins("IMainWindowPlugin").value(0,NULL);
  if (plugin)
    FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());

  plugin = APluginManager->getPlugins("IRostersViewPlugin").value(0,NULL);
  if (plugin)
    FRostersViewPlugin = qobject_cast<IRostersViewPlugin *>(plugin->instance());

  return FXmppStreams!=NULL && FSettingsPlugin!=NULL && FSettings!=NULL;
}

bool AccountManager::startPlugin()
{
  FSettingsPlugin->openOptionsNode(OPTIONS_NODE_ACCOUNTS,tr("Accounts"),
    tr("Creating and removing accounts"),QIcon());
  FSettingsPlugin->appendOptionsHolder(this);

  if (FRostersViewPlugin)
  {
    connect(FRostersViewPlugin->rostersView(),SIGNAL(contextMenu(const QModelIndex &, Menu *)),
      SLOT(onRostersViewContextMenu(const QModelIndex &, Menu *)));
  }

  if (FMainWindowPlugin)
  {
    FAccountsSetup = new Action(this);
    FAccountsSetup->setIcon(SYSTEM_ICONSETFILE,"psi/account");
    FAccountsSetup->setText(tr("Account setup..."));
    FAccountsSetup->setData(Action::DR_Parametr1,OPTIONS_NODE_ACCOUNTS);
    connect(FAccountsSetup,SIGNAL(triggered(bool)),
      FSettingsPlugin->instance(),SLOT(openOptionsAction(bool)));
    FMainWindowPlugin->mainWindow()->mainMenu()->addAction(FAccountsSetup,MAINMENU_ACTION_GROUP_OPTIONS,true);
  }
  return true;
}

//IAccountManager
IAccount *AccountManager::addAccount(const QString &AName, const Jid &AStreamJid)
{
  IAccount *account = accountByStream(AStreamJid);
  if (!account)
  {
    IXmppStream *stream = FXmppStreams->newStream(AStreamJid.full());
    account = new Account(newId(),FSettings,stream,this);
    account->setName(AName);
    FAccounts.append((Account *)account);
    emit added(account);
    openAccountOptionsNode(account->accountId());
  }
  return account;
}

IAccount *AccountManager::addAccount(const QString &AAccountId, const QString &AName, 
                                     const Jid &AStreamJid)
{
  IAccount *account = accountById(AAccountId);
  if (!account && !AAccountId.isEmpty())
  {
    QString name = AName;
    if (name.isEmpty())
      name = FSettings->valueNS("account[]:name",AAccountId,AAccountId).toString();

    Jid streamJid = AStreamJid;
    if (!streamJid.isValid())
      streamJid = FSettings->valueNS("account[]:streamJid",AAccountId).toString();

    IXmppStream *stream = FXmppStreams->newStream(streamJid);
    account = new Account(AAccountId,FSettings,stream,this);
    account->setName(name);
    FAccounts.append((Account *)account);
    emit added(account);
    openAccountOptionsNode(AAccountId);
  }
  return account;
}

void AccountManager::showAccount(IAccount *AAccount)
{
  if (!FXmppStreams->isActive(AAccount->xmppStream()))
  {
    FXmppStreams->addStream(AAccount->xmppStream());
    emit shown(AAccount);
  }
}

void AccountManager::hideAccount(IAccount *AAccount)
{
  if (FXmppStreams->isActive(AAccount->xmppStream()))
  {
    AAccount->xmppStream()->close();
    FXmppStreams->removeStream(AAccount->xmppStream());
    emit hidden(AAccount);
  }
}

IAccount *AccountManager::accountById(const QString &AAcoountId) const
{
  Account *account;
  foreach(account, FAccounts)
    if (account->accountId() == AAcoountId)
      return account;
  return NULL;
}

IAccount *AccountManager::accountByName(const QString &AName) const
{
  Account *account;
  foreach(account, FAccounts)
    if (account->name() == AName)
      return account;
  return NULL;
}

IAccount *AccountManager::accountByStream(const Jid &AStreamJid) const
{
  Account *account;
  foreach(account, FAccounts)
    if (account->xmppStream()->jid() == AStreamJid)
      return account;
  return NULL;
}

void AccountManager::removeAccount(IAccount *AAccount)
{
  if (FAccounts.contains((Account *)AAccount))
  {
    closeAccountOptionsNode(AAccount->accountId());
    hideAccount(AAccount);
    FAccounts.removeAt(FAccounts.indexOf((Account *)AAccount));
    emit removed(AAccount);
    FXmppStreams->destroyStream(AAccount->streamJid());
    delete qobject_cast<Account *>(AAccount->instance());
  }
}

void AccountManager::destroyAccount(const QString &AAccountId)
{
  IAccount *account = accountById(AAccountId);
  if (account)
  {
    account->clear();
    emit destroyed(account);
    removeAccount(account);
  }
}


//IOptionsHolder
QWidget *AccountManager::optionsWidget(const QString &ANode, int &/*AOrder*/) const
{
  if (ANode == OPTIONS_NODE_ACCOUNTS)
  {
    if (FAccountManage.isNull())
    {
      FAccountManage = new AccountManage(NULL);
      connect(FAccountManage,SIGNAL(accountAdded(const QString &)),
        SLOT(onOptionsAccountAdded(const QString &)));
      connect(FAccountManage,SIGNAL(accountRemoved(const QString &)),
        SLOT(onOptionsAccountRemoved(const QString &)));
      IAccount *account;
      foreach(account,FAccounts)
      {
        FAccountManage->setAccount(account->accountId(),account->name(),
          account->streamJid().full(),account->isActive());
      }
    }
    return FAccountManage;
  }
  else if (ANode.startsWith(OPTIONS_NODE_ACCOUNTS + QString("::")))
  {
    QStringList nodeTree = ANode.split("::",QString::SkipEmptyParts);
    QString id = nodeTree.value(1);
    AccountOptions *options = new AccountOptions(id);
    IAccount *account = accountById(id);
    if (account)
    {
      options->setOption(AccountOptions::AO_Name,account->name());
      options->setOption(AccountOptions::AO_StreamJid,account->streamJid().full());
      options->setOption(AccountOptions::AO_Password,account->password());
      options->setOption(AccountOptions::AO_ManualHostPort,account->manualHostPort());
      options->setOption(AccountOptions::AO_Host,account->host());
      options->setOption(AccountOptions::AO_Port,account->port());
      options->setOption(AccountOptions::AO_DefLang,account->defaultLang());
      options->setOption(AccountOptions::AO_ProxyTypes,account->xmppStream()->connection()->proxyTypes());
      options->setOption(AccountOptions::AO_ProxyType,account->proxyType());
      options->setOption(AccountOptions::AO_ProxyHost,account->proxyHost());
      options->setOption(AccountOptions::AO_ProxyPort,account->proxyPort());
      options->setOption(AccountOptions::AO_ProxyUser,account->proxyUsername());
      options->setOption(AccountOptions::AO_ProxyPassword,account->proxyPassword());
      options->setOption(AccountOptions::AO_PollServer,account->pollServer());
      options->setOption(AccountOptions::AO_AutoConnect,account->autoConnect());
      options->setOption(AccountOptions::AO_AutoReconnect,account->autoReconnect());
    }
    else
    {
      options->setOption(AccountOptions::AO_Name,FAccountManage->accountName(id));
      options->setOption(AccountOptions::AO_Port,5222);
      options->setOption(AccountOptions::AO_ProxyTypes,tr("Direct connection"));
      options->setOption(AccountOptions::AO_ProxyType,0);
      options->setOption(AccountOptions::AO_AutoReconnect,true);
   }
    FAccountOptions.insert(id,options);
    return options;
  }
  return NULL;
}

QString AccountManager::newId() const
{
  QString id;
  while (id.isEmpty() || accountById(id))
    id = QString::number((rand()<<16)+rand(),36);
  return id;
}

void AccountManager::openAccountOptionsNode(const QString &AAccountId, const QString &AName)
{
  QString node = OPTIONS_NODE_ACCOUNTS+QString("::")+AAccountId;
  QString name = AName;
  if (AName.isEmpty())
  {
    IAccount *account = accountById(AAccountId);
    if (account)
      name = account->name();
  }
  if (!FAccountOptions.contains(AAccountId))
    FAccountOptions.insert(AAccountId,NULL);
  FSettingsPlugin->openOptionsNode(node,name,tr("Account details and connection options"),QIcon());
}

void AccountManager::closeAccountOptionsNode(const QString &AAccountId)
{
  QString node = OPTIONS_NODE_ACCOUNTS+QString("::")+AAccountId;
  FSettingsPlugin->closeOptionsNode(node);
  if (FAccountOptions.contains(AAccountId))
  {
    QPointer <AccountOptions> options = FAccountOptions.take(AAccountId);
    if (!options.isNull())
      delete options;
  }
}

void AccountManager::onOptionsAccountAdded(const QString &AName)
{
  QString id = newId();
  FAccountManage->setAccount(id,AName,QString(),Qt::Unchecked);
  openAccountOptionsNode(id,AName);
  FSettingsPlugin->openOptionsDialog(OPTIONS_NODE_ACCOUNTS+QString("::")+id);
}

void AccountManager::onOptionsAccountRemoved(const QString &AAccountId)
{
  FAccountManage->removeAccount(AAccountId);
  closeAccountOptionsNode(AAccountId);
}

void AccountManager::onOptionsDialogAccepted()
{
  IAccount *account;
  QSet<QString> curAccounts;
  foreach(account,FAccounts)
    curAccounts += account->accountId(); 

  QSet<QString> allAccounts = FAccountOptions.keys().toSet();
  QSet<QString> newAccounts = allAccounts - curAccounts;
  QSet<QString> oldAccounts = curAccounts - allAccounts;

  QString id;
  foreach(id,oldAccounts)
    destroyAccount(id);

  foreach(id,allAccounts)
  {
    QPointer<AccountOptions> options = FAccountOptions.value(id);
    QString name = options->option(AccountOptions::AO_Name).toString();
    Jid streamJid = options->option(AccountOptions::AO_StreamJid).toString();
    if (!name.isEmpty() && !streamJid.node().isEmpty() && !streamJid.domane().isEmpty())
    {
      if (!newAccounts.contains(id))
      {
        account = accountById(id);
        account->setName(name);
        account->setStreamJid(streamJid);
      }
      else
        account = addAccount(id,name,streamJid);
      account->setManualHostPort(options->option(AccountOptions::AO_ManualHostPort).toBool());
      account->setHost(options->option(AccountOptions::AO_Host).toString());
      account->setPort(options->option(AccountOptions::AO_Port).toInt());
      account->setPassword(options->option(AccountOptions::AO_Password).toString());
      account->setDefaultLang(options->option(AccountOptions::AO_DefLang).toString());
      account->setProxyType(options->option(AccountOptions::AO_ProxyType).toInt());
      account->setProxyHost(options->option(AccountOptions::AO_ProxyHost).toString());
      account->setProxyPort(options->option(AccountOptions::AO_ProxyPort).toInt());
      account->setProxyUsername(options->option(AccountOptions::AO_ProxyUser).toString());
      account->setProxyPassword(options->option(AccountOptions::AO_ProxyPassword).toString());
      account->setPollServer(options->option(AccountOptions::AO_PollServer).toString());
      account->setAutoConnect(options->option(AccountOptions::AO_AutoConnect).toBool());
      account->setAutoReconnect(options->option(AccountOptions::AO_AutoReconnect).toBool());
      account->setActive(FAccountManage->accountActive(id));
      FAccountManage->setAccount(id,name,streamJid.full(),account->isActive());
      openAccountOptionsNode(id,name);
      if (account->isActive())
        showAccount(account);
      else
        hideAccount(account);
    } 
    else if (newAccounts.contains(id))
    {
      FAccountManage->removeAccount(id);
      closeAccountOptionsNode(id);
    }
  }
}

void AccountManager::onOptionsDialogRejected()
{
  IAccount *account;
  QSet<QString> curAccounts;
  foreach(account,FAccounts)
    curAccounts += account->accountId(); 

  QSet<QString> allAccounts = FAccountOptions.keys().toSet();
  QSet<QString> newAccounts = allAccounts - curAccounts;

  QString id;
  foreach(id,newAccounts)
  {
    FAccountManage->removeAccount(id);
    closeAccountOptionsNode(id);
  }
}

void AccountManager::onSettingsOpened()
{
  QString id;
  QList<QString> acoountsId = FSettings->values("account[]").keys();
  foreach(id,acoountsId)
  {
    IAccount *account = addAccount(id);
    if (account && account->isActive())
      showAccount(account);
  }
}

void AccountManager::onSettingsClosed()
{
  while (FAccounts.count() > 0)
    removeAccount(FAccounts.at(0));
}

void AccountManager::onRostersViewContextMenu(const QModelIndex &AIndex, Menu *AMenu)
{
  if (AIndex.isValid() && AIndex.data(IRosterIndex::DR_Type).toInt() == IRosterIndex::IT_StreamRoot)
  {
    QString streamJid = AIndex.data(IRosterIndex::DR_StreamJid).toString();
    IAccount *account = accountByStream(streamJid);
    if (account)
    {
      Action *modify = new Action(AMenu);
      modify->setIcon(SYSTEM_ICONSETFILE,"psi/account");
      modify->setText(tr("Modify account..."));
      modify->setData(Action::DR_Parametr1,OPTIONS_NODE_ACCOUNTS+QString("::")+account->accountId());
      connect(modify,SIGNAL(triggered(bool)),
        FSettingsPlugin->instance(),SLOT(openOptionsAction(bool)));
      AMenu->addAction(modify,MAINMENU_ACTION_GROUP_OPTIONS,true);
    }
  }
}

Q_EXPORT_PLUGIN2(AccountManagerPlugin, AccountManager)
