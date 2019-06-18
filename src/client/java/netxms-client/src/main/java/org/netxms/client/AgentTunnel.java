/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Victor Kirhenshtein
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
package org.netxms.client;

import java.net.InetAddress;
import java.util.UUID;
import org.netxms.base.NXCPMessage;
import org.netxms.base.NXCommon;

/**
 * Agent tunnel information
 */
public class AgentTunnel
{
   private int id;
   private UUID guid;
   private InetAddress address;
   private long nodeId;
   private UUID agentId;
   private String systemName;
   private String systemInformation;
   private String platformName;
   private String agentVersion;
   private long zoneUIN;
   private int activeChannelCount;
   private String hostname;
   private boolean agentProxy;
   private boolean snmpProxy;
   private boolean snmpTrapProxy;
   private boolean userAgentInstalled;
   
   /**
    * Create from NXCP message
    * 
    * @param msg NXCP message
    * @param baseId base field ID
    */
   protected AgentTunnel(NXCPMessage msg, long baseId)
   {
      id = msg.getFieldAsInt32(baseId);
      guid = msg.getFieldAsUUID(baseId + 1);
      nodeId = msg.getFieldAsInt64(baseId + 2);
      address = msg.getFieldAsInetAddress(baseId + 3);
      systemName = msg.getFieldAsString(baseId + 4);
      systemInformation = msg.getFieldAsString(baseId + 5);
      platformName = msg.getFieldAsString(baseId + 6);
      agentVersion = msg.getFieldAsString(baseId + 7);
      activeChannelCount = msg.getFieldAsInt32(baseId + 8);
      zoneUIN = msg.getFieldAsInt64(baseId + 9);
      hostname = msg.getFieldAsString(baseId + 10);
      agentId = msg.getFieldAsUUID(baseId + 11);
      userAgentInstalled = msg.getFieldAsBoolean(baseId + 12);
      agentProxy = msg.getFieldAsBoolean(baseId + 13);
      snmpProxy = msg.getFieldAsBoolean(baseId + 14);
      snmpTrapProxy = msg.getFieldAsBoolean(baseId + 15);
   }
   
   /**
    * Check if tunnel is bound
    * 
    * @return true if tunnel is bound
    */
   public boolean isBound()
   {
      return nodeId != 0;
   }

   /**
    * Get tunnel internal ID.
    * 
    * @return tunnel internal ID
    */
   public int getId()
   {
      return id;
   }

   /**
    * Get tunnel globally unique identifier.
    * 
    * @return tunnel globally unique identifier
    */
   public UUID getGuid()
   {
      return guid;
   }

   /**
    * @return the address
    */
   public InetAddress getAddress()
   {
      return address;
   }

   /**
    * @return the nodeId
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * @return the agentId
    */
   public UUID getAgentId()
   {
      return (agentId != null) ? agentId : NXCommon.EMPTY_GUID;
   }

   /**
    * @return the systemName
    */
   public String getSystemName()
   {
      return systemName;
   }

   /**
    * @return the systemInformation
    */
   public String getSystemInformation()
   {
      return systemInformation;
   }

   /**
    * Get platform name for this agent's host.
    * 
    * @return platform name for this agent's host
    */
   public String getPlatformName()
   {
      return platformName;
   }

   /**
    * Get agent version.
    * 
    * @return agent version
    */
   public String getAgentVersion()
   {
      return agentVersion;
   }

   /**
    * Get zone UIN for this agent. For unbound tunnels it is zone UIN
    * set in agent's configuration file.
    * 
    * @return zone UIN for this agent
    */
   public long getZoneUIN()
   {
      return zoneUIN;
   }

   /**
    * Get active channel count for this tunnel.
    * 
    * @return active channel count for this tunnel
    */
   public int getActiveChannelCount()
   {
      return activeChannelCount;
   }
   
   /**
    * Get remote host name.
    * 
    * @return remote host name
    */
   public String getHostname()
   {
      return hostname;
   }
   
   /**
    * Check if agent proxy is enabled on this agent.
    * 
    * @return true if agent proxy is enabled on this agent
    */
   public boolean isAgentProxy()
   {
      return agentProxy;
   }

   /**
    * Check if SNMP proxy is enabled on this agent.
    * 
    * @return true if SNMP proxy is enabled on this agent
    */
   public boolean isSnmpProxy()
   {
      return snmpProxy;
   }

   /**
    * Check if SNMP trap proxy is enabled on this agent.
    * 
    * @return true if SNMP trap proxy is enabled on this agent
    */
   public boolean isSnmpTrapProxy()
   {
      return snmpTrapProxy;
   }

   /**
    * Check if user agent is installed on remote system.
    * 
    * @return true if user agent is installed on remote system
    */
   public boolean isUserAgentInstalled()
   {
      return userAgentInstalled;
   }

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "AgentTunnel [id=" + id + ", guid=" + guid + ", address=" + address + ", nodeId=" + nodeId + ", systemName="
            + systemName + ", hostname=" + hostname + ", systemInformation=" + systemInformation + ", platformName="
            + platformName + ", agentVersion=" + agentVersion + ", zoneUIN=" + zoneUIN + ", activeChannelCount=" 
            + activeChannelCount + ", userAgentInstalled=" + userAgentInstalled + "]";
   }
}
