#include "configgui.h"

#include <ostream>
#include <iostream>

#include <QCloseEvent>
#include <QDialogButtonBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QUrl>

ConfigGui::ConfigGui(QWidget * parent, Qt::WindowFlags f)
	: QDialog(parent, f)
	, defaultConfigDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation))
	, httpRequestAborted(false)
{
	setupUi(this);

	manager = new QNetworkAccessManager(this);
	
	QObject::connect(fileRadioButton, SIGNAL(clicked()), this, SLOT(configSourceRadioButtonClicked()));
	QObject::connect(URLRadioButton, SIGNAL(clicked()), this, SLOT(configSourceRadioButtonClicked()));
	QObject::connect(browsePushButton, SIGNAL(clicked()), this, SLOT(browseFileClicked()));
	QObject::connect(URLLineEdit, SIGNAL(textChanged(const QString &)), this, SLOT(URLChanged(const QString&)));
	QObject::connect(updateButton, SIGNAL(clicked()), this, SLOT(updateClicked()));
	QPushButton* applyButton = buttonBox->button(QDialogButtonBox::Apply);
	QObject::connect(applyButton, SIGNAL(clicked()), this, SLOT(apply()));
	QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(hide()));

	settings.beginGroup("DbPassGui");
	configFilePath = settings.value("filepath").toString();
	configURLPath = settings.value("urlpath").toString();
	settings.endGroup();

	QFileInfo file(configFilePath);
	if (file.isReadable())
		fileLineEdit->setText(configFilePath);
	QUrl url(configURLPath);
	if (url.isValid())
		URLLineEdit->setText(configURLPath);

	fileRadioButton->setChecked(!url.isValid());
	fileLineEdit->setEnabled(!url.isValid());
	browsePushButton->setEnabled(!url.isValid());
	URLRadioButton->setChecked(url.isValid());
	URLLineEdit->setEnabled(url.isValid());
	updateButton->setEnabled(url.isValid());
}

void ConfigGui::showNormal()
{
	settings.beginGroup("DbPassGui");
	restoreGeometry(settings.value("configgeometry", QByteArray()).toByteArray());
	settings.endGroup();

	QDialog::showNormal();
}

void ConfigGui::hide()
{
	settings.beginGroup("DbPassGui");
	settings.setValue("configgeometry", saveGeometry());
	settings.endGroup();
	QDialog::hide();
}

void ConfigGui::closeEvent(QCloseEvent *event)
{
	hide();
	event->ignore();
}

void ConfigGui::URLChanged(const QString& text)
{
	QUrl url(text);
	updateButton->setEnabled(url.isValid());
}

void ConfigGui::configSourceRadioButtonClicked()
{
	fileLineEdit->setEnabled(fileRadioButton->isChecked());
	browsePushButton->setEnabled(fileRadioButton->isChecked());
	URLLineEdit->setEnabled(URLRadioButton->isChecked());
	progressBar->setEnabled(URLRadioButton->isChecked());
	progressBar->setVisible(URLRadioButton->isChecked());
	updateButton->setEnabled(URLRadioButton->isChecked() && URLLineEdit->text().startsWith("http"));
}

void ConfigGui::browseFileClicked()
{
	QString filter("*.xml");
	QString retval;
	QFileInfo fi(configFilePath);
	if (!configFilePath.isEmpty() && fi.exists())
		retval = QFileDialog::getOpenFileName(this, "Open File", fi.absoluteDir().absolutePath(), filter);
	else
		retval = QFileDialog::getOpenFileName(this, "Open File", defaultConfigDir, filter);

	if (!retval.isEmpty())
		fileLineEdit->setText(retval);
		
}

void ConfigGui::updateClicked()
{
	QDir configDir(defaultConfigDir);
	if (!configDir.mkpath(configDir.absolutePath()))
		return;
	if (!QDir::setCurrent(configDir.path()))
		return;
	
	url = (URLLineEdit->text());

	QFileInfo fileInfo(url.path());
	QString fileName = fileInfo.fileName();

	if (fileName.isEmpty())
		fileName = "servers.xml";
	
	if (QFile::exists(fileName)) {
		if (QMessageBox::question(this, tr("HTTP"),
					  tr("There already exists a file called %1 in "
					     "the current directory. Overwrite?").arg(fileName),
					  QMessageBox::Yes|QMessageBox::No, QMessageBox::No)
		    == QMessageBox::No)
			return;
		QFile::remove(fileName);
	}

	file = new QFile(fileName);
	if (!file->open(QIODevice::WriteOnly)) {
		QMessageBox::information(this, tr("HTTP"),
					 tr("Unable to save the file %1: %2.")
					 .arg(fileName).arg(file->errorString()));
		delete file;
		file = 0;
		return;
	}

	// used for progressDialog
	// This will be set true when canceled from progress dialog
	httpRequestAborted = false;

	// download button disabled after requesting download
	updateButton->setEnabled(false);

	startRequest(url);
}

void ConfigGui::startRequest(QUrl url)
{
	// emits the readyRead() signal whenever new data arrives.
	reply = manager->get(QNetworkRequest(url));

	// Whenever more data is received from the network, this readyRead() signal is emitted
	QObject::connect(reply, SIGNAL(readyRead()), this, SLOT(httpReadyRead()));

	// Also, downloadProgress() signal is emitted when data is received
	connect(reply, SIGNAL(downloadProgress(qint64,qint64)),
		this, SLOT(updateDownloadProgress(qint64,qint64)));

	// This signal is emitted when the reply has finished processing.
	QObject::connect(reply, SIGNAL(finished()), this, SLOT(httpDownloadFinished()));
}

void ConfigGui::httpReadyRead()
{
	// signal of the QNetworkReply
	if (file)
		file->write(reply->readAll());	
}

void ConfigGui::httpDownloadFinished()
{
	// when canceled
	if (httpRequestAborted) {
		if (file) {
			file->close();
			file->remove();
			delete file;
			file = 0;
		}
		reply->deleteLater();
		//progressBar->hide();
		return;
	}

	// download finished normally
	//progressBar->hide();
	file->flush();
	file->close();

	// get redirection url
	QVariant redirectionTarget = reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
	if (reply->error()) {
		file->remove();
		QMessageBox::information(this, tr("HTTP"),
					 tr("Download failed: %1.")
					 .arg(reply->errorString()));
		updateButton->setEnabled(true);
	} else if (!redirectionTarget.isNull()) {
		QUrl newUrl = url.resolved(redirectionTarget.toUrl());
		if (QMessageBox::question(this, tr("HTTP"),
					  tr("Redirect to %1 ?").arg(newUrl.toString()),
					  QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
			url = newUrl;
			reply->deleteLater();
			file->open(QIODevice::WriteOnly);
			file->resize(0);
			startRequest(url);
			return;
		}
	} else {
		//QString fileName = QFileInfo(QUrl(URLLineEdit->text()).path()).fileName();
		//ui->statusLabel->setText(tr("Downloaded %1 to %2.").arg(fileName).arg(QDir::currentPath()));
		updateButton->setEnabled(true);
		fileLineEdit->setText(QFileInfo(*file).absoluteFilePath());
	}

	reply->deleteLater();
	reply = 0;
	delete file;
	file = 0;
}

void ConfigGui::updateDownloadProgress(qint64 bytesRead, qint64 totalBytes)
{
	if (httpRequestAborted)
		return;

	progressBar->setMaximum(totalBytes);
	progressBar->setValue(bytesRead);
}

void ConfigGui::apply()
{
	settings.beginGroup("DbPassGui");
	QUrl url(URLLineEdit->text());
	if (url.isValid())
		settings.setValue("urlpath", URLLineEdit->text());

	QString f = fileLineEdit->text();
	QFileInfo file(fileLineEdit->text());
	if (file.isReadable())
	{
		settings.setValue("filepath", fileLineEdit->text());
		emit newServerList(file.absoluteFilePath());
	}
	settings.endGroup();
	hide();
}
