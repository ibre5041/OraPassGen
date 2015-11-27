#include "configgui.h"

#include <QStandardPaths>
#include <QFileInfo>
#include <QFileDialog>

ConfigGui::ConfigGui(QWidget * parent, Qt::WindowFlags f)
	: QDialog(parent, f)
{
	setupUi(this);
	progressBar->hide();

	QString defaultConfigPath = QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, "servers.xml");
	QString defaultConfigDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	settings.beginGroup("DbPassGui");
	QString configFilePath = settings.value("filepath").toString();
	QString configURLPath = settings.value("urlpath").toString();
	settings.endGroup();

	QObject::connect(fileRadioButton, SIGNAL(clicked()), this, SLOT(configSourceRadioButtonClicked()));
	QObject::connect(URLRadioButton, SIGNAL(clicked()), this, SLOT(configSourceRadioButtonClicked()));
	QObject::connect(browsePushButton, SIGNAL(clicked()), this, SLOT(browseFileClicked()));

	defaultConfigPath = QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, "servers.xml");
	defaultConfigDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	settings.beginGroup("DbPassGui");
	configFilePath = settings.value("filepath").toString();
	configURLPath = settings.value("urlpath").toString();
	settings.endGroup();
	appDir = qApp->applicationDirPath();
	std::cout << "defaultConfigPath: " << defaultConfigPath.toStdString() << std::endl
		<< "defaultConfigDir: " << defaultConfigDir.toStdString() << std::endl
		<< "configFilePath: " << configFilePath.toStdString() << std::endl
		<< "configURLPath: " << configURLPath.toStdString() << std::endl
		<< "appDir: " << appDir.toStdString() << std::endl;
}

void ConfigGui::setVisible(bool visible)
{
	QDialog::setVisible(visible);
}

bool ConfigGui::eventFilter(QObject *obj, QEvent *event)
{
	// if (obj == dbidEdit)
	// {
	// 	if (event->type() == QEvent::FocusIn)
	// 	{
	// 		QString dbid = hostSidToDbid.value(hostnameEdit->text()).value(sidEdit->text());
	// 		if (!dbid.isEmpty())
	// 			dbidEdit->setText(dbid);
	// 	}
	// }
	// pass the event on to the parent class
	return QDialog::eventFilter(obj, event);
}

#include <ostream>
#include <iostream>

void ConfigGui::showNormal()
{
	//settings.beginGroup("ConfigGui");
	//restoreGeometry(settings.value("configgeometry", QByteArray()).toByteArray());
	//settings.endGroup();

	defaultConfigPath = QStandardPaths::locate(QStandardPaths::AppLocalDataLocation, "servers.xml");
	defaultConfigDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	settings.beginGroup("DbPassGui");
	configFilePath = settings.value("filepath").toString();
	configURLPath = settings.value("urlpath").toString();
	settings.endGroup();
	appDir = qApp->applicationDirPath();
	std::cout << "defaultConfigPath: " << defaultConfigPath.toStdString() << std::endl
		<< "defaultConfigDir: " << defaultConfigDir.toStdString() << std::endl
		<< "configFilePath: " << configFilePath.toStdString() << std::endl
		<< "configURLPath: " << configURLPath.toStdString() << std::endl
		<< "appDir: " << appDir.toStdString() << std::endl;

	QDialog::showNormal();
}

void ConfigGui::hide()
{	
	//settings.beginGroup("ConfigGui");
	//settings.setValue("configgeometry", saveGeometry());
	//settings.endGroup();
	QDialog::hide();
}

void ConfigGui::closeEvent(QCloseEvent *event)
{
	//hide();
	//event->ignore();
	QDialog::closeEvent(event);
}

void ConfigGui::close()
{
	QDialog::close();
}

void ConfigGui::configSourceRadioButtonClicked()
{
	URLLineEdit->setEnabled(URLRadioButton->isChecked());
	fileLineEdit->setEnabled(fileRadioButton->isChecked());
	progressBar->setEnabled(URLRadioButton->isChecked());
	progressBar->setVisible(URLRadioButton->isChecked());
	browsePushButton->setEnabled(fileRadioButton->isChecked());
	updateButton->setEnabled(URLRadioButton->isChecked() && URLLineEdit->text().startsWith("http"));
}

void ConfigGui::browseFileClicked()
{
	QString filter("*.xml");
	QString retval;
	QFileInfo fi(configFilePath);
	if (!configFilePath.isEmpty() && fi.absoluteDir().exists())
		retval = QFileDialog::getOpenFileName(this, "Open File", fi.absoluteDir().absolutePath(), filter);
	else
	{
		retval = QFileDialog::getOpenFileName(this, "Open File", defaultConfigDir, filter);
	}

	if (!retval.isEmpty())
		fileLineEdit->setText(retval);
}
