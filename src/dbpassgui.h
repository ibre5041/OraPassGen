#pragma once

#include <QSystemTrayIcon>
#include <QDialog>
#include <QSettings>

class QAction;
class QLabel;
class QLineEdit;
class QMenu;
class QPushButton;

#include "ui_dbpassguiui.h"

class DbPassGui : public QDialog, public Ui_DbPassGui
{
	Q_OBJECT

public:
	DbPassGui(QWidget * parent = 0);
	void setVisible(bool visible);
	bool eventFilter(QObject *obj, QEvent *event);
public slots:
	void showNormal();
	void hide();

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void iconActivated(QSystemTrayIcon::ActivationReason reason);
	void generatePressed();
	void flipCheckBoxA(int);
	void flipCheckBoxB(int);
	
	void hostnameEntered();
	void sidEntered();
	void hostnameCleared(const QString &);
	void sidCleared(const QString &);
	void clearAll();
private:
	void createActions();
	void createTrayIcon();
	void setDbid(QString const&);

	QAction *minimizeAction;
	QAction *maximizeAction;
	QAction *restoreAction;
	QAction *quitAction;

	QIcon *icon;
	QSystemTrayIcon *trayIcon;
	QMenu *trayIconMenu;

	QSettings settings;
	QString n;
	QMap<QString, QMap<QString, QString> > hostSidToDbid, sidHostToDbid;
};
