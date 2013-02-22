/* 
** NetXMS - Network Management System
** Copyright (C) 2003-2012 Victor Kirhenshtein
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
** File: correlate.cpp
**
**/

#include "nxcore.h"

/**
 * Static data
 */
static QWORD m_networkLostEventId = 0;

/**
 * Correlate SYS_NODE_DOWN event
 */
static void C_SysNodeDown(Node *pNode, Event *pEvent)
{
	// Check for NetXMS server netwok connectivity
	if (g_dwFlags & AF_NO_NETWORK_CONNECTIVITY)
	{
		pEvent->setRootId(m_networkLostEventId);
		return;
	}

	// Check directly connected switch
	Interface *iface = pNode->findInterface(0, pNode->IpAddr());
	if ((iface != NULL) && (iface->getPeerNodeId() != 0))
	{
		Node *peerNode = (Node *)FindObjectById(iface->getPeerNodeId(), OBJECT_NODE);
		if ((peerNode != NULL) && peerNode->isDown())
		{
         pEvent->setRootId(peerNode->getLastEventId(LAST_EVENT_NODE_DOWN));
			DbgPrintf(5, _T("C_SysNodeDown: upstream node %s [%d] for current node %s [%d] is down"),
			          peerNode->Name(), peerNode->Id(), pNode->Name(), pNode->Id());
			return;
		}
	}

   // Trace route from management station to failed node and
   // check for failed intermediate nodes or interfaces
   Node *pMgmtNode = (Node *)FindObjectById(g_dwMgmtNode);
   if (pMgmtNode == NULL)
	{
		DbgPrintf(5, _T("C_SysNodeDown: cannot find management node"));
		return;
	}

	NetworkPath *trace = TraceRoute(pMgmtNode, pNode);
	if (trace == NULL)
	{
		DbgPrintf(5, _T("C_SysNodeDown: trace to node %s [%d] not available"), pNode->Name(), pNode->Id());
		return;
	}

   for(int i = 0; i < trace->getHopCount(); i++)
   {
		HOP_INFO *hop = trace->getHopInfo(i);
      if ((hop->object == NULL) || (hop->object == pNode) || (hop->object->Type() != OBJECT_NODE))
			continue;

      if (((Node *)hop->object)->isDown())
      {
         pEvent->setRootId(((Node *)hop->object)->getLastEventId(LAST_EVENT_NODE_DOWN));
			DbgPrintf(5, _T("C_SysNodeDown: upstream node %s [%d] for current node %s [%d] is down"),
			          hop->object->Name(), hop->object->Id(), pNode->Name(), pNode->Id());
			break;
      }

      if (hop->isVpn)
      {
         // Next hop is behind VPN tunnel
         VPNConnector *vpnConn = (VPNConnector *)FindObjectById(hop->ifIndex, OBJECT_VPNCONNECTOR);
         if ((vpnConn != NULL) &&
             (vpnConn->Status() == STATUS_CRITICAL))
         {
            /* TODO: set root id */
         }
      }
      else
      {
         Interface *pInterface = ((Node *)hop->object)->findInterface(hop->ifIndex, INADDR_ANY);
         if ((pInterface != NULL) && ((pInterface->Status() == STATUS_CRITICAL) || (pInterface->Status() == STATUS_DISABLED)))
         {
				DbgPrintf(5, _T("C_SysNodeDown: upstream interface %s [%d] on node %s [%d] for current node %s [%d] is down"),
				          pInterface->Name(), pInterface->Id(), hop->object->Name(), hop->object->Id(), pNode->Name(), pNode->Id());
            pEvent->setRootId(pInterface->getLastDownEventId());
				break;
         }
      }
   }
   delete trace;
}

/**
 * Correlate event
 */
void CorrelateEvent(Event *pEvent)
{
   Node *node = (Node *)FindObjectById(pEvent->getSourceId(), OBJECT_NODE);
   if (node == NULL)
		return;

	DbgPrintf(6, _T("CorrelateEvent: event %s id ") UINT64_FMT _T(" source %s [%d]"),
	          pEvent->getName(), pEvent->getId(), node->Name(), node->Id());

   switch(pEvent->getCode())
   {
      case EVENT_INTERFACE_DISABLED:
			{
				Interface *pInterface = node->findInterface(pEvent->getParameterAsULong(4), INADDR_ANY);
				if (pInterface != NULL)
				{
					pInterface->setLastDownEventId(pEvent->getId());
				}
			}
			break;
      case EVENT_INTERFACE_DOWN:
			{
				Interface *pInterface = node->findInterface(pEvent->getParameterAsULong(4), INADDR_ANY);
				if (pInterface != NULL)
				{
					pInterface->setLastDownEventId(pEvent->getId());
				}
			}
			// there are intentionally no break
      case EVENT_SERVICE_DOWN:
      case EVENT_SNMP_FAIL:
      case EVENT_AGENT_FAIL:
         if (node->getRuntimeFlags() & NDF_UNREACHABLE)
         {
            pEvent->setRootId(node->getLastEventId(LAST_EVENT_NODE_DOWN));
         }
         break;
      case EVENT_NODE_DOWN:
		case EVENT_NODE_UNREACHABLE:
         node->setLastEventId(LAST_EVENT_NODE_DOWN, pEvent->getId());
         C_SysNodeDown(node, pEvent);
         break;
      case EVENT_NODE_UP:
         node->setLastEventId(LAST_EVENT_NODE_DOWN, 0);
         break;
		case EVENT_NETWORK_CONNECTION_LOST:
			m_networkLostEventId = pEvent->getId();
			break;
      default:
         break;
   }

	DbgPrintf(6, _T("CorrelateEvent: finished, rootId=") UINT64_FMT, pEvent->getRootId());
}
