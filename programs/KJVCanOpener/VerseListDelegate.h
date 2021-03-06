/****************************************************************************
**
** Copyright (C) 2012 Donna Whisnant, a.k.a. Dewtronics.
** Contact: http://www.dewtronics.com/
**
** This file is part of the KJVCanOpener Application as originally written
** and developed for Bethel Church, Festus, MO.
**
** GNU General Public License Usage
** This file may be used under the terms of the GNU General Public License
** version 3.0 as published by the Free Software Foundation and appearing
** in the file gpl-3.0.txt included in the packaging of this file. Please
** review the following information to ensure the GNU General Public License
** version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and
** Dewtronics.
**
****************************************************************************/

#ifndef VERSELISTDELEGATE_H
#define VERSELISTDELEGATE_H

#include "VerseListModel.h"

#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QWidget>
#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QTreeView>
#include <QHelpEvent>

class CVerseListDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	CVerseListDelegate(CVerseListModel &model, QObject *parent = NULL);

	virtual void paint(QPainter * painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
	virtual QString displayText(const QVariant &value, const QLocale &locale) const;

signals:

public slots:
	virtual bool helpEvent(QHelpEvent *event, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index);

protected:
	void SetDocumentText(const QStyleOptionViewItemV4 &option, QTextDocument &doc, const QModelIndex &index, bool bDoingSizeHint) const;
	int indentationForIndex(const QModelIndex &index) const;

	QTreeView* parentView() const { return static_cast<QTreeView*>(QObject::parent()); }

private:
	CVerseListModel &m_model;
};

#endif // VERSELISTDELEGATE_H
