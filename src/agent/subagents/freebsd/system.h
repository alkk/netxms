/* $Id: system.h,v 1.3 2005-01-23 05:08:06 alk Exp $ */

/* 
** NetXMS subagent for FreeBSD
** Copyright (C) 2004 Alex Kirhenshtein
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
**/

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

enum
{
	PHYSICAL_FREE,
	PHYSICAL_USED,
	PHYSICAL_TOTAL,
	SWAP_FREE,
	SWAP_USED,
	SWAP_TOTAL,
	VIRTUAL_FREE,
	VIRTUAL_USED,
	VIRTUAL_TOTAL,
};

LONG H_ProcessList(char *, char *, NETXMS_VALUES_LIST *);
LONG H_Uptime(char *, char *, char *);
LONG H_Uname(char *, char *, char *);
LONG H_Hostname(char *, char *, char *);
LONG H_Hostname(char *, char *, char *);
LONG H_CpuCount(char *, char *, char *);
LONG H_CpuLoad(char *, char *, char *);
LONG H_CpuUsage(char *, char *, char *);
LONG H_ProcessCount(char *, char *, char *);
LONG H_MemoryInfo(char *, char *, char *);
LONG H_SourcePkgSupport(char *, char *, char *);

#endif // __SYSTEM_H__

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.2  2005/01/17 23:25:48  alk
Agent.SourcePackageSupport added

Revision 1.1  2005/01/17 17:14:32  alk
freebsd agent, incomplete (but working)

Revision 1.1  2004/10/22 22:08:35  alk
source restructured;
implemented:
	Net.IP.Forwarding
	Net.IP6.Forwarding
	Process.Count(*)
	Net.ArpCache
	Net.InterfaceList (if-type not implemented yet)
	System.ProcessList


*/
