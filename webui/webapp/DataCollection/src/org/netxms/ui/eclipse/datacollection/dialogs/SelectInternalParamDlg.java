/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2020 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.datacollection.dialogs;

import java.util.ArrayList;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.AgentParameter;
import org.netxms.client.AgentTable;
import org.netxms.client.constants.DataType;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.MobileDevice;
import org.netxms.client.objects.Template;
import org.netxms.ui.eclipse.datacollection.Messages;

/**
 * Dialog for selecting parameters with "Internal" source
 */
public class SelectInternalParamDlg extends AbstractSelectParamDlg
{   
   public SelectInternalParamDlg(Shell parentShell, long nodeId, boolean selectTables)
	{
		super(parentShell, nodeId, selectTables);
	}

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.AbstractSelectParamDlg#fillList()
    */
   @Override
	protected void fillList()
	{
	   if (selectTables)
	      fillTableList();
	   else
	      fillMetricList();
	}

   /**
    * Fill table parameter list
    */
   private void fillTableList()
   {
      ArrayList<AgentTable> list = new ArrayList<AgentTable>(10);

      list.add(new AgentTable("Topology.RoutingTable", "Routing table", new String[] { "ID" }));
      String[] topologyInstanceColumns = { "MAC_ADDRESS" }; // Same for both topology tables
      list.add(new AgentTable("Topology.SwitchForwardingDatabase", "Switch forwarding database", topologyInstanceColumns));
      list.add(new AgentTable("Topology.WirelessStations", "Wireless stations table", topologyInstanceColumns));

      if ((object instanceof Template) || ((object instanceof AbstractNode) && ((AbstractNode)object).isManagementServer()))
      {
         list.add(new AgentTable("Server.EventProcessors", "Event processors", new String[] { "ID" }));
      }

      viewer.setInput(list.toArray());
   }

   /**
    * Fill parameter list
    */
   private void fillMetricList()
	{
		ArrayList<AgentParameter> list = new ArrayList<AgentParameter>(10);

		// Internal parameters common for all objects
		list.add(new AgentParameter("ChildStatus(*)", Messages.get().SelectInternalParamDlg_DCI_ChildObjectStatus, DataType.INT32)); //$NON-NLS-1$
		list.add(new AgentParameter("ConditionStatus(*)", Messages.get().SelectInternalParamDlg_DCI_ConditionStatus, DataType.INT32)); //$NON-NLS-1$
		list.add(new AgentParameter("Dummy", Messages.get().SelectInternalParamDlg_DCI_Dummy, DataType.INT32)); //$NON-NLS-1$
      list.add(new AgentParameter("Dummy(*)", Messages.get().SelectInternalParamDlg_DCI_Dummy, DataType.INT32)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Configuration.Average", "Poll time (configuration): average", DataType.UINT64)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Configuration.Last", "Poll time (configuration): last", DataType.UINT64)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Configuration.Max", "Poll time (configuration): max", DataType.UINT64)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Configuration.Min", "Poll time (configuration): min", DataType.UINT64)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Instance.Average", "Poll time (instance): average", DataType.UINT64)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Instance.Last", "Poll time (instance): last", DataType.UINT64)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Instance.Max", "Poll time (instance): max", DataType.UINT64)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Instance.Min", "Poll time (instance): min", DataType.UINT64)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Status.Average", "Poll time (status): average", DataType.UINT64)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Status.Last", "Poll time (status): last", DataType.UINT64)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Status.Max", "Poll time (status): max", DataType.UINT64)); //$NON-NLS-1$
		list.add(new AgentParameter("PollTime.Status.Min", "Poll time (status): min", DataType.UINT64)); //$NON-NLS-1$
      list.add(new AgentParameter("Status", Messages.get().SelectInternalParamDlg_DCI_Status, DataType.INT32)); //$NON-NLS-1$
		
		if ((object instanceof Template) || (object instanceof AbstractNode))
		{
         list.add(new AgentParameter("ICMP.PacketLoss", "ICMP ping: packet loss", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("ICMP.PacketLoss(*)", "ICMP ping to {instance}: packet loss", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("ICMP.ResponseTime.Average", "ICMP ping: average response time", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("ICMP.ResponseTime.Average(*)", "ICMP ping to {instance}: average response time", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("ICMP.ResponseTime.Last", "ICMP ping: last response time", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("ICMP.ResponseTime.Last(*)", "ICMP ping to {instance}: last response time", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("ICMP.ResponseTime.Max", "ICMP ping: maximum response time", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("ICMP.ResponseTime.Max(*)", "ICMP ping to {instance}: maximum response time", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("ICMP.ResponseTime.Min", "ICMP ping: minimum response time", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("ICMP.ResponseTime.Min(*)", "ICMP ping to {instance}: minimum response time", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Net.IP.NextHop(*)", Messages.get().SelectInternalParamDlg_DCI_NextHop, DataType.STRING)); //$NON-NLS-1$
         list.add(new AgentParameter("NetSvc.ResponseTime(*)", "Network service {instance} response time", DataType.UINT32)); //$NON-NLS-1$
		   list.add(new AgentParameter("PollTime.RoutingTable.Average", "Poll time (routing table): average", DataType.UINT64)); //$NON-NLS-1$
		   list.add(new AgentParameter("PollTime.RoutingTable.Last", "Poll time (routing table): last", DataType.UINT64)); //$NON-NLS-1$
		   list.add(new AgentParameter("PollTime.RoutingTable.Max", "Poll time (routing table): max", DataType.UINT64)); //$NON-NLS-1$
		   list.add(new AgentParameter("PollTime.RoutingTable.Min", "Poll time (routing table): min", DataType.UINT64)); //$NON-NLS-1$
		   list.add(new AgentParameter("PollTime.Topology.Average", "Poll time (topology): average", DataType.UINT64)); //$NON-NLS-1$
		   list.add(new AgentParameter("PollTime.Topology.Last", "Poll time (topology): last", DataType.UINT64)); //$NON-NLS-1$
		   list.add(new AgentParameter("PollTime.Topology.Max", "Poll time (topology): max", DataType.UINT64)); //$NON-NLS-1$
		   list.add(new AgentParameter("PollTime.Topology.Min", "Poll time (topology): min", DataType.UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("ReceivedSNMPTraps", "Total SNMP traps received from this node", DataType.UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("ReceivedSyslogMessages", "Total syslog messages received from this node", DataType.UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("ZoneProxy.Assignments", "Zone proxy: number of assignments", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("ZoneProxy.State", "Zone proxy: state", DataType.INT32)); //$NON-NLS-1$
         list.add(new AgentParameter("ZoneProxy.ZoneUIN", "Zone proxy: UIN of parent zone", DataType.UINT32)); //$NON-NLS-1$
		}
		
		if ((object instanceof Template) || ((object instanceof AbstractNode) && ((AbstractNode)object).hasAgent()))
		{
			list.add(new AgentParameter("AgentStatus", Messages.get().SelectInternalParamDlg_DCI_AgentStatus, DataType.INT32)); //$NON-NLS-1$
		}
		
		if ((object instanceof Template) || ((object instanceof AbstractNode) && ((AbstractNode)object).isManagementServer()))
		{
         list.add(new AgentParameter("Server.ActiveAlarms", "Number of active alarms in the system", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AgentTunnels.Bound.Total", "Number of bound agent tunnels", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AgentTunnels.Bound.AgentProxy", "Number of bound agent tunnels with enabled agent proxy", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AgentTunnels.Bound.SnmpProxy", "Number of bound agent tunnels with enabled SNMP proxy", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AgentTunnels.Bound.SnmpTrapProxy", "Number of bound agent tunnels with enabled SNMP trap proxy", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AgentTunnels.Bound.SyslogProxy", "Number of bound agent tunnels with enabled syslog proxy", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AgentTunnels.Bound.UserAgent", "Number of bound agent tunnels with installed user agent", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AgentTunnels.Unbound.Total", "Number of unbound agent tunnels", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.AverageDCIQueuingTime", Messages.get().SelectInternalParamDlg_DCI_AvgDCIQueueTime, DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ClientSessions.Authenticated", "Client sessions: authenticated", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ClientSessions.Authenticated(*)", "Client sessions for user {instance}: authenticated", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ClientSessions.Desktop", "Client sessions: desktop", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ClientSessions.Desktop(*)", "Client sessions for user {instance}: desktop", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ClientSessions.Mobile", "Client sessions: mobile", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ClientSessions.Mobile(*)", "Client sessions for user {instance}: mobile", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ClientSessions.Total", "Client sessions: total", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ClientSessions.Web", "Client sessions: web clients", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ClientSessions.Web(*)", "Client sessions for user {instance}: web clients", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DataCollectionItems", "Number of data collection items in the system", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DB.Queries.Failed", "Failed DB queries", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DB.Queries.LongRunning", "Long running DB queries", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DB.Queries.NonSelect", "Non-SELECT DB queries", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DB.Queries.Select", "SELECT DB queries", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DB.Queries.Total", "Total DB queries", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DBWriter.Requests.IData", "DB writer requests (DCI data)", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DBWriter.Requests.Other", "DB writer requests (other queries)", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.DBWriter.Requests.RawData", "DB writer requests (raw DCI data)", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.EventProcessor.AverageWaitTime(*)", "Event processor {instance}: average event wait time", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.EventProcessor.Bindings(*)", "Event processor {instance}: active bindings", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.EventProcessor.ProcessedEvents(*)", "Event processor {instance}: total number of processed events", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.EventProcessor.QueueSize(*)", "Event processor {instance}: queue size", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.Heap.Active", "Active server heap memory", DataType.UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.Heap.Allocated", "Allocated server heap memory", DataType.UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.Heap.Mapped", "Mapped server heap memory", DataType.UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.MemoryUsage.Alarms", "Server memory usage: alarms", DataType.UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.MemoryUsage.DataCollectionCache", "Server memory usage: data collection cache", DataType.UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.MemoryUsage.RawDataWriter", "Server memory usage: raw data writer", DataType.UINT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ObjectCount.Clusters", "Objects: clusters", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ObjectCount.Nodes", "Objects: nodes", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ObjectCount.Sensors", "Objects: sensors", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ObjectCount.Total", "Objects: total", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.QueueSize.Average(*)", "Server queue {instance}: average size", DataType.FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.QueueSize.Current(*)", "Server queue {instance}: current size", DataType.INT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.QueueSize.Max(*)", "Server queue {instance}: max size", DataType.INT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.QueueSize.Min(*)", "Server queue {instance}: min size", DataType.INT64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ReceivedSNMPTraps", "SNMP traps received since server start", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ReceivedSyslogMessages", "Syslog messages received since server start", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ReceivedWindowsEvents", "Windows events received since server start", DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.SyncerRunTime.Average", "Syncer run time: average", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.SyncerRunTime.Last", "Syncer run time: last", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.SyncerRunTime.Max", "Syncer run time: max", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.SyncerRunTime.Min", "Syncer run time: min", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.ActiveRequests(*)", "Thread pool {instance}: active requests", DataType.INT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.AverageWaitTime(*)", "Thread pool {instance}: average wait time", DataType.UINT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.CurrSize(*)", "Thread pool {instance}: current size", DataType.INT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.Load(*)", "Thread pool {instance}: current load", DataType.INT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.LoadAverage(*)", "Thread pool {instance}: load average (1 minute)", DataType.FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.LoadAverage5(*)", "Thread pool {instance}: load average (5 minutes)", DataType.FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.LoadAverage15(*)", "Thread pool {instance}: load average (15 minutes)", DataType.FLOAT)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.MaxSize(*)", "Thread pool {instance}: maximum size", DataType.INT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.MinSize(*)", "Thread pool {instance}: minimum size", DataType.INT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.ScheduledRequests(*)", "Thread pool {instance}: scheduled requests", DataType.INT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.ThreadPool.Usage(*)", "Thread pool {instance}: usage", DataType.INT32)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.TotalEventsProcessed", Messages.get().SelectInternalParamDlg_DCI_TotalEventsProcessed, DataType.COUNTER64)); //$NON-NLS-1$
         list.add(new AgentParameter("Server.Uptime", "Server uptime", DataType.UINT32)); //$NON-NLS-1$
		}

		if ((object instanceof Template) || (object instanceof MobileDevice))
		{
			list.add(new AgentParameter("MobileDevice.BatteryLevel", Messages.get().SelectInternalParamDlg_DCI_BatteryLevel, DataType.INT32)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.DeviceId", Messages.get().SelectInternalParamDlg_DCI_DeviceID, DataType.STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.LastReportTime", Messages.get().SelectInternalParamDlg_DCI_LastReportTime, DataType.INT64)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.Model", Messages.get().SelectInternalParamDlg_DCI_Model, DataType.STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.OS.Name", Messages.get().SelectInternalParamDlg_DCI_OSName, DataType.STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.OS.Version", Messages.get().SelectInternalParamDlg_DCI_OSVersion, DataType.STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.SerialNumber", Messages.get().SelectInternalParamDlg_DCI_SerialNumber, DataType.STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.Vendor", Messages.get().SelectInternalParamDlg_DCI_Vendor, DataType.STRING)); //$NON-NLS-1$
			list.add(new AgentParameter("MobileDevice.UserId", Messages.get().SelectInternalParamDlg_DCI_UserID, DataType.STRING)); //$NON-NLS-1$
		}

		if ((object instanceof Template) || ((object instanceof AbstractNode) && ((AbstractNode)object).isWirelessController()))
		{
			list.add(new AgentParameter("WirelessController.AdoptedAPCount", Messages.get().SelectInternalParamDlg_AdoptedAPs, DataType.INT32)); //$NON-NLS-1$
			list.add(new AgentParameter("WirelessController.TotalAPCount", Messages.get().SelectInternalParamDlg_TotalAPs, DataType.INT32)); //$NON-NLS-1$
			list.add(new AgentParameter("WirelessController.UnadoptedAPCount", Messages.get().SelectInternalParamDlg_UnadoptedAPs, DataType.INT32)); //$NON-NLS-1$
		}

		viewer.setInput(list.toArray());
	}

   /**
    * @see org.netxms.ui.eclipse.datacollection.dialogs.AbstractSelectParamDlg#getConfigurationPrefix()
    */
	@Override
	protected String getConfigurationPrefix()
	{
		return "SelectInternalParamDlg"; //$NON-NLS-1$
	}
}
