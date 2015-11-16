#include <openssl/bn.h>
#include <openssl/md5.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <ostream>

#include "common.h"
#include "crypto.h"
#include "dbutils.h"
#include "dbpassgui.h"

#include <QtGui>
#include <QtWidgets>

using namespace std;

int main(int argc, char *argv[])
{
	Q_INIT_RESOURCE(systray);

	QApplication app(argc, argv);

	if (QCoreApplication::organizationName().isEmpty())
		QCoreApplication::setOrganizationName("OraDbPass");
	if (QCoreApplication::organizationDomain().isEmpty())
		QCoreApplication::setOrganizationDomain("OraDbPass");
	if (QCoreApplication::applicationName().isEmpty())
		QCoreApplication::setApplicationName("OraDbPass");

	if (!QSystemTrayIcon::isSystemTrayAvailable()) {
		QMessageBox::critical(0, QObject::tr("Systray"), QObject::tr("I couldn't detect any system tray on this system."));
		return 1;
	}
	QApplication::setQuitOnLastWindowClosed(false);

	DbPassGui dbpassgui;
	dbpassgui.show();
	return app.exec();	
}

DbPassGui::DbPassGui(QWidget * parent)
	: QDialog(parent)
	, icon(new QIcon(":/resources/data-storage4.svg"))
{
	setupUi(this);

	QRegExp number("[0-9]+");
	dbidEdit->setValidator(new QRegExpValidator(number, dbidEdit));

	QSettings s;
	s.beginGroup("DbPassGui");
	restoreGeometry(s.value("geometry", QByteArray()).toByteArray());
	s.endGroup();

	createActions();
	createTrayIcon();
	
	QFile nfile(":/resources/n.txt");
	nfile.open(QIODevice::ReadOnly);
	QTextStream in(&nfile);
	n = in.readAll();

	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
	connect(generateButton, SIGNAL(pressed()), this, SLOT(generatePressed()));
	setWindowIcon(*icon);
	setWindowTitle(tr("DbPass"));
}

void DbPassGui::setVisible(bool visible)
{
	minimizeAction->setEnabled(visible);
	maximizeAction->setEnabled(!isMaximized());
	restoreAction->setEnabled(isMaximized() || !visible);
	QDialog::setVisible(visible);
}

void DbPassGui::showNormal()
{
	QSettings s;
	s.beginGroup("DbPassGui");
	restoreGeometry(s.value("geometry", QByteArray()).toByteArray());
	s.endGroup();
	QDialog::showNormal();
}

void DbPassGui::hide()
{
	QSettings s;
	s.beginGroup("DbPassGui");
	s.setValue("geometry", saveGeometry());
	s.endGroup();
	QDialog::hide();
}

void DbPassGui::closeEvent(QCloseEvent *event)
{
	QSettings s;
	s.beginGroup("DbPassGui");
	bool notificationShown = s.value("notification", false).toBool();
	if (trayIcon->isVisible()) {
		if (!notificationShown) {
			QMessageBox::information(this, tr("Systray"),
				tr("The program will keep running in the "
					"system tray. To terminate the program, "
					"choose <b>Quit</b> in the context menu "
					"of the system tray entry."));
			s.setValue("notification", true);
		}
		hide();
		event->ignore();
	}
	s.endGroup();
}

void DbPassGui::iconActivated(QSystemTrayIcon::ActivationReason reason)
{
	switch (reason) {
	case QSystemTrayIcon::Unknown:
	case QSystemTrayIcon::Trigger:
	case QSystemTrayIcon::DoubleClick:
	case QSystemTrayIcon::MiddleClick:
		if (isVisible())
			hide();
		else
			showNormal();
		break;
	default:
		;
	}
}

void DbPassGui::generatePressed()
{
	QString dbid = dbidEdit->text();
	QString pass = passwordEdit->text();
	std::string utf8_dbid = dbid.toStdString();
	std::string utf8_pass = pass.toStdString();
	std::string gen = genpasswd(utf8_dbid, utf8_pass, n.toStdString());

	QClipboard *p_Clipboard = QApplication::clipboard();
	p_Clipboard->setText(gen.c_str());
	passphraseEdit->setText(gen.c_str());	
}

void DbPassGui::createActions()
{
	minimizeAction = new QAction(tr("Mi&nimize"), this);
	connect(minimizeAction, SIGNAL(triggered()), this, SLOT(hide()));

	maximizeAction = new QAction(tr("Ma&ximize"), this);
	connect(maximizeAction, SIGNAL(triggered()), this, SLOT(showMaximized()));

	restoreAction = new QAction(tr("&Restore"), this);
	connect(restoreAction, SIGNAL(triggered()), this, SLOT(showNormal()));

	quitAction = new QAction(tr("&Quit"), this);
	connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
}

void DbPassGui::createTrayIcon()
{
	trayIconMenu = new QMenu(this);
	trayIconMenu->addAction(minimizeAction);
	trayIconMenu->addAction(maximizeAction);
	trayIconMenu->addAction(restoreAction);
	trayIconMenu->addSeparator();
	trayIconMenu->addAction(quitAction);

	trayIcon = new QSystemTrayIcon(this);
	trayIcon->setContextMenu(trayIconMenu);

	trayIcon->setToolTip("DBA Pass");
	trayIcon->setIcon(*icon);
	trayIcon->show();
}
