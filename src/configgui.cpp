#include "configgui.h"

ConfigGui::ConfigGui(QWidget * parent)
	: QDialog(parent)
{
	setupUi(this);
	//progressBar->hide();
}

void ConfigGui::setVisible(bool visible)
{
	//minimizeAction->setEnabled(visible);
	//openAction->setEnabled(!visible);
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

void ConfigGui::showNormal()
{
	//settings.beginGroup("ConfigGui");
	//restoreGeometry(settings.value("configgeometry", QByteArray()).toByteArray());
	//settings.endGroup();
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
}
