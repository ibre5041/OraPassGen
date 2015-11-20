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

#include "completelineedit.h"

#include <Qt>
#include <QtGlobal>
#include <QtGui>
#include <QtWidgets>
#include <QDomDocument>

#ifdef Q_OS_WIN32
#include <QtPlugin>
Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
Q_IMPORT_PLUGIN(QSvgIconPlugin)
Q_IMPORT_PLUGIN(QSvgPlugin)
#endif

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

	QFont font(generatedPasswordEdit->font());
	QString char16("12345678901234567890123456789012");
	QFontMetrics fm(font);
	int width = fm.width(char16);
	passphraseEdit->setMinimumWidth(width+10);
	generatedPasswordEdit->setMinimumWidth(width+10);
	generatedPasswordEdit->setStyleSheet("QLineEdit[readOnly=\"true\"] {"
		"color: #808080;"
		"background-color: #F0F0F0;"
		"border: 1px solid #B0B0B0;"
		"border-radius: 2px;}");

	settings.beginGroup("DbPassGui");
	restoreGeometry(settings.value("geometry", QByteArray()).toByteArray());
	showPassphraseCheckbox->setCheckState(settings.value("CheckBoxAState").toBool() ? Qt::Checked : Qt::Unchecked);
	showGeneratedPasswordCheckbox->setCheckState(settings.value("CheckBoxBState").toBool() ? Qt::Checked : Qt::Unchecked);
	settings.endGroup();

	createActions();
	createTrayIcon();
	
	QFile nfile(":/resources/n.txt");
	nfile.open(QIODevice::ReadOnly);
	QTextStream in(&nfile);
	n = in.readAll();

	QFile xmlfile(":/resources/databases.xml");
	xmlfile.open(QIODevice::ReadOnly);
	QDomDocument doc;
	doc.setContent(&xmlfile);
	QDomElement docElem = doc.documentElement();
	QDomNode node = docElem.firstChild();
	while (!node.isNull())
	{
		QString hostname = node.toElement().attribute("value");
		printf("%s\n", hostname.toStdString().c_str());
		QDomNode child = node.firstChild();
		while (!child.isNull())
		{
			QString sid = child.toElement().attribute("value");
			QString dbid = child.toElement().text();
			printf(" %s\t%s\n", sid.toStdString().c_str(), dbid.toStdString().c_str());
			
			if (hostSidToDbid.contains(hostname))
			{
				hostSidToDbid[hostname][sid] = dbid;
			} else {
				hostSidToDbid.insert(hostname, QMap<QString, QString>());
				hostSidToDbid[hostname][sid] = dbid;
			}

			if (sidHostToDbid.contains(sid))
			{
				sidHostToDbid[sid][hostname] = dbid;
			}
			else {
				sidHostToDbid.insert(sid, QMap<QString, QString>());
				sidHostToDbid[sid][hostname] = dbid;
			}
			child = child.nextSibling();
		}
		//do something
		node = node.nextSibling();
	}
	hostnameEdit->setWords(hostSidToDbid.keys());
	sidEdit->setWords(sidHostToDbid.keys());

	dbidEdit->installEventFilter(this);

	connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
		this, SLOT(iconActivated(QSystemTrayIcon::ActivationReason)));
	connect(generateButton, SIGNAL(pressed()), this, SLOT(generatePressed()));
	setWindowIcon(*icon);
	setWindowTitle(tr("DbPass"));

	setWindowFlags(Qt::WindowStaysOnTopHint);
#if 0
	setWindowFlags(Qt::Widget | Qt::FramelessWindowHint);
	setParent(0); // Create TopLevel-Widget
	setAttribute(Qt::WA_NoSystemBackground, true);
	setAttribute(Qt::WA_TranslucentBackground, true);
	setAttribute(Qt::WA_PaintOnScreen);
#endif
}

void DbPassGui::setVisible(bool visible)
{
	minimizeAction->setEnabled(visible);
	maximizeAction->setEnabled(!isMaximized());
	restoreAction->setEnabled(isMaximized() || !visible);
	QDialog::setVisible(visible);
}

bool DbPassGui::eventFilter(QObject *obj, QEvent *event)
{
	if (obj == dbidEdit)
	{
		if (event->type() == QEvent::FocusIn)
		{
			QString dbid = hostSidToDbid.value(hostnameEdit->text()).value(sidEdit->text());
			if (!dbid.isEmpty())
				dbidEdit->setText(dbid);
		}
	}
	// pass the event on to the parent class
	return QDialog::eventFilter(obj, event);
}

void DbPassGui::showNormal()
{
	settings.beginGroup("DbPassGui");
	restoreGeometry(settings.value("geometry", QByteArray()).toByteArray());
	settings.endGroup();
	QDialog::showNormal();
}

void DbPassGui::hide()
{	
	settings.beginGroup("DbPassGui");
	settings.setValue("geometry", saveGeometry());
	settings.endGroup();
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
	QString pass = passphraseEdit->text();
	std::string utf8_dbid = dbid.toStdString();
	std::string utf8_pass = pass.toStdString();
	std::string utf8_n    = n.toStdString();
	std::string gen = genpasswd(utf8_dbid, utf8_pass, utf8_n);

	QClipboard *p_Clipboard = QApplication::clipboard();
	p_Clipboard->setText(gen.c_str());
	generatedPasswordEdit->setText(gen.c_str());
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

	connect(showPassphraseCheckbox, SIGNAL(stateChanged(int)), this, SLOT(flipCheckBoxA(int)));
	connect(showGeneratedPasswordCheckbox, SIGNAL(stateChanged(int)), this, SLOT(flipCheckBoxB(int)));

	connect(hostnameEdit, SIGNAL(returnPressed()), this, SLOT(hostnameEntered()));
	connect(sidEdit, SIGNAL(returnPressed()), this, SLOT(sidEntered()));

	connect(hostnameEdit, SIGNAL(textChanged(const QString&)), this, SLOT(hostnameCleared(const QString&)));
	connect(sidEdit, SIGNAL(textChanged(const QString&)), this, SLOT(sidCleared(const QString&)));

	flipCheckBoxA(showPassphraseCheckbox->checkState());
	flipCheckBoxB(showGeneratedPasswordCheckbox->checkState());
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

void DbPassGui::flipCheckBoxA(int state)
{
	if (state == Qt::Checked)
		passphraseEdit->setEchoMode(QLineEdit::Normal);
	else
		passphraseEdit->setEchoMode(QLineEdit::Password);

	settings.beginGroup("DbPassGui");
	settings.setValue("CheckBoxAState", state == Qt::Checked);
	settings.endGroup();
}

void DbPassGui::flipCheckBoxB(int state)
{
	if (state == Qt::Checked)
		generatedPasswordEdit->setEchoMode(QLineEdit::Normal);
	else
		generatedPasswordEdit->setEchoMode(QLineEdit::Password);

	settings.beginGroup("DbPassGui");
	settings.setValue("CheckBoxBState", state == Qt::Checked);
	settings.endGroup();
}

void DbPassGui::hostnameEntered()
{
	QString hostname = hostnameEdit->text();
	QStringList sids = hostSidToDbid.value(hostname).keys();
	if (sids.size() == 1)
	{
		QString sid = sids.at(0);
		sidEdit->setWords(QStringList());
		sidEdit->setText(sid);
		setDbid(hostSidToDbid.value(hostname).value(sid));
	} else {
		sidEdit->setWords(sids);
		focusNextChild();
	}
}

void DbPassGui::sidEntered()
{
	QString sid = sidEdit->text();
	QStringList hostnames = sidHostToDbid.value(sid).keys();
	if (hostnames.size() == 1 && hostnameEdit->text().isEmpty())
	{
		QString hostname = hostnames.at(0);
		hostnameEdit->setWords(QStringList());
		hostnameEdit->setText(hostname);
		setDbid(sidHostToDbid.value(sid).value(hostname));
	}
	else {
		hostnameEdit->setWords(hostnames);
	}

	if (hostnameEdit->text().isEmpty()) {
		focusPreviousChild();
	} else {
		passphraseEdit->setFocus();
		setDbid(sidHostToDbid.value(sid).value(hostnameEdit->text()));
	}
}

void DbPassGui::clearAll()
{
	hostnameEdit->clear();
	sidEdit->clear();
	dbidEdit->clear();
	passphraseEdit->clear();
	generatedPasswordEdit->clear();
}

void DbPassGui::hostnameCleared(const QString &hostname)
{
	if (!hostname.isEmpty())
		return;
	sidEdit->setWords(sidHostToDbid.keys());
}

void DbPassGui::sidCleared(const QString &sid)
{
	if (!sid.isEmpty())
		return;
	hostnameEdit->setWords(hostSidToDbid.keys());
}

void DbPassGui::setDbid(QString const& dbid)
{
	dbidEdit->setText(dbid);
	QTimer::singleShot(30000, this, SLOT(clearAll()));
}
