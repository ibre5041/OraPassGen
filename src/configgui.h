#pragma once

#include "ui_configui.h"

#include <QSettings>
#include <QFileInfo>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QFile;

class ConfigGui : public QDialog, public Ui_Config
{
	Q_OBJECT

public:
	ConfigGui(QWidget * parent = 0, Qt::WindowFlags f = 0);

public slots:
	void showNormal();
	void hide();

protected:
	void closeEvent(QCloseEvent *event);
	void startRequest(QUrl url);

private slots:
	void configSourceRadioButtonClicked();
	void browseFileClicked();
	void updateClicked();
	void URLChanged(const QString&);

	void httpReadyRead();
	void updateDownloadProgress(qint64, qint64);
	void httpDownloadFinished();

	void apply();
signals:
	void newServerList(QString);

private:
	QSettings settings;
	const QString defaultConfigDir;
	QString configFilePath;
	QString configURLPath;
	QUrl url;
	QNetworkAccessManager *manager;
	QNetworkReply *reply;
	QFile *file;
	bool httpRequestAborted;
};
