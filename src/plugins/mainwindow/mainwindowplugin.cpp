#include "mainwindowplugin.h"

#include <QShortcut>
#include <QApplication>
#include <QDesktopWidget>

MainWindowPlugin::MainWindowPlugin()
{
	FPluginManager = NULL;
	FOptionsManager = NULL;
	FTrayManager = NULL;

	FActivationChanged = QTime::currentTime();
	FMainWindow = new MainWindow(new QWidget, Qt::Window|Qt::CustomizeWindowHint|Qt::WindowTitleHint|Qt::WindowCloseButtonHint);
	FMainWindow->installEventFilter(this);

	QShortcut *shortcutClose = new QShortcut(tr("Esc","Close Roster"),FMainWindow);
	connect(shortcutClose,SIGNAL(activated()),FMainWindow,SLOT(close()));
}

MainWindowPlugin::~MainWindowPlugin()
{
	delete FMainWindow;
}

void MainWindowPlugin::pluginInfo(IPluginInfo *APluginInfo)
{
	APluginInfo->name = tr("Main Window");
	APluginInfo->description = tr("Allows other modules to place their widgets in the main window");
	APluginInfo->version = "1.0";
	APluginInfo->author = "Potapov S.A. aka Lion";
	APluginInfo->homePage = "http://www.vacuum-im.org";
}

bool MainWindowPlugin::initConnections(IPluginManager *APluginManager, int &AInitOrder)
{
	Q_UNUSED(AInitOrder);
	FPluginManager = APluginManager;

	IPlugin *plugin = FPluginManager->pluginInterface("IOptionsManager").value(0,NULL);
	if (plugin)
	{
		FOptionsManager = qobject_cast<IOptionsManager *>(plugin->instance());
		if (FOptionsManager)
		{
			connect(FOptionsManager->instance(), SIGNAL(profileRenamed(const QString &, const QString &)),
				SLOT(onProfileRenamed(const QString &, const QString &)));
		}
	}

	plugin = APluginManager->pluginInterface("ITrayManager").value(0,NULL);
	if (plugin)
	{
		FTrayManager = qobject_cast<ITrayManager *>(plugin->instance());
		if (FTrayManager)
		{
			connect(FTrayManager->instance(),SIGNAL(notifyActivated(int, QSystemTrayIcon::ActivationReason)),
				SLOT(onTrayNotifyActivated(int,QSystemTrayIcon::ActivationReason)));
		}
	}

	connect(Options::instance(),SIGNAL(optionsOpened()),SLOT(onOptionsOpened()));
	connect(Options::instance(),SIGNAL(optionsClosed()),SLOT(onOptionsClosed()));

	return true;
}

bool MainWindowPlugin::initObjects()
{
	Action *action = new Action(this);
	action->setText(tr("Quit"));
	action->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_QUIT);
	connect(action,SIGNAL(triggered()),FPluginManager->instance(),SLOT(quit()));
	FMainWindow->mainMenu()->addAction(action,AG_MMENU_MAINWINDOW,true);

	if (FTrayManager)
	{
		action = new Action(this);
		action->setText(tr("Show roster"));
		action->setIcon(RSR_STORAGE_MENUICONS,MNI_MAINWINDOW_SHOW_ROSTER);
		connect(action,SIGNAL(triggered(bool)),SLOT(onShowMainWindowByAction(bool)));
		FTrayManager->addAction(action,AG_TMTM_MAINWINDOW,true);
	}

	return true;
}

bool MainWindowPlugin::initSettings()
{
	Options::setDefaultValue(OPV_MAINWINDOW_SHOW,true);
	Options::setDefaultValue(OPV_MAINWINDOW_SIZE,QSize(200,500));
	Options::setDefaultValue(OPV_MAINWINDOW_POSITION,QPoint(0,0));
	return true;
}

bool MainWindowPlugin::startPlugin()
{
	updateTitle();
	return true;
}

IMainWindow *MainWindowPlugin::mainWindow() const
{
	return FMainWindow;
}

void MainWindowPlugin::updateTitle()
{
	if (FOptionsManager && FOptionsManager->isOpened())
		FMainWindow->setWindowTitle(CLIENT_NAME" - "+FOptionsManager->currentProfile());
	else
		FMainWindow->setWindowTitle(CLIENT_NAME);
}

void MainWindowPlugin::showMainWindow()
{
	FMainWindow->show();
	correctWindowPosition();
	WidgetManager::showActivateRaiseWindow(FMainWindow);
}

void MainWindowPlugin::correctWindowPosition()
{
	QRect windowRect = FMainWindow->geometry();
	QRect screenRect = qApp->desktop()->availableGeometry(qApp->desktop()->screenNumber(windowRect.topLeft()));
	if (!screenRect.isEmpty() && !screenRect.adjusted(10,10,-10,-10).intersects(windowRect))
	{
		if (windowRect.right() <= screenRect.left())
			windowRect.moveLeft(screenRect.left());
		else if (windowRect.left() >= screenRect.right())
			windowRect.moveRight(screenRect.right());
		if (windowRect.top() >= screenRect.bottom())
			windowRect.moveBottom(screenRect.bottom());
		else if (windowRect.bottom() <= screenRect.top())
			windowRect.moveTop(screenRect.top());
		FMainWindow->move(windowRect.topLeft());
	}
}

bool MainWindowPlugin::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AWatched==FMainWindow && AEvent->type()==QEvent::ActivationChange)
		FActivationChanged = QTime::currentTime();
	return QObject::eventFilter(AWatched,AEvent);
}

void MainWindowPlugin::onOptionsOpened()
{
	FMainWindow->resize(Options::node(OPV_MAINWINDOW_SIZE).value().toSize());
	FMainWindow->move(Options::node(OPV_MAINWINDOW_POSITION).value().toPoint());
	if (Options::node(OPV_MAINWINDOW_SHOW).value().toBool())
		showMainWindow();
	updateTitle();
}

void MainWindowPlugin::onOptionsClosed()
{
	Options::node(OPV_MAINWINDOW_SHOW).setValue(FMainWindow->isVisible());
	Options::node(OPV_MAINWINDOW_SIZE).setValue(FMainWindow->size());
	Options::node(OPV_MAINWINDOW_POSITION).setValue(FMainWindow->pos());
	updateTitle();
	FMainWindow->close();
}

void MainWindowPlugin::onProfileRenamed(const QString &AProfile, const QString &ANewName)
{
	Q_UNUSED(AProfile);
	Q_UNUSED(ANewName);
	updateTitle();
}

void MainWindowPlugin::onTrayNotifyActivated(int ANotifyId, QSystemTrayIcon::ActivationReason AReason)
{
	if (ANotifyId==0 && AReason==QSystemTrayIcon::Trigger)
	{
		if (FMainWindow->isActive() || qAbs(FActivationChanged.msecsTo(QTime::currentTime())) < qApp->doubleClickInterval())
			FMainWindow->close();
		else
			showMainWindow();
	}
}

void MainWindowPlugin::onShowMainWindowByAction(bool)
{
	showMainWindow();
}

Q_EXPORT_PLUGIN2(plg_mainwindow, MainWindowPlugin)
