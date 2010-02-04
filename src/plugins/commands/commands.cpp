#include "commands.h"

#define COMMAND_TAG_NAME              "command"
#define COMMANDS_TIMEOUT              60000

#define SHC_COMMANDS                  "/iq[@type='set']/query[@xmlns='" NS_COMMANDS "']"

#define ADR_STREAM_JID                Action::DR_StreamJid
#define ADR_COMMAND_JID               Action::DR_Parametr1
#define ADR_COMMAND_NODE              Action::DR_Parametr2

#define DIC_CLIENT                    "client"
#define DIC_AUTOMATION                "automation"
#define DIT_COMMAND_NODE              "command-node"
#define DIT_COMMAND_LIST              "command-list"

Commands::Commands()
{
  FDataForms = NULL;
  FXmppStreams = NULL;
  FStanzaProcessor = NULL;
  FDiscovery = NULL;
  FPresencePlugin = NULL;
  FXmppUriQueries = NULL;
}

Commands::~Commands()
{

}

void Commands::pluginInfo(IPluginInfo *APluginInfo)
{
  APluginInfo->name = tr("Ad-Hoc Commands");
  APluginInfo->description = tr("Allows to perform special commands provided by various services");
  APluginInfo->version = "1.0";
  APluginInfo->author = "Potapov S.A. aka Lion";
  APluginInfo->homePage = "http://www.vacuum-im.org";
  APluginInfo->dependences.append(DATAFORMS_UUID);
  APluginInfo->dependences.append(XMPPSTREAMS_UUID);
  APluginInfo->dependences.append(STANZAPROCESSOR_UUID);
}

bool Commands::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
  IPlugin *plugin = APluginManager->pluginInterface("IServiceDiscovery").value(0,NULL);
  if (plugin)
  {
    FDiscovery = qobject_cast<IServiceDiscovery *>(plugin->instance());
    if (FDiscovery)
    {
      connect(FDiscovery->instance(),SIGNAL(discoInfoReceived(const IDiscoInfo &)),SLOT(onDiscoInfoReceived(const IDiscoInfo &)));
      connect(FDiscovery->instance(),SIGNAL(discoInfoRemoved(const IDiscoInfo &)),SLOT(onDiscoInfoRemoved(const IDiscoInfo &)));
      connect(FDiscovery->instance(),SIGNAL(discoItemsReceived(const IDiscoItems &)),SLOT(onDiscoItemsReceived(const IDiscoItems &)));
    }
  }

  plugin = APluginManager->pluginInterface("IXmppStreams").value(0,NULL);
  if (plugin)
  {
    FXmppStreams = qobject_cast<IXmppStreams *>(plugin->instance());
    if (FXmppStreams)
    {
      connect(FXmppStreams->instance(),SIGNAL(opened(IXmppStream *)),SLOT(onStreamOpened(IXmppStream *)));
      connect(FXmppStreams->instance(),SIGNAL(closed(IXmppStream *)),SLOT(onStreamClosed(IXmppStream *)));
    }
  }

  plugin = APluginManager->pluginInterface("IStanzaProcessor").value(0,NULL);
  if (plugin)
    FStanzaProcessor = qobject_cast<IStanzaProcessor *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IDataForms").value(0,NULL);
  if (plugin)
    FDataForms = qobject_cast<IDataForms *>(plugin->instance());

  plugin = APluginManager->pluginInterface("IPresencePlugin").value(0,NULL);
  if (plugin)
  {
    FPresencePlugin = qobject_cast<IPresencePlugin *>(plugin->instance());
    if (FPresencePlugin)
    {
      connect(FPresencePlugin->instance(),SIGNAL(presenceReceived(IPresence *, const IPresenceItem &)),SLOT(onPresenceReceived(IPresence *, const IPresenceItem &)));
    }
  }

  plugin = APluginManager->pluginInterface("IXmppUriQueries").value(0,NULL);
  if (plugin)
    FXmppUriQueries = qobject_cast<IXmppUriQueries *>(plugin->instance());

  return FXmppStreams!=NULL && FStanzaProcessor!=NULL && FDataForms!=NULL;
}

bool Commands::initObjects()
{
  ErrorHandler::addErrorItem("malformed-action",ErrorHandler::MODIFY,ErrorHandler::BAD_REQUEST,
    tr("Can not understand the specified action"),NS_COMMANDS);
  ErrorHandler::addErrorItem("bad-action",ErrorHandler::MODIFY,ErrorHandler::BAD_REQUEST,
    tr("Can not accept the specified action"),NS_COMMANDS);
  ErrorHandler::addErrorItem("bad-locale",ErrorHandler::MODIFY,ErrorHandler::BAD_REQUEST,
    tr("Can not accept the specified language/locale"),NS_COMMANDS);
  ErrorHandler::addErrorItem("bad-payload",ErrorHandler::MODIFY,ErrorHandler::BAD_REQUEST,
    tr("The data form did not provide one or more required fields"),NS_COMMANDS);
  ErrorHandler::addErrorItem("bad-sessionid",ErrorHandler::MODIFY,ErrorHandler::BAD_REQUEST,
    tr("Specified session not present"),NS_COMMANDS);
  ErrorHandler::addErrorItem("session-expired",ErrorHandler::CANCEL,ErrorHandler::NOT_ALLOWED,
    tr("Specified session is no longer active"),NS_COMMANDS);

  if (FDiscovery)
  {
    registerDiscoFeatures();
    FDiscovery->insertDiscoHandler(this);
    FDiscovery->insertFeatureHandler(NS_COMMANDS,this,DFO_DEFAULT);
  }
  if (FXmppUriQueries)
  {
    FXmppUriQueries->insertUriHandler(this,XUHO_DEFAULT);
  }
  return true;
}

bool Commands::stanzaEdit(int AHandlerId, const Jid &AStreamJid, Stanza &AStanza, bool &AAccept)
{
  Q_UNUSED(AHandlerId);
  Q_UNUSED(AStreamJid);
  Q_UNUSED(AStanza);
  Q_UNUSED(AAccept);
  return false;
}

bool Commands::stanzaRead(int AHandlerId, const Jid &AStreamJid, const Stanza &AStanza, bool &AAccept)
{
  if (FSHICommands.value(AStreamJid) == AHandlerId)
  {
    ICommandRequest request;
    request.streamJid = AStreamJid;
    request.commandJid = AStanza.from();
    request.stanzaId = AStanza.id();

    QDomElement cmdElem = AStanza.firstElement(COMMAND_TAG_NAME,NS_COMMANDS);
    request.sessionId = cmdElem.attribute("sessionid");
    request.node = cmdElem.attribute("node");
    request.action = cmdElem.attribute("action",COMMAND_ACTION_EXECUTE);

    if (FDataForms)
    {
      QDomElement formElem = cmdElem.firstChildElement("x");
      while(!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
        formElem = formElem.nextSiblingElement("x");
      if (!formElem.isNull())
        request.form = FDataForms->dataForm(formElem);
    }

    ICommandServer *server = FServers.value(request.node);
    if (!server || !server->receiveCommandRequest(request))
    {
      Stanza reply = AStanza.replyError("malformed-action",NS_COMMANDS,ErrorHandler::BAD_REQUEST);
      FStanzaProcessor->sendStanzaOut(AStreamJid,reply);
    }
    AAccept = true;
  }
  return false;
}

void Commands::stanzaRequestResult(const Jid &AStreamJid, const Stanza &AStanza)
{
  if (FRequests.contains(AStanza.id()))
  {
    FRequests.removeAt(FRequests.indexOf(AStanza.id()));
    if (AStanza.type() == "result")
    {
      ICommandResult result;
      result.streamJid = AStreamJid;
      result.commandJid = AStanza.from();
      result.stanzaId = AStanza.id();

      QDomElement cmdElem = AStanza.firstElement(COMMAND_TAG_NAME,NS_COMMANDS);
      result.sessionId = cmdElem.attribute("sessionid");
      result.node = cmdElem.attribute("node");
      result.status = cmdElem.attribute("status");

      QDomElement actElem = cmdElem.firstChildElement("actions");
      result.execute = actElem.attribute("execute");
      actElem = actElem.firstChildElement();
      while (!actElem.isNull())
      {
        result.actions.append(actElem.tagName());
        actElem = actElem.nextSiblingElement();
      }

      QDomElement noteElem = cmdElem.firstChildElement("note");
      while(!noteElem.isNull())
      {
        ICommandNote note;
        note.type = noteElem.attribute("type",COMMAND_NOTE_INFO);
        note.message = noteElem.text();
        result.notes.append(note);
        noteElem = noteElem.nextSiblingElement("note");
      }

      if (FDataForms)
      {
        QDomElement formElem = cmdElem.firstChildElement("x");
        while(!formElem.isNull() && formElem.namespaceURI()!=NS_JABBER_DATA)
          formElem = formElem.nextSiblingElement("x");
        if (!formElem.isNull())
          result.form = FDataForms->dataForm(formElem);
      }

      foreach(ICommandClient *client, FClients)
        if (client->receiveCommandResult(result))
          break;
    }
    else
    {
      ICommandError error;
      error.stanzaId = AStanza.id();
      ErrorHandler err(AStanza.element(),NS_COMMANDS);
      error.code = err.code();
      error.condition = err.condition();
      error.message = err.message();
      foreach(ICommandClient *client, FClients)
        if (client->receiveCommandError(error))
          break;
    }
  }
}

void Commands::stanzaRequestTimeout(const Jid &AStreamJid, const QString &AStanzaId)
{
  Q_UNUSED(AStreamJid);
  if (FRequests.contains(AStanzaId))
  {
    ICommandError error;
    error.stanzaId = AStanzaId;
    ErrorHandler err(ErrorHandler::REQUEST_TIMEOUT);
    error.code = err.code();
    error.condition = err.condition();
    error.message = err.message();
    foreach(ICommandClient *client, FClients)
      if (client->receiveCommandError(error))
        break;
  }
}

bool Commands::xmppUriOpen(const Jid &AStreamJid, const Jid &AContactJid, const QString &AAction, const QMultiMap<QString, QString> &AParams)
{
  if (AAction == "command")
  {
    QString node = AParams.value("node");
    if (!node.isEmpty())
    {
      QString action = AParams.value("action","execute");
      if (action == "execute")
      {
        executeCommnad(AStreamJid, AContactJid, node);
      }
    }
    return true;
  }
  return false;
}

void Commands::fillDiscoInfo(IDiscoInfo &ADiscoInfo)
{
  if (ADiscoInfo.node == NS_COMMANDS)
  {
    IDiscoIdentity identity;
    identity.category = DIC_AUTOMATION;
    identity.type = DIT_COMMAND_LIST;
    identity.name = "Commands";
    ADiscoInfo.identity.append(identity);

    if (!ADiscoInfo.features.contains(NS_COMMANDS))
      ADiscoInfo.features.append(NS_COMMANDS);
  }
  else if (FServers.contains(ADiscoInfo.node))
  {
    IDiscoIdentity identity;
    identity.category = DIC_AUTOMATION;
    identity.type = DIT_COMMAND_NODE;
    identity.name = FServers.value(ADiscoInfo.node)->commandName(ADiscoInfo.node);
    ADiscoInfo.identity.append(identity);

    if (!ADiscoInfo.features.contains(NS_COMMANDS))
      ADiscoInfo.features.append(NS_COMMANDS);
    if (!ADiscoInfo.features.contains(NS_JABBER_DATA))
      ADiscoInfo.features.append(NS_JABBER_DATA);
  }
}

void Commands::fillDiscoItems(IDiscoItems &ADiscoItems)
{
  if (!FServers.isEmpty())
  {
    if (ADiscoItems.node == NS_COMMANDS)
    {
      QList<QString> nodes = FServers.keys();
      foreach(QString node, nodes)
      {
        QString name = FServers.value(node)->commandName(node);
        if (!name.isEmpty())
        {
          IDiscoItem ditem;
          ditem.itemJid = ADiscoItems.streamJid;
          ditem.node = node;
          ditem.name = name;
          ADiscoItems.items.append(ditem);
        }
      }
    }
    else if (ADiscoItems.node.isEmpty())
    {
      IDiscoItem ditem;
      ditem.itemJid = ADiscoItems.streamJid;
      ditem.node = NS_COMMANDS;
      ditem.name = "Commands";
    }
  }
}

bool Commands::execDiscoFeature(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo)
{
  if (AFeature==NS_COMMANDS && !ADiscoInfo.node.isEmpty() && FDiscovery->findIdentity(ADiscoInfo.identity,DIC_AUTOMATION,DIT_COMMAND_NODE)>=0)
  {
    executeCommnad(AStreamJid,ADiscoInfo.contactJid,ADiscoInfo.node);
    return true;
  }
  return false;
}

Action *Commands::createDiscoFeatureAction(const Jid &AStreamJid, const QString &AFeature, const IDiscoInfo &ADiscoInfo, QWidget *AParent)
{
  if (FSHICommands.contains(AStreamJid) && AFeature==NS_COMMANDS)
  {
    if (FDiscovery->findIdentity(ADiscoInfo.identity,DIC_AUTOMATION,DIT_COMMAND_NODE)>=0)
    {
      if (!ADiscoInfo.node.isEmpty())
      {
        Action *action = new Action(AParent);
        action->setText(tr("Execute"));
        action->setIcon(RSR_STORAGE_MENUICONS,MNI_COMMANDS);
        action->setData(ADR_STREAM_JID,AStreamJid.full());
        action->setData(ADR_COMMAND_JID,ADiscoInfo.contactJid.full());
        action->setData(ADR_COMMAND_NODE,ADiscoInfo.node);
        connect(action,SIGNAL(triggered(bool)),SLOT(onExecuteActionTriggered(bool)));
        return action;
      }
    }
    else if (FCommands.value(AStreamJid).contains(ADiscoInfo.contactJid))
    {
      QList<ICommand> commands = FCommands.value(AStreamJid).value(ADiscoInfo.contactJid);
      if (!commands.isEmpty())
      {
        Menu *execMenu = new Menu(AParent);
        execMenu->setTitle(tr("Commands"));
        execMenu->setIcon(RSR_STORAGE_MENUICONS,MNI_COMMANDS);
        foreach (ICommand command, commands)
        {
          Action *action = new Action(execMenu);
          action->setText(command.name);
          action->setData(ADR_STREAM_JID,AStreamJid.full());
          action->setData(ADR_COMMAND_JID,command.itemJid.full());
          action->setData(ADR_COMMAND_NODE,command.node);
          connect(action,SIGNAL(triggered(bool)),SLOT(onExecuteActionTriggered(bool)));
          execMenu->addAction(action,AG_DEFAULT,false);
        }
        return execMenu->menuAction();
      }
    }
    else if (ADiscoInfo.features.contains(NS_COMMANDS))
    {
      Action *action = new Action(AParent);
      action->setText(tr("Request commands"));
      action->setIcon(RSR_STORAGE_MENUICONS,MNI_COMMANDS);
      action->setData(ADR_STREAM_JID,AStreamJid.full());
      action->setData(ADR_COMMAND_JID,ADiscoInfo.contactJid.full());
      connect(action,SIGNAL(triggered(bool)),SLOT(onRequestActionTriggered(bool)));
      return action;
    }
  }
  return NULL;
}

QList<QString> Commands::commandNodes() const
{
  return FServers.keys();
}

ICommandServer *Commands::commandServer(const QString &ANode) const
{
  return FServers.value(ANode);
}

void Commands::insertServer(const QString &ANode, ICommandServer *AServer)
{
  if (AServer && !FServers.contains(ANode))
  {
    FServers.insert(ANode,AServer);
    emit serverInserted(ANode, AServer);
  }
}

void Commands::removeServer(const QString &ANode)
{
  if (FServers.contains(ANode))
  {
    FServers.remove(ANode);
    emit serverRemoved(ANode);
  }
}

void Commands::insertClient(ICommandClient *AClient)
{
  if (AClient && !FClients.contains(AClient))
  {
    FClients.append(AClient);
    emit clientInserted(AClient);
  }
}

void Commands::removeClient(ICommandClient *AClient)
{
  if (FClients.contains(AClient))
  {
    FClients.removeAt(FClients.indexOf(AClient));
    emit clientRemoved(AClient);
  }
}

QString Commands::sendCommandRequest(const ICommandRequest &ARequest)
{
  if (FStanzaProcessor)
  {
    Stanza request("iq");
    request.setTo(ARequest.commandJid.eFull()).setType("set").setId(FStanzaProcessor->newId());
    QDomElement cmdElem = request.addElement(COMMAND_TAG_NAME,NS_COMMANDS);
    cmdElem.setAttribute("node",ARequest.node);
    if (!ARequest.sessionId.isEmpty())
      cmdElem.setAttribute("sessionid",ARequest.sessionId);
    if (!ARequest.action.isEmpty())
      cmdElem.setAttribute("action",ARequest.action);
    if (FDataForms && !ARequest.form.type.isEmpty())
      FDataForms->xmlForm(ARequest.form,cmdElem);
    if (FStanzaProcessor->sendStanzaRequest(this,ARequest.streamJid,request,COMMANDS_TIMEOUT))
    {
      FRequests.append(request.id());
      return request.id();
    }
  }
  return QString::null;
}

bool Commands::sendCommandResult(const ICommandResult &AResult)
{
  if (FStanzaProcessor)
  {
    Stanza result("iq");
    result.setTo(AResult.commandJid.eFull()).setType("result").setId(AResult.stanzaId);

    QDomElement cmdElem = result.addElement(COMMAND_TAG_NAME,NS_COMMANDS);
    cmdElem.setAttribute("node",AResult.node);
    cmdElem.setAttribute("sessionid",AResult.sessionId);
    cmdElem.setAttribute("status",AResult.status);

    if (!AResult.actions.isEmpty())
    {
      QDomElement actElem = cmdElem.appendChild(result.createElement("actions")).toElement();
      actElem.setAttribute("execute",AResult.execute);
      foreach(QString action,AResult.actions)
        actElem.appendChild(result.createElement(action));
    }

    if (FDataForms && !AResult.form.type.isEmpty())
      FDataForms->xmlForm(AResult.form,cmdElem);

    foreach(ICommandNote note,AResult.notes)
    {
      QDomElement noteElem = cmdElem.appendChild(result.createElement("note")).toElement();
      noteElem.setAttribute("type",note.type);
      noteElem.appendChild(result.createTextNode(note.message));
    }

    return FStanzaProcessor->sendStanzaOut(AResult.streamJid,result);
  }
  return false;
}

QList<ICommand> Commands::contactCommands(const Jid &AStreamJid, const Jid &AContactJid) const
{
  return FCommands.value(AStreamJid).value(AContactJid);
}

bool Commands::executeCommnad(const Jid &AStreamJid, const Jid &ACommandJid, const QString &ANode)
{
  IXmppStream *stream = FXmppStreams!=NULL ? FXmppStreams->xmppStream(AStreamJid) : NULL;
  if (FDataForms && stream && stream->isOpen())
  {
    CommandDialog *dialog = new CommandDialog(this,FDataForms,AStreamJid,ACommandJid,ANode,NULL);
    connect(stream->instance(),SIGNAL(closed()),dialog,SLOT(reject()));
    dialog->executeCommand();
    dialog->show();
    return true;
  }
  return false;
}

void Commands::registerDiscoFeatures()
{
  IDiscoFeature dfeature;
  dfeature.active = true;
  dfeature.icon = IconStorage::staticStorage(RSR_STORAGE_MENUICONS)->getIcon(MNI_COMMANDS);
  dfeature.var = NS_COMMANDS;
  dfeature.name = tr("Ad-Hoc Commands");
  dfeature.description = tr("Supports the running or performing of the special services commands");
  FDiscovery->insertDiscoFeature(dfeature);
}

void Commands::onStreamOpened(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor)
  {
    IStanzaHandle shandle;
    shandle.handler = this;
    shandle.order = SHO_DEFAULT;
    shandle.direction = IStanzaHandle::DirectionIn;
    shandle.streamJid = AXmppStream->streamJid();
    shandle.conditions.append(SHC_COMMANDS);
    FSHICommands.insert(AXmppStream->streamJid(), FStanzaProcessor->insertStanzaHandle(shandle));
  }
}

void Commands::onStreamClosed(IXmppStream *AXmppStream)
{
  if (FStanzaProcessor)
  {
    FStanzaProcessor->removeStanzaHandle(FSHICommands.take(AXmppStream->streamJid()));
  }
  FCommands.remove(AXmppStream->streamJid());
  FOnlineAgents.remove(AXmppStream->streamJid());
}

void Commands::onDiscoInfoReceived(const IDiscoInfo &AInfo)
{
  if (AInfo.node.isEmpty() && FDiscovery->findIdentity(AInfo.identity,DIC_CLIENT,QString::null)<0)
    if (AInfo.features.contains(NS_COMMANDS) && !FCommands.value(AInfo.streamJid).contains(AInfo.contactJid))
      FDiscovery->requestDiscoItems(AInfo.streamJid,AInfo.contactJid,NS_COMMANDS);
}

void Commands::onDiscoInfoRemoved(const IDiscoInfo &AInfo)
{
  if (AInfo.node.isEmpty())
    FCommands[AInfo.streamJid].remove(AInfo.contactJid);
}

void Commands::onDiscoItemsReceived(const IDiscoItems &AItems)
{
  if (AItems.node == NS_COMMANDS)
  {
    QList<ICommand> &commands = FCommands[AItems.streamJid][AItems.contactJid];
    commands.clear();
    foreach(IDiscoItem ditem, AItems.items)
    {
      if (!ditem.node.isEmpty() && ditem.itemJid.isValid())
      {
        ICommand command;
        command.node = ditem.node;
        command.name = !ditem.name.isEmpty() ? ditem.name : ditem.node;
        command.itemJid = ditem.itemJid;
        commands.append(command);
      }
    }
    emit commandsUpdated(AItems.streamJid,AItems.contactJid,commands);
  }
}

void Commands::onPresenceReceived(IPresence *APresence, const IPresenceItem &APresenceItem)
{
  if (FDiscovery && APresence->isOpen() && APresenceItem.itemJid.node().isEmpty())
  {
    if (FDiscovery->discoInfo(APresence->streamJid(),APresenceItem.itemJid).features.contains(NS_COMMANDS))
    {
      QList<Jid> &online = FOnlineAgents[APresence->streamJid()];
      if (APresenceItem.show==IPresence::Offline || APresenceItem.show==IPresence::Error)
      {
        if (online.contains(APresenceItem.itemJid))
        {
          online.removeAll(APresenceItem.itemJid);
          FDiscovery->requestDiscoItems(APresence->streamJid(),APresenceItem.itemJid,NS_COMMANDS);
        }
      }
      else 
      {
        if (!online.contains(APresenceItem.itemJid))
        {
          online.append(APresenceItem.itemJid);
          FDiscovery->requestDiscoItems(APresence->streamJid(),APresenceItem.itemJid,NS_COMMANDS);
        }
      }
    }
  }
}

void Commands::onExecuteActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid commandJid = action->data(ADR_COMMAND_JID).toString();
    QString node = action->data(ADR_COMMAND_NODE).toString();
    executeCommnad(streamJid,commandJid,node);
  }
}

void Commands::onRequestActionTriggered(bool)
{
  Action *action = qobject_cast<Action *>(sender());
  if (FDiscovery && action)
  {
    Jid streamJid = action->data(ADR_STREAM_JID).toString();
    Jid commandJid = action->data(ADR_COMMAND_JID).toString();
    FDiscovery->requestDiscoItems(streamJid,commandJid,NS_COMMANDS);
  }
}

Q_EXPORT_PLUGIN2(plg_commands, Commands)
