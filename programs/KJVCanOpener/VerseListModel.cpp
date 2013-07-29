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

#include "VerseListModel.h"
#include "PersistentSettings.h"
#include "UserNotesDatabase.h"

#include <QVector>
#include <QModelIndexList>
#include <iterator>
#include <list>

// ============================================================================

static bool ascendingLessThanVLI(const CVerseListItem &s1, const CVerseListItem &s2)
{
	// Both normalized and denormalized are in order, but it's more expensive
	//	 to convert to normal when we already have relative
	return s1.getIndexDenormalized() < s2.getIndexDenormalized();
}

static bool decendingLessThanVLI(const CVerseListItem &s1, const CVerseListItem &s2)
{
	// Both normalized and denormalized are in order, but it's more expensive
	//	 to convert to normal when we already have relative
	return s1.getIndexDenormalized() > s2.getIndexDenormalized();
}

void sortVerseList(CVerseList &aVerseList, Qt::SortOrder order)
{
	if (order == Qt::AscendingOrder)
		qSort(aVerseList.begin(), aVerseList.end(), ascendingLessThanVLI);
	else
		qSort(aVerseList.begin(), aVerseList.end(), decendingLessThanVLI);
}

// ============================================================================

CVerseListModel::TVerseListModelPrivate::TVerseListModelPrivate(CBibleDatabasePtr pBibleDatabase)
	:	m_pBibleDatabase(pBibleDatabase),
		m_nDisplayMode(VDME_HEADING),
		m_nTreeMode(VTME_LIST),
		m_nViewMode(VVME_SEARCH_RESULTS),
		m_bShowMissingLeafs(false)
{

}

CVerseListModel::CVerseListModel(CBibleDatabasePtr pBibleDatabase, QObject *pParent)
	:	QAbstractItemModel(pParent),
		m_private(pBibleDatabase),
		m_searchResults(&m_private)
{
	m_private.m_richifierTags.setWordsOfJesusTagsByColor(CPersistentSettings::instance()->colorWordsOfJesus());
	connect(CPersistentSettings::instance(), SIGNAL(changedColorWordsOfJesus(const QColor &)), this, SLOT(en_WordsOfJesusColorChanged(const QColor &)));

	assert(g_pUserNotesDatabase != NULL);
	if (g_pUserNotesDatabase != NULL) {
		connect(g_pUserNotesDatabase.data(), SIGNAL(highlighterTagsChanged(CBibleDatabasePtr, const QString &)), this, SLOT(en_highlighterTagsChanged(CBibleDatabasePtr, const QString &)));
		connect(g_pUserNotesDatabase.data(), SIGNAL(changedHighlighters()), this, SLOT(en_changedHighlighters()));
	}

	en_changedHighlighters();		// Make sure we've loaded the initial default highlighters (or from the current set if we are rebuilding this class for some reason)
}

int CVerseListModel::rowCount(const QModelIndex &zParent) const
{
	const TVerseListModelResults &zResults = results(zParent);

	bool bHighlighterNode = ((m_private.m_nViewMode == VVME_SEARCH_RESULTS) ? false : !zParent.isValid());
	bool bTreeTop = ((m_private.m_nViewMode == VVME_SEARCH_RESULTS) ?
						!zParent.isValid() :			// Search Results top is the root node
						 (zParent.isValid() && !toVerseIndex(zParent)->relIndex().isSet() && !parent(zParent).isValid()));	// Highlighter Results top is the node whose parent has no relIndex and whose parent's parent is the root node

	if (bHighlighterNode) {
		return m_vlmrListHighlighters.size();
	} else {
		switch (m_private.m_nTreeMode) {
			case VTME_LIST:
			{
				if (!bTreeTop) return 0;
				return zResults.m_mapVerses.size();
			}
			case VTME_TREE_BOOKS:
			{
				if (bTreeTop) return zResults.GetBookCount();
				CRelIndex ndxRel(toVerseIndex(zParent)->m_nRelIndex);
				assert(ndxRel.isSet());
				if (ndxRel.chapter() == 0) return zResults.GetVerseCount(ndxRel.book());
				return 0;
			}
			case VTME_TREE_CHAPTERS:
			{
				if (bTreeTop) return zResults.GetBookCount();
				CRelIndex ndxRel(toVerseIndex(zParent)->m_nRelIndex);
				assert(ndxRel.isSet());
				if (ndxRel.chapter() == 0) return zResults.GetChapterCount(ndxRel.book());
				if (ndxRel.verse() == 0) return zResults.GetVerseCount(ndxRel.book(), ndxRel.chapter());
				return 0;
			}
			default:
				break;
		}
	}

	return 0;
}

int CVerseListModel::columnCount(const QModelIndex &zParent) const
{
	bool bHighlighterNode = ((m_private.m_nViewMode == VVME_SEARCH_RESULTS) ? false : !zParent.isValid());
	bool bTreeTop = ((m_private.m_nViewMode == VVME_SEARCH_RESULTS) ?
						!zParent.isValid() :			// Search Results top is the root node
						 (zParent.isValid() && !toVerseIndex(zParent)->relIndex().isSet() && !parent(zParent).isValid()));	// Highlighter Results top is the node whose parent has no relIndex and whose parent's parent is the root node

	if (bHighlighterNode) {
		return 1;
	} else {
		switch (m_private.m_nTreeMode) {
			case VTME_LIST:
			{
				if (bTreeTop) return 1;
				return 0;
			}
			case VTME_TREE_BOOKS:
			{
				if (bTreeTop) return 1;
				CRelIndex ndxRel(toVerseIndex(zParent)->m_nRelIndex);
				assert(ndxRel.isSet());
				if (ndxRel.chapter() == 0) return 1;
				return 0;
			}
			case VTME_TREE_CHAPTERS:
			{
				if (bTreeTop) return 1;
				CRelIndex ndxRel(toVerseIndex(zParent)->m_nRelIndex);
				assert(ndxRel.isSet());
				if (ndxRel.chapter() == 0) return 1;
				if (ndxRel.verse() == 0) return 1;
				return 0;
			}
			default:
				break;
		}
	}

	return 0;
}

QModelIndex	CVerseListModel::index(int row, int column, const QModelIndex &zParent) const
{
	if (!hasIndex(row, column, zParent)) return QModelIndex();

	bool bHighlighterNode = ((m_private.m_nViewMode == VVME_SEARCH_RESULTS) ? false : !zParent.isValid());
	const TVerseListModelResults &zResults = (!bHighlighterNode ? results(zParent) : results(row));			// If this is the highlighter entry, the parent will be invalid but our row is our highlighter results index

	bool bTreeTop = ((m_private.m_nViewMode == VVME_SEARCH_RESULTS) ?
						!zParent.isValid() :			// Search Results top is the root node
						 (zParent.isValid() && !toVerseIndex(zParent)->relIndex().isSet() && !parent(zParent).isValid()));	// Highlighter Results top is the node whose parent has no relIndex and whose parent's parent is the root node

	if (bHighlighterNode) {
		assert(row < m_vlmrListHighlighters.size());
		if (row < m_vlmrListHighlighters.size()) {
			return createIndex(row, column, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(CRelIndex(), row)).data()));
		}
	} else {
		switch (m_private.m_nTreeMode) {
			case VTME_LIST:
			{
				assert(row < zResults.m_mapVerses.size());
				CVerseMap::const_iterator itrVerse = zResults.GetVerse(row);
				if (itrVerse == zResults.m_mapVerses.constEnd()) return QModelIndex();
				return createIndex(row, column, fromVerseIndex(itrVerse->verseIndex().data()));
			}
			case VTME_TREE_BOOKS:
			{
				if (bTreeTop) {
					return createIndex(row, column, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(CRelIndex(zResults.BookByIndex(row), 0, 0, 0), zResults.highlighterIndex())).data()));
				}
				CRelIndex ndxRel(toVerseIndex(zParent)->m_nRelIndex);
				assert(ndxRel.isSet());
				if (ndxRel.chapter() == 0) {
					CVerseMap::const_iterator itrVerse = zResults.GetVerse(row, ndxRel.book());
					if (itrVerse == zResults.m_mapVerses.constEnd()) return QModelIndex();
					return createIndex(row, column, fromVerseIndex(itrVerse->verseIndex().data()));
				}
				return QModelIndex();
			}
			case VTME_TREE_CHAPTERS:
			{
				if (bTreeTop) {
					return createIndex(row, column, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(CRelIndex(zResults.BookByIndex(row), 0, 0, 0), zResults.highlighterIndex())).data()));
				}
				CRelIndex ndxRel(toVerseIndex(zParent)->m_nRelIndex);
				assert(ndxRel.isSet());
				if (ndxRel.chapter() == 0) {
					return createIndex(row, column, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(CRelIndex(ndxRel.book(), zResults.ChapterByIndex(zParent.row(), row), 0, 0), zResults.highlighterIndex())).data()));
				}
				if (ndxRel.verse() == 0) {
					CVerseMap::const_iterator itrVerse = zResults.GetVerse(row, ndxRel.book(), ndxRel.chapter());
					if (itrVerse == zResults.m_mapVerses.constEnd()) return QModelIndex();
					return createIndex(row, column, fromVerseIndex(itrVerse->verseIndex().data()));
				}
				return QModelIndex();
			}
			default:
				break;
		}
	}

	return QModelIndex();
}

QModelIndex CVerseListModel::parent(const QModelIndex &index) const
{
	if (!index.isValid()) return QModelIndex();

	const TVerseListModelResults &zResults = results(index);

	bool bHighlighterNode = ((m_private.m_nViewMode == VVME_SEARCH_RESULTS) ? false : (!toVerseIndex(index)->relIndex().isSet()));

	if (bHighlighterNode) {
		return QModelIndex();
	} else {
		switch (m_private.m_nTreeMode) {
			case VTME_LIST:
			{
				if (m_private.m_nViewMode == VVME_SEARCH_RESULTS) return QModelIndex();
				return createIndex(zResults.highlighterIndex(), 0, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(CRelIndex(), zResults.highlighterIndex())).data()));
			}
			case VTME_TREE_BOOKS:
			{
				CRelIndex ndxRel(toVerseIndex(index)->m_nRelIndex);
				assert(ndxRel.isSet());
				if (ndxRel.verse() != 0) {
					if (zResults.m_mapVerses.contains(ndxRel)) {
						return createIndex(zResults.IndexByBook(ndxRel.book()), 0, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(CRelIndex(ndxRel.book(), 0, 0, 0), zResults.highlighterIndex())).data()));
					} else {
						assert(false);
					}
				}
				if (m_private.m_nViewMode == VVME_SEARCH_RESULTS) return QModelIndex();
				return createIndex(zResults.highlighterIndex(), 0, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(CRelIndex(), zResults.highlighterIndex())).data()));
			}
			case VTME_TREE_CHAPTERS:
			{
				CRelIndex ndxRel(toVerseIndex(index)->m_nRelIndex);
				assert(ndxRel.isSet());
				if (ndxRel.verse() != 0) {
					if (zResults.m_mapVerses.contains(ndxRel)) {
						return createIndex(zResults.IndexByChapter(ndxRel.book(), ndxRel.chapter()), 0, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(CRelIndex(ndxRel.book(), ndxRel.chapter(), 0, 0), zResults.highlighterIndex())).data()));
					} else {
						assert(false);
					}
				} else if (ndxRel.chapter() != 0) {
					return createIndex(zResults.IndexByBook(ndxRel.book()), 0, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(CRelIndex(ndxRel.book(), 0, 0, 0), zResults.highlighterIndex())).data()));
				}
				if (m_private.m_nViewMode == VVME_SEARCH_RESULTS) return QModelIndex();
				return createIndex(zResults.highlighterIndex(), 0, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(CRelIndex(), zResults.highlighterIndex())).data()));
			}
			default:
				break;
		}
	}

	return QModelIndex();
}

QVariant CVerseListModel::data(const QModelIndex &index, int role) const
{
	assert(m_private.m_pBibleDatabase);

	if (!index.isValid()) return QVariant();

	const TVerseListModelResults &zResults = results(index);

	if (role == Qt::SizeHintRole) return zResults.m_mapSizeHints.value(*toVerseIndex(index), QSize());

	bool bHighlighterNode = ((m_private.m_nViewMode == VVME_SEARCH_RESULTS) ? false : !parent(index).isValid());

	if (bHighlighterNode) {
		if ((role == Qt::DisplayRole) || (role == Qt::EditRole)) {
			return zResults.resultsName();
		}
	} else {
		CRelIndex ndxRel(toVerseIndex(index)->m_nRelIndex);
		assert(ndxRel.isSet());
		if (!ndxRel.isSet()) return QVariant();

		if ((ndxRel.chapter() == 0) && (ndxRel.verse() == 0)) {
			if ((role == Qt::DisplayRole) || (role == Qt::EditRole)) {
				QString strBookText = m_private.m_pBibleDatabase->bookName(ndxRel);
				if (m_private.m_nDisplayMode != VDME_HEADING) return strBookText;		// For Rich Text, Let delegate add results so it can be formatted
				int nVerses = zResults.GetVerseCount(ndxRel.book());
				int nResults = zResults.GetResultsCount(ndxRel.book());
				if ((nResults) || (nVerses)) strBookText = QString("{%1} (%2) ").arg(nVerses).arg(nResults) + strBookText;
				return strBookText;
			}
			if ((role == Qt::ToolTipRole) ||
				(role == TOOLTIP_ROLE) ||
				(role == TOOLTIP_PLAINTEXT_ROLE) ||
				(role == TOOLTIP_NOHEADING_ROLE) ||
				(role == TOOLTIP_NOHEADING_PLAINTEXT_ROLE)) {
				return QString();
			}
			return QVariant();
		}

		if (ndxRel.verse() == 0) {
			if ((role == Qt::DisplayRole) || (role == Qt::EditRole)) {
				QString strChapterText = m_private.m_pBibleDatabase->bookName(ndxRel) + QString(" %1").arg(ndxRel.chapter());
				if (m_private.m_nDisplayMode != VDME_HEADING) return strChapterText;	// For Rich Text, Let delegate add results so it can be formatted
				int nVerses = zResults.GetVerseCount(ndxRel.book(), ndxRel.chapter());
				int nResults = zResults.GetResultsCount(ndxRel.book(), ndxRel.chapter());
				if ((nResults) || (nVerses)) strChapterText = QString("{%1} (%2) ").arg(nVerses).arg(nResults) + strChapterText;
				return strChapterText;
			}
			if ((role == Qt::ToolTipRole) ||
				(role == TOOLTIP_ROLE) ||
				(role == TOOLTIP_PLAINTEXT_ROLE) ||
				(role == TOOLTIP_NOHEADING_ROLE) ||
				(role == TOOLTIP_NOHEADING_PLAINTEXT_ROLE)) {
				return QString();
			}
			return QVariant();
		}

		if (!zResults.m_mapVerses.contains(ndxRel)) return QVariant();

		if (role == Qt::ToolTipRole) return QString();		// en_viewDetails replaces normal ToolTip

		return dataForVerse(toVerseIndex(index), role);
	}

	return QVariant();
}

QVariant CVerseListModel::dataForVerse(const TVerseIndex *pVerseIndex, int role) const
{
	assert(pVerseIndex != NULL);

	const TVerseListModelResults &zResults = results(*pVerseIndex);

	CRelIndex ndxVerse = pVerseIndex->relIndex();
	ndxVerse.setWord(0);
	CVerseMap::const_iterator itrVerse = zResults.m_mapVerses.find(ndxVerse);
	if (itrVerse == zResults.m_mapVerses.constEnd()) return QVariant();

	if ((role == Qt::DisplayRole) || (role == Qt::EditRole)) {
		switch (m_private.m_nDisplayMode) {
			case VDME_HEADING:
				return itrVerse->getHeading();
			case VDME_VERYPLAIN:
				return itrVerse->getVerseVeryPlainText();
			case VDME_RICHTEXT:
				return itrVerse->getVerseRichText(m_private.m_richifierTags);
			case VDME_COMPLETE:
				return itrVerse->getVerseRichText(m_private.m_richifierTags);		// TODO : FINISH THIS ONE!!!
			default:
				return QString();
		}
	}

	if (m_private.m_nViewMode == VVME_SEARCH_RESULTS) {
		if ((role == TOOLTIP_ROLE) ||
			(role == TOOLTIP_PLAINTEXT_ROLE) ||
			(role == TOOLTIP_NOHEADING_ROLE) ||
			(role == TOOLTIP_NOHEADING_PLAINTEXT_ROLE)) {

			// Switch to Search Results as our incoming index may not have been for Search Results,
			//		even though we are now in Search Results View Mode:
			itrVerse = m_searchResults.m_mapVerses.find(ndxVerse);
			if (itrVerse == m_searchResults.m_mapVerses.constEnd()) return QVariant();

			bool bHeading = ((role != TOOLTIP_NOHEADING_ROLE) && (role != TOOLTIP_NOHEADING_PLAINTEXT_ROLE));
			QString strToolTip;
			if ((role != TOOLTIP_PLAINTEXT_ROLE) &&
				(role != TOOLTIP_NOHEADING_PLAINTEXT_ROLE)) strToolTip += "<qt><pre>";
			if (bHeading) strToolTip += itrVerse->getHeading() + "\n";
			QPair<int, int> nResultsIndexes = m_searchResults.GetResultsIndexes(itrVerse);
			if (nResultsIndexes.first != nResultsIndexes.second) {
				strToolTip += QString("%1").arg(bHeading ? "    " : "") +
							tr("Search Results %1-%2 of %3 phrase occurrences")
										.arg(nResultsIndexes.first)
										.arg(nResultsIndexes.second)
										.arg(m_searchResults.GetResultsCount()) + "\n";
			} else {
				assert(nResultsIndexes.first != 0);		// This will assert if the row was beyond those defined in our list
				strToolTip += QString("%1").arg(bHeading ? "    " : "") +
							tr("Search Result %1 of %2 phrase occurrences")
										.arg(nResultsIndexes.first)
										.arg(m_searchResults.GetResultsCount()) + "\n";
			}
			QPair<int, int> nVerseResult = m_searchResults.GetVerseIndexAndCount(itrVerse);
			strToolTip += QString("%1    ").arg(bHeading ? "    " : "") + tr("Verse %1 of %2 in Search Scope").arg(nVerseResult.first).arg(nVerseResult.second) + "\n";
			QPair<int, int> nChapterResult = m_searchResults.GetChapterIndexAndCount(itrVerse);
			strToolTip += QString("%1    ").arg(bHeading ? "    " : "") + tr("Chapter %1 of %2 in Search Scope").arg(nChapterResult.first).arg(nChapterResult.second) + "\n";
			QPair<int, int> nBookResult = m_searchResults.GetBookIndexAndCount(itrVerse);
			strToolTip += QString("%1    ").arg(bHeading ? "    " : "") + tr("Book %1 of %2 in Search Scope").arg(nBookResult.first).arg(nBookResult.second) + "\n";
			strToolTip += itrVerse->getToolTip(m_searchResults.m_lstParsedPhrases);
			if ((role != TOOLTIP_PLAINTEXT_ROLE) &&
				(role != TOOLTIP_NOHEADING_PLAINTEXT_ROLE)) strToolTip += "</pre></qt>";
			return strToolTip;
		}
	}

	if (role == VERSE_ENTRY_ROLE) {
		return QVariant::fromValue(*itrVerse);
	}

	return QVariant();
}

bool CVerseListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::SizeHintRole) {
		if (!index.isValid()) {
			// Special Case:  QModelIndex() is "invalidate all":
			clearAllSizeHints();
			emit cachedSizeHintsInvalidated();
			return false;				// But return false because we can't actually set a SizeHint for an invalid index
		}

		TVerseListModelResults &zResults = const_cast<TVerseListModelResults &>(results(index));

		zResults.m_mapSizeHints[*toVerseIndex(index)] = value.toSize();
		// Note: Do not fire dataChanged() here, as this is just a cache used by ReflowDelegate
		return true;
	}

/*
	if (index.row() < 0 || index.row() >= m_lstVerses.size()) return false;

	if ((role == Qt::EditRole) || (role == Qt::DisplayRole)) {
		switch (m_private.m_nDisplayMode) {
			case VDME_HEADING:
			case VDME_VERYPLAIN:
			case VDME_RICHTEXT:
			case VDME_COMPLETE:
				return false;		// read-only
		}
	}

	if ((role == TOOLTIP_ROLE) ||
		(role == TOOLTIP_PLAINTEXT_ROLE) ||
		(role == TOOLTIP_NOHEADING_ROLE) ||
		(role == TOOLTIP_NOHEADING_PLAINTEXT_ROLE)) {
		return false;				// read-only
	}

	if (role == VERSE_ENTRY_ROLE) {
		m_lstVerses.replace(index.row(), value.value<CVerseListItem>());
		emit dataChanged(index, index);
		return true;
	}
*/

	return false;
}

Qt::ItemFlags CVerseListModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;

	CRelIndex ndxRel(toVerseIndex(index)->m_nRelIndex);
	if ((ndxRel.isSet()) && (ndxRel.verse() != 0))
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable /* | Qt::ItemIsEditable */ | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

	return Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

bool CVerseListModel::insertRows(int row, int count, const QModelIndex &zParent)
{
	Q_UNUSED(row);
	Q_UNUSED(count);
	Q_UNUSED(zParent);

	return false;
/*
	if (count < 1 || row < 0 || row > m_lstVerses.size())
		return false;
	if (zParent.isValid()) return false;

	beginInsertRows(QModelIndex(), row, row + count - 1);

	for (int r = 0; r < count; ++r)
		m_lstVerses.insert(row, CVerseListItem());

	endInsertRows();

	return true;
*/
}

bool CVerseListModel::removeRows(int row, int count, const QModelIndex &zParent)
{
	Q_UNUSED(row);
	Q_UNUSED(count);
	Q_UNUSED(zParent);

	return false;

/*
	if (count <= 0 || row < 0 || (row + count) > m_lstVerses.size())
		return false;
	if (zParent.isValid()) return false;

	beginRemoveRows(QModelIndex(), row, row + count - 1);

	for (int r = 0; r < count; ++r)
		m_lstVerses.removeAt(row);

	endRemoveRows();

	return true;
*/
}

/*
static bool ascendingLessThan(const QPair<CVerseListItem, int> &s1, const QPair<CVerseListItem, int> &s2)
{
	// Both normalized and denormalized are in order, but it's more expensive
	//	 to convert to normal when we already have relative
	return s1.first.getIndexDenormalized() < s2.first.getIndexDenormalized();
}

static bool decendingLessThan(const QPair<CVerseListItem, int> &s1, const QPair<CVerseListItem, int> &s2)
{
	// Both normalized and denormalized are in order, but it's more expensive
	//	 to convert to normal when we already have relative
	return s1.first.getIndexDenormalized() > s2.first.getIndexDenormalized();
}
*/

void CVerseListModel::sort(int /* column */, Qt::SortOrder order)
{
	Q_UNUSED(order);
	assert(false);

/*

	emit layoutAboutToBeChanged();

	QList<QPair<CVerseListItem, int> > list;
	for (int i = 0; i < m_lstVerses.count(); ++i)
		list.append(QPair<CVerseListItem, int>(m_lstVerses.at(i), i));

	if (order == Qt::AscendingOrder)
		qSort(list.begin(), list.end(), ascendingLessThan);
	else
		qSort(list.begin(), list.end(), decendingLessThan);

	m_lstVerses.clear();
	QVector<int> forwarding(list.count());
	for (int i = 0; i < list.count(); ++i) {
		m_lstVerses.append(list.at(i).first);
		forwarding[list.at(i).second] = i;
	}

	QModelIndexList oldList = persistentIndexList();
	QModelIndexList newList;
	for (int i = 0; i < oldList.count(); ++i)
		newList.append(index(forwarding.at(oldList.at(i).row()), 0));
	changePersistentIndexList(oldList, newList);

	emit layoutChanged();
*/
}

Qt::DropActions CVerseListModel::supportedDropActions() const
{
	return QAbstractItemModel::supportedDropActions() | Qt::MoveAction;
}

QModelIndex CVerseListModel::locateIndex(const TVerseIndex &ndxVerse) const
{
	const CRelIndex &ndxRel = ndxVerse.relIndex();
	if (!ndxRel.isSet()) return QModelIndex();

	const TVerseListModelResults &zResults = results(ndxVerse);

	// See if this is a verse (search result) reference.  If so resolve:
	if (ndxRel.verse() != 0) {
		CVerseMap::const_iterator itrFirst;
		CVerseMap::const_iterator itrTarget;

		// Set ndxVerse to first verse m_lstVerses array for this parent node:
		if (m_private.m_nTreeMode == VTME_LIST) {
			itrFirst = zResults.m_mapVerses.constBegin();		// For list mode, the list includes everything, so start with the first index
		} else {
			itrFirst = zResults.GetVerse(0, ndxRel.book(), ((m_private.m_nTreeMode == VTME_TREE_CHAPTERS ) ? ndxRel.chapter() : 0));
		}
		itrTarget = zResults.FindVerseIndex(ndxRel);
		if (itrTarget == zResults.m_mapVerses.constEnd()) return QModelIndex();
		return createIndex(std::distance(itrFirst, itrTarget), 0, fromVerseIndex(itrTarget->verseIndex().data()));		// Use index from actual verse instead of ndxRel since word() isn't required to match
	}

	// If we are in list mode and caller only gave us a book/chapter reference,
	//		then we simply don't have an index to return.  That makes more
	//		sense than possibly returning the first one that might match:
	if (m_private.m_nTreeMode == VTME_LIST) return QModelIndex();

	if ((ndxRel.chapter() != 0) && (m_private.m_nTreeMode == VTME_TREE_CHAPTERS)) {
		// If this is a book/chapter reference, resolve it:
		int ndxTarget = zResults.IndexByChapter(ndxRel.book(), ndxRel.chapter());
		if (ndxTarget == -1) return QModelIndex();
		CRelIndex ndxChapter(ndxRel.book(), ndxRel.chapter(), 0, 0);			// Create CRelIndex rather than using ndxRel, since we aren't requiring word() to match
		return createIndex(ndxTarget, 0, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(ndxChapter, zResults.highlighterIndex())).data()));
	} else {
		// If this is a book-only reference, resolve it:
		int ndxTarget = zResults.IndexByBook(ndxRel.book());
		if (ndxTarget == -1) return QModelIndex();
		CRelIndex ndxBook(ndxRel.book(), 0, 0, 0);			// Create CRelIndex rather than using ndxRel, since we aren't requiring word() to match
		return createIndex(ndxTarget, 0, fromVerseIndex(zResults.extraVerseIndex(TVerseIndex(ndxBook, zResults.highlighterIndex())).data()));
	}

	return QModelIndex();
}

TVerseIndex CVerseListModel::resolveVerseIndex(const CRelIndex &ndxRel, const QString &strHighlighterName) const
{
	if (strHighlighterName.isEmpty()) return TVerseIndex(ndxRel, -1);
	for (int ndxHighlighter = 0; ndxHighlighter < m_vlmrListHighlighters.size(); ++ndxHighlighter) {
		if (m_vlmrListHighlighters.at(ndxHighlighter).resultsName().compare(strHighlighterName, Qt::CaseInsensitive) == 0)
			return TVerseIndex(ndxRel, ndxHighlighter);
	}

	return TVerseIndex();
}

// ----------------------------------------------------------------------------

TParsedPhrasesList CVerseListModel::parsedPhrases() const
{
	return m_searchResults.m_lstParsedPhrases;
}

void CVerseListModel::setParsedPhrases(const CSearchCriteria &aSearchCriteria, const TParsedPhrasesList &phrases)
{
	// Note: Basic setting of this list doesn't change the model, as the phrases
	//		themselves are used primarily for building of tooltips that are
	//		appropriate for the entire search scope.  However, once these are
	//		set, we'll call the buildVerseListFromParsedPhrases function that
	//		will build and set the VerseList, which will change the model.
	//		Therefore, the beginResetModel/endResetModel calls don't exist here,
	//		but down in buildScopedResultsFromParsedPhrases():
	m_searchResults.m_lstParsedPhrases = phrases;
	m_searchResults.m_SearchCriteria = aSearchCriteria;
	buildScopedResultsFromParsedPhrases();
}

// ----------------------------------------------------------------------------

void CVerseListModel::en_highlighterTagsChanged(CBibleDatabasePtr pBibleDatabase, const QString &strUserDefinedHighlighterName)
{
	if ((pBibleDatabase == NULL) ||
		(pBibleDatabase->compatibilityUUID().compare(m_private.m_pBibleDatabase->compatibilityUUID(), Qt::CaseInsensitive) == 0)) {
		TVerseIndex verseIndex = resolveVerseIndex(CRelIndex(), strUserDefinedHighlighterName);
		buildHighlighterResults(verseIndex.highlighterIndex());
	}
}

void CVerseListModel::en_changedHighlighters()
{
	buildHighlighterResults();
}

void CVerseListModel::buildHighlighterResults(int ndxHighlighter)
{
	assert(g_pUserNotesDatabase != NULL);

	emit verseListAboutToChange();
	emit beginResetModel();

	if (ndxHighlighter == -1) {
		m_vlmrListHighlighters.clear();
		const TUserDefinedColorMap &mapHighlighters = g_pUserNotesDatabase->highlighterDefinitionsMap();
		ndxHighlighter = 0;
		for (TUserDefinedColorMap::const_iterator itrHighlighters = mapHighlighters.constBegin(); itrHighlighters != mapHighlighters.constEnd(); ++itrHighlighters) {
			// Must add it to our list before calling buildHighlighterResults(ndx):
			m_vlmrListHighlighters.push_back(TVerseListModelResults(&m_private, itrHighlighters.key(), ndxHighlighter));
			buildHighlighterResults(ndxHighlighter, g_pUserNotesDatabase->highlighterTagsFor(m_private.m_pBibleDatabase, itrHighlighters.key()));
			ndxHighlighter++;
		}

//		const THighlighterTagMap *pHighlighters = g_pUserNotesDatabase->highlighterTagsFor(m_private.m_pBibleDatabase);
//		if (pHighlighters != NULL) {
//			ndxHighlighter = 0;
//			for (THighlighterTagMap::const_iterator itrHighlighters = pHighlighters->begin(); itrHighlighters != pHighlighters->end(); ++itrHighlighters) {
//				// Must add it to our list before calling buildHighlighterResults(ndx):
//				m_vlmrListHighlighters.push_back(TVerseListModelResults(&m_private, itrHighlighters->first, ndxHighlighter));
//				buildHighlighterResults(ndxHighlighter, &(itrHighlighters->second));
//				ndxHighlighter++;
//			}
//		}
	} else {
		assert((ndxHighlighter >= 0) && (ndxHighlighter < m_vlmrListHighlighters.size()));
		TVerseListModelResults &zResults = const_cast<TVerseListModelResults &>(results(ndxHighlighter));

		buildHighlighterResults(ndxHighlighter, g_pUserNotesDatabase->highlighterTagsFor(m_private.m_pBibleDatabase, zResults.resultsName()));
	}

	emit endResetModel();
	emit verseListChanged();
}

void CVerseListModel::buildHighlighterResults(int ndxHighlighter, const TPhraseTagList *pTags)
{
	assert((ndxHighlighter >= 0) && (ndxHighlighter < m_vlmrListHighlighters.size()));
	TVerseListModelResults &zResults = const_cast<TVerseListModelResults &>(results(ndxHighlighter));

	zResults.m_mapVerses.clear();
	zResults.m_lstVerseIndexes.clear();
	zResults.m_mapExtraVerseIndexes.clear();
	zResults.m_mapSizeHints.clear();

	if (pTags) {
		for (TPhraseTagList::const_iterator itrTags = pTags->constBegin(); itrTags != pTags->constEnd(); ++itrTags) {
			CRelIndex ndxNextRelative = itrTags->relIndex();
			unsigned int nWordCount = itrTags->count();
			assert(nWordCount != 0);			// Shouldn't have any highlighter tags with empty ranges
			nWordCount += ((ndxNextRelative.word() != 0) ? (ndxNextRelative.word() - 1) : 0);					// Calculate back to start of verse to figure out how many verses this tag encompasses
			ndxNextRelative.setWord(0);
			while (nWordCount > 0) {
				if (zResults.m_mapVerses.contains(ndxNextRelative)) {
					zResults.m_mapVerses[ndxNextRelative].addPhraseTag(*itrTags);
				} else {
					zResults.m_mapVerses.insert(ndxNextRelative, CVerseListItem(TVerseIndex(ndxNextRelative, zResults.highlighterIndex()), m_private.m_pBibleDatabase, *itrTags));
				}
				unsigned int nNumWordsInVerse = m_private.m_pBibleDatabase->verseEntry(ndxNextRelative)->m_nNumWrd;
				if (nNumWordsInVerse >= nWordCount) {
					nWordCount = 0;
				} else {
					nWordCount -= nNumWordsInVerse;
					ndxNextRelative.setWord(1);
					// Add number of words in verse to find start of next verse:
					ndxNextRelative = CRelIndex(m_private.m_pBibleDatabase->DenormalizeIndex((m_private.m_pBibleDatabase->NormalizeIndex(ndxNextRelative) + nNumWordsInVerse)));
					assert(ndxNextRelative.word() == 1);		// We better end up at the first word of the next verse, or something bad happened
					ndxNextRelative.setWord(0);					// But, add as whole verse
				}
			}
		}
	}

	zResults.m_lstVerseIndexes.reserve(zResults.m_mapVerses.size());
	for (CVerseMap::const_iterator itr = zResults.m_mapVerses.constBegin(); (itr != zResults.m_mapVerses.constEnd()); ++itr) {
		zResults.m_lstVerseIndexes.append(itr.key());
	}
}

// ----------------------------------------------------------------------------

void CVerseListModel::setDisplayMode(VERSE_DISPLAY_MODE_ENUM nDisplayMode)
{
	if (m_private.m_nDisplayMode == nDisplayMode) return;

	clearAllSizeHints();
	emit layoutAboutToBeChanged();
	m_private.m_nDisplayMode = nDisplayMode;
	emit layoutChanged();
}

void CVerseListModel::setTreeMode(VERSE_TREE_MODE_ENUM nTreeMode)
{
	if (m_private.m_nTreeMode == nTreeMode) return;

	clearAllSizeHints();
	emit beginResetModel();
	m_private.m_nTreeMode = nTreeMode;
	emit endResetModel();
}

void CVerseListModel::setViewMode(CVerseListModel::VERSE_VIEW_MODE_ENUM nViewMode)
{
	if (m_private.m_nViewMode == nViewMode) return;

	clearAllSizeHints();
	emit beginResetModel();
	m_private.m_nViewMode = nViewMode;
	emit endResetModel();
}

void CVerseListModel::setShowMissingLeafs(bool bShowMissing)
{
	if (m_private.m_bShowMissingLeafs == bShowMissing) return;

	// Note: No need to clear sizeHints on this mode change as the size of existing items shouldn't change
	if ((m_private.m_nTreeMode != VTME_LIST) || (m_private.m_nViewMode != VVME_SEARCH_RESULTS)) beginResetModel();
	m_private.m_bShowMissingLeafs = bShowMissing;
	if ((m_private.m_nTreeMode != VTME_LIST) || (m_private.m_nViewMode != VVME_SEARCH_RESULTS)) endResetModel();
}

// ----------------------------------------------------------------------------

QPair<int, int> CVerseListModel::TVerseListModelSearchResults::GetResultsIndexes(CVerseMap::const_iterator itrVerse) const
{
	QPair<int, int> nResultsIndexes;
	nResultsIndexes.first = 0;
	nResultsIndexes.second = 0;

	assert(itrVerse != CVerseMap::const_iterator());

	for (CVerseMap::const_iterator itr = m_mapVerses.constBegin(); ((itr != itrVerse) && (itr != m_mapVerses.constEnd())); ++itr) {
		nResultsIndexes.first += itr->phraseTags().size();
	}
	nResultsIndexes.second = nResultsIndexes.first;
	if (itrVerse != m_mapVerses.constEnd()) {
		nResultsIndexes.first++;
		nResultsIndexes.second += itrVerse->phraseTags().size();
	}

	return nResultsIndexes;		// Result first = first result index, second = last result index for specified row
}

QPair<int, int> CVerseListModel::TVerseListModelSearchResults::GetBookIndexAndCount(CVerseMap::const_iterator itrVerse) const
{
	int ndxBook = 0;		// Index into Books
	int nBooks = 0;			// Results counts in Books
	bool bFlag = false;

	for (CVerseMap::const_iterator itr = m_mapVerses.constBegin(); (itr != m_mapVerses.constEnd()); ++itr) {
		nBooks++;			// Count the book we are on and skip the ones that are on the same book:
		if (!bFlag) ndxBook++;
		uint32_t nCurrentBook = itr.key().book();
		for (CVerseMap::const_iterator itr2 = itr+1; (itr2 != m_mapVerses.constEnd()); ++itr2) {
			if (itr2.key().book() != nCurrentBook) break;			// Look ahead at next entry and see if it's the same book.  If not, move on to count it...
			++itr;
			if (itr == itrVerse) bFlag = true;
		}

		if (itr == itrVerse) bFlag = true;
	}

	return QPair<int, int>(ndxBook, nBooks);
}

QPair<int, int> CVerseListModel::TVerseListModelSearchResults::GetChapterIndexAndCount(CVerseMap::const_iterator itrVerse) const
{
	int ndxChapter = 0;		// Index into Chapters
	int nChapters = 0;		// Results counts in Chapters
	bool bFlag = false;

	for (CVerseMap::const_iterator itr = m_mapVerses.constBegin(); (itr != m_mapVerses.constEnd()); ++itr) {
		nChapters++;		// Count the chapter we are on and skip the ones that are on the same book/chapter:
		if (!bFlag) ndxChapter++;
		uint32_t nCurrentBook = itr.key().book();
		uint32_t nCurrentChapter = itr.key().chapter();
		for (CVerseMap::const_iterator itr2 = itr+1; (itr2 != m_mapVerses.constEnd()); ++itr2) {
			if ((itr2.key().book() != nCurrentBook) ||
				(itr2.key().chapter() != nCurrentChapter)) break;		// Look ahead at next entry and see if it's the same book/chapter.  If not, move on to count it...
			++itr;
			if (itr == itrVerse) bFlag = true;
		}

		if (itr == itrVerse) bFlag = true;
	}

	return QPair<int, int>(ndxChapter, nChapters);
}

QPair<int, int> CVerseListModel::TVerseListModelSearchResults::GetVerseIndexAndCount(CVerseMap::const_iterator itrVerse) const
{
	return QPair<int, int>(((itrVerse != CVerseMap::const_iterator()) ? (std::distance(m_mapVerses.constBegin(), itrVerse)+1) : 0), m_mapVerses.size());
}

// ----------------------------------------------------------------------------

int CVerseListModel::TVerseListModelResults::GetBookCount() const
{
	assert(m_private->m_pBibleDatabase != NULL);

	if (m_private->m_bShowMissingLeafs) return m_private->m_pBibleDatabase->bibleEntry().m_nNumBk;

	CVerseMap::const_iterator itrVerseMapBookFirst = m_mapVerses.begin();
	CVerseMap::const_iterator itrVerseMapBookLast = m_mapVerses.end();

	int nCount = 0;
	while (itrVerseMapBookFirst != itrVerseMapBookLast) {
		// Find next book (bypassing any chapters/verses in the current book):
		itrVerseMapBookFirst = m_mapVerses.lowerBound(CRelIndex(itrVerseMapBookFirst.key().book() + 1, 0, 0, 0));
		++nCount;
	}
	return nCount;
}

int CVerseListModel::TVerseListModelResults::IndexByBook(unsigned int nBk) const
{
	assert(m_private->m_pBibleDatabase != NULL);

	if (m_private->m_bShowMissingLeafs) {
		if ((nBk < 1) || (nBk > m_private->m_pBibleDatabase->bibleEntry().m_nNumBk)) return -1;
		return (nBk-1);
	}

	// Find the first entry with the correct Book number:
	CVerseMap::const_iterator itrVerseMapBookFirst = m_mapVerses.begin();
	CVerseMap::const_iterator itrVerseMapBookLast = m_mapVerses.lowerBound(CRelIndex(nBk, 0, 0, 0));

	// If we didn't find the book, return -1 (not found):
	if (itrVerseMapBookLast == m_mapVerses.end()) return -1;
	if (itrVerseMapBookLast.key().book() != nBk) return -1;

	int nIndex = 0;
	while (itrVerseMapBookFirst != itrVerseMapBookLast) {
		// Find next book (bypassing any chapters/verses in the current book):
		itrVerseMapBookFirst = m_mapVerses.lowerBound(CRelIndex(itrVerseMapBookFirst.key().book() + 1, 0, 0, 0));
		assert(itrVerseMapBookFirst != m_mapVerses.end());		// Shouldn't hit the end because we already know the correct book exists
		++nIndex;
	}
	return nIndex;
}

unsigned int CVerseListModel::TVerseListModelResults::BookByIndex(int ndxBook) const
{
	assert(m_private->m_pBibleDatabase != NULL);

	if (m_private->m_bShowMissingLeafs) {
		if ((ndxBook < 0) || (static_cast<unsigned int>(ndxBook) >= m_private->m_pBibleDatabase->bibleEntry().m_nNumBk)) return 0;
		return (ndxBook+1);
	}

	CVerseMap::const_iterator itrVerseMapBookFirst = m_mapVerses.begin();
	CVerseMap::const_iterator itrVerseMapBookLast = m_mapVerses.end();

	int nIndex = 0;
	while (itrVerseMapBookFirst != itrVerseMapBookLast) {
		if (nIndex == ndxBook) return itrVerseMapBookFirst.key().book();			// If we've found the right index, return the book
		// Find next book (bypassing any chapters/verses in the current book):
		itrVerseMapBookFirst = m_mapVerses.lowerBound(CRelIndex(itrVerseMapBookFirst.key().book() + 1, 0, 0, 0));
		++nIndex;
	}
	assert(false);
	return 0;				// Should have already returned a chapter above, but 0 if we're given an index beyond the list
}

int CVerseListModel::TVerseListModelResults::GetChapterCount(unsigned int nBk) const
{
	assert(m_private->m_pBibleDatabase != NULL);

	if (nBk == 0) return 0;
	if (m_private->m_bShowMissingLeafs) {
		if (nBk > m_private->m_pBibleDatabase->bibleEntry().m_nNumBk) return 0;
		return m_private->m_pBibleDatabase->bookEntry(nBk)->m_nNumChp;
	}

	// Find the first and last entries with the correct Book number:
	CVerseMap::const_iterator itrVerseMapBookChapterFirst;
	CVerseMap::const_iterator itrVerseMapBookChapterLast;
	itrVerseMapBookChapterFirst = m_mapVerses.lowerBound(CRelIndex(nBk, 0, 0, 0));			// This will be the first verse of the first chapter of this book
	itrVerseMapBookChapterLast = m_mapVerses.lowerBound(CRelIndex(nBk+1, 0, 0, 0));			// This will be the first verse of the next book/chapter

	if (itrVerseMapBookChapterFirst == m_mapVerses.end()) return 0;
	if (itrVerseMapBookChapterFirst.key().book() != nBk) return 0;

	int nCount = 0;
	while (itrVerseMapBookChapterFirst != itrVerseMapBookChapterLast) {
		// Find next chapter (bypassing any verses in the current chapter):
		itrVerseMapBookChapterFirst = m_mapVerses.lowerBound(CRelIndex(nBk, itrVerseMapBookChapterFirst.key().chapter() + 1, 0, 0));
		++nCount;
	}
	return nCount;
}

int CVerseListModel::TVerseListModelResults::IndexByChapter(unsigned int nBk, unsigned int nChp) const
{
	assert(m_private->m_pBibleDatabase);

	if ((nBk == 0) || (nChp == 0)) return -1;
	if (m_private->m_bShowMissingLeafs) {
		if (nBk > m_private->m_pBibleDatabase->bibleEntry().m_nNumBk) return -1;
		if (nChp > m_private->m_pBibleDatabase->bookEntry(nBk)->m_nNumChp) return -1;
		return (nChp-1);
	}

	// Find the first entry with the correct Book number and with the correct Book/Chapter number:
	CVerseMap::const_iterator itrVerseMapBook = m_mapVerses.lowerBound(CRelIndex(nBk, 0, 0, 0));
	CVerseMap::const_iterator itrVerseMapBookChapter = m_mapVerses.lowerBound(CRelIndex(nBk, nChp, 0, 0));

	// If we didn't find the book and/or book/chapter, return -1 (not found):
	if ((itrVerseMapBook == m_mapVerses.end()) || (itrVerseMapBookChapter == m_mapVerses.end())) return -1;
	if (itrVerseMapBook.key().book() != nBk) return -1;
	if ((itrVerseMapBookChapter.key().book() != nBk) || (itrVerseMapBookChapter.key().chapter() != nChp)) return -1;

	int nIndex = 0;
	while (itrVerseMapBook != itrVerseMapBookChapter) {
		// Find next chapter (bypassing any verses in the current chapter):
		itrVerseMapBook = m_mapVerses.lowerBound(CRelIndex(nBk, itrVerseMapBook.key().chapter() + 1, 0, 0));
		assert(itrVerseMapBook != m_mapVerses.end());		// Shouldn't hit the end because we already know the correct book/chapter exists
		++nIndex;
	}
	return nIndex;
}

unsigned int CVerseListModel::TVerseListModelResults::ChapterByIndex(int ndxBook, int ndxChapter) const
{
	assert(m_private->m_pBibleDatabase);

	if ((ndxBook < 0) || (ndxChapter < 0)) return 0;
	if (m_private->m_bShowMissingLeafs) {
		if (static_cast<unsigned int>(ndxBook) >= m_private->m_pBibleDatabase->bibleEntry().m_nNumBk) return 0;
		if (static_cast<unsigned int>(ndxChapter) >= m_private->m_pBibleDatabase->bookEntry(ndxBook+1)->m_nNumChp) return 0;
		return (ndxChapter+1);
	}

	unsigned int nBk = BookByIndex(ndxBook);
	if (nBk == 0) return 0;

	// Find the first and last entries with the correct Book number:
	CVerseMap::const_iterator itrVerseMapBookChapterFirst = m_mapVerses.lowerBound(CRelIndex(nBk, 0, 0, 0));		// This will be the first verse of the first chapter of this book
	CVerseMap::const_iterator itrVerseMapBookChapterLast = m_mapVerses.lowerBound(CRelIndex(nBk+1, 0, 0, 0));		// This will be the first verse of the next book/chapter

	// We should have found the book, because of the above BookByIndex() call and nBk check, but safe-guard:
	assert(itrVerseMapBookChapterFirst != m_mapVerses.end());
	if (itrVerseMapBookChapterFirst == m_mapVerses.end()) return 0;

	int nIndex = 0;
	while (itrVerseMapBookChapterFirst != itrVerseMapBookChapterLast) {
		if (nIndex == ndxChapter) return itrVerseMapBookChapterFirst.key().chapter();			// If we've found the right index, return the chapter
		// Find next chapter (bypassing any verses in the current chapter):
		itrVerseMapBookChapterFirst = m_mapVerses.lowerBound(CRelIndex(nBk, itrVerseMapBookChapterFirst.key().chapter() + 1, 0, 0));
		++nIndex;
	}
	assert(false);
	return 0;				// Should have already returned a chapter above, but 0 if we're given an index beyond the list
}

CVerseMap::const_iterator CVerseListModel::TVerseListModelResults::FindVerseIndex(const CRelIndex &ndxRel) const
{
	if (!ndxRel.isSet()) return m_mapVerses.constEnd();

	CRelIndex ndxSearch(ndxRel);
	ndxSearch.setWord(0);			// Make sure we don't consider the word
	return m_mapVerses.find(ndxSearch);
}

CVerseMap::const_iterator CVerseListModel::TVerseListModelResults::GetVerse(int ndxVerse, unsigned int nBk, unsigned int nChp) const
{
	// Note: This function has a special case for nBk == 0 and nChp == 0 (unlike the other index functions)

	if (ndxVerse < 0) return m_mapVerses.constEnd();

	if ((nBk == 0) && (nChp == 0)) {
		assert((ndxVerse >= 0) && (ndxVerse < m_lstVerseIndexes.size()));
		if (ndxVerse >= m_lstVerseIndexes.size()) return m_mapVerses.constEnd();	// Note: (ndxVerse < 0) is handled above for both Map/List methods
		return m_mapVerses.find(m_lstVerseIndexes.at(ndxVerse));
	}

	// Find the first and last entries with the correct Book/Chapter number:
	CVerseMap::const_iterator itrVerseMapBookChapterFirst;
	CVerseMap::const_iterator itrVerseMapBookChapterLast;
	itrVerseMapBookChapterFirst = m_mapVerses.lowerBound(CRelIndex(nBk, nChp, 0, 0));			// This will be the first verse of this chapter of this book
	if (nChp != 0) {
		itrVerseMapBookChapterLast = m_mapVerses.lowerBound(CRelIndex(nBk, nChp+1, 0, 0));	// This will be the first verse of the next book/chapter
	} else {
		itrVerseMapBookChapterLast = m_mapVerses.lowerBound(CRelIndex(nBk+1, 0, 0, 0));
	}

	// If we didn't find the book and/or book/chapter, return (not found):
	if (itrVerseMapBookChapterFirst == m_mapVerses.end()) return m_mapVerses.constEnd();
	if ((itrVerseMapBookChapterFirst.key().book() != nBk) ||
		((nChp != 0) && (itrVerseMapBookChapterFirst.key().chapter() != nChp))) return m_mapVerses.constEnd();

	int nVerses = 0;
	while (itrVerseMapBookChapterFirst != itrVerseMapBookChapterLast) {
		if (nVerses == ndxVerse) return itrVerseMapBookChapterFirst;
		++itrVerseMapBookChapterFirst;
		++nVerses;
	}
	assert(false);
	return m_mapVerses.constEnd();			// Should have already returned a verse above, but end() if we're given an index beyond the list
}

// ----------------------------------------------------------------------------

int CVerseListModel::TVerseListModelResults::GetVerseCount(unsigned int nBk, unsigned int nChp) const
{
	// Note: This function has special cases for nBk == 0 and nChp == 0 (unlike the other count functions)

	if (nBk == 0) return m_mapVerses.size();		// Quick special-case

	// Find the first and last entries with the correct Book/Chapter number:
	CVerseMap::const_iterator itrVerseMapBookChapterFirst;
	CVerseMap::const_iterator itrVerseMapBookChapterLast;
	itrVerseMapBookChapterFirst = m_mapVerses.lowerBound(CRelIndex(nBk, nChp, 0, 0));			// This will be the first verse of this chapter of this book
	if (nChp != 0) {
		itrVerseMapBookChapterLast = m_mapVerses.lowerBound(CRelIndex(nBk, nChp+1, 0, 0));		// This will be the first verse of the next book/chapter
	} else {
		itrVerseMapBookChapterLast = m_mapVerses.lowerBound(CRelIndex(nBk+1, 0, 0, 0));			// This will be the first verse of the next book
	}

	// If we didn't find the book and/or book/chapter, return none found:
	if (itrVerseMapBookChapterFirst == m_mapVerses.end()) return 0;
	if ((itrVerseMapBookChapterFirst.key().book() != nBk) || ((nChp != 0) && (itrVerseMapBookChapterFirst.key().chapter() != nChp))) return 0;

	int nVerses = 0;
	while (itrVerseMapBookChapterFirst != itrVerseMapBookChapterLast) {
		++itrVerseMapBookChapterFirst;
		++nVerses;
	}
	return nVerses;
}

int CVerseListModel::TVerseListModelResults::GetResultsCount(unsigned int nBk, unsigned int nChp) const
{
	int nResults = 0;

	for (CVerseMap::const_iterator itrVerse = m_mapVerses.constBegin(); itrVerse != m_mapVerses.constEnd(); ++itrVerse) {
		if ((nBk != 0) && (itrVerse.key().book() != nBk)) continue;
		if ((nBk != 0) && (nChp != 0) && (itrVerse.key().chapter() != nChp)) continue;
		nResults += itrVerse->phraseTags().size();
	}

	return nResults;
}

// ----------------------------------------------------------------------------

int CVerseListModel::GetVerseCount(unsigned int nBk, unsigned int nChp) const
{
	if (m_private.m_nViewMode == VVME_SEARCH_RESULTS) return m_searchResults.GetVerseCount(nBk, nChp);

	int nCount = 0;
	for (THighlighterVLMRList::const_iterator itrHighlighter = m_vlmrListHighlighters.constBegin(); itrHighlighter != m_vlmrListHighlighters.constEnd(); ++itrHighlighter) {
		nCount += itrHighlighter->GetVerseCount(nBk, nChp);
	}

	return nCount;
}

int CVerseListModel::GetResultsCount(unsigned int nBk, unsigned int nChp) const
{
	if (m_private.m_nViewMode == VVME_SEARCH_RESULTS) return m_searchResults.GetResultsCount(nBk, nChp);

	int nCount = 0;
	for (THighlighterVLMRList::const_iterator itrHighlighter = m_vlmrListHighlighters.constBegin(); itrHighlighter != m_vlmrListHighlighters.constEnd(); ++itrHighlighter) {
		nCount += itrHighlighter->GetResultsCount(nBk, nChp);
	}

	return nCount;
}

// ----------------------------------------------------------------------------

void CVerseListModel::clearAllSizeHints()
{
	m_searchResults.m_mapSizeHints.clear();
	for (THighlighterVLMRList::iterator itrHighlighter = m_vlmrListHighlighters.begin(); itrHighlighter != m_vlmrListHighlighters.end(); ++itrHighlighter) {
		itrHighlighter->m_mapSizeHints.clear();
	}
}

void CVerseListModel::clearAllExtraVerseIndexes()
{
	m_searchResults.m_mapExtraVerseIndexes.clear();
	for (THighlighterVLMRList::iterator itrHighlighter = m_vlmrListHighlighters.begin(); itrHighlighter != m_vlmrListHighlighters.end(); ++itrHighlighter) {
		itrHighlighter->m_mapExtraVerseIndexes.clear();
	}
}

// ----------------------------------------------------------------------------

void CVerseListModel::buildScopedResultsFromParsedPhrases()
{
	TVerseListModelSearchResults &zResults = m_searchResults;

	QList<TPhraseTagList::const_iterator> lstItrStart;
	QList<TPhraseTagList::const_iterator> lstItrEnd;
	QList<CRelIndex> lstScopedRefs;
	QList<bool> lstNeedScope;
	int nNumPhrases = zResults.m_lstParsedPhrases.size();

	emit verseListAboutToChange();
	emit beginResetModel();

	zResults.m_mapVerses.clear();
	zResults.m_lstVerseIndexes.clear();
	zResults.m_mapExtraVerseIndexes.clear();
	zResults.m_mapSizeHints.clear();

	// Fetch results from all phrases and build a list of lists, denormalizing entries, and
	//		setting the phrase size details:
	for (int ndx=0; ndx<nNumPhrases; ++ndx) {
		const TPhraseTagList &lstSearchResultsPhraseTags = zResults.m_lstParsedPhrases.at(ndx)->GetPhraseTagSearchResults();
		lstItrStart.append(lstSearchResultsPhraseTags.constBegin());
		lstItrEnd.append(lstSearchResultsPhraseTags.constBegin());
		lstScopedRefs.append(CRelIndex());
		lstNeedScope.append(true);
	}

	// Now, we'll go through our lists and compress the results to the scope specified
	//		for each phrase.  We'll then find the lowest valued one and see if the others
	//		match.  If they do, we'll push all of those results onto the output.  If not,
	//		we'll toss results for the lowest until we get a match.  When any list hits
	//		its end, we're done and can break out since we have no more matches

	bool bDone = (nNumPhrases == 0);		// We're done if we have no phrases (or phrases with results)
	while (!bDone) {
		uint32_t nMaxScope = 0;
		for (int ndx=0; ndx<nNumPhrases; ++ndx) {
			const CParsedPhrase *phrase = zResults.m_lstParsedPhrases.at(ndx);
			const TPhraseTagList &lstSearchResultsPhraseTags = phrase->GetPhraseTagSearchResults();
			if (!lstNeedScope[ndx]) {
				nMaxScope = qMax(nMaxScope, lstScopedRefs[ndx].index());
				continue;		// Only find next scope for a phrase if we need it
			}
			lstItrStart[ndx] = lstItrEnd[ndx];		// Begin at the last ending position
			if (lstItrStart[ndx] == lstSearchResultsPhraseTags.constEnd()) {
				bDone = true;
				break;
			}
			lstScopedRefs[ndx] = ScopeIndex(lstItrStart[ndx]->relIndex(), zResults.m_SearchCriteria.searchScopeMode());
			for (lstItrEnd[ndx] = lstItrStart[ndx]+1; lstItrEnd[ndx] != lstSearchResultsPhraseTags.constEnd(); ++lstItrEnd[ndx]) {
				CRelIndex ndxScopedTemp = ScopeIndex(lstItrEnd[ndx]->relIndex(), zResults.m_SearchCriteria.searchScopeMode());
				if (lstScopedRefs[ndx].index() != ndxScopedTemp.index()) break;
			}
			// Here lstItrEnd will be one more than the number of matching, either the next index
			//		off the end of the array, or the first non-matching entry.  So the scoped
			//		area is from lstItrStart to lstItrEnd-1.
			nMaxScope = qMax(nMaxScope, lstScopedRefs[ndx].index());
			lstNeedScope[ndx] = false;
		}
		if (bDone) continue;		// If we run out of phrase matches on any phrase, we're done
		// Now, check the scoped references.  If they match for all indexes, we'll push the
		//	results to our output and set flags to get all new scopes.  Otherwise, compare them
		//	all against our maximum scope value and tag any that's less than that as needing a
		//	new scope (they weren't matches).  Then loop back until we've either pushed all
		//	results or run out of matches.
		bool bMatch = true;
		for (int ndx=0; ndx<nNumPhrases; ++ndx) {
			if (lstScopedRefs[ndx].index() != nMaxScope) {
				lstNeedScope[ndx] = true;
				bMatch = false;
			}
		}
		if (bMatch) {
			// We got a match, so push results to output and flag for new scopes:
			for (int ndx=0; ndx<nNumPhrases; ++ndx) {
				TPhraseTagList &lstScopedPhraseTags = zResults.m_lstParsedPhrases.at(ndx)->GetScopedPhraseTagSearchResultsNonConst();
				lstScopedPhraseTags.reserve(lstScopedPhraseTags.size() + std::distance(lstItrStart[ndx], lstItrEnd[ndx]));
				for (TPhraseTagList::const_iterator itr = lstItrStart[ndx]; itr != lstItrEnd[ndx]; ++itr) {
					lstScopedPhraseTags.append(*itr);
					CRelIndex ndxNextRelative = itr->relIndex();
					ndxNextRelative.setWord(0);
					if (zResults.m_mapVerses.contains(ndxNextRelative)) {
						zResults.m_mapVerses[ndxNextRelative].addPhraseTag(*itr);
					} else {
						zResults.m_mapVerses.insert(ndxNextRelative, CVerseListItem(TVerseIndex(ndxNextRelative, zResults.highlighterIndex()), m_private.m_pBibleDatabase, *itr));
					}
				}
				lstNeedScope[ndx] = true;
			}
		}
	}

	zResults.m_lstVerseIndexes.reserve(zResults.m_mapVerses.size());
	for (CVerseMap::const_iterator itr = zResults.m_mapVerses.constBegin(); (itr != zResults.m_mapVerses.constEnd()); ++itr) {
		zResults.m_lstVerseIndexes.append(itr.key());
	}

	emit endResetModel();
	emit verseListChanged();
}

CRelIndex CVerseListModel::ScopeIndex(const CRelIndex &index, CSearchCriteria::SEARCH_SCOPE_MODE_ENUM nMode)
{
	assert(m_private.m_pBibleDatabase);

	CRelIndex indexScoped;

	switch (nMode) {
		case (CSearchCriteria::SSME_WHOLE_BIBLE):
			// For Whole Bible, we'll set the Book to 1 so that anything in the Bible matches:
			if (index.isSet()) indexScoped = CRelIndex(1, 0, 0, 0);
			break;
		case (CSearchCriteria::SSME_TESTAMENT):
			// For Testament, set the Book to the 1st Book of the corresponding Testament:
			if (index.book()) {
				if (index.book() <= m_private.m_pBibleDatabase->bibleEntry().m_nNumBk) {
					const CBookEntry &book = *m_private.m_pBibleDatabase->bookEntry(index.book());
					unsigned int nTestament = book.m_nTstNdx;
					unsigned int nBook = 1;
					for (unsigned int i=1; i<nTestament; ++i)
						nBook += m_private.m_pBibleDatabase->testamentEntry(i)->m_nNumBk;
					indexScoped = CRelIndex(nBook, 0, 0 ,0);
				}
			}
			break;
		case (CSearchCriteria::SSME_BOOK):
			// For Book, mask off Chapter, Verse, and Word:
			indexScoped = CRelIndex(index.book(), 0, 0, 0);
			break;
		case (CSearchCriteria::SSME_CHAPTER):
			// For Chapter, mask off Verse and Word:
			indexScoped = CRelIndex(index.book(), index.chapter(), 0, 0);
			break;
		case (CSearchCriteria::SSME_VERSE):
			// For Verse, mask off word:
			indexScoped = CRelIndex(index.book(), index.chapter(), index.verse(), 0);
			break;
		default:
			break;
	}

	return indexScoped;
}

// ----------------------------------------------------------------------------

void CVerseListModel::setFont(const QFont& aFont)
{
	m_private.m_font = aFont;
	emit layoutAboutToBeChanged();
	setData(QModelIndex(), QSize(), Qt::SizeHintRole);			// Invalidate all sizeHints on fontChange
	emit layoutChanged();
}

void CVerseListModel::en_WordsOfJesusColorChanged(const QColor &color)
{
	m_private.m_richifierTags.setWordsOfJesusTagsByColor(color);
}

// ============================================================================

