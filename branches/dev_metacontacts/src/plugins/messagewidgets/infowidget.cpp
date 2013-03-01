#include "infowidget.h"

#include <QIcon>
#include <QMovie>
#include <QTimer>
#include <QToolTip>
#include <QHelpEvent>
#include <QHBoxLayout>
#include <QContextMenuEvent>
#include <utils/textmanager.h>

#define ADR_STREAM_JID           Action::DR_StreamJid
#define ADR_CONTACT_JID          Action::DR_Parametr1

InfoWidget::InfoWidget(IMessageWidgets *AMessageWidgets, IMessageWindow *AWindow, QWidget *AParent) : QWidget(AParent)
{
	ui.setupUi(this);

	FWindow = AWindow;
	FMessageWidgets = AMessageWidgets;

	FAddressMenuVisible = false;
	ui.lblAvatar->setVisible(false);
	ui.lblIcon->setVisible(false);

	QToolBar *toolBar = new QToolBar;
	toolBar->setMovable(false);
	toolBar->setFloatable(false);
	toolBar->setIconSize(QSize(16,16));
	toolBar->layout()->setMargin(0);
	toolBar->setStyleSheet("QToolBar { border: none; }");
	toolBar->setSizePolicy(QSizePolicy::Preferred,QSizePolicy::Preferred);

	FInfoToolBar = new ToolBarChanger(toolBar);
	FInfoToolBar->setSeparatorsVisible(false);
	toolBar->installEventFilter(this);

	ui.wdtInfoToolBar->setLayout(new QHBoxLayout);
	ui.wdtInfoToolBar->layout()->setMargin(0);
	ui.wdtInfoToolBar->layout()->addWidget(toolBar);

	FAddressMenu = new Menu(this);
	FAddressMenu->menuAction()->setToolTip(tr("Contact address"));
	connect(FAddressMenu,SIGNAL(aboutToShow()),SLOT(onAddressMenuAboutToShow()));

	initialize();
}

InfoWidget::~InfoWidget()
{

}

IMessageWindow *InfoWidget::messageWindow() const
{
	return FWindow;
}

ToolBarChanger *InfoWidget::toolBarChanger() const
{
	return FInfoToolBar;
}

Menu *InfoWidget::addressMenu() const
{
	return FAddressMenu;
}

bool InfoWidget::isAddressMenuVisible() const
{
	return FAddressMenuVisible;
}

void InfoWidget::setAddressMenuVisible(bool AVisible)
{
	if (FAddressMenuVisible != AVisible)
	{
		FAddressMenuVisible = AVisible;
		if (AVisible)
		{
			QToolButton *button = FInfoToolBar->insertAction(FAddressMenu->menuAction());
			button->setPopupMode(QToolButton::InstantPopup);
			button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		}
		else
		{
			FInfoToolBar->removeItem(FInfoToolBar->actionHandle(FAddressMenu->menuAction()));
		}
		emit addressMenuVisibleChanged(AVisible);
	}
}

QVariant InfoWidget::fieldValue(int AField) const
{
	return FFieldValues.value(AField);
}

void InfoWidget::setFieldValue(int AField, const QVariant &AValue)
{
	if (FFieldValues.value(AField) != AValue)
	{
		if (AValue.isNull())
			FFieldValues.remove(AField);
		else
			FFieldValues.insert(AField,AValue);
		updateFieldView(AField);
		emit fieldValueChanged(AField);
	}
}

void InfoWidget::initialize()
{
	FAvatars = NULL;

	IPlugin *plugin = FMessageWidgets->pluginManager()->pluginInterface("IAvatars").value(0);
	if (plugin)
	{
		FAvatars = qobject_cast<IAvatars *>(plugin->instance());
	}
}

void InfoWidget::updateFieldView(int AField)
{
	switch (AField)
	{
	case IMessageInfoWidget::Avatar:
		{
			bool avatarVisible = false;
			if (ui.lblAvatar->movie())
			{
				ui.lblAvatar->movie()->deleteLater();
				ui.lblAvatar->setMovie(NULL);
			}

			QVariant avatar = fieldValue(IMessageInfoWidget::Avatar);
			switch (avatar.type())
			{
			case QVariant::String:
				if (FAvatars)
				{
					QString avatarFile = FAvatars->avatarFileName(avatar.toString());
					if (!avatarFile.isEmpty())
					{
						avatarVisible = true;
						QMovie *movie = new QMovie(avatarFile,QByteArray(),ui.lblAvatar);
						QSize size = QImageReader(avatarFile).size();
						size.scale(ui.lblAvatar->maximumSize(),Qt::KeepAspectRatio);
						movie->setScaledSize(size);
						ui.lblAvatar->setMovie(movie);
						movie->start();
					}
				}
				break;
			case QVariant::Pixmap:
				avatarVisible = true;
				ui.lblAvatar->setPixmap(avatar.value<QPixmap>());
				break;
			case QVariant::Image:
				avatarVisible = true;
				ui.lblAvatar->setPixmap(QPixmap::fromImage(avatar.value<QImage>()));
				break;
			default:
				ui.lblAvatar->clear();
			}

			ui.lblAvatar->setVisible(avatarVisible);
			break;
		}
	case IMessageInfoWidget::Name:
	case IMessageInfoWidget::StatusText:
		{
			QString info;
			QString name = fieldValue(IMessageInfoWidget::Name).toString();
			QString status = fieldValue(IMessageInfoWidget::StatusText).toString();
			if (!name.isEmpty() && !status.isEmpty())
				info = QString("<big><b>%1</b></big> - %2").arg(Qt::escape(name),Qt::escape(status));
			else if (!name.isEmpty())
				info = QString("<big><b>%1</b></big>").arg(Qt::escape(name));
			else if (!status.isEmpty())
				info = Qt::escape(status);
			ui.lblInfo->setText(info);
			break;
		}
	case IMessageInfoWidget::StatusIcon:
		{
			bool iconVisible = false;
			QVariant icon = fieldValue(IMessageInfoWidget::StatusIcon);

			switch (icon.type())
			{
			case QVariant::Icon:
				{
					iconVisible = true;
					QIcon iconIcon = icon.value<QIcon>();
					ui.lblIcon->setPixmap(iconIcon.pixmap(iconIcon.actualSize(ui.lblIcon->maximumSize())));
				}
				break;
			case QVariant::Pixmap:
				iconVisible = true;
				ui.lblIcon->setPixmap(icon.value<QPixmap>());
			case QVariant::Image:
				iconVisible = true;
				ui.lblIcon->setPixmap(QPixmap::fromImage(icon.value<QImage>()));
			default:
				ui.lblIcon->clear();
			}

			ui.lblIcon->setVisible(iconVisible);
			break;
		}
	}
}

bool InfoWidget::event(QEvent *AEvent)
{
	if (AEvent->type() == QEvent::ToolTip)
	{
		QHelpEvent *helpEvent = static_cast<QHelpEvent *>(AEvent);
		if (helpEvent)
		{
			QMap<int,QString> toolTipsMap;
			emit toolTipsRequested(toolTipsMap);
			if (!toolTipsMap.isEmpty())
			{
				QString tooltip = QString("<span>%1</span>").arg(QStringList(toolTipsMap.values()).join("<p/><nbsp>"));
				QToolTip::showText(helpEvent->globalPos(),tooltip,this);
			}
		}
		return true;
	}
	return QWidget::event(AEvent);
}

void InfoWidget::contextMenuEvent(QContextMenuEvent *AEvent)
{
	Menu *menu = new Menu(this);
	menu->setAttribute(Qt::WA_DeleteOnClose,true);

	emit contextMenuRequested(menu);

	if (!menu->isEmpty())
		menu->popup(AEvent->globalPos());
	else
		delete menu;
}

bool InfoWidget::eventFilter(QObject *AWatched, QEvent *AEvent)
{
	if (AWatched == FInfoToolBar->toolBar())
	{
		if (AEvent->type() == QEvent::LayoutRequest)
			QTimer::singleShot(0,this,SLOT(onUpdateInfoToolBarMaxWidth()));
	}
	return QWidget::eventFilter(AWatched,AEvent);
}

void InfoWidget::onAddressMenuAboutToShow()
{
	FAddressMenu->clear();
	emit addressMenuRequested(FAddressMenu);
}

void InfoWidget::onUpdateInfoToolBarMaxWidth()
{
	int widgetWidth = 0;
	int visibleItemsCount = 0;
	for (int itemIndex=0; visibleItemsCount<2 && itemIndex<FInfoToolBar->toolBar()->layout()->count(); itemIndex++)
	{
		QWidget *widget = FInfoToolBar->toolBar()->layout()->itemAt(itemIndex)->widget();
		if (widget && widget->isVisible())
		{
			visibleItemsCount++;
			widgetWidth = widget->sizeHint().width();
		}
	}
	FInfoToolBar->toolBar()->setMaximumWidth(visibleItemsCount==1 ? widgetWidth : QWIDGETSIZE_MAX);
}
