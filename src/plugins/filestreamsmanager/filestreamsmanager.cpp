#include "filestreamsmanager.h"

#include <QSet>
#include <QDir>

FileStreamsManager::FileStreamsManager()
{
	FDataManager = NULL;
	FOptionsManager = NULL;
	FTrayManager = NULL;
	FMainWindowPlugin = NULL;
}

FileStreamsManager::~FileStreamsManager()
{

}

void FileStreamsManager::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("File Streams Manager");
	APluginInfo->description = tr("Allows to initiate a thread for transferring files between two XMPP entities");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
	APluginInfo->dependences.append(DATASTREAMSMANAGER_UUID);
}

bool FileStreamsManager::initConnections(IPluginManager *APluginManager, int &/*AInitOrder*/)
{
	IPlugin *plugin = APluginManager->pluginInterface("IDataStreamsManager").value(0,NULL);
	if (plugin)
	{
		FDataManager = qobject_cast<IDataStreamsManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IMainWindowPlugin").value(0,NULL);
	if (plugin)
	{
		FMainWindowPlugin = qobject_cast<IMainWindowPlugin *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
	}

	plugin = APluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
		{
			connect(FOptionsManager->instance(),SIGNAL(profileClosed(const QString &)),SLOT(onProfileClosed(const QString &)));
		}
	}

	return FDataManager!=NULL;
}

bool FileStreamsManager::initObjects()
{
	if (FDataManager)
	{
		FDataManager->insertProfile(this);
	}
	if (FTrayManager || FMainWindowPlugin)
	{
		Action *action = new Action;
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_FILESTREAMSMANAGER);
		action->setText(tr("File Transfers"));
		connect(action,SIGNAL(triggered(bool)),SLOT(onShowFileStreamsWindow(bool)));
		if (FMainWindowPlugin)
			FMainWindowPlugin->mainWindow()->mainMenu()->addAction(action,AG_MMENU_FILESTREAMSMANAGER,true);
		if (FTrayManager)
			FTrayManager->addAction(action, AG_TMTM_FILESTREAMSMANAGER, true);
	}
	return true;
}

bool FileStreamsManager::initSettings()
{
	QStringList availMethods = FDataManager!=NULL ? FDataManager->methods() : QStringList();
	Options::setDefaultValue(OPV_FILESTREAMS_DEFAULTDIR,QDir::homePath()+"/"+tr("Downloads"));
	Options::setDefaultValue(OPV_FILESTREAMS_DEFAULTMETHOD,QString(availMethods.contains(NS_SOCKS5_BYTESTREAMS) ? NS_SOCKS5_BYTESTREAMS : ""));
	Options::setDefaultValue(OPV_FILESTREAMS_ACCEPTABLEMETHODS,availMethods);

	if (FOptionsManager)
	{
		IOptionsDialogNode fileNode = { ONO_FILETRANSFER, OPN_FILETRANSFER, tr("File Transfer"),tr("Common options for file transfer"), MNI_FILESTREAMSMANAGER };
		FOptionsManager->insertOptionsDialogNode(fileNode);
		FOptionsManager->insertOptionsHolder(this);
	}
	return true;
}

IOptionsWidget *FileStreamsManager::optionsWidget(const QString &ANodeId, int &AOrder, QWidget *AParent)
{
	if (FDataManager && ANodeId == OPN_FILETRANSFER)
	{
		AOrder = OWO_FILESTREAMSMANAGER;
		return new FileStreamsOptions(FDataManager, this, AParent);
	}
	return NULL;
}

QString FileStreamsManager::profileNS() const
{
	return NS_SI_FILETRANSFER;
}

bool FileStreamsManager::requestDataStream(const QString &AStreamId, Stanza &ARequest) const
{
	IFileStream *stream = streamById(AStreamId);
	if (stream && stream->streamKind()==IFileStream::SendFile)
	{
		if (!stream->fileName().isEmpty() && stream->fileSize()>0)
		{
			QDomElement siElem = ARequest.firstElement("si",NS_STREAM_INITIATION);
			if (!siElem.isNull())
			{
				siElem.setAttribute("mime-type","text/plain");
				QDomElement fileElem = siElem.appendChild(ARequest.createElement("file",NS_SI_FILETRANSFER)).toElement();
				fileElem.setAttribute("name",stream->fileName().split("/").last());
				fileElem.setAttribute("size",stream->fileSize());
				if (!stream->fileHash().isEmpty())
					fileElem.setAttribute("hash",stream->fileHash());
				if (!stream->fileDate().isValid())
					fileElem.setAttribute("date",DateTime(stream->fileDate()).toX85Date());
				if (!stream->fileDescription().isEmpty())
					fileElem.appendChild(ARequest.createElement("desc")).appendChild(ARequest.createTextNode(stream->fileDescription()));
				if (stream->isRangeSupported())
					fileElem.appendChild(ARequest.createElement("range"));
				return true;
			}
		}
	}
	return false;
}

bool FileStreamsManager::responceDataStream(const QString &AStreamId, Stanza &AResponce) const
{
	IFileStream *stream = streamById(AStreamId);
	if (stream && stream->streamKind()==IFileStream::ReceiveFile)
	{
		if (stream->isRangeSupported() && (stream->rangeOffset()>0 || stream->rangeLength()>0))
		{
			QDomElement siElem = AResponce.firstElement("si",NS_STREAM_INITIATION);
			if (!siElem.isNull())
			{
				QDomElement fileElem = siElem.appendChild(AResponce.createElement("file",NS_SI_FILETRANSFER)).toElement();
				QDomElement rangeElem = fileElem.appendChild(AResponce.createElement("range")).toElement();
				if (stream->rangeOffset()>0)
					rangeElem.setAttribute("offset",stream->rangeOffset());
				if (stream->rangeLength()>0)
					rangeElem.setAttribute("length",stream->rangeLength());
			}
		}
		return true;
	}
	return false;
}

bool FileStreamsManager::dataStreamRequest(const QString &AStreamId, const Stanza &ARequest, const QList<QString> &AMethods)
{
	if (!FStreams.contains(AStreamId))
	{
		QList<QString> methods = AMethods.toSet().intersect(Options::node(OPV_FILESTREAMS_ACCEPTABLEMETHODS).value().toStringList().toSet()).toList();
		if (!methods.isEmpty())
		{
			for (QMultiMap<int, IFileStreamsHandler *>::const_iterator it = FHandlers.constBegin(); it!=FHandlers.constEnd(); it++)
			{
				IFileStreamsHandler *handler = it.value();
				if (handler->fileStreamRequest(it.key(),AStreamId,ARequest,methods))
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool FileStreamsManager::dataStreamResponce(const QString &AStreamId, const Stanza &AResponce, const QString &AMethod)
{
	IFileStreamsHandler *handler = streamHandler(AStreamId);
	return handler!=NULL ? handler->fileStreamResponce(AStreamId,AResponce,AMethod) : false;
}

bool FileStreamsManager::dataStreamError(const QString &AStreamId, const QString &AError)
{
	IFileStream *stream = streamById(AStreamId);
	if (stream)
	{
		stream->abortStream(AError);
		return true;
	}
	return false;
}

QList<IFileStream *> FileStreamsManager::streams() const
{
	return FStreams.values();
}

IFileStream *FileStreamsManager::streamById(const QString &AStreamId) const
{
	return FStreams.value(AStreamId, NULL);
}

IFileStream *FileStreamsManager::createStream(IFileStreamsHandler *AHandler, const QString &AStreamId, const Jid &AStreamJid,
    const Jid &AContactJid, IFileStream::StreamKind AKind, QObject *AParent)
{
	if (FDataManager && AHandler && !AStreamId.isEmpty() && !FStreams.contains(AStreamId))
	{
		IFileStream *stream = new FileStream(FDataManager,AStreamId,AStreamJid,AContactJid,AKind,AParent);
		connect(stream->instance(),SIGNAL(streamDestroyed()),SLOT(onStreamDestroyed()));
		FStreams.insert(AStreamId,stream);
		FStreamHandler.insert(AStreamId,AHandler);
		emit streamCreated(stream);
		return stream;
	}
	return NULL;
}

IFileStreamsHandler *FileStreamsManager::streamHandler(const QString &AStreamId) const
{
	return FStreamHandler.value(AStreamId);
}

void FileStreamsManager::insertStreamsHandler(IFileStreamsHandler *AHandler, int AOrder)
{
	if (AHandler!=NULL)
		FHandlers.insertMulti(AOrder,AHandler);
}

void FileStreamsManager::removeStreamsHandler(IFileStreamsHandler *AHandler, int AOrder)
{
	FHandlers.remove(AOrder,AHandler);
}

void FileStreamsManager::onStreamDestroyed()
{
	IFileStream *stream = qobject_cast<IFileStream *>(sender());
	if (stream)
	{
		FStreams.remove(stream->streamId());
		FStreamHandler.remove(stream->streamId());
		emit streamDestroyed(stream);
	}
}

void FileStreamsManager::onShowFileStreamsWindow(bool)
{
	if (FFileStreamsWindow.isNull())
		FFileStreamsWindow = new FileStreamsWindow(this, NULL);
	FFileStreamsWindow->show();
	WidgetManager::raiseWidget(FFileStreamsWindow);
	FFileStreamsWindow->activateWindow();
}

void FileStreamsManager::onProfileClosed(const QString &AName)
{
	Q_UNUSED(AName);

	if (!FFileStreamsWindow.isNull())
		delete FFileStreamsWindow;

	foreach(IFileStream *stream, FStreams.values())
		delete stream->instance();
}

Q_EXPORT_PLUGIN2(plg_filestreamsmanager, FileStreamsManager);
