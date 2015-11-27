#pragma once

#include "ui_configui.h"

#include <QSettings>

class ConfigGui : public QDialog, public Ui_Config
{
	Q_OBJECT

public:
	ConfigGui(QWidget * parent = 0);
	void setVisible(bool visible);
	bool eventFilter(QObject *obj, QEvent *event);
public slots:
	void showNormal();
	void hide();
protected:
	void closeEvent(QCloseEvent *event);

private slots:
private:
	QSettings settings;
};
