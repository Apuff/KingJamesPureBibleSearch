#ifndef DBSTRUCT_H
#define DBSTRUCT_H

#include <string.h>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <stdint.h>
#include <QString>
#include <QStringList>
#include <QList>
#include <QVariant>
#include <QPair>

#ifndef uint32_t
#define uint32_t unsigned int
#endif


// ============================================================================

// CRelIndex Masks:
#define RIMASK_HEADING	0x10
#define RIMASK_BOOK		0x08
#define RIMASK_CHAPTER	0x04
#define RIMASK_VERSE	0x02
#define RIMASK_WORD		0x01
#define RIMASK_ALL		0x1F

class CRelIndex {
public:
	CRelIndex(uint32_t ndx = 0) :
		m_ndx(ndx)
	{
	}
	CRelIndex(const QString &strAnchor) :
		m_ndx(strAnchor.toUInt())
	{
	}
	CRelIndex(uint32_t nBk, uint32_t nChp, uint32_t nVrs, uint32_t nWrd)
	{
		setIndex(nBk, nChp, nVrs, nWrd);
	}
	~CRelIndex() { }

	QString asAnchor() const {					// Anchor is a text string unique to this reference
		return QString("%1").arg(m_ndx);
	}

	QString SearchResultToolTip(int nRIMask = RIMASK_ALL) const;		// Create complete reference statistics report
	QString PassageReferenceText() const;		// Creates a reference text string like "Genesis 1:1 [5]"

	QString testamentName() const;
	uint32_t testament() const;

	QString bookName() const;
	uint32_t book() const { return ((m_ndx >> 24) & 0xFF); }
	void setBook(uint32_t nBk) {
		m_ndx = ((m_ndx & 0x00FFFFFF) | ((nBk & 0xFF) << 24));
	}
	uint32_t chapter() const { return ((m_ndx >> 16) & 0xFF); }
	void setChapter(uint32_t nChp) {
		m_ndx = ((m_ndx & 0xFF00FFFF) | ((nChp & 0xFF) << 16));
	}
	uint32_t verse() const { return ((m_ndx >> 8) & 0xFF); }
	void setVerse(uint32_t nVrs) {
		m_ndx = ((m_ndx & 0xFFFF00FF) | ((nVrs & 0xFF) << 8));
	}
	uint32_t word() const { return (m_ndx & 0xFF); }
	void setWord(uint32_t nWrd) {
		m_ndx = ((m_ndx & 0xFFFFFF00) | (nWrd & 0xFF));
	}
	bool isSet() const { return (m_ndx != 0); }

	uint32_t index() const { return m_ndx; }
	void setIndex(uint32_t nBk, uint32_t nChp, uint32_t nVrs, uint32_t nWrd) {
		m_ndx = (((nBk & 0xFF) << 24) | ((nChp & 0xFF) << 16) | ((nVrs & 0xFF) << 8) | (nWrd & 0xFF));
	}

private:
	uint32_t m_ndx;
};

extern uint32_t NormalizeIndex(const CRelIndex &nRelIndex);
extern uint32_t NormalizeIndex(uint32_t nRelIndex);
extern uint32_t DenormalizeIndex(uint32_t nNormalIndex);

// ============================================================================

class CRefCountCalc			// Calculates the reference count information for creating ToolTips and indices
{
public:
	enum REF_TYPE_ENUM {
		RTE_TESTAMENT = 0,
		RTE_BOOK = 1,
		RTE_CHAPTER = 2,
		RTE_VERSE = 3,
		RTE_WORD = 4
	};

	CRefCountCalc(REF_TYPE_ENUM nRefType, const CRelIndex &refIndex);
	~CRefCountCalc() { }

	static QString SearchResultToolTip(const CRelIndex &refIndex, int nRIMask = RIMASK_ALL);		// Create complete reference statistics report
	static QString PassageReferenceText(const CRelIndex &refIndex);		// Creates a reference text string like "Genesis 1:1 [5]"

	REF_TYPE_ENUM refType() const { return m_nRefType; }
	CRelIndex refIndex() const { return m_ndxRef; }

	unsigned int ofBible() const { return m_nOfBible; }
	unsigned int ofTestament() const { return m_nOfTst; }
	unsigned int ofBook() const { return m_nOfBk; }
	unsigned int ofChapter() const { return m_nOfChp; }
	unsigned int ofVerse() const { return m_nOfVrs; }

	// calcRelIndex - Calculates a relative index from counts.  For example, starting from (0,0,0,0):
	//			calcRelIndex(1, 1, 666, 0, 1);						// Returns (21,7,1,1) or Ecclesiastes 7:1 [1], Word 1 of Verse 1 of Chapter 666 of the Bible
	//			calcRelIndex(1, 393, 0, 5, 0);						// Returns (5, 13, 13, 1) or Deuteronomy 13:13 [1], Word 1 of Verse 393 of Book 5 of the Bible
	//			calcRelIndex(1, 13, 13, 5, 0);						// Returns (5, 13, 13, 1) or Deuteronomy 13:13 [1], Word 1 of Verse 13 of Chapter 13 of Book 5 of the Bible
	//			calcRelIndex(1, 13, 13, 5, 1);						// Returns (5, 13, 13, 1) or Deuteronomy 13:13 [1], Word 1 of Verse 13 of Chapter 13 of Book 5 of the Old Testament
	//			calcRelIndex(1, 13, 13, 5, 2);						// Returns (44, 13, 13, 1) or Acts 13:13 [1], Word 1 of Verse 13 of Chapter 13 of Book 5 of the New Testament
	//			calcRelIndex(0, 13, 13, 5, 2);						// Returns (44, 13, 13, 1) or Acts 13:13 [1], Word 1 of Verse 13 of Chapter 13 of Book 5 of the New Testament
	static CRelIndex calcRelIndex(
						unsigned int nWord, unsigned int nVerse, unsigned int nChapter,
						unsigned int nBook, unsigned int nTestament,
						CRelIndex ndxStart = CRelIndex(),
						bool bReverse = false);

private:
	CRelIndex m_ndxRef;			// Relative Index

	REF_TYPE_ENUM m_nRefType;	// Type of Reference these counts are for

	// All entries above the type specified will be valid:
	unsigned int m_nOfBible;	// Testament, Book, Chapter, Verse, Word of the whole Bible
	unsigned int m_nOfTst;		// Book, Chapter, Verse, Word of the Testament
	unsigned int m_nOfBk;		// Chapter, Verse, Word of the Book
	unsigned int m_nOfChp;		// Verse, Word of the Chapter
	unsigned int m_nOfVrs;		// Word of the Verse
};


// ============================================================================

typedef std::vector<uint32_t> TIndexList;			// Index List for words into book/chapter/verse/word

struct IndexSortPredicate {
	bool operator() (const CRelIndex &v1, const CRelIndex &v2) const
	{
		return (v1.index() < v2.index());
	}
};

struct XformLower {
	int operator()(int c)
	{
		return tolower(c);
	}
};

// ============================================================================

// TESTAMENT -- Table of Testaments:
//
class CTestamentEntry
{
public:
	CTestamentEntry()
	:	m_nNumBk(0),
		m_nNumChp(0),
		m_nNumVrs(0),
		m_nNumWrd(0)
	{ }
	~CTestamentEntry() { }

	QString  m_strTstName;		// Name of testament (display name)
	unsigned int m_nNumBk;		// Number of Books in this testament
	unsigned int m_nNumChp;		// Number of Chapters in this testament
	unsigned int m_nNumVrs;		// Number of Verses in this testament
	unsigned int m_nNumWrd;		// Number of Words in this testament
};

typedef std::vector<CTestamentEntry> TTestamentList;		// Index by nTst-1

extern TTestamentList g_lstTestaments;		// Global Testament List

// ============================================================================

// TOC -- Table of Contents:
//
class CTOCEntry
{
public:
	CTOCEntry()
	:   m_nTstBkNdx(0),
		m_nTstNdx(0),
		m_nNumChp(0),
		m_nNumVrs(0),
		m_nNumWrd(0)
	{ }
	~CTOCEntry() { }

	unsigned int m_nTstBkNdx;	// Testament Book Index (Index within the books of the testament) 1-39 or 1-27
	unsigned int m_nTstNdx;		// Testament Index (1=Old, 2=New, etc)
	QString m_strBkName;		// Name of book (display name)
	QString m_strBkAbbr;		// Book Abbreviation
	QString m_strTblName;		// Name of Table for this book
	unsigned int m_nNumChp;		// Number of chapters in this book
	unsigned int m_nNumVrs;		// Number of verses in this book
	unsigned int m_nNumWrd;		// Number of words in this book
	QString m_strCat;			// Category name
	QString m_strDesc;			// Description (subtitle)
};

typedef std::vector<CTOCEntry> TTOCList;	// Index by nBk-1

extern TTOCList g_lstTOC;		// Global Table of Contents

// ============================================================================

// LAYOUT -- Book/Chapter Layout:
//
class CLayoutEntry
{
public:
	CLayoutEntry()
	:   m_nNumVrs(0),
		m_nNumWrd(0)
	{ }
	~CLayoutEntry() { }

	unsigned int m_nNumVrs;		// Number of verses in this chapter
	unsigned int m_nNumWrd;		// Number of words in this chapter
};

typedef std::map<CRelIndex, CLayoutEntry, IndexSortPredicate> TLayoutMap;	// Index by [nBk|nChp|0|0]

extern TLayoutMap g_mapLayout;	// Global Layout

// ============================================================================

// BOOK -- Chapter/Verse Layout:
//
class CBookEntry
{
public:
	CBookEntry()
	:   m_nNumWrd(0),
		m_bPilcrow(false)
	{ }
	~CBookEntry() { }

	unsigned int m_nNumWrd;		// Number of words in this verse
	bool m_bPilcrow;			// Start of verse Pilcrow Flag
	QString GetRichText() const		// We'll use a function to fetch the rich text (on mobile this will be a database lookup)
	{
		return m_strRichText;
	}
	void SetRichText(const QString &strRichText)		// This will be a noop on mobile
	{
		m_strRichText = strRichText;
	}
	QString m_strFootnote;		// Footnote text for this verse (if any)

private:
	QString m_strRichText;		// Rich text for the verse (Note: for mobile versions, this element will be removed and fetched from the database)
};

typedef std::map<CRelIndex, CBookEntry, IndexSortPredicate> TBookEntryMap;		// Index by [0|nChp|nVrs|0]

typedef std::vector<TBookEntryMap> TBookList;	// Index by nBk-1

extern TBookList g_lstBooks;	// Global Books

// ============================================================================

// WORDS -- Word List and Mapping
//
class CWordEntry
{
public:
	CWordEntry()
	:	m_bCasePreserve(false)
	{ }
	~CWordEntry() { }

	QString m_strWord;			// Word Text
	bool m_bCasePreserve;		// Special Word Case Preserve
	QStringList m_lstAltWords;	// List of alternate synonymous words for searching (such as hyphenated and non-hyphenated)
	QList<unsigned int> m_lstAltWordCount;		// Count for each alternate word.  This will be the number of entries for this word in the mapping below
	TIndexList m_ndxMapping;	// Indexes
	TIndexList m_ndxNormalized;	// Normalized index into entire Bible (Number of entries here should match number of indexes in Mapping above)

	struct SortPredicate {
		bool operator() (const QString &s1, const QString &s2) const
		{
			return (s1.compare(s2) < 0);
		}
	};
};

typedef std::map<QString, CWordEntry, CWordEntry::SortPredicate> TWordListMap;

extern TWordListMap g_mapWordList;	// Our one and only master word list (Indexed by lowercase word)

// ============================================================================

// Concordance -- Mapping of words and their Normalized positions:
//

typedef QStringList TConcordanceList;

extern TConcordanceList g_lstConcordanceWords;		// List of all Unique Words in the order for the concordance with names of the TWordListMap key (starts at index 0)
extern TIndexList g_lstConcordanceMapping;			// List of WordNdx#+1 (in ConcordanceWords) for all 789629 words of the text (starts at index 1)

// ============================================================================

// Phrases -- Common and Saved Search Phrase Lists:
//

class CPhraseEntry
{
public:
	CPhraseEntry()
		:	m_bCaseSensitive(false),
			m_nNumWrd(0)
	{ }
	~CPhraseEntry() { }

	bool m_bCaseSensitive;
	QString m_strPhrase;
	unsigned int m_nNumWrd;		// Number of words in phrase
};

Q_DECLARE_METATYPE(CPhraseEntry)

typedef QList<CPhraseEntry> CPhraseList;

extern CPhraseList g_lstCommonPhrases;			// Common phrases read from database
extern CPhraseList g_lstUserPhrases;			// User-defined phrases read from optional user database

// ============================================================================

typedef QPair<uint32_t, unsigned int> TPhraseTag;		// Normalized Index and Word Count pair used for highlight phrases found
typedef QList<TPhraseTag> TPhraseTagList;				// List of tags used for highlighting found phrases

struct TPhraseTagListSortPredicate {
	static bool ascendingLessThan(const TPhraseTag &s1, const TPhraseTag &s2)
	{
		return (s1.first < s2.first);
	}
};

// ============================================================================

#endif // DBSTRUCT_H