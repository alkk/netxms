/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.websvc.handlers;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Map;
import org.json.JSONException;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;

public class GrafanaAlarms extends AbstractHandler
{
   private String[] states = { "Outstanding", "Acknowledged", "Resolved", "Terminated" };
   
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   public Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      if (!session.isObjectsSynchronized())
         session.syncObjects();
      
      if (query.containsKey("node") && query.get("node").equals("Select source"))
      {
         
      }
      JsonObject root = new JsonObject();

      JsonArray columns = new JsonArray();
      columns.add(createColumn("Severity", true, true));
      columns.add(createColumn("State", true, false));
      columns.add(createColumn("Source", true, false));
      columns.add(createColumn("Message", true, false));
      columns.add(createColumn("Count", true, false));
      columns.add(createColumn("Helpdesk ID", true, false));
      columns.add(createColumn("Ack/Resolved By", true, false));
      columns.add(createColumn("Created", true, false));
      columns.add(createColumn("Last Change", true, false));
      root.add("columns", columns);
      
      JsonArray rows = new JsonArray();
      
      JsonArray r = new JsonArray();
      DateFormat df = new SimpleDateFormat("dd.MM.yyyy HH:mm:ss");
      AbstractObject object = null;
      AbstractUserObject user = null;
      
      Map<Long, Alarm> alarms = session.getAlarms();
      for( Alarm a : alarms.values())
      {
         r.add(a.getCurrentSeverity().name());
         r.add(states[a.getState()]);
         
         object = getSession().findObjectById(a.getSourceObjectId());
         if (object == null)
            r.add(a.getSourceObjectId());
         else
            r.add(object.getObjectName());
         
         r.add(a.getMessage());
         r.add(a.getRepeatCount());
         r.add(a.getHelpdeskReference());
         
         user = getSession().findUserDBObjectById(a.getAckByUser());
         if (user == null)
            r.add("");
         else
            r.add(user.getName());
         
         r.add(df.format(a.getCreationTime()));
         r.add(df.format(a.getLastChangeTime()));
         rows.add(r);
         r = new JsonArray();
      }
      
      root.add("rows", rows);
      
      root.addProperty("type", "table");

      JsonArray wrapper = new JsonArray();
      wrapper.add(root);
      return wrapper;
   }
   
   /**
    * @param name
    * @param sort
    * @param desc
    * @return
    * @throws JSONException
    */
   private static JsonObject createColumn(String name, boolean sort, boolean desc) throws JSONException
   {
      JsonObject column = new JsonObject();
      column.addProperty("text", name);
      column.addProperty("sort", sort);
      column.addProperty("sort", desc);
      return column;
   }
}