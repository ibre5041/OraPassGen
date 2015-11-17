#pragma once

#include <QLineEdit>
#include <QStringList>

class QListView;
class QStringListModel;
class QModelIndex;

class CompleteLineEdit : public QLineEdit
{
    Q_OBJECT
public:
	CompleteLineEdit(QWidget *parent = 0);
    CompleteLineEdit(QStringList words, QWidget *parent = 0);
	void setFont(const QFont &);
	void setWords(QStringList const& words);
public slots:
    void setCompleter(const QString &text);
    void completeText(const QModelIndex &index);
protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void focusOutEvent(QFocusEvent *e);
private:
    QStringList words;
    QListView *listView;
    QStringListModel *model;
};
