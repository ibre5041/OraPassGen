#pragma once

#include "ui_configui.h"

#include <QSettings>

class ConfigGui : public QDialog, public Ui_Config
{
	Q_OBJECT

public:
	ConfigGui(QWidget * parent = 0, Qt::WindowFlags f = 0);
	void setVisible(bool visible);
	bool eventFilter(QObject *obj, QEvent *event);
public slots:
	void showNormal();
	void hide();
	void close();
protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void configSourceRadioButtonClicked();
	void browseFileClicked();
signals:
	void newServerList(QString);
private:
	QSettings settings;
	QString defaultConfigPath;
	QString defaultConfigDir;
	QString configFilePath;
	QString configURLPath;
	QString appDir;
};
