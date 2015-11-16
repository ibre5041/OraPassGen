#pragma once

#include <QSystemTrayIcon>
#include <QDialog>

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

public slots:
	void showNormal();
	void hide();

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void iconActivated(QSystemTrayIcon::ActivationReason reason);
	void generatePressed();

private:
	void createActions();
	void createTrayIcon();

	QAction *minimizeAction;
	QAction *maximizeAction;
	QAction *restoreAction;
	QAction *quitAction;

	QIcon *icon;
	QSystemTrayIcon *trayIcon;
	QMenu *trayIconMenu;

	QString n;
};
