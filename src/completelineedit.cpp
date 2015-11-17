#include "completelineedit.h"
#include <QKeyEvent>
#include <QListView>
#include <QStringListModel>
#include <QDebug>

CompleteLineEdit::CompleteLineEdit(QStringList words, QWidget *parent)
    : QLineEdit(parent), words(words) 
{
    listView = new QListView(this);
    model = new QStringListModel(this);
    listView->setWindowFlags(Qt::ToolTip);
    connect(this, SIGNAL(textChanged(const QString &)), this, SLOT(setCompleter(const QString &)));
    connect(listView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(completeText(const QModelIndex &)));
}

CompleteLineEdit::CompleteLineEdit(QWidget *parent)
	: QLineEdit(parent), words()
{
	listView = new QListView(this);
	model = new QStringListModel(this);
	listView->setWindowFlags(Qt::ToolTip);
	connect(this, SIGNAL(textChanged(const QString &)), this, SLOT(setCompleter(const QString &)));
	connect(listView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(completeText(const QModelIndex &)));
}

void CompleteLineEdit::setFont(const QFont &font)
{
	QLineEdit::setFont(font);
	listView->setFont(font);
}

void CompleteLineEdit::setWords(QStringList const& w)
{
	words = w;
}

void CompleteLineEdit::focusOutEvent(QFocusEvent *e) {
	QLineEdit::focusOutEvent(e);
    listView->hide();
}

void CompleteLineEdit::keyPressEvent(QKeyEvent *e) {
    if (!listView->isHidden()) {
        int key = e->key();
        int count = listView->model()->rowCount();
        QModelIndex currentIndex = listView->currentIndex();

        if (Qt::Key_Down == key) {
            int row = currentIndex.row() + 1;
            if (row >= count) {
                row = 0;
            }

            QModelIndex index = listView->model()->index(row, 0);
            listView->setCurrentIndex(index);
        } else if (Qt::Key_Up == key) {
            int row = currentIndex.row() - 1;
            if (row < 0) {
                row = count - 1;
            }

            QModelIndex index = listView->model()->index(row, 0);
            listView->setCurrentIndex(index);
        } else if (Qt::Key_Escape == key) {
            listView->hide();
        } else if (Qt::Key_Enter == key || Qt::Key_Return == key) {
            if (currentIndex.isValid()) {
                QString text = listView->currentIndex().data().toString();
                setText(text);
            }
			if (model->rowCount() == 1) {
				QModelIndex index = listView->model()->index(0, 0);
				QString text = model->data(index, Qt::DisplayRole).toString();
				setText(text);
			}
            listView->hide();
			emit returnPressed();
        } else {
            listView->hide();
            QLineEdit::keyPressEvent(e);
        }
    } else {
        QLineEdit::keyPressEvent(e);
    }
}

void CompleteLineEdit::setCompleter(const QString &text) {
    if (text.isEmpty()) {
        listView->hide();
        return;
    }

    if ((text.length() > 1) && (!listView->isHidden())) {
        return;
    }

    QStringList sl;
    foreach(QString word, words) {
        if (word.contains(text)) {
            sl << word;
        }
    }
    model->setStringList(sl);
    listView->setModel(model);
    if (model->rowCount() == 0) {
        return;
    }
    // Position the text edit
    listView->setMinimumWidth(width());
    listView->setMaximumWidth(width());
    QPoint p(0, height());
    int x = mapToGlobal(p).x();
    int y = mapToGlobal(p).y() + 1;
    listView->move(x, y);
    listView->show();
}

void CompleteLineEdit::completeText(const QModelIndex &index) {
    QString text = index.data().toString();
    setText(text);
    listView->hide();
}


