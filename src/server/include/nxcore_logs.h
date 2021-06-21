/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2020 Victor Kirhenshtein
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** File: nxcore_logs.h
**
**/

#ifndef _nxcore_logs_h_
#define _nxcore_logs_h_

#define MAX_COLUMN_NAME_LEN    64

/**
 * Column types
 */
#define LC_TEXT            0
#define LC_SEVERITY        1
#define LC_OBJECT_ID       2
#define LC_USER_ID         3
#define LC_EVENT_CODE      4
#define LC_TIMESTAMP       5
#define LC_INTEGER         6
#define LC_ALARM_STATE     7
#define LC_ALARM_HD_STATE  8
#define LC_ZONE_UIN        9
#define LC_EVENT_ORIGIN    10
#define LC_TEXT_DETAILS    11
#define LC_JSON_DETAILS    12
#define LC_STATUS          13
#define LC_ACTION_CODE     14

/**
 * Column filter types
 */
#define FILTER_EQUALS      0
#define FILTER_RANGE       1
#define FILTER_SET         2
#define FILTER_LIKE        3
#define FILTER_LESS        4
#define FILTER_GREATER     5
#define FILTER_CHILDOF     6

/**
 * Filter set operations
 */
#define SET_OPERATION_AND  0
#define SET_OPERATION_OR   1

/**
 * Log column flags
 */
#define LCF_TSDB_TIMESTAMPTZ  0x0001   /* Column is of timestamptz data type in TimescaleDB */

/**
 * Log column definition structure
 */
struct LOG_COLUMN
{
	const TCHAR *name;
	const TCHAR *description;
	int type;
	uint32_t flags;
};

/**
 * Log definition structure
 */
struct NXCORE_LOG
{
	const TCHAR *name;
	const TCHAR *table;
	const TCHAR *idColumn;
	const TCHAR *relatedObjectIdColumn;
	uint64_t requiredAccess;
	LOG_COLUMN columns[32];
};

/**
 * Ordering column information
 */
struct OrderingColumn
{
	TCHAR name[MAX_COLUMN_NAME_LEN];
	bool descending;
};

class LogHandle;

/**
 * Column filter
 */
class ColumnFilter
{
private:
   int m_varCount;   // Number of variables read from NXCP message during construction
   int m_type;
   TCHAR *m_column;
   uint32_t m_columnFlags;
   bool m_negated;
	union t_ColumnFilterValue
	{
		int64_t numericValue;   // numeric value for <, >, and = operations
		struct
		{
		   int64_t start;
		   int64_t end;
		} range;
		TCHAR *like;
		struct
		{
			int operation;
			int count;
			ColumnFilter **filters;
		} set;
	} m_value;

public:
	ColumnFilter(const NXCPMessage& msg, const TCHAR *column, uint32_t baseId, LogHandle *log);
	~ColumnFilter();

	int getVariableCount() const { return m_varCount; }

	StringBuffer generateSql();
};

/**
 * Log filter
 */
class LogFilter
{
private:
   int m_numColumnFilters;
   ColumnFilter **m_columnFilters;
   int m_numOrderingColumns;
   OrderingColumn *m_orderingColumns;

public:
   LogFilter(const NXCPMessage& msg, LogHandle *log);
   ~LogFilter();

   StringBuffer buildOrderClause();

   int getNumColumnFilter() const
   {
      return m_numColumnFilters;
   }

   ColumnFilter *getColumnFilter(int offset)
   {
      return m_columnFilters[offset];
   }

   int getNumOrderingColumns() const
   {
      return m_numOrderingColumns;
   }
};

/**
 * Log handle - object used to access log
 */
class LogHandle
{
private:
   const NXCORE_LOG *m_log;
   LogFilter *m_filter;
   MUTEX m_lock;
   StringBuffer m_queryColumns;
   uint32_t m_rowCountLimit;
   int64_t m_maxRecordId;
   DB_RESULT m_resultSet;

   void buildQueryColumnList();
   StringBuffer buildObjectAccessConstraint(uint32_t userId);
   void deleteQueryResults();
   bool queryInternal(int64_t *rowCount, uint32_t userId);
   Table *createTable();

public:
   LogHandle(const NXCORE_LOG *log);
   ~LogHandle();

   void lock() { MutexLock(m_lock); }
   void release() { MutexUnlock(m_lock); }

   bool query(LogFilter *filter, int64_t *rowCount, uint32_t userId);
   Table *getData(int64_t startRow, int64_t numRows, bool refresh, uint32_t userId);
   void getRecordDetails(int64_t recordId, NXCPMessage *msg);
   void getColumnInfo(NXCPMessage *msg);
   const LOG_COLUMN *getColumnDefinition(const TCHAR *name) const;
};

// API functions
int32_t OpenLog(const TCHAR *name, ClientSession *session, uint32_t *rcc);
uint32_t CloseLog(ClientSession *session, int32_t logHandle);
void CloseAllLogsForSession(session_id_t sessionId);
shared_ptr<LogHandle> AcquireLogHandleObject(ClientSession *session, int32_t logHandle);

#endif
