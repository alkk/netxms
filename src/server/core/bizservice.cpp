/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2011 NetXMS Team
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
** File: bizservice.cpp
**
**/

#include "nxcore.h"

#define QUERY_LENGTH		(512)

//
// Service default constructor
//

BusinessService::BusinessService() : ServiceContainer()
{
	m_busy = false;
	m_lastPollTime = time_t(0);
	m_lastPollStatus = STATUS_UNKNOWN;
	_tcscpy(m_szName, _T("Default"));
}


//
// Constructor for new service object
//

BusinessService::BusinessService(const TCHAR *name) : ServiceContainer(name)
{
	m_busy = false;
	m_lastPollTime = time_t(0);
	m_lastPollStatus = STATUS_UNKNOWN;
	nx_strncpy(m_szName, name, MAX_OBJECT_NAME);
}


//
// Service class destructor
//

BusinessService::~BusinessService()
{
}


//
// Create object from database data
//

BOOL BusinessService::CreateFromDB(DWORD id)
{
	if (!ServiceContainer::CreateFromDB(id))
		return FALSE;

	// now it doesn't make any sense but hopefully will do in the future
	DB_STATEMENT hStmt = DBPrepare(g_hCoreDB, _T("SELECT service_id FROM business_services WHERE service_id=?"));
	if (hStmt == NULL)
	{
		DbgPrintf(4, _T("Cannot prepare select from business_services"));
		return FALSE;
	}
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult == NULL)
	{
		DBFreeStatement(hStmt);
		return FALSE;
	}

	if (DBGetNumRows(hResult) == 0)
	{
		DBFreeResult(hResult);
		DBFreeStatement(hStmt);
		DbgPrintf(4, _T("Cannot load biz service object %ld - record missing"), (long)m_dwId);
		return FALSE;
	}

	DBFreeResult(hResult);
	DBFreeStatement(hStmt);

	return TRUE;
}


//
// Save service to database
//

BOOL BusinessService::SaveToDB(DB_HANDLE hdb)
{
	BOOL bNewObject = TRUE;

	LockData();

	DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT service_id FROM business_services WHERE service_id=?"));
	if (hStmt == NULL)
		return FALSE;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	DB_RESULT hResult = DBSelectPrepared(hStmt);
	if (hResult != NULL)
	{
		bNewObject = (DBGetNumRows(hResult) <= 0);
		DBFreeResult(hResult);
	}
	DBFreeStatement(hStmt);

	hStmt = DBPrepare(g_hCoreDB, bNewObject ? _T("INSERT INTO business_services (service_id) VALUES (?)") :
											  _T("UPDATE business_services SET service_id=service_id WHERE service_id=?"));
	if (hStmt == NULL)	
		return FALSE;
	DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, m_dwId);
	if (!DBExecute(hStmt))
	{
		DBFreeStatement(hStmt);
		return FALSE;
	}

	DBFreeStatement(hStmt);

	saveACLToDB(hdb);

	// Unlock object and clear modification flag
	m_bIsModified = FALSE;
	UnlockData();

	return ServiceContainer::SaveToDB(hdb);
}


//
// Delete object from database
//

BOOL BusinessService::DeleteFromDB()
{
   TCHAR szQuery[QUERY_LENGTH];
   BOOL bSuccess;

   bSuccess = ServiceContainer::DeleteFromDB();
   if (bSuccess)
   {
      _sntprintf(szQuery, QUERY_LENGTH, _T("DELETE FROM business_services WHERE id=%d"), m_dwId);
      QueueSQLRequest(szQuery);
   }

   return bSuccess;
}


//
// Create CSCP message with object's data
//

void BusinessService::CreateMessage(CSCPMessage *pMsg)
{
   ServiceContainer::CreateMessage(pMsg);
}


//
// Modify object from message
//

DWORD BusinessService::ModifyFromMessage(CSCPMessage *pRequest, BOOL bAlreadyLocked)
{
   if (!bAlreadyLocked)
      LockData();

   return ServiceContainer::ModifyFromMessage(pRequest, TRUE);
}


//
// Check if service is ready for poll
//

bool BusinessService::isReadyForPolling()
{
	return time(NULL) - m_lastPollTime > g_dwSlmPollingInterval && !m_busy;
}


//
// Lock service for polling
//

void BusinessService::lockForPolling()
{
	m_busy = true;
}


//
// A callback for poller threads
//

void BusinessService::poll(ClientSession *pSession, DWORD dwRqId, int nPoller)
{
	DbgPrintf(5, _T("Started polling of business service %s [%d]"), m_szName, (int)m_dwId);
	m_lastPollTime = time(NULL);

	// Loop through the kids and execute their either scripts or thresholds
   LockChildList(FALSE);
	for (DWORD i = 0; i < m_dwChildCount; i++)
	{
		if (m_pChildList[i]->Type() == OBJECT_SLMCHECK)
			((SlmCheck *)m_pChildList[i])->execute();
		else if (m_pChildList[i]->Type() == OBJECT_NODELINK)
			((NodeLink*)m_pChildList[i])->execute();
	}
   UnlockChildList();

	// Set the status based on what the kids' been up to
	calculateCompoundStatus();

	m_lastPollStatus = m_iStatus;
	DbgPrintf(5, _T("Finished polling of business service %s [%d]"), m_szName, (int)m_dwId);
	m_busy = false;
}


//
// Get template checks applicable for given target
//

void BusinessService::getApplicableTemplates(ServiceContainer *target, ObjectArray<SlmCheck> *templates)
{
	LockChildList(FALSE);
	for(DWORD i = 0; i < m_dwChildCount; i++)
	{
		if ((m_pChildList[i]->Type() == OBJECT_SLMCHECK) &&
          ((SlmCheck *)m_pChildList[i])->isTemplate())
		{
			m_pChildList[i]->IncRefCount();
			templates->add((SlmCheck *)m_pChildList[i]);
		}
	}
	UnlockChildList();

	LockParentList(FALSE);
	for(DWORD i = 0; i < m_dwParentCount; i++)
	{
		if (m_pParentList[i]->Type() == OBJECT_BUSINESSSERVICE)
		{
			((BusinessService *)m_pParentList[i])->getApplicableTemplates(target, templates);
		}
	}
	UnlockParentList();
}
