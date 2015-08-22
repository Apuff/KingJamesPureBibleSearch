/****************************************************************************
**
** Copyright (C) 2015 Donna Whisnant, a.k.a. Dewtronics.
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

#ifndef WEBCHANNEL_OBJECTS_H
#define WEBCHANNEL_OBJECTS_H

#include "dbstruct.h"
#include "VerseListModel.h"
#include "headlessSearchResults.h"

#include <QObject>
#include <QPointer>

// Forward declarations:


//
// CWebChannelObjects
//
class CWebChannelObjects : public QObject
{
	Q_OBJECT

public:
	CWebChannelObjects(QObject *pParent = NULL);
	virtual ~CWebChannelObjects();

public slots:
	void selectBible(const QString &strUUID);

	void setSearchPhrases(const QString &strPhrases);		// Phrases, separated by semicolon, to search for
	void autoCorrect(const QString &strElementID, const QString &strPhrase, int nCursorPos);			// Returns HTML Auto-Correction string for passed phrase and triggers autoCompleter list
	void calcUpdatedPhrase(const QString &strElementID, const QString &strPhrase, const QString &strAutoCompleter, int nCursorPos);		// Runs Phrase Parser and determines current subphrase.  Replaces that subphrase with passed strAutoCompleter value

	void gotoIndex(uint32_t ndxRel);						// Passage to navigate Scripture Browser to

signals:
	void bibleSelected(bool bSuccess);

	void searchResultsChanged(const QString &strHtmlLiSearchResults);					// Triggered by en_searchResultsReady() when we have data to send to channel
	void setAutoCorrectText(const QString &strElementID, const QString &strAC);			// Triggered after call to autoCorrect() will return the HTML of the Auto Correct text
	void setAutoCompleter(const QString &strElementID, const QString &strWordList);		// Triggered after call to autoCorrect() will return a list of words separated by ";"
	void updatePhrase(const QString &strElementID, const QString &strNewPhrase);		// Triggered after call to calcUpdatedPhrase() to return new phrase with subphrase substituted

	void scriptureBrowserRender(uint32_t ndxRel, const QString &strHtmlScripture);		// Triggered by scripture browser navigation to display rendered text

protected:

protected slots:
	void en_searchResultsReady();							// Called by verseListModel in CHeadlessSearchResults when results have been updated


private:
	QPointer<CHeadlessSearchResults> m_pSearchResults;		// Search Results that we are controlling
	CSearchResultsData m_searchResultsData;					// Data (phrases and criteria) that we are using
	TSharedParsedPhrasesList m_lstParsedPhrases;			// Phrase parsers
};

#endif	// WEBCHANNEL_OBJECTS_H
