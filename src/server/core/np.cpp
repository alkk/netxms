/* 
** Project X - Network Management System
** Copyright (C) 2003 Victor Kirhenshtein
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
** $module: discovery.cpp
**
**/

#include "nms_core.h"


//
// Poll new node for configuration
//

static void PollNode(DWORD dwIpAddr, DWORD dwNetMask, DWORD dwFlags)
{
   Node *pNode;
   Interface *pInterface;
   Subnet *pSubnet;
   DWORD dwSubnetAddr;

   printf("Polling IP: %s\n", IpToStr(dwIpAddr, NULL));

   // Check for node existence
   if ((FindNodeByIP(dwIpAddr) != NULL) ||
       (FindSubnetByIP(dwIpAddr) != NULL))
   {
      char szBuffer[32];

      WriteLog(MSG_NODE_EXIST, EVENTLOG_WARNING_TYPE, "s", IpToStr(dwIpAddr, szBuffer));
      return;
   }

   // Find appropriate subnet
   dwSubnetAddr = dwIpAddr & dwNetMask;
printf("Subnet: %s\n", IpToStr(dwSubnetAddr, NULL));
   pSubnet = FindSubnetByIP(dwSubnetAddr);
printf("Subnet object: %p\n", pSubnet);
   if (pSubnet == NULL)
   {
printf("Subnet object not found, creating new one\n");
      pSubnet = new Subnet(dwSubnetAddr, dwNetMask);
      NetObjInsert(pSubnet);
printf("Subnet added with ID %d\n",pSubnet->Id());
   }

printf("Creating interface\n");
   pInterface = new Interface(dwIpAddr, dwNetMask);
   NetObjInsert(pInterface);
printf("Interface added with ID %d\n",pInterface->Id());
   pNode = new Node(dwIpAddr, dwFlags, 0);
   pNode->AddInterface(pInterface);
   NetObjInsert(pNode);
printf("Node added with ID %d\n",pNode->Id());

   // Link node to subnet
   pSubnet->AddNode(pNode);
printf("Finished node processing\n");
}


//
// Node poller thread (poll new nodes and put them into the database)
//

void NodePoller(void *arg)
{
   DB_RESULT hResult;

   while(!ShutdownInProgress())
   {
//      ThreadSleep(30);
      ThreadSleep(5);
      if (ShutdownInProgress())
         break;

      hResult = DBSelect(g_hCoreDB, "SELECT id,ip_addr,ip_netmask,discovery_flags FROM NewNodes");
      if (hResult != 0)
      {
         int i, iNumRows;
         DWORD dwId, dwIpAddr, dwNetMask, dwFlags;
         char szQuery[256];

         iNumRows = DBGetNumRows(hResult);
         for(i = 0; i < iNumRows; i++)
         {
            dwId = DBGetFieldULong(hResult, i, 0);
            dwIpAddr = DBGetFieldULong(hResult, i, 1);
            dwNetMask = DBGetFieldULong(hResult, i, 2);
            dwFlags = DBGetFieldULong(hResult, i, 3);

            PollNode(dwIpAddr, dwNetMask, dwFlags);

            // Delete processed node
            sprintf(szQuery, "DELETE FROM NewNodes WHERE id=%ld", dwId);
            DBQuery(g_hCoreDB, szQuery);
         }
         DBFreeResult(hResult);
      }
   }
}
