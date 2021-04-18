/* 
** Oracle Database Driver
** Copyright (C) 2007-2021 Victor Kirhenshtein
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
** File: oracle.cpp
**
**/

#include "oracledrv.h"

DECLARE_DRIVER_HEADER("ORACLE")

static uint32_t DrvQueryInternal(ORACLE_CONN *pConn, const WCHAR *pwszQuery, WCHAR *errorText);

#ifdef UNICODE_UCS4

/**
 * Convert wide character string to UCS-2 using internal buffer when possible
 */
inline UCS2CHAR *WideStringToUCS2(const WCHAR *str, UCS2CHAR *localBuffer, size_t size)
{
   size_t len = ucs4_ucs2len(str, -1);
   UCS2CHAR *buffer = (len <= size) ? localBuffer : MemAllocArrayNoInit<UCS2CHAR>(len);
   ucs4_to_ucs2(str, -1, buffer, len);
   return buffer;
}

/**
 * Free converted string if local buffer was not used
 */
inline void FreeConvertedString(UCS2CHAR *str, UCS2CHAR *localBuffer)
{
   if (str != localBuffer)
      MemFree(str);
}

#endif   /* UNICODE_UCS4 */

/**
 * Get error text from error handle
 */
static void GetErrorFromHandle(OCIError *handle, sb4 *errorCode, WCHAR *errorText)
{
#if UNICODE_UCS2
   OCIErrorGet(handle, 1, NULL, errorCode, (text *)errorText, DBDRV_MAX_ERROR_TEXT, OCI_HTYPE_ERROR);
   errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#else
   UCS2CHAR buffer[DBDRV_MAX_ERROR_TEXT];

   OCIErrorGet(handle, 1, NULL, errorCode, (text *)buffer, DBDRV_MAX_ERROR_TEXT, OCI_HTYPE_ERROR);
   buffer[DBDRV_MAX_ERROR_TEXT - 1] = 0;
   ucs2_to_ucs4(buffer, ucs2_strlen(buffer) + 1, errorText, DBDRV_MAX_ERROR_TEXT);
   errorText[DBDRV_MAX_ERROR_TEXT - 1] = 0;
#endif
   RemoveTrailingCRLFW(errorText);
}

/**
 * Set last error text
 */
static void SetLastError(ORACLE_CONN *pConn)
{
   GetErrorFromHandle(pConn->handleError, &pConn->lastErrorCode, pConn->lastErrorText);
}

/**
 * Check if last error was caused by lost connection to server
 */
static inline uint32_t IsConnectionError(ORACLE_CONN *conn)
{
   ub4 nStatus = 0;
   OCIAttrGet(conn->handleServer, OCI_HTYPE_SERVER, &nStatus, nullptr, OCI_ATTR_SERVER_STATUS, conn->handleError);
   return (nStatus == OCI_SERVER_NOT_CONNECTED) ? DBERR_CONNECTION_LOST : DBERR_OTHER_ERROR;
}

/**
 * Check for successful completion
 */
static inline bool IsSuccess(sword code, ORACLE_CONN *conn = nullptr, const TCHAR *logMessage = nullptr)
{
   if (code == OCI_SUCCESS)
      return true;
   if (code != OCI_SUCCESS_WITH_INFO)
      return false;
   if (logMessage != nullptr)
   {
      WCHAR errorText[DBDRV_MAX_ERROR_TEXT];
      GetErrorFromHandle(conn->handleError, &conn->lastErrorCode, errorText);
      nxlog_write_tag(NXLOG_WARNING, DEBUG_TAG, logMessage, errorText);
   }
   return true;
}

/**
 * Global environment handle
 */
static OCIEnv *s_handleEnv = nullptr;

/**
 * Major OCI version
 */
static int s_ociVersionMajor = 0;

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
extern "C" WCHAR __EXPORT *DrvPrepareStringW(const WCHAR *str)
{
	int len = (int)wcslen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	WCHAR *out = (WCHAR *)malloc(bufferSize * sizeof(WCHAR));
	out[0] = L'\'';

	const WCHAR *src = str;
	int outPos;
	for(outPos = 1; *src != 0; src++)
	{
		if (*src == L'\'')
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (WCHAR *)realloc(out, bufferSize * sizeof(WCHAR));
			}
			out[outPos++] = L'\'';
			out[outPos++] = L'\'';
		}
		else
		{
			out[outPos++] = *src;
		}
	}
	out[outPos++] = L'\'';
	out[outPos++] = 0;

	return out;
}

/**
 * Prepare string for using in SQL query - enclose in quotes and escape as needed
 */
extern "C" char __EXPORT *DrvPrepareStringA(const char *str)
{
	int len = (int)strlen(str) + 3;   // + two quotes and \0 at the end
	int bufferSize = len + 128;
	char *out = (char *)malloc(bufferSize);
	out[0] = '\'';

	const char *src = str;
	int outPos;
	for(outPos = 1; *src != 0; src++)
	{
		if (*src == '\'')
		{
			len++;
			if (len >= bufferSize)
			{
				bufferSize += 128;
				out = (char *)realloc(out, bufferSize);
			}
			out[outPos++] = '\'';
			out[outPos++] = '\'';
		}
		else
		{
			out[outPos++] = *src;
		}
	}
	out[outPos++] = '\'';
	out[outPos++] = 0;

	return out;
}

/**
 * Initialize driver
 */
extern "C" bool __EXPORT DrvInit(const char *cmdLine)
{
   sword major, minor, update, patch, pupdate;
   OCIClientVersion(&major, &minor, &update, &patch, &pupdate);
   nxlog_debug_tag(DEBUG_TAG, 1, _T("OCI version %d.%d.%d.%d.%d"), (int)major, (int)minor, (int)update, (int)patch, (int)pupdate);
   s_ociVersionMajor = (int)major;

   if (OCIEnvNlsCreate(&s_handleEnv, OCI_THREADED | OCI_NCHAR_LITERAL_REPLACE_OFF,
                       NULL, NULL, NULL, NULL, 0, NULL, OCI_UTF16ID, OCI_UTF16ID) != OCI_SUCCESS)
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Cannot allocate environment handle"));
      return false;
   }
	return true;
}

/**
 * Unload handler
 */
extern "C" void __EXPORT DrvUnload()
{
   if (s_handleEnv != NULL)
      OCIHandleFree(s_handleEnv, OCI_HTYPE_ENV);
	OCITerminate(OCI_DEFAULT);
}

/**
 * Destroy query result
 */
static void DestroyQueryResult(ORACLE_RESULT *pResult)
{
	int i, nCount;

	nCount = pResult->nCols * pResult->nRows;
	for(i = 0; i < nCount; i++)
		MemFree(pResult->pData[i]);
	MemFree(pResult->pData);

	for(i = 0; i < pResult->nCols; i++)
	   MemFree(pResult->columnNames[i]);
	MemFree(pResult->columnNames);

	MemFree(pResult);
}

/**
 * Connect to database
 */
extern "C" DBDRV_CONNECTION __EXPORT DrvConnect(const char *host, const char *login, const char *password,
         const char *database, const char *schema, WCHAR *errorText)
{
	ORACLE_CONN *pConn = MemAllocStruct<ORACLE_CONN>();
	if (pConn != nullptr)
	{
      OCIHandleAlloc(s_handleEnv, (void **)&pConn->handleError, OCI_HTYPE_ERROR, 0, nullptr);
		OCIHandleAlloc(s_handleEnv, (void **)&pConn->handleServer, OCI_HTYPE_SERVER, 0, nullptr);
		UCS2CHAR *pwszStr = UCS2StringFromUTF8String(host);
		if (IsSuccess(OCIServerAttach(pConn->handleServer, pConn->handleError,
		                    (text *)pwszStr, (sb4)ucs2_strlen(pwszStr) * sizeof(UCS2CHAR), OCI_DEFAULT)))
		{
			MemFree(pwszStr);

			// Initialize service handle
			OCIHandleAlloc(s_handleEnv, (void **)&pConn->handleService, OCI_HTYPE_SVCCTX, 0, nullptr);
			OCIAttrSet(pConn->handleService, OCI_HTYPE_SVCCTX, pConn->handleServer, 0, OCI_ATTR_SERVER, pConn->handleError);
			
			// Initialize session handle
			OCIHandleAlloc(s_handleEnv, (void **)&pConn->handleSession, OCI_HTYPE_SESSION, 0, nullptr);
			pwszStr = UCS2StringFromUTF8String(login);
			OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, pwszStr,
			           (ub4)ucs2_strlen(pwszStr) * sizeof(UCS2CHAR), OCI_ATTR_USERNAME, pConn->handleError);
			MemFree(pwszStr);
			pwszStr = UCS2StringFromUTF8String(password);
			OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, pwszStr, (ub4)ucs2_strlen(pwszStr) * sizeof(UCS2CHAR), OCI_ATTR_PASSWORD, pConn->handleError);

			// Authenticate
			if (IsSuccess(OCISessionBegin(pConn->handleService, pConn->handleError, pConn->handleSession,
			         OCI_CRED_RDBMS, OCI_STMT_CACHE), pConn, _T("Connected to database with warning (%s)")))
			{
				OCIAttrSet(pConn->handleService, OCI_HTYPE_SVCCTX, pConn->handleSession, 0, OCI_ATTR_SESSION, pConn->handleError);
				pConn->mutexQueryLock = MutexCreate();
				pConn->nTransLevel = 0;
				pConn->lastErrorCode = 0;
				pConn->lastErrorText[0] = 0;
            pConn->prefetchLimit = 10;

				if ((schema != nullptr) && (schema[0] != 0))
				{
					MemFree(pwszStr);
					pwszStr = UCS2StringFromUTF8String(schema);
					OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, pwszStr,
								  (ub4)ucs2_strlen(pwszStr) * sizeof(UCS2CHAR), OCI_ATTR_CURRENT_SCHEMA, pConn->handleError);
				}

            // LOB prefetch
            ub4 lobPrefetchSize = 16384;  // 16K
            OCIAttrSet(pConn->handleSession, OCI_HTYPE_SESSION, &lobPrefetchSize, 0, OCI_ATTR_DEFAULT_LOBPREFETCH_SIZE, pConn->handleError);

            // Setup session
            DrvQueryInternal(pConn, L"ALTER SESSION SET NLS_LANGUAGE='AMERICAN' NLS_NUMERIC_CHARACTERS='.,'", NULL);

            UCS2CHAR version[1024];
            if (IsSuccess(OCIServerVersion(pConn->handleService, pConn->handleError, (OraText *)version, sizeof(version), OCI_HTYPE_SVCCTX)))
            {
#ifdef UNICODE
#if UNICODE_UCS4
               WCHAR *wver = UCS4StringFromUCS2String(version);
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Connected to %s"), wver);
               MemFree(wver);
#else
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Connected to %s"), version);
#endif
#else
               char *mbver = MBStringFromUCS2String(version);
               nxlog_debug_tag(DEBUG_TAG, 5, _T("Connected to %s"), mbver);
               MemFree(mbver);
#endif
            }
         }
			else
			{
				GetErrorFromHandle(pConn->handleError, &pConn->lastErrorCode, errorText);
		      OCIServerDetach(pConn->handleServer, pConn->handleError, OCI_DEFAULT);
			   OCIHandleFree(pConn->handleService, OCI_HTYPE_SVCCTX);
				OCIHandleFree(pConn->handleServer, OCI_HTYPE_SERVER);
			   OCIHandleFree(pConn->handleError, OCI_HTYPE_ERROR);
			   MemFreeAndNull(pConn);
			}
		}
		else
		{
			GetErrorFromHandle(pConn->handleError, &pConn->lastErrorCode, errorText);
			OCIHandleFree(pConn->handleServer, OCI_HTYPE_SERVER);
			OCIHandleFree(pConn->handleError, OCI_HTYPE_ERROR);
         MemFreeAndNull(pConn);
		}
		MemFree(pwszStr);
	}
	else
	{
		wcscpy(errorText, L"Memory allocation error");
	}
   return pConn;
}

/**
 * Disconnect from database
 */
extern "C" void __EXPORT DrvDisconnect(ORACLE_CONN *pConn)
{
	if (pConn == NULL)
	   return;

   OCISessionEnd(pConn->handleService, pConn->handleError, NULL, OCI_DEFAULT);
   OCIServerDetach(pConn->handleServer, pConn->handleError, OCI_DEFAULT);
   OCIHandleFree(pConn->handleSession, OCI_HTYPE_SESSION);
   OCIHandleFree(pConn->handleService, OCI_HTYPE_SVCCTX);
   OCIHandleFree(pConn->handleServer, OCI_HTYPE_SERVER);
   OCIHandleFree(pConn->handleError, OCI_HTYPE_ERROR);
   MutexDestroy(pConn->mutexQueryLock);
   MemFree(pConn);
}

/**
 * Set prefetch limit
 */
extern "C" void __EXPORT DrvSetPrefetchLimit(ORACLE_CONN *pConn, int limit)
{
	if (pConn != NULL)
      pConn->prefetchLimit = limit;
}

/**
 * Convert query from NetXMS portable format to native Oracle format
 */
static UCS2CHAR *ConvertQuery(WCHAR *query, UCS2CHAR *localBuffer, size_t bufferSize)
{
   int count = NumCharsW(query, L'?');
   if (count == 0)
   {
#if UNICODE_UCS4
      return WideStringToUCS2(query, localBuffer, bufferSize);
#else
      return query;
#endif
   }

#if UNICODE_UCS4
   UCS2CHAR srcQueryBuffer[1024];
	UCS2CHAR *srcQuery = WideStringToUCS2(query, srcQueryBuffer, 1024);
#else
	UCS2CHAR *srcQuery = query;
#endif

	size_t dstQuerySize = ucs2_strlen(srcQuery) + count * 3 + 1;
	UCS2CHAR *dstQuery = (dstQuerySize <= bufferSize) ? localBuffer : MemAllocArrayNoInit<UCS2CHAR>(dstQuerySize);

	bool inString = false;
	int pos = 1;
	UCS2CHAR *src, *dst;
	for(src = srcQuery, dst = dstQuery; *src != 0; src++)
	{
		switch(*src)
		{
			case '\'':
				*dst++ = *src;
				inString = !inString;
				break;
			case '\\':
				*dst++ = *src++;
				*dst++ = *src;
				break;
			case '?':
				if (inString)
				{
					*dst++ = '?';
				}
				else
				{
					*dst++ = ':';
					if (pos < 10)
					{
						*dst++ = pos + '0';
					}
					else if (pos < 100)
					{
						*dst++ = pos / 10 + '0';
						*dst++ = pos % 10 + '0';
					}
					else
					{
						*dst++ = pos / 100 + '0';
						*dst++ = (pos % 100) / 10 + '0';
						*dst++ = pos % 10 + '0';
					}
					pos++;
				}
				break;
			default:
				*dst++ = *src;
				break;
		}
	}
	*dst = 0;
#if UNICODE_UCS4
	FreeConvertedString(srcQuery, srcQueryBuffer);
#endif
	return dstQuery;
}

/**
 * Prepare statement
 */
extern "C" ORACLE_STATEMENT __EXPORT *DrvPrepare(ORACLE_CONN *pConn, WCHAR *pwszQuery, bool optimizeForReuse, DWORD *pdwError, WCHAR *errorText)
{
	ORACLE_STATEMENT *stmt = nullptr;

	UCS2CHAR localBuffer[1024];
	UCS2CHAR *ucs2Query = ConvertQuery(pwszQuery, localBuffer, 1024);

	MutexLock(pConn->mutexQueryLock);
   OCIStmt *handleStmt;
	if (IsSuccess(OCIStmtPrepare2(pConn->handleService, &handleStmt, pConn->handleError, (text *)ucs2Query,
	                    (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), nullptr, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)))
	{
		stmt = MemAllocStruct<ORACLE_STATEMENT>();
		stmt->connection = pConn;
		stmt->handleStmt = handleStmt;
		stmt->bindings = new ObjectArray<OracleBind>(32, 32, Ownership::True);
      stmt->batchBindings = nullptr;
      stmt->batchMode = false;
      stmt->batchSize = 0;
		OCIHandleAlloc(s_handleEnv, (void **)&stmt->handleError, OCI_HTYPE_ERROR, 0, nullptr);
		*pdwError = DBERR_SUCCESS;
	}
	else
	{
		SetLastError(pConn);
		*pdwError = IsConnectionError(pConn);
	}

	if (errorText != nullptr)
	{
		wcslcpy(errorText, pConn->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	MutexUnlock(pConn->mutexQueryLock);

#if UNICODE_UCS2
	if ((ucs2Query != pwszQuery) && (ucs2Query != localBuffer))
	   MemFree(ucs2Query);
#else
	FreeConvertedString(ucs2Query, localBuffer);
#endif
	return stmt;
}

/**
 * Open batch
 */
extern "C" bool __EXPORT DrvOpenBatch(ORACLE_STATEMENT *stmt)
{
   if (stmt->batchBindings != nullptr)
      stmt->batchBindings->clear();
   else
      stmt->batchBindings = new ObjectArray<OracleBatchBind>(32, 32, Ownership::True);
   stmt->batchMode = true;
   stmt->batchSize = 0;
   return true;
}

/**
 * Start next batch row
 */
extern "C" void __EXPORT DrvNextBatchRow(ORACLE_STATEMENT *stmt)
{
   if (!stmt->batchMode)
      return;
   
   for(int i = 0; i < stmt->batchBindings->size(); i++)
   {
	   OracleBatchBind *bind = stmt->batchBindings->get(i);
      if (bind != nullptr)
         bind->addRow();
   }
   stmt->batchSize++;
}

/**
 * Buffer sizes for different C types
 */
static DWORD s_bufferSize[] = { 0, sizeof(LONG), sizeof(DWORD), sizeof(INT64), sizeof(QWORD), sizeof(double) };

/**
 * Corresponding Oracle types for C types
 */
static ub2 s_oracleType[] = { SQLT_STR, SQLT_INT, SQLT_UIN, SQLT_INT, SQLT_UIN, SQLT_FLT };

/**
 * Bind parameter to statement - normal mode (non-batch)
 */
static void BindNormal(ORACLE_STATEMENT *stmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
	OracleBind *bind = stmt->bindings->get(pos - 1);
	if (bind == nullptr)
	{
	   bind = new OracleBind(stmt->connection->handleService, stmt->handleError);
	   stmt->bindings->set(pos - 1, bind);
	}
	else
	{
	   MemFreeAndNull(bind->data);
	   bind->freeTemporaryLob();
	}

	UCS2CHAR *convertedString = nullptr;
	if ((sqlType == DB_SQLTYPE_TEXT) && ((cType == DB_CTYPE_STRING) || (cType == DB_CTYPE_UTF8_STRING)))
	{
	   ub4 textLen;
	   if (cType == DB_CTYPE_UTF8_STRING)
	   {
	      convertedString = UCS2StringFromUTF8String(static_cast<char*>(buffer));
	      textLen = static_cast<ub4>(ucs2_strlen(convertedString));
	   }
	   else
	   {
#if UNICODE_UCS4
	      convertedString = UCS2StringFromUCS4String(static_cast<WCHAR*>(buffer));
         textLen = static_cast<ub4>(ucs2_strlen(convertedString));
#else
         textLen = static_cast<ub4>(wcslen(static_cast<WCHAR*>(buffer)));
#endif
	   }
	   if (textLen > 2000)
	   {
         if (bind->createTemporaryLob(s_handleEnv))
         {
            if (OCILobWrite(bind->serviceContext, bind->errorHandle, bind->lobLocator, &textLen, 1,
                     (convertedString != nullptr) ? convertedString : buffer, textLen * sizeof(UCS2CHAR),
                     OCI_ONE_PIECE, nullptr, nullptr, OCI_UTF16ID, SQLCS_IMPLICIT) == OCI_SUCCESS)
            {
               OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, &bind->lobLocator,
                     static_cast<sb4>(sizeof(OCILobLocator*)), SQLT_CLOB, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
            }
            else
            {
               sb4 errorCode;
               WCHAR errorText[DBDRV_MAX_ERROR_TEXT];
               GetErrorFromHandle(stmt->handleError, &errorCode, errorText);
#ifdef UNICODE
               nxlog_debug_tag(DEBUG_TAG, 5, _T("BindNormal: cannot write to temporary LOB (%s)"), errorText);
#else
               nxlog_debug_tag(DEBUG_TAG, 5, _T("BindNormal: cannot write to temporary LOB (%S)"), errorText);
#endif
               bind->freeTemporaryLob();
            }
         }
         MemFree(convertedString);
         if (allocType == DB_BIND_DYNAMIC)
            MemFree(buffer);
	      return;
	   }
	   // Fallback to varchar binding
	}

	void *sqlBuffer;
	switch(cType)
	{
		case DB_CTYPE_STRING:
#if UNICODE_UCS4
			bind->data = (convertedString != nullptr) ? convertedString : UCS2StringFromUCS4String(static_cast<WCHAR*>(buffer));
			if (allocType == DB_BIND_DYNAMIC)
				MemFree(buffer);
			sqlBuffer = bind->data;
#else
         if (allocType == DB_BIND_TRANSIENT)
			{
				bind->data = MemCopyStringW(static_cast<WCHAR*>(buffer));
	         sqlBuffer = bind->data;
			}
			else
			{
				if (allocType == DB_BIND_DYNAMIC)
					bind->data = buffer;
            sqlBuffer = buffer;
			}
#endif
         OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, sqlBuffer,
               static_cast<sb4>((ucs2_strlen(static_cast<UCS2CHAR*>(sqlBuffer)) + 1) * sizeof(UCS2CHAR)),
               SQLT_STR, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
			break;
      case DB_CTYPE_UTF8_STRING:
         bind->data = (convertedString != nullptr) ? convertedString : UCS2StringFromUTF8String(static_cast<char*>(buffer));
         sqlBuffer = bind->data;
         if (allocType == DB_BIND_DYNAMIC)
            MemFree(buffer);
         OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, sqlBuffer,
               static_cast<sb4>((ucs2_strlen((UCS2CHAR *)sqlBuffer) + 1) * sizeof(UCS2CHAR)), SQLT_STR, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
         break;
		case DB_CTYPE_INT64:	// OCI prior to 11.2 cannot bind 64 bit integers
		   OCINumberFromInt(stmt->handleError, buffer, sizeof(int64_t), OCI_NUMBER_SIGNED, &bind->number);
         OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, &bind->number, sizeof(OCINumber),
               SQLT_VNU, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
			if (allocType == DB_BIND_DYNAMIC)
				MemFree(buffer);
			break;
		case DB_CTYPE_UINT64:	// OCI prior to 11.2 cannot bind 64 bit integers
         OCINumberFromInt(stmt->handleError, buffer, sizeof(int64_t), OCI_NUMBER_UNSIGNED, &bind->number);
         OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, &bind->number, sizeof(OCINumber),
               SQLT_VNU, nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
         if (allocType == DB_BIND_DYNAMIC)
            MemFree(buffer);
         break;
		default:
		   switch(allocType)
		   {
			   case DB_BIND_STATIC:
				   sqlBuffer = buffer;
				   break;
			   case DB_BIND_DYNAMIC:
				   sqlBuffer = buffer;
				   bind->data = buffer;
				   break;
			   case DB_BIND_TRANSIENT:
				   sqlBuffer = MemCopyBlock(buffer, s_bufferSize[cType]);
				   bind->data = sqlBuffer;
				   break;
			   default:
				   return;	// Invalid call
		   }
		   OCIBindByPos(stmt->handleStmt, &bind->handle, stmt->handleError, pos, sqlBuffer, s_bufferSize[cType],
		         s_oracleType[cType], nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
		   break;
	}
}

/**
 * Batch bind - constructor
 */
OracleBatchBind::OracleBatchBind(int cType, int sqlType)
{
   m_cType = cType;
   m_size = 0;
   m_allocated = 256;
   if ((cType == DB_CTYPE_STRING) || (cType == DB_CTYPE_INT64) || (cType == DB_CTYPE_UINT64))
   {
      m_elementSize = sizeof(UCS2CHAR);
      m_string = true;
      m_oraType = (sqlType == DB_SQLTYPE_TEXT) ? SQLT_LNG : SQLT_STR;
      m_data = nullptr;
      m_strings = MemAllocArray<UCS2CHAR*>(m_allocated);
   }
   else
   {
      m_elementSize = s_bufferSize[cType];
      m_string = false;
      m_oraType = s_oracleType[cType];
      m_data = MemAllocZeroed(m_allocated * m_elementSize);
      m_strings = nullptr;
   }
}

/**
 * Batch bind - destructor
 */
OracleBatchBind::~OracleBatchBind()
{
   if (m_strings != nullptr)
   {
      for(int i = 0; i < m_size; i++)
         MemFree(m_strings[i]);
      MemFree(m_strings);
   }
   MemFree(m_data);
}

/**
 * Batch bind - add row
 */
void OracleBatchBind::addRow()
{
   if (m_size == m_allocated)
   {
      m_allocated += 256;
      if (m_string)
      {
         m_strings = MemRealloc(m_strings, m_allocated * sizeof(UCS2CHAR *));
         memset(m_strings + m_size, 0, (m_allocated - m_size) * sizeof(UCS2CHAR *));
      }
      else
      {
         m_data = MemRealloc(m_data, m_allocated * m_elementSize);
         memset((char *)m_data + m_size * m_elementSize, 0, (m_allocated - m_size) * m_elementSize);
      }
   }
   if (m_size > 0)
   {
      // clone last element
      if (m_string)
      {
         UCS2CHAR *p = m_strings[m_size - 1];
         m_strings[m_size] = (p != nullptr) ? ucs2_strdup(p) : nullptr;
      }
      else
      {
         memcpy((char *)m_data + m_size * m_elementSize, (char *)m_data + (m_size - 1) * m_elementSize, m_elementSize);
      }
   }
   m_size++;
}

/**
 * Batch bind - set value
 */
void OracleBatchBind::set(void *value)
{
   if (m_string)
   {
      MemFree(m_strings[m_size - 1]);
      m_strings[m_size - 1] = (UCS2CHAR *)value;
      if (value != NULL)
      {
         int l = (int)(ucs2_strlen((UCS2CHAR *)value) + 1) * sizeof(UCS2CHAR);
         if (l > m_elementSize)
            m_elementSize = l;
      }
   }
   else
   {
      memcpy((char *)m_data + (m_size - 1) * m_elementSize, value, m_elementSize);
   }
}

/**
 * Get data for OCI bind
 */
void *OracleBatchBind::getData()
{
   if (!m_string)
      return m_data;

   MemFree(m_data);
   m_data = MemAllocZeroed(m_size * m_elementSize);
   char *p = (char *)m_data;
   for(int i = 0; i < m_size; i++)
   {
      if (m_strings[i] == nullptr)
         continue;
      memcpy(p, m_strings[i], ucs2_strlen(m_strings[i]) * sizeof(UCS2CHAR));
      p += m_elementSize;
   }
   return m_data;
}

/**
 * Bind parameter to statement - batch mode
 */
static void BindBatch(ORACLE_STATEMENT *stmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
   if (stmt->batchSize == 0)
      return;  // no batch rows added yet

	OracleBatchBind *bind = stmt->batchBindings->get(pos - 1);
   if (bind == nullptr)
   {
      bind = new OracleBatchBind(cType, sqlType);
      stmt->batchBindings->set(pos - 1, bind);
      for(int i = 0; i < stmt->batchSize; i++)
         bind->addRow();
   }

   if (bind->getCType() != cType)
      return;

	void *sqlBuffer;
   switch(bind->getCType())
	{
		case DB_CTYPE_STRING:
#if UNICODE_UCS4
			sqlBuffer = UCS2StringFromUCS4String((WCHAR *)buffer);
         if (allocType == DB_BIND_DYNAMIC)
				MemFree(buffer);
#else
         if (allocType == DB_BIND_DYNAMIC)
         {
				sqlBuffer = buffer;
         }
         else
			{
				sqlBuffer = MemCopyStringW((WCHAR *)buffer);
			}
#endif
         bind->set(sqlBuffer);
			break;
      case DB_CTYPE_UTF8_STRING:
#if UNICODE_UCS4
         sqlBuffer = UCS2StringFromUTF8String((char *)buffer);
#else
         sqlBuffer = WideStringFromUTF8String((char *)buffer);
#endif
         if (allocType == DB_BIND_DYNAMIC)
            MemFree(buffer);
         bind->set(sqlBuffer);
         break;
		case DB_CTYPE_INT64:	// OCI prior to 11.2 cannot bind 64 bit integers
#ifdef UNICODE_UCS2
			sqlBuffer = malloc(64 * sizeof(WCHAR));
			swprintf((WCHAR *)sqlBuffer, 64, INT64_FMTW, *((INT64 *)buffer));
#else
			{
				char temp[64];
				snprintf(temp, 64, INT64_FMTA, *((INT64 *)buffer));
				sqlBuffer = UCS2StringFromMBString(temp);
			}
#endif
         bind->set(sqlBuffer);
			if (allocType == DB_BIND_DYNAMIC)
				free(buffer);
			break;
		case DB_CTYPE_UINT64:	// OCI prior to 11.2 cannot bind 64 bit integers
#ifdef UNICODE_UCS2
			sqlBuffer = malloc(64 * sizeof(WCHAR));
			swprintf((WCHAR *)sqlBuffer, 64, UINT64_FMTW, *((QWORD *)buffer));
#else
			{
				char temp[64];
				snprintf(temp, 64, UINT64_FMTA, *((QWORD *)buffer));
				sqlBuffer = UCS2StringFromMBString(temp);
			}
#endif
         bind->set(sqlBuffer);
			if (allocType == DB_BIND_DYNAMIC)
				free(buffer);
			break;
		default:
         bind->set(buffer);
			if (allocType == DB_BIND_DYNAMIC)
				free(buffer);
			break;
	}
}

/**
 * Bind parameter to statement
 */
extern "C" void __EXPORT DrvBind(ORACLE_STATEMENT *stmt, int pos, int sqlType, int cType, void *buffer, int allocType)
{
   if (stmt->batchMode)
      BindBatch(stmt, pos, sqlType, cType, buffer, allocType);
   else
      BindNormal(stmt, pos, sqlType, cType, buffer, allocType);
}

/**
 * Execute prepared non-select statement
 */
extern "C" DWORD __EXPORT DrvExecute(ORACLE_CONN *pConn, ORACLE_STATEMENT *stmt, WCHAR *errorText)
{
	DWORD dwResult;

   if (stmt->batchMode)
   {
      if (stmt->batchSize == 0)
      {
         stmt->batchMode = false;
         stmt->batchBindings->clear();
         return DBERR_SUCCESS;   // empty batch
      }

      for(int i = 0; i < stmt->batchBindings->size(); i++)
      {
         OracleBatchBind *b = stmt->batchBindings->get(i);
         if (b == NULL)
            continue;

         OCIBind *handleBind = nullptr;
         OCIBindByPos(stmt->handleStmt, &handleBind, stmt->handleError, i + 1, b->getData(), 
               b->getElementSize(), b->getOraType(), nullptr, nullptr, nullptr, 0, nullptr, OCI_DEFAULT);
      }
   }

	MutexLock(pConn->mutexQueryLock);
   if (IsSuccess(OCIStmtExecute(pConn->handleService, stmt->handleStmt, pConn->handleError,
                      stmt->batchMode ? stmt->batchSize : 1, 0, nullptr, nullptr,
	                   (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
	{
		dwResult = DBERR_SUCCESS;
	}
	else
	{
		SetLastError(pConn);
		dwResult = IsConnectionError(pConn);
	}

	if (errorText != nullptr)
	{
		wcslcpy(errorText, pConn->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	MutexUnlock(pConn->mutexQueryLock);

   if (stmt->batchMode)
   {
      stmt->batchMode = false;
      stmt->batchBindings->clear();
   }

	return dwResult;
}

/**
 * Destroy prepared statement
 */
extern "C" void __EXPORT DrvFreeStatement(ORACLE_STATEMENT *stmt)
{
	if (stmt == nullptr)
		return;

	MutexLock(stmt->connection->mutexQueryLock);
   delete stmt->bindings;
   delete stmt->batchBindings;
	OCIStmtRelease(stmt->handleStmt, stmt->handleError, nullptr, 0, OCI_DEFAULT);
	OCIHandleFree(stmt->handleError, OCI_HTYPE_ERROR);
	MutexUnlock(stmt->connection->mutexQueryLock);
	MemFree(stmt);
}

/**
 * Perform non-SELECT query
 */
static uint32_t DrvQueryInternal(ORACLE_CONN *pConn, const WCHAR *pwszQuery, WCHAR *errorText)
{
   uint32_t result;

#if UNICODE_UCS4
   UCS2CHAR localBuffer[1024];
   UCS2CHAR *ucs2Query = WideStringToUCS2(pwszQuery, localBuffer, 1024);
#else
   const UCS2CHAR *ucs2Query = pwszQuery;
#endif

   MutexLock(pConn->mutexQueryLock);
   OCIStmt *handleStmt;
	if (IsSuccess(OCIStmtPrepare2(pConn->handleService, &handleStmt, pConn->handleError, (text *)ucs2Query,
	                    (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), NULL, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)))
	{
		if (IsSuccess(OCIStmtExecute(pConn->handleService, handleStmt, pConn->handleError, 1, 0, NULL, NULL,
		                   (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
		{
			result = DBERR_SUCCESS;
		}
		else
		{
			SetLastError(pConn);
			result = IsConnectionError(pConn);
		}
		OCIStmtRelease(handleStmt, pConn->handleError, NULL, 0, OCI_DEFAULT);
	}
	else
	{
		SetLastError(pConn);
		result = IsConnectionError(pConn);
	}
	if (errorText != NULL)
	{
		wcslcpy(errorText, pConn->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	MutexUnlock(pConn->mutexQueryLock);

#if UNICODE_UCS4
	FreeConvertedString(ucs2Query, localBuffer);
#endif
	return result;
}

/**
 * Perform non-SELECT query - entry point
 */
extern "C" DWORD __EXPORT DrvQuery(ORACLE_CONN *conn, const WCHAR *query, WCHAR *errorText)
{
   return DrvQueryInternal(conn, query, errorText);
}

/**
 * Get column name from parameter handle
 *
 * OCI library has memory leak when retrieving column name in UNICODE mode
 * Driver attempts to use workaround described in https://github.com/vrogier/ocilib/issues/31
 * (accessing internal OCI structure directly to avoid conversion to UTF-16 by OCI library)
 */
static char *GetColumnName(OCIParam *handleParam, OCIError *handleError)
{
   if ((s_ociVersionMajor == 11) || (s_ociVersionMajor == 12))
   {
      OCI_PARAM_STRUCT *p = (OCI_PARAM_STRUCT *)handleParam;
      if ((p->columnInfo != NULL) && (p->columnInfo->name != NULL) && (p->columnInfo->attributes[1] != 0))
      {
         size_t len = p->columnInfo->attributes[1];
         char *n = (char *)malloc(len + 1);
         memcpy(n, p->columnInfo->name, len);
         n[len] = 0;
         return n;
      }
   }

   // Use standard method as fallback
   text *colName;
   ub4 size;
   if (OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &colName, &size, OCI_ATTR_NAME, handleError) == OCI_SUCCESS)
   {
      // We are in UTF-16 mode, so OCIAttrGet will return UTF-16 strings
      return MBStringFromUCS2String((UCS2CHAR *)colName);
   }
   else
   {
      return MemCopyStringA("");
   }
}

/**
 * Process SELECT results
 */
static ORACLE_RESULT *ProcessQueryResults(ORACLE_CONN *pConn, OCIStmt *handleStmt, DWORD *pdwError)
{
	OCIDefine *handleDefine;
	ub4 nCount;
	ub2 nWidth;

	ORACLE_RESULT *pResult = MemAllocStruct<ORACLE_RESULT>();
	OCIAttrGet(handleStmt, OCI_HTYPE_STMT, &nCount, nullptr, OCI_ATTR_PARAM_COUNT, pConn->handleError);
	pResult->nCols = nCount;
	if (pResult->nCols > 0)
	{
      sword nStatus;

		// Prepare receive buffers and fetch column names
		pResult->columnNames = MemAllocArray<char*>(pResult->nCols);
		OracleFetchBuffer *pBuffers = MemAllocArray<OracleFetchBuffer>(pResult->nCols);
		for(int i = 0; i < pResult->nCols; i++)
		{
		   OCIParam *handleParam;
			if ((nStatus = OCIParamGet(handleStmt, OCI_HTYPE_STMT, pConn->handleError,
			                           (void **)&handleParam, (ub4)(i + 1))) == OCI_SUCCESS)
			{
				// Column name
            pResult->columnNames[i] = GetColumnName(handleParam, pConn->handleError);

				// Data size
            ub2 type = 0;
				OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &type, NULL, OCI_ATTR_DATA_TYPE, pConn->handleError);
            if (type == OCI_TYPECODE_CLOB)
            {
               pBuffers[i].data = nullptr;
               OCIDescriptorAlloc(s_handleEnv, (void **)&pBuffers[i].lobLocator, OCI_DTYPE_LOB, 0, nullptr);
				   handleDefine = nullptr;
				   nStatus = OCIDefineByPos(handleStmt, &handleDefine, pConn->handleError, i + 1,
                                        &pBuffers[i].lobLocator, 0, SQLT_CLOB, &pBuffers[i].isNull, 
                                        nullptr, nullptr, OCI_DEFAULT);
            }
            else
            {
               pBuffers[i].lobLocator = nullptr;
				   OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &nWidth, nullptr, OCI_ATTR_DATA_SIZE, pConn->handleError);
				   pBuffers[i].data = (UCS2CHAR *)MemAlloc((nWidth + 31) * sizeof(UCS2CHAR));
				   handleDefine = nullptr;
				   nStatus = OCIDefineByPos(handleStmt, &handleDefine, pConn->handleError, i + 1,
                                        pBuffers[i].data, (nWidth + 31) * sizeof(UCS2CHAR),
				                            SQLT_CHR, &pBuffers[i].isNull, &pBuffers[i].length,
				                            &pBuffers[i].code, OCI_DEFAULT);
            }
            if (nStatus != OCI_SUCCESS)
			   {
				   SetLastError(pConn);
				   *pdwError = IsConnectionError(pConn);
			   }
				OCIDescriptorFree(handleParam, OCI_DTYPE_PARAM);
			}
			else
			{
				SetLastError(pConn);
				*pdwError = IsConnectionError(pConn);
			}
		}

		// Fetch data
		if (nStatus == OCI_SUCCESS)
		{
			int nPos = 0;
			while(1)
			{
				nStatus = OCIStmtFetch2(handleStmt, pConn->handleError, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
				if (nStatus == OCI_NO_DATA)
				{
					*pdwError = DBERR_SUCCESS;	// EOF
					break;
				}
				if ((nStatus != OCI_SUCCESS) && (nStatus != OCI_SUCCESS_WITH_INFO))
				{
					SetLastError(pConn);
					*pdwError = IsConnectionError(pConn);
					break;
				}

				// New row
				pResult->nRows++;
				pResult->pData = MemRealloc(pResult->pData, sizeof(WCHAR *) * pResult->nCols * pResult->nRows);
				for(int i = 0; i < pResult->nCols; i++)
				{
					if (pBuffers[i].isNull)
					{
						pResult->pData[nPos] = (WCHAR *)MemCopyBlock("\0\0\0", sizeof(WCHAR));
					}
               else if (pBuffers[i].lobLocator != nullptr)
               {
                  ub4 length = 0;
                  ub4 amount = length;
                  OCILobGetLength(pConn->handleService, pConn->handleError, pBuffers[i].lobLocator, &length);
						pResult->pData[nPos] = MemAllocStringW(length + 1);
#if UNICODE_UCS4
                  UCS2CHAR *ucs2buffer = MemAllocArrayNoInit<UCS2CHAR>(length);
                  OCILobRead(pConn->handleService, pConn->handleError, pBuffers[i].lobLocator, &amount, 1, 
                             ucs2buffer, length * sizeof(UCS2CHAR), nullptr, nullptr, OCI_UCS2ID, SQLCS_IMPLICIT);
						ucs2_to_ucs4(ucs2buffer, length, pResult->pData[nPos], length + 1);
                  MemFree(ucs2buffer);
#else
                  OCILobRead(pConn->handleService, pConn->handleError, pBuffers[i].lobLocator, &amount, 1, 
                             pResult->pData[nPos], (length + 1) * sizeof(WCHAR), NULL, NULL, OCI_UCS2ID, SQLCS_IMPLICIT);
#endif
						pResult->pData[nPos][length] = 0;
               }
					else
					{
						int length = pBuffers[i].length / sizeof(UCS2CHAR);
						pResult->pData[nPos] = MemAllocArrayNoInit<WCHAR>(length + 1);
#if UNICODE_UCS4
						ucs2_to_ucs4(pBuffers[i].data, length, pResult->pData[nPos], length + 1);
#else
						memcpy(pResult->pData[nPos], pBuffers[i].data, pBuffers[i].length);
#endif
						pResult->pData[nPos][length] = 0;
					}
					nPos++;
				}
			}
		}

		// Cleanup
		for(int i = 0; i < pResult->nCols; i++)
      {
			MemFree(pBuffers[i].data);
         if (pBuffers[i].lobLocator != nullptr)
         {
            OCIDescriptorFree(pBuffers[i].lobLocator, OCI_DTYPE_LOB);
         }
      }
		MemFree(pBuffers);

		// Destroy results in case of error
		if (*pdwError != DBERR_SUCCESS)
		{
			DestroyQueryResult(pResult);
			pResult = nullptr;
		}
	}

	return pResult;
}

/**
 * Perform SELECT query
 */
static ORACLE_RESULT *DrvSelectInternal(ORACLE_CONN *conn, WCHAR *query, DWORD *errorCode, WCHAR *errorText)
{
	ORACLE_RESULT *pResult = NULL;
	OCIStmt *handleStmt;

#if UNICODE_UCS4
	UCS2CHAR localBuffer[1024];
	UCS2CHAR *ucs2Query = WideStringToUCS2(query, localBuffer, 1024);
#else
	UCS2CHAR *ucs2Query = query;
#endif

	MutexLock(conn->mutexQueryLock);
	if (IsSuccess(OCIStmtPrepare2(conn->handleService, &handleStmt, conn->handleError, (text *)ucs2Query,
	                    (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), NULL, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)))
	{
      OCIAttrSet(handleStmt, OCI_HTYPE_STMT, &conn->prefetchLimit, 0, OCI_ATTR_PREFETCH_ROWS, conn->handleError);
		if (IsSuccess(OCIStmtExecute(conn->handleService, handleStmt, conn->handleError,
		                   0, 0, NULL, NULL, (conn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
		{
			pResult = ProcessQueryResults(conn, handleStmt, errorCode);
		}
		else
		{
			SetLastError(conn);
			*errorCode = IsConnectionError(conn);
		}
		OCIStmtRelease(handleStmt, conn->handleError, NULL, 0, OCI_DEFAULT);
	}
	else
	{
		SetLastError(conn);
		*errorCode = IsConnectionError(conn);
	}
	if (errorText != nullptr)
	{
		wcslcpy(errorText, conn->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	MutexUnlock(conn->mutexQueryLock);

#if UNICODE_UCS4
	FreeConvertedString(ucs2Query, localBuffer);
#endif
	return pResult;
}

/**
 * Perform SELECT query - public entry point
 */
extern "C" DBDRV_RESULT __EXPORT DrvSelect(ORACLE_CONN *conn, WCHAR *query, DWORD *errorCode, WCHAR *errorText)
{
   return DrvSelectInternal(conn, query, errorCode, errorText);
}

/**
 * Perform SELECT query using prepared statement
 */
extern "C" DBDRV_RESULT __EXPORT DrvSelectPrepared(ORACLE_CONN *pConn, ORACLE_STATEMENT *stmt, DWORD *pdwError, WCHAR *errorText)
{
	ORACLE_RESULT *pResult = NULL;

	MutexLock(pConn->mutexQueryLock);
   OCIAttrSet(stmt->handleStmt, OCI_HTYPE_STMT, &pConn->prefetchLimit, 0, OCI_ATTR_PREFETCH_ROWS, pConn->handleError);
	if (IsSuccess(OCIStmtExecute(pConn->handleService, stmt->handleStmt, pConn->handleError,
	                   0, 0, NULL, NULL, (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
	{
		pResult = ProcessQueryResults(pConn, stmt->handleStmt, pdwError);
	}
	else
	{
		SetLastError(pConn);
		*pdwError = IsConnectionError(pConn);
	}

	if (errorText != NULL)
	{
		wcslcpy(errorText, pConn->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	MutexUnlock(pConn->mutexQueryLock);

	return pResult;
}

/**
 * Get field length from result
 */
extern "C" LONG __EXPORT DrvGetFieldLength(ORACLE_RESULT *pResult, int nRow, int nColumn)
{
	if (pResult == NULL)
		return -1;

	if ((nRow >= 0) && (nRow < pResult->nRows) &&
		 (nColumn >= 0) && (nColumn < pResult->nCols))
		return (LONG)wcslen(pResult->pData[pResult->nCols * nRow + nColumn]);
	
	return -1;
}

/**
 * Get field value from result
 */
extern "C" WCHAR __EXPORT *DrvGetField(ORACLE_RESULT *pResult, int nRow, int nColumn, WCHAR *pBuffer, int nBufLen)
{
   WCHAR *pValue = nullptr;
   if (pResult != nullptr)
   {
      if ((nRow < pResult->nRows) && (nRow >= 0) && (nColumn < pResult->nCols) && (nColumn >= 0))
      {
         wcslcpy(pBuffer, pResult->pData[nRow * pResult->nCols + nColumn], nBufLen);
         pValue = pBuffer;
      }
   }
   return pValue;
}

/**
 * Get number of rows in result
 */
extern "C" int __EXPORT DrvGetNumRows(ORACLE_RESULT *pResult)
{
	return (pResult != nullptr) ? pResult->nRows : 0;
}

/**
 * Get column count in query result
 */
extern "C" int __EXPORT DrvGetColumnCount(ORACLE_RESULT *pResult)
{
	return (pResult != NULL) ? pResult->nCols : 0;
}

/**
 * Get column name in query result
 */
extern "C" const char __EXPORT *DrvGetColumnName(ORACLE_RESULT *pResult, int column)
{
	return ((pResult != NULL) && (column >= 0) && (column < pResult->nCols)) ? pResult->columnNames[column] : NULL;
}

/**
 * Free SELECT results
 */
extern "C" void __EXPORT DrvFreeResult(ORACLE_RESULT *pResult)
{
	if (pResult != NULL)
		DestroyQueryResult(pResult);
}

/**
 * Destroy unbuffered query result
 */
static void DestroyUnbufferedQueryResult(ORACLE_UNBUFFERED_RESULT *result, bool freeStatement)
{
   if (freeStatement)
      OCIStmtRelease(result->handleStmt, result->connection->handleError, nullptr, 0, OCI_DEFAULT);

   for(int i = 0; i < result->nCols; i++)
   {
      MemFree(result->pBuffers[i].data);
      if (result->pBuffers[i].lobLocator != nullptr)
      {
         OCIDescriptorFree(result->pBuffers[i].lobLocator, OCI_DTYPE_LOB);
      }
   }
   MemFree(result->pBuffers);

   for(int i = 0; i < result->nCols; i++)
      MemFree(result->columnNames[i]);
   MemFree(result->columnNames);

   MemFree(result);
}

/**
 * Process results of unbuffered query execution (prepare for fetching results)
 */
static ORACLE_UNBUFFERED_RESULT *ProcessUnbufferedQueryResults(ORACLE_CONN *pConn, OCIStmt *handleStmt, DWORD *pdwError)
{
   ORACLE_UNBUFFERED_RESULT *result = (ORACLE_UNBUFFERED_RESULT *)malloc(sizeof(ORACLE_UNBUFFERED_RESULT));
   result->handleStmt = handleStmt;
   result->connection = pConn;

   ub4 nCount;
   OCIAttrGet(result->handleStmt, OCI_HTYPE_STMT, &nCount, NULL, OCI_ATTR_PARAM_COUNT, pConn->handleError);
   result->nCols = nCount;
   if (result->nCols > 0)
   {
      // Prepare receive buffers and fetch column names
      result->columnNames = MemAllocArray<char*>(result->nCols);
      result->pBuffers = MemAllocArray<OracleFetchBuffer>(result->nCols);
      for(int i = 0; i < result->nCols; i++)
      {
         OCIParam *handleParam;

         result->pBuffers[i].isNull = 1;   // Mark all columns as NULL initially
         if (OCIParamGet(result->handleStmt, OCI_HTYPE_STMT, pConn->handleError,
                         (void **)&handleParam, (ub4)(i + 1)) == OCI_SUCCESS)
         {
            // Column name
            result->columnNames[i] = GetColumnName(handleParam, pConn->handleError);

            // Data size
            sword nStatus;
            ub2 type = 0;
            OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &type, nullptr, OCI_ATTR_DATA_TYPE, pConn->handleError);
            OCIDefine *handleDefine;
            if (type == OCI_TYPECODE_CLOB)
            {
               result->pBuffers[i].data = nullptr;
               OCIDescriptorAlloc(s_handleEnv, (void **)&result->pBuffers[i].lobLocator, OCI_DTYPE_LOB, 0, nullptr);
               handleDefine = nullptr;
               nStatus = OCIDefineByPos(result->handleStmt, &handleDefine, pConn->handleError, i + 1,
                                        &result->pBuffers[i].lobLocator, 0, SQLT_CLOB, &result->pBuffers[i].isNull,
                                        nullptr, nullptr, OCI_DEFAULT);
            }
            else
            {
               ub2 nWidth;
               result->pBuffers[i].lobLocator = nullptr;
               OCIAttrGet(handleParam, OCI_DTYPE_PARAM, &nWidth, nullptr, OCI_ATTR_DATA_SIZE, pConn->handleError);
               result->pBuffers[i].data = (UCS2CHAR *)MemAlloc((nWidth + 31) * sizeof(UCS2CHAR));
               handleDefine = nullptr;
               nStatus = OCIDefineByPos(result->handleStmt, &handleDefine, pConn->handleError, i + 1,
                                        result->pBuffers[i].data, (nWidth + 31) * sizeof(UCS2CHAR),
                                        SQLT_CHR, &result->pBuffers[i].isNull, &result->pBuffers[i].length,
                                        &result->pBuffers[i].code, OCI_DEFAULT);
            }
            OCIDescriptorFree(handleParam, OCI_DTYPE_PARAM);
            if (nStatus == OCI_SUCCESS)
            {
               *pdwError = DBERR_SUCCESS;
            }
            else
            {
               SetLastError(pConn);
               *pdwError = IsConnectionError(pConn);
               DestroyUnbufferedQueryResult(result, false);
               result = nullptr;
               break;
            }
         }
         else
         {
            SetLastError(pConn);
            *pdwError = IsConnectionError(pConn);
            DestroyUnbufferedQueryResult(result, false);
            result = nullptr;
            break;
         }
      }
   }
   else
   {
      MemFree(result);
      result = nullptr;
   }

   return result;
}

/**
 * Perform unbuffered SELECT query
 */
extern "C" DBDRV_UNBUFFERED_RESULT __EXPORT DrvSelectUnbuffered(ORACLE_CONN *pConn, WCHAR *pwszQuery, DWORD *pdwError, WCHAR *errorText)
{
   ORACLE_UNBUFFERED_RESULT *result = nullptr;

#if UNICODE_UCS4
   UCS2CHAR localBuffer[1024];
	UCS2CHAR *ucs2Query = WideStringToUCS2(pwszQuery, localBuffer, 1024);
#else
	UCS2CHAR *ucs2Query = pwszQuery;
#endif

	MutexLock(pConn->mutexQueryLock);

	OCIStmt *handleStmt;
	if (IsSuccess(OCIStmtPrepare2(pConn->handleService, &handleStmt, pConn->handleError, (text *)ucs2Query,
	                    (ub4)ucs2_strlen(ucs2Query) * sizeof(UCS2CHAR), NULL, 0, OCI_NTV_SYNTAX, OCI_DEFAULT)))
	{
      OCIAttrSet(handleStmt, OCI_HTYPE_STMT, &pConn->prefetchLimit, 0, OCI_ATTR_PREFETCH_ROWS, pConn->handleError);
		if (IsSuccess(OCIStmtExecute(pConn->handleService, handleStmt, pConn->handleError,
		                   0, 0, NULL, NULL, (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
		{
		   result = ProcessUnbufferedQueryResults(pConn, handleStmt, pdwError);
		}
		else
		{
			SetLastError(pConn);
			*pdwError = IsConnectionError(pConn);
		}
	}
	else
	{
		SetLastError(pConn);
		*pdwError = IsConnectionError(pConn);
	}

#if UNICODE_UCS4
	FreeConvertedString(ucs2Query, localBuffer);
#endif

	if ((*pdwError == DBERR_SUCCESS) && (result != NULL))
		return result;

	// On failure, unlock query mutex and do cleanup
	OCIStmtRelease(handleStmt, pConn->handleError, NULL, 0, OCI_DEFAULT);
	if (errorText != NULL)
	{
		wcslcpy(errorText, pConn->lastErrorText, DBDRV_MAX_ERROR_TEXT);
	}
	MutexUnlock(pConn->mutexQueryLock);
	return NULL;
}

/**
 * Perform SELECT query using prepared statement
 */
extern "C" DBDRV_UNBUFFERED_RESULT __EXPORT DrvSelectPreparedUnbuffered(ORACLE_CONN *pConn, ORACLE_STATEMENT *stmt, DWORD *pdwError, WCHAR *errorText)
{
   ORACLE_UNBUFFERED_RESULT *result = NULL;

   MutexLock(pConn->mutexQueryLock);

   OCIAttrSet(stmt->handleStmt, OCI_HTYPE_STMT, &pConn->prefetchLimit, 0, OCI_ATTR_PREFETCH_ROWS, pConn->handleError);
   if (IsSuccess(OCIStmtExecute(pConn->handleService, stmt->handleStmt, pConn->handleError,
                      0, 0, NULL, NULL, (pConn->nTransLevel == 0) ? OCI_COMMIT_ON_SUCCESS : OCI_DEFAULT)))
   {
      result = ProcessUnbufferedQueryResults(pConn, stmt->handleStmt, pdwError);
   }
   else
   {
      SetLastError(pConn);
      *pdwError = IsConnectionError(pConn);
   }

   if ((*pdwError == DBERR_SUCCESS) && (result != NULL))
      return result;

   // On failure, unlock query mutex and do cleanup
   if (errorText != NULL)
   {
      wcslcpy(errorText, pConn->lastErrorText, DBDRV_MAX_ERROR_TEXT);
   }
   MutexUnlock(pConn->mutexQueryLock);
   return NULL;
}

/**
 * Fetch next result line from unbuffered SELECT results
 */
extern "C" bool __EXPORT DrvFetch(ORACLE_UNBUFFERED_RESULT *result)
{
	bool success;

	if (result == NULL)
		return false;

	sword rc = OCIStmtFetch2(result->handleStmt, result->connection->handleError, 1, OCI_FETCH_NEXT, 0, OCI_DEFAULT);
	if ((rc == OCI_SUCCESS) || (rc == OCI_SUCCESS_WITH_INFO))
	{
		success = true;
	}
	else
	{
		SetLastError(result->connection);
		success = false;
	}
	return success;
}

/**
 * Get field length from current row in unbuffered query result
 */
extern "C" LONG __EXPORT DrvGetFieldLengthUnbuffered(ORACLE_UNBUFFERED_RESULT *result, int nColumn)
{
	if (result == NULL)
		return 0;

	if ((nColumn < 0) || (nColumn >= result->nCols))
		return 0;

	if (result->pBuffers[nColumn].isNull)
		return 0;

   if (result->pBuffers[nColumn].lobLocator != nullptr)
   {
      ub4 length = 0;
      OCILobGetLength(result->connection->handleService, result->connection->handleError, result->pBuffers[nColumn].lobLocator, &length);
      return (LONG)length;
   }

	return (LONG)(result->pBuffers[nColumn].length / sizeof(UCS2CHAR));
}

/**
 * Get field from current row in unbuffered query result
 */
extern "C" WCHAR __EXPORT *DrvGetFieldUnbuffered(ORACLE_UNBUFFERED_RESULT *result, int nColumn, WCHAR *pBuffer, int nBufSize)
{
	if (result == nullptr)
		return nullptr;

	if ((nColumn < 0) || (nColumn >= result->nCols))
		return nullptr;

	if (result->pBuffers[nColumn].isNull)
	{
		*pBuffer = 0;
	}
   else if (result->pBuffers[nColumn].lobLocator != nullptr)
   {
      ub4 length = 0;
      OCILobGetLength(result->connection->handleService, result->connection->handleError, result->pBuffers[nColumn].lobLocator, &length);

		int nLen = std::min(nBufSize - 1, (int)length);
      ub4 amount = nLen;
#if UNICODE_UCS4
      UCS2CHAR *ucs2buffer = MemAllocArrayNoInit<UCS2CHAR>(nLen);
      OCILobRead(result->connection->handleService, result->connection->handleError, result->pBuffers[nColumn].lobLocator, &amount, 1,
                 ucs2buffer, nLen * sizeof(UCS2CHAR), nullptr, nullptr, OCI_UCS2ID, SQLCS_IMPLICIT);
		ucs2_to_ucs4(ucs2buffer, nLen, pBuffer, nLen);
      MemFree(ucs2buffer);
#else
      OCILobRead(result->connection->handleService, result->connection->handleError, result->pBuffers[nColumn].lobLocator, &amount, 1,
                 pBuffer, nBufSize * sizeof(WCHAR), nullptr, nullptr, OCI_UCS2ID, SQLCS_IMPLICIT);
#endif
		pBuffer[nLen] = 0;
   }
	else
	{
		int nLen = std::min(nBufSize - 1, ((int)(result->pBuffers[nColumn].length / sizeof(UCS2CHAR))));
#if UNICODE_UCS4
		ucs2_to_ucs4(result->pBuffers[nColumn].data, nLen, pBuffer, nLen + 1);
#else
		memcpy(pBuffer, result->pBuffers[nColumn].data, nLen * sizeof(WCHAR));
#endif
		pBuffer[nLen] = 0;
	}

	return pBuffer;
}

/**
 * Get column count in unbuffered query result
 */
extern "C" int __EXPORT DrvGetColumnCountUnbuffered(ORACLE_UNBUFFERED_RESULT *result)
{
	return (result != nullptr) ? result->nCols : 0;
}

/**
 * Get column name in unbuffered query result
 */
extern "C" const char __EXPORT *DrvGetColumnNameUnbuffered(ORACLE_UNBUFFERED_RESULT *result, int column)
{
	return ((result != nullptr) && (column >= 0) && (column < result->nCols)) ? result->columnNames[column] : nullptr;
}

/**
 * Destroy result of unbuffered query
 */
extern "C" void __EXPORT DrvFreeUnbufferedResult(ORACLE_UNBUFFERED_RESULT *result)
{
	if (result == nullptr)
		return;

	MUTEX mutex = result->connection->mutexQueryLock;
	DestroyUnbufferedQueryResult(result, true);
	MutexUnlock(mutex);
}

/**
 * Begin transaction
 */
extern "C" DWORD __EXPORT DrvBegin(ORACLE_CONN *pConn)
{
	if (pConn == nullptr)
		return DBERR_INVALID_HANDLE;

	MutexLock(pConn->mutexQueryLock);
	pConn->nTransLevel++;
	MutexUnlock(pConn->mutexQueryLock);
	return DBERR_SUCCESS;
}

/**
 * Commit transaction
 */
extern "C" DWORD __EXPORT DrvCommit(ORACLE_CONN *pConn)
{
	if (pConn == nullptr)
		return DBERR_INVALID_HANDLE;

   DWORD dwResult;

	MutexLock(pConn->mutexQueryLock);
	if (pConn->nTransLevel > 0)
	{
		if (IsSuccess(OCITransCommit(pConn->handleService, pConn->handleError, OCI_DEFAULT)))
		{
			dwResult = DBERR_SUCCESS;
			pConn->nTransLevel = 0;
		}
		else
		{
			SetLastError(pConn);
			dwResult = IsConnectionError(pConn);
		}
	}
	else
	{
		dwResult = DBERR_SUCCESS;
	}
	MutexUnlock(pConn->mutexQueryLock);
	return dwResult;
}

/**
 * Rollback transaction
 */
extern "C" DWORD __EXPORT DrvRollback(ORACLE_CONN *pConn)
{
	if (pConn == nullptr)
		return DBERR_INVALID_HANDLE;

   DWORD dwResult;

	MutexLock(pConn->mutexQueryLock);
	if (pConn->nTransLevel > 0)
	{
		if (IsSuccess(OCITransRollback(pConn->handleService, pConn->handleError, OCI_DEFAULT)))
		{
			dwResult = DBERR_SUCCESS;
			pConn->nTransLevel = 0;
		}
		else
		{
			SetLastError(pConn);
			dwResult = IsConnectionError(pConn);
		}
	}
	else
	{
		dwResult = DBERR_SUCCESS;
	}
	MutexUnlock(pConn->mutexQueryLock);
	return dwResult;
}

/**
 * Check if table exist
 */
extern "C" int __EXPORT DrvIsTableExist(ORACLE_CONN *conn, const WCHAR *name)
{
   WCHAR query[256];
   swprintf(query, 256, L"SELECT count(*) FROM user_tables WHERE table_name=upper('%ls')", name);
   DWORD error;
   int rc = DBIsTableExist_Failure;
   ORACLE_RESULT *hResult = DrvSelectInternal(conn, query, &error, nullptr);
   if (hResult != nullptr)
   {
      if ((hResult->nRows > 0) && (hResult->nCols > 0))
      {
         rc = (wcstol(hResult->pData[0], nullptr, 10) > 0) ? DBIsTableExist_Found : DBIsTableExist_NotFound;
      }
      else
      {
         rc = DBIsTableExist_NotFound;
      }
      DestroyQueryResult(hResult);
   }
   return rc;
}

#ifdef _WIN32

/**
 * DLL Entry point
 */
bool WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
		DisableThreadLibraryCalls(hInstance);
	return true;
}

#endif
