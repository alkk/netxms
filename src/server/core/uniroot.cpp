/* 
** NetXMS - Network Management System
** Copyright (C) 2003, 2004 Victor Kirhenshtein
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
** $module: uniroot.cpp
**
**/

#include "nxcore.h"


//
// Constructor
//

UniversalRoot::UniversalRoot()
              :NetObj()
{
}


//
// Destructor
//

UniversalRoot::~UniversalRoot()
{
}


//
// Link child objects
// This method is expected to be called only at startup, so we don't lock
//

void UniversalRoot::LinkChildObjects(void)
{
   DWORD i, dwNumChilds, dwObjectId;
   NetObj *pObject;
   char szQuery[256];
   DB_RESULT hResult;

   // Load child list and link objects
   sprintf(szQuery, "SELECT object_id FROM container_members WHERE container_id=%d", m_dwId);
   hResult = DBSelect(g_hCoreDB, szQuery);
   if (hResult != NULL)
   {
      dwNumChilds = DBGetNumRows(hResult);
      for(i = 0; i < dwNumChilds; i++)
      {
         dwObjectId = DBGetFieldULong(hResult, i, 0);
         pObject = FindObjectById(dwObjectId);
         if (pObject != NULL)
            LinkObject(pObject);
         else
            WriteLog(MSG_ROOT_INVALID_CHILD_ID, EVENTLOG_WARNING_TYPE, "ds", 
                     dwObjectId, g_szClassName[Type()]);
      }
      DBFreeResult(hResult);
   }
}


//
// Save object to database
//

BOOL UniversalRoot::SaveToDB(void)
{
   char szQuery[1024];
   DWORD i;

   Lock();

   SaveCommonProperties();

   // Update members list
   sprintf(szQuery, "DELETE FROM container_members WHERE container_id=%d", m_dwId);
   DBQuery(g_hCoreDB, szQuery);
   for(i = 0; i < m_dwChildCount; i++)
   {
      sprintf(szQuery, "INSERT INTO container_members (container_id,object_id) VALUES (%ld,%ld)", m_dwId, m_pChildList[i]->Id());
      DBQuery(g_hCoreDB, szQuery);
   }

   // Save access list
   SaveACLToDB();

   // Unlock object and clear modification flag
   Unlock();
   m_bIsModified = FALSE;
   return TRUE;
}


//
// Load properties from database
//

void UniversalRoot::LoadFromDB(void)
{
   Lock();
   LoadCommonProperties();
   LoadACLFromDB();
   Unlock();
}
