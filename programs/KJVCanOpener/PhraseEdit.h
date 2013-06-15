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

#ifndef PHRASEEDIT_H
#define PHRASEEDIT_H

#include "dbstruct.h"
#include "Highlighter.h"
#include "VerseRichifier.h"

#include <QTextDocument>
#include <QTextEdit>
#include <QTextCursor>
#include <QCompleter>
#include <QString>
#include <QStringList>
#include <QColor>
#include <QHelpEvent>
#include <QList>

// ============================================================================

class CParsedPhrase
{
public:
	CParsedPhrase(CBibleDatabasePtr pBibleDatabase = CBibleDatabasePtr(), bool bCaseSensitive = false)
		:	m_pBibleDatabase(pBibleDatabase),
			m_bIsDuplicate(false),
			m_bCaseSensitive(bCaseSensitive),
			m_nLevel(0),
			m_nCursorLevel(0),
			m_nCursorWord(-1),
			m_nLastMatchWord(-1)
	{ }
	virtual ~CParsedPhrase()
	{ }

	// ------- Helpers functions for data maintained by controlling CKJVCanOpener class to
	//			use for maintaining statistics about this phrase in context with others and
	//			to build Search Results Verse Lists, do highlighting, etc.
	inline bool IsDuplicate() const { return m_bIsDuplicate; }
	inline void SetIsDuplicate(bool bIsDuplicate) const { m_bIsDuplicate = bIsDuplicate; }
	inline int GetContributingNumberOfMatches() const { return m_lstScopedPhraseTagResults.size(); }
	inline const TPhraseTagList &GetScopedPhraseTagSearchResults() const { return m_lstScopedPhraseTagResults; }			// Returned as reference so we don't have to keep copying
	inline TPhraseTagList &GetScopedPhraseTagSearchResultsNonConst() const { return m_lstScopedPhraseTagResults; }			// Non-const version used by VerseListModel for setting
	inline void ClearScopedPhraseTagSearchResults() const { m_lstScopedPhraseTagResults.clear(); }
	// -------
	bool isCompleteMatch() const { return (GetMatchLevel() == phraseSize()); }
	uint32_t GetNumberOfMatches() const;
	const TIndexList &GetNormalizedSearchResults() const;			// Returned as reference so we don't have to keep copying
	const TPhraseTagList &GetPhraseTagSearchResults() const;		// Returned as reference so we don't have to keep copying
	uint32_t GetMatchLevel() const;
	uint32_t GetCursorMatchLevel() const;
	QString GetCursorWord() const;
	int GetCursorWordPos() const;
	QString phrase() const;						// Return reconstituted phrase
	QString phraseRaw() const;					// Return reconstituted phrase without punctuation or regexp symbols
	unsigned int phraseSize() const;			// Return number of words in reconstituted phrase
	unsigned int phraseRawSize() const;			// Return number of words in reconstituted raw phrase
	QStringList phraseWords() const;			// Return reconstituted phrase words
	QStringList phraseWordsRaw() const;			// Return reconstituted raw phrase words
	static QString makeRawPhrase(const QString &strPhrase);

	virtual void ParsePhrase(const QTextCursor &curInsert);		// Parses the phrase in the editor.  Sets m_lstWords and m_nCursorWord
	virtual void ParsePhrase(const QString &strPhrase);			// Parses a fixed phrase
	virtual void ParsePhrase(const QStringList &lstPhrase);		// Parses a fixed phrase already divided into words (like getSelectedPhrase from CPhraseNavigator)

	virtual bool isCaseSensitive() const { return m_bCaseSensitive; }
	virtual void setCaseSensitive(bool bCaseSensitive) { m_bCaseSensitive = bCaseSensitive; }

	bool operator==(const CParsedPhrase &src) const
	{
		return ((m_bCaseSensitive == src.m_bCaseSensitive) &&
				(phrase().compare(src.phrase(), Qt::CaseSensitive) == 0));
	}

	bool operator==(const CPhraseEntry &src) const
	{
		return ((m_bCaseSensitive == src.m_bCaseSensitive) &&
				(phrase().compare(src.m_strPhrase, Qt::CaseSensitive) == 0));
	}

	void UpdateCompleter(const QTextCursor &curInsert, QCompleter &aCompleter);
	QTextCursor insertCompletion(const QTextCursor &curInsert, const QString& completion);
	void clearCache() const;

private:
	void FindWords();			// Uses m_lstWords and m_nCursorWord to populate m_lstNextWords, m_lstMapping, and m_nLevel

protected:
	CBibleDatabasePtr m_pBibleDatabase;
	mutable QStringList m_cache_lstPhraseWords;				// Cached Phrase Words (Set on call to phraseWords, cleared on ClearCache)
	mutable QStringList m_cache_lstPhraseWordsRaw;			// Cached Raw Phrase Words (Set on call to phraseWordsRaw, cleared on ClearCache)
	mutable TIndexList m_cache_lstNormalizedSearchResults;	// Cached Normalized Search Results (Set on call to GetNormalizedSearchResults, cleared on ClearCache)
	mutable TPhraseTagList m_cache_lstPhraseTagResults;		// Cached Denormalized Search Results converted to phrase tags (Set on call to GetPhraseTagSearchResults, cleared on ClearCache, uses GetNormalizedSearchResults internally)
	// -------
	mutable bool m_bIsDuplicate;					// Indicates this phrase is exact duplicate of another phrase.  Set/Cleared by parent phraseChanged logic.
	mutable TPhraseTagList m_lstScopedPhraseTagResults;		// List of Denormalized Search Results from Scope.  Set/Cleared by parent phraseChanged logic and buildScopedResultsInParsedPhrases on VerseListModel.  The size of this list is the ContributingMatchCount
	// -------
	bool m_bCaseSensitive;
	uint32_t m_nLevel;			// Level of the search (Number of words matched).  This is the offset value for entries in m_lstMatchMapping (at 0 mapping is ALL words) (Set by FindWords())
	TIndexList m_lstMatchMapping;	// Mapping for entire search -- This is the search result, but with each entry offset by the search level (Set by FindWords())
	uint32_t m_nCursorLevel;	// Matching level at cursor
	TIndexList m_lstMapping;	// Mapping for search through current cursor (Set by FindWords())
	QStringList m_lstNextWords;	// List of words mapping next for this phrase (Set by FindWords()) (Stored as decomposed-normalized strings to help sorting order in auto-completer)

	QStringList m_lstWords;		// Fully Parsed Word list.  Blank entries only at first or last entry to indicate an insertion point. (Filled by ParsePhrase())
	int m_nCursorWord;			// Index in m_lstWords where the cursor is at -- If insertion point is in the middle of two words, Cursor will be at the left word (Set by ParsePhrase())
	int m_nLastMatchWord;		// Index in m_lstWords where the last match was found (Set by FindWords())

	QStringList m_lstLeftWords;		// Raw Left-hand Words list from extraction.  Punctionation appears clustered in separate entities (Set by ParsePhrase())
	QStringList m_lstRightWords;	// Raw Right-hand Words list from extraction.  Punctionation appears clustered in separate entities (Set by ParsePhrase())
	QString m_strCursorWord;	// Word at the cursor point between the left and right hand halves (Set by ParsePhrase())
};

typedef QList <const CParsedPhrase *> TParsedPhrasesList;

class CSelectedPhrase
{
public:
	CSelectedPhrase(CBibleDatabasePtr pBibleDatabase, bool bCaseSensitive = false)
		:	m_ParsedPhrase(pBibleDatabase, bCaseSensitive)
	{ }

	inline const CParsedPhrase &phrase() const { return m_ParsedPhrase; }
	inline CParsedPhrase &phrase() { return m_ParsedPhrase; }
	inline const TPhraseTag &tag() const { return m_Tag; }
	inline TPhraseTag &tag() { return m_Tag; }

private:
	CParsedPhrase m_ParsedPhrase;
	TPhraseTag m_Tag;
};

// ============================================================================

class CPhraseCursor : public QTextCursor
{
public:
	CPhraseCursor(const QTextCursor &aCursor);
	CPhraseCursor(const CPhraseCursor &aCursor);
	CPhraseCursor(QTextDocument *pDocument);
	virtual ~CPhraseCursor();

	bool moveCursorCharLeft(MoveMode mode = MoveAnchor);
	bool moveCursorCharRight(MoveMode mode = MoveAnchor);
	inline QChar charUnderCursor();

	bool moveCursorWordLeft(MoveMode mode = MoveAnchor);
	bool moveCursorWordRight(MoveMode mode = MoveAnchor);
	bool moveCursorWordStart(MoveMode mode = MoveAnchor);
	bool moveCursorWordEnd(MoveMode mode = MoveAnchor);
	QString wordUnderCursor();

	void selectWordUnderCursor();
	void selectCursorToLineStart();
	void selectCursorToLineEnd();
};

// ============================================================================

class CPhraseNavigator : public QObject
{
	Q_OBJECT
public:
	CPhraseNavigator(CBibleDatabasePtr pBibleDatabase, QTextDocument &textDocument, QObject *parent = NULL)
		:	QObject(parent),
			m_pBibleDatabase(pBibleDatabase),
			m_TextDocument(textDocument)
	{ }

	// AnchorPosition returns the document postion for the specified anchor or -1 if none found:
	int anchorPosition(const QString &strAnchorName) const;

	// Highlight the areas marked in the PhraseTags.  If bClear=True, removes
	//		the highlighting, which is used to swapout the current tag list
	//		for a new one without redrawing everything.  ndxCurrent is used
	//		as an optimization to skip areas not within current chapter.  Use
	//		empty index to ignore.  Highlighting is done in the specified
	//		color.
	void doHighlighting(const CBasicHighlighter &aHighlighter, bool bClear = false, const CRelIndex &ndxCurrent = CRelIndex()) const;

	// Text Fill Functions:
	void setDocumentToChapter(const CRelIndex &ndx, bool bNoAnchors = false);
	void setDocumentToVerse(const CRelIndex &ndx, bool bAddDividerLineBefore = false, bool bNoAnchors = false);
	void setDocumentToFormattedVerses(const TPhraseTag &tag);		// Note: By definition, this one doesn't include anchors

	TPhraseTag getSelection(const CPhraseCursor &aCursor) const;				// Returns the tag for the cursor's currently selected text (less expensive than getSelectPhrase since we don't have to generate the CParsedPhrase object)
	CSelectedPhrase getSelectedPhrase(const CPhraseCursor &aCursor) const;		// Returns the parsed phrase and tag for the cursor's currently selected text

	void removeAnchors();

signals:
	void changedDocumentText();

protected:
	CBibleDatabasePtr m_pBibleDatabase;
	CVerseTextRichifierTags m_richifierTags;	// Richifier tags used to render the text in this browser document

private:
	QTextDocument &m_TextDocument;
};

// ============================================================================

class CPhraseEditNavigator : public CPhraseNavigator
{
	Q_OBJECT
public:
	CPhraseEditNavigator(CBibleDatabasePtr pBibleDatabase, QTextEdit &textEditor, bool bUseToolTipEdit = true, QObject *parent = NULL)
		:	CPhraseNavigator(pBibleDatabase, *textEditor.document(), parent),
			m_TextEditor(textEditor),
			m_bUseToolTipEdit(bUseToolTipEdit)
	{
		assert(m_pBibleDatabase.data() != NULL);
	}

	enum TOOLTIP_TYPE_ENUM {
		TTE_COMPLETE = 0,
		TTE_REFERENCE_ONLY = 1,
		TTE_STATISTICS_ONLY = 2
	};

	// Text Selection/ToolTip Functions:
	void selectWords(const TPhraseTag &tag);
	using CPhraseNavigator::getSelection;
	using CPhraseNavigator::getSelectedPhrase;
	TPhraseTag getSelection() const;				// Returns the tag for the cursor's currently selected text (less expensive than getSelectPhrase since we don't have to generate the CParsedPhrase object)
	CSelectedPhrase getSelectedPhrase() const;		// Returns the parsed phrase and tag for the cursor's currently selected text
	bool handleToolTipEvent(const QHelpEvent *pHelpEvent, CBasicHighlighter &aHighlighter, const TPhraseTag &selection) const;
	bool handleToolTipEvent(CBasicHighlighter &aHighlighter, const TPhraseTag &tag, const TPhraseTag &selection) const;
	void highlightTag(CBasicHighlighter &aHighlighter, const TPhraseTag &tag = TPhraseTag()) const;
	QString getToolTip(const TPhraseTag &tag, const TPhraseTag &selection, TOOLTIP_TYPE_ENUM nToolTipType = TTE_COMPLETE, bool bPlainText = false) const;

private:
	QTextEdit &m_TextEditor;
	bool m_bUseToolTipEdit;			// True = Use CToolTipEdit instead of QToolTip
};

// ============================================================================

#endif // PHRASEEDIT_H
