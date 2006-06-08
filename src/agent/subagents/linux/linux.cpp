/* $Id: linux.cpp,v 1.25 2006-06-08 15:21:29 victor Exp $ */

/* 
** NetXMS subagent for GNU/Linux
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
** $module: linux.cpp
**
**/

#include <nms_common.h>
#include <nms_agent.h>

#include "net.h"
#include "system.h"
#include "disk.h"

//
// Cleanup callback
//

static void Cleanup(void)
{
	ShutdownCpuUsageCollector();
}


//
// Externals
//

LONG H_DRBDDeviceList(char *pszParam, char *pszArg, NETXMS_VALUES_LIST *pValue);
LONG H_DRBDDeviceInfo(TCHAR *pszCmd, TCHAR *pArg, TCHAR *pValue);
LONG H_PhysicalDiskInfo(char *pszParam, char *pszArg, char *pValue);


//
// Subagent information
//

static NETXMS_SUBAGENT_PARAM m_parameters[] =
{
   { "Disk.Avail(*)",                H_DiskInfo,        (char *)DISK_AVAIL,
			DCI_DT_UINT64,	"Available disk space on {instance}" },
   { "Disk.AvailPerc(*)",            H_DiskInfo,        (char *)DISK_AVAIL_PERC,
			DCI_DT_FLOAT,	"Percentage of available disk space on {instance}" },
   { "Disk.Free(*)",                 H_DiskInfo,        (char *)DISK_FREE,
			DCI_DT_UINT64,	"Free disk space on {instance}" },
   { "Disk.FreePerc(*)",             H_DiskInfo,        (char *)DISK_FREE_PERC,
			DCI_DT_FLOAT,	"Percentage of free disk space on {instance}" },
   { "Disk.Total(*)",                H_DiskInfo,        (char *)DISK_TOTAL,
			DCI_DT_UINT64,	"Total disk space on {instance}" },
   { "Disk.Used(*)",                 H_DiskInfo,        (char *)DISK_USED,
			DCI_DT_UINT64,	"Used disk space on {instance}" },
   { "Disk.UsedPerc(*)",             H_DiskInfo,        (char *)DISK_USED_PERC,
			DCI_DT_FLOAT,	"Percentage of used disk space on {instance}" },

   { "DRBD.ConnState(*)",            H_DRBDDeviceInfo,  "c",
		   DCI_DT_INT,    "Connection state of DRBD device {instance}" },
   { "DRBD.ConnStateText(*)",        H_DRBDDeviceInfo,  "C",
		   DCI_DT_INT,    "Connection state of DRBD device {instance} as text" },
   { "DRBD.PeerState(*)",            H_DRBDDeviceInfo,  "p",
		   DCI_DT_INT,    "State of DRBD peer device {instance}" },
   { "DRBD.PeerStateText(*)",        H_DRBDDeviceInfo,  "P",
		   DCI_DT_INT,    "State of DRBD peer device {instance} as text" },
   { "DRBD.State(*)",                H_DRBDDeviceInfo,  "s",
		   DCI_DT_INT,    "State of DRBD device {instance}" },
   { "DRBD.StateText(*)",            H_DRBDDeviceInfo,  "S",
		   DCI_DT_INT,    "State of DRBD device {instance} as text" },

   { "Net.Interface.AdminStatus(*)", H_NetIfInfoFromIOCTL, (char *)IF_INFO_ADMIN_STATUS,
			DCI_DT_INT,		"Administrative status of interface {instance}" },
   { "Net.Interface.BytesIn(*)",     H_NetIfInfoFromProc, (char *)IF_INFO_BYTES_IN,
			DCI_DT_UINT,	"Number of input bytes on interface {instance}" },
   { "Net.Interface.BytesOut(*)",    H_NetIfInfoFromProc, (char *)IF_INFO_BYTES_OUT,
			DCI_DT_UINT,	"Number of output bytes on interface {instance}" },
   { "Net.Interface.Description(*)", H_NetIfInfoFromIOCTL, (char *)IF_INFO_DESCRIPTION,
			DCI_DT_STRING, "Description of interface {instance}" },
   { "Net.Interface.InErrors(*)",    H_NetIfInfoFromProc, (char *)IF_INFO_IN_ERRORS,
			DCI_DT_UINT,	"Number of input errors on interface {instance}" },
   { "Net.Interface.Link(*)",        H_NetIfInfoFromIOCTL, (char *)IF_INFO_OPER_STATUS,
			DCI_DT_INT,		"Operational status of interface {instance}" },
   { "Net.Interface.OutErrors(*)",   H_NetIfInfoFromProc, (char *)IF_INFO_OUT_ERRORS,
			DCI_DT_UINT,	"Number of output errors on interface {instance}" },
   { "Net.Interface.PacketsIn(*)",   H_NetIfInfoFromProc, (char *)IF_INFO_PACKETS_IN,
			DCI_DT_UINT,	"Number of input packets on interface {instance}" },
   { "Net.Interface.PacketsOut(*)",  H_NetIfInfoFromProc, (char *)IF_INFO_PACKETS_OUT,
			DCI_DT_UINT,	"Number of output packets on interface {instance}" },
   { "Net.IP.Forwarding",            H_NetIpForwarding, (char *)4,
			DCI_DT_INT,		"IP forwarding status" },
   { "Net.IP6.Forwarding",           H_NetIpForwarding, (char *)6,
			DCI_DT_INT,		"IPv6 forwarding status" },

   { "PhysicalDisk.SmartAttr(*)",    H_PhysicalDiskInfo, "A",
			DCI_DT_STRING,	"" },
   { "PhysicalDisk.SmartStatus(*)",  H_PhysicalDiskInfo, "S",
			DCI_DT_INT,		"Status of hard disk {instance} reported by SMART" },
   { "PhysicalDisk.Temperature(*)",  H_PhysicalDiskInfo, "T",
			DCI_DT_INT,		"Temperature of hard disk {instance}" },

   { "Process.Count(*)",             H_ProcessCount,    (char *)0,
			DCI_DT_UINT,	"Number of {instance} processes" },
   { "System.ProcessCount",          H_ProcessCount,    (char *)1,
			DCI_DT_UINT,	"Total number of processes" },

   { "System.CPU.LoadAvg",           H_CpuLoad,         NULL,
			DCI_DT_FLOAT,	"Average CPU load for last minute" },
   { "System.CPU.LoadAvg5",          H_CpuLoad,         NULL,
			DCI_DT_FLOAT,	"Average CPU load for last 5 minutes" },
   { "System.CPU.LoadAvg15",         H_CpuLoad,         NULL,
			DCI_DT_FLOAT,	"Average CPU load for last 15 minutes" },
   { "System.CPU.Usage",             H_CpuUsage,        (char *)0,
			DCI_DT_FLOAT,	"" },
   { "System.CPU.Usage5",            H_CpuUsage,        (char *)5,
			DCI_DT_FLOAT,	"" },
   { "System.CPU.Usage15",           H_CpuUsage,        (char *)15,
			DCI_DT_FLOAT,	"" },
   { "System.Hostname",              H_Hostname,        NULL,
			DCI_DT_STRING,	"Host name" },
   { "System.Memory.Physical.Free",  H_MemoryInfo,      (char *)PHYSICAL_FREE,
			DCI_DT_UINT64,	"Free physical memory" },
   { "System.Memory.Physical.Total", H_MemoryInfo,      (char *)PHYSICAL_TOTAL,
			DCI_DT_UINT64,	"Total amount of physical memory" },
   { "System.Memory.Physical.Used",  H_MemoryInfo,      (char *)PHYSICAL_USED,
			DCI_DT_UINT64,	"Used physical memory" },
   { "System.Memory.Swap.Free",      H_MemoryInfo,      (char *)SWAP_FREE,
			DCI_DT_UINT64,	"Free swap space" },
   { "System.Memory.Swap.Total",     H_MemoryInfo,      (char *)SWAP_TOTAL,
			DCI_DT_UINT64,	"Total amount of swap space" },
   { "System.Memory.Swap.Used",      H_MemoryInfo,      (char *)SWAP_USED,
			DCI_DT_UINT64,	"Used swap space" },
   { "System.Memory.Virtual.Free",   H_MemoryInfo,      (char *)VIRTUAL_FREE,
			DCI_DT_UINT64,	"Free virtual memory" },
   { "System.Memory.Virtual.Total",  H_MemoryInfo,      (char *)VIRTUAL_TOTAL,
			DCI_DT_UINT64,	"Total amount of virtual memory" },
   { "System.Memory.Virtual.Used",   H_MemoryInfo,      (char *)VIRTUAL_USED,
			DCI_DT_UINT64,	"Used virtual memory" },
   { "System.Uname",                 H_Uname,           NULL,
			DCI_DT_STRING,	"System uname" },
   { "System.Uptime",                H_Uptime,          NULL,
			DCI_DT_UINT,	"System uptime" },

   { "Agent.SourcePackageSupport",   H_SourcePkgSupport,NULL,
			DCI_DT_INT,		"" },
};

static NETXMS_SUBAGENT_ENUM m_enums[] =
{
   { "DRBD.DeviceList",              H_DRBDDeviceList,  NULL },
   { "Net.ArpCache",                 H_NetArpCache,     NULL },
   { "Net.IP.RoutingTable",          H_NetRoutingTable, NULL },
   { "Net.InterfaceList",            H_NetIfList,       NULL },
   { "System.ProcessList",           H_ProcessList,     NULL },
};

static NETXMS_SUBAGENT_INFO m_info =
{
   NETXMS_SUBAGENT_INFO_MAGIC,
	"Linux",
	NETXMS_VERSION_STRING,
	&Cleanup, /* unload handler */
	NULL, /* command handler */
	sizeof(m_parameters) / sizeof(NETXMS_SUBAGENT_PARAM),
	m_parameters,
	sizeof(m_enums) / sizeof(NETXMS_SUBAGENT_ENUM),
	m_enums
};

//
// Entry point for NetXMS agent
//

DECLARE_SUBAGENT_INIT(LINUX)
{
	StartCpuUsageCollector();

   *ppInfo = &m_info;
   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
/*

$Log: not supported by cvs2svn $
Revision 1.24  2006/03/02 21:08:20  alk
implemented:
	System.CPU.Usage5
	System.CPU.Usage15

Revision 1.23  2006/03/01 22:13:09  alk
added System.CPU.Usage [broken]

Revision 1.22  2005/09/15 21:47:02  victor
Added macro DECLARE_SUBAGENT_INIT to simplify initialization function declaration

Revision 1.21  2005/09/15 21:24:54  victor
Minor changes

Revision 1.20  2005/09/15 21:22:58  victor
Added possibility to build statically linked agents (with platform subagent linked in)
For now, agent has to be linked manually. I'll fix it later.

Revision 1.19  2005/08/22 00:11:46  alk
Net.IP.RoutingTable added

Revision 1.18  2005/08/19 15:23:50  victor
Added new parameters

Revision 1.17  2005/06/11 16:28:24  victor
Implemented all Net.Interface.* parameters except Net.Interface.Speed

Revision 1.16  2005/06/09 12:15:43  victor
Added support for Net.Interface.AdminStatus and Net.Interface.Link parameters

Revision 1.15  2005/02/24 17:38:49  victor
Added HDD monitring via SMART on Linux

Revision 1.14  2005/02/21 20:16:05  victor
Fixes in parameter data types and descriptions

Revision 1.13  2005/01/24 19:46:50  alk
SourcePackageSupport; return type/comment addded

Revision 1.12  2005/01/24 19:40:31  alk
return type/comments added for command list

System.ProcessCount/Process.Count(*) misunderstanding resolved

Revision 1.11  2005/01/17 23:31:01  alk
Agent.SourcePackageSupport added

Revision 1.10  2004/12/29 19:42:44  victor
Linux compatibility fixes

Revision 1.9  2004/10/22 22:08:34  alk
source restructured;
implemented:
	Net.IP.Forwarding
	Net.IP6.Forwarding
	Process.Count(*)
	Net.ArpCache
	Net.InterfaceList (if-type not implemented yet)
	System.ProcessList

Revision 1.8  2004/10/16 06:32:04  victor
Parameter name System.CPU.Procload changed to System.CPU.LoadAvg

Revision 1.7  2004/10/06 13:23:32  victor
Necessary changes to build everything on Linux

Revision 1.6  2004/08/26 23:51:26  alk
cosmetic changes

Revision 1.5  2004/08/18 00:12:56  alk
+ System.CPU.Procload* added, SINGLE processor only.

Revision 1.4  2004/08/17 23:19:20  alk
+ Disk.* implemented

Revision 1.3  2004/08/17 15:17:32  alk
+ linux agent: system.uptime, system.uname, system.hostname
! skeleton: amount of _PARM & _ENUM filled with sizeof()

Revision 1.1  2004/08/17 10:24:18  alk
+ subagent selection based on "uname -s"


*/
