#ifndef READDB_H
#define READDB_H

#include <QSqlDatabase>
#include <QWidget>
#include <QByteArray>
#include <QString>
#include "dbstruct.h"

class CReadDatabase
{
public:
	CReadDatabase(QWidget *pParent = NULL)
		:	m_pParent(pParent)
	{ }
	~CReadDatabase() { }

	bool ReadDatabase(const QString &strDatabaseFilename);
	bool ReadUserDatabase(const QString &strDatabaseFilename);

	static bool IndexBlobToIndexList(const QByteArray &baBlob, TIndexList &anIndexList);

protected:
	bool ReadTestamentTable();
	bool ReadTOCTable();
	bool ReadLAYOUTTable();
	bool ReadBookTables();
	bool ReadWORDSTable();
	bool ReadPHRASESTable(bool bUserPhrases = false);
	bool ValidateData();

private:
	QWidget *m_pParent;
	QSqlDatabase m_myDatabase;
};

#endif // READDB_H
