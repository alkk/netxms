/*
** nxdbmgr - NetXMS database manager
** Copyright (C) 2020-2021 Raden Solutions
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
** File: upgrade_v40.cpp
**
**/

#include "nxdbmgr.h"
#include <nxevent.h>



/**
 * Upgrade from 40.62 to 40.63
 */
static bool H_UpgradeFromV62()
{
   CHK_EXEC(CreateTable(
      _T("CREATE TABLE notification_log (")
      _T("   id $SQL:INT64 not null,")
      _T("   notification_channel varchar(63) not null,")
      _T("   notification_timestamp integer not null,")
      _T("   recipient varchar(2000) null,")
      _T("   subject varchar(2000) null,")
      _T("   message varchar(2000) null,")
      _T("   success integer not null,")
      _T("PRIMARY KEY(id))")));

      CHK_EXEC(CreateTable(
      _T("CREATE TABLE server_action_execution_log (")
      _T("   id $SQL:INT64 not null,")
      _T("   action_timestamp integer not null,")
      _T("   action_code integer not null,")
      _T("   action_name varchar(63) null,")
      _T("   channel_name varchar(63) null,")
      _T("   recipient varchar(2000) null,")
      _T("   subject varchar(2000) null,")
      _T("   message varchar(2000) null,")
      _T("   success integer not null,")
      _T("PRIMARY KEY(id))")));
   CHK_EXEC(SetMinorSchemaVersion(63));
   return true;
}


/**
 * Upgrade from 40.61 to 40.62
 */
static bool H_UpgradeFromV61()
{
   if (GetSchemaLevelForMajorVersion(39) < 6)
   {
      CHK_EXEC(CreateConfigParam(_T("LDAP.Mapping.Email"),
            _T(""),
            _T("The name of an attribute whose value will be used as a user's email."),
            nullptr, 'S', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("LDAP.Mapping.PhoneNumber"),
            _T(""),
            _T("The name of an attribute whose value will be used as a user's phone number."),
            nullptr, 'S', true, false, false, false));

      static const TCHAR *batch =
            _T("UPDATE config SET var_name='LDAP.Mapping.Description' WHERE var_name='LDAP.MappingDescription'\n")
            _T("UPDATE config SET var_name='LDAP.Mapping.FullName' WHERE var_name='LDAP.MappingFullName'\n")
            _T("UPDATE config SET var_name='LDAP.Mapping.GroupName' WHERE var_name='LDAP.GroupMappingName'\n")
            _T("UPDATE config SET var_name='LDAP.Mapping.UserName' WHERE var_name='LDAP.UserMappingName'\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(SQLQuery(_T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
            _T("VALUES ('4fae91b5-8802-4f6c-aace-a03f9f7fa8ef',25,'Hook::LDAPSynchronization','/* Available global variables:\r\n *  $ldapObject - LDAP object being synchronized (object of ''LDAPObject'' class)\r\n *\r\n * Expected return value:\r\n *  true/false - boolean - whether processing of this LDAP object should continue\r\n */\r\n')")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 6));
   }
   CHK_EXEC(SetMinorSchemaVersion(62));
   return true;
}

/**
 * Upgrade from 40.60 to 40.61
 */
static bool H_UpgradeFromV60()
{
   if (GetSchemaLevelForMajorVersion(39) < 5)
   {
      CHK_EXEC(CreateConfigParam(_T("Geolocation.History.RetentionTime"),
            _T("90"),
            _T("Retention time in days for object's geolocation history. All records older than specified will be deleted by housekeeping process."),
            _T("days"), 'I', true, false, false, false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(61));
   return true;
}

/**
 * Upgrade from 40.59 to 40.60
 */
static bool H_UpgradeFromV59()
{
   if (GetSchemaLevelForMajorVersion(39) < 4)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE input_fields ADD flags integer")));

      DB_RESULT hResult = SQLSelect(_T("SELECT category,owner_id,name,config FROM input_fields"));
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            TCHAR category[2], name[MAX_OBJECT_NAME];
            DBGetField(hResult, i, 0, category, 2);
            uint32_t ownerId = DBGetFieldLong(hResult, i, 1);
            DBGetField(hResult, i, 2, name, MAX_OBJECT_NAME);
            TCHAR *config = DBGetField(hResult, i, 3, nullptr, 0);

            TCHAR query[1024];
            _sntprintf(query, 1024, _T("UPDATE input_fields SET flags=%d WHERE category='%s' AND owner_id=%u AND name=%s"),
                     (_tcsstr(config, _T("<validatePassword>true</validatePassword>")) != nullptr) ? 1 : 0, category, ownerId, DBPrepareString(g_dbHandle, name).cstr());
            MemFree(config);

            if (!SQLQuery(query) && !g_ignoreErrors)
            {
               DBFreeResult(hResult);
               return false;
            }
         }
         DBFreeResult(hResult);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }

      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("input_fields"), _T("flags")));
      CHK_EXEC(DBDropColumn(g_dbHandle, _T("input_fields"), _T("config")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 4));
   }
   CHK_EXEC(SetMinorSchemaVersion(60));
   return true;
}

/**
 * Upgrade from 40.58 to 40.59
 */
static bool H_UpgradeFromV58()
{
   if (GetSchemaLevelForMajorVersion(39) < 3)
   {
      CHK_EXEC(DBDropPrimaryKey(g_dbHandle, _T("object_tools_input_fields")));
      CHK_EXEC(DBRenameTable(g_dbHandle, _T("object_tools_input_fields"), _T("input_fields")));
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("input_fields"), _T("tool_id"), _T("owner_id")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE input_fields ADD category char(1)")));
      CHK_EXEC(SQLQuery(_T("UPDATE input_fields SET category='T'")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("input_fields"), _T("category")));
      CHK_EXEC(DBAddPrimaryKey(g_dbHandle, _T("input_fields"), _T("category,owner_id,name")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(59));
   return true;
}

/**
 * Upgrade from 40.57 to 40.58
 */
static bool H_UpgradeFromV57()
{
   if (GetSchemaLevelForMajorVersion(39) < 2)
   {
      if (DBIsTableExist(g_dbHandle, _T("zmq_subscription")) == DBIsTableExist_Found)
         CHK_EXEC(SQLQuery(_T("DROP TABLE zmq_subscription")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(58));
   return true;
}

/**
 * Upgrade from 40.56 to 40.57
 */
static bool H_UpgradeFromV56()
{
   if (GetSchemaLevelForMajorVersion(39) < 1)
   {
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE object_queries (")
            _T("   id integer not null,")
            _T("   guid varchar(36) not null,")
            _T("   name varchar(63) not null,")
            _T("   description varchar(255) null,")
            _T("   script $SQL:TEXT null,")
            _T("PRIMARY KEY(id))")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(39, 1));
   }
   CHK_EXEC(SetMinorSchemaVersion(57));
   return true;
}

/**
 * Upgrade from 40.55 to 40.56
 */
static bool H_UpgradeFromV55()
{
   CHK_EXEC(CreateTable(
         _T("CREATE TABLE two_factor_auth_methods (")
         _T("  name varchar(63) not null,")
         _T("  driver varchar(63) null,")
         _T("  description varchar(255) null,")
         _T("  configuration $SQL:TEXT null,")
         _T("PRIMARY KEY(name))")));

   CHK_EXEC(CreateTable(
         _T("CREATE TABLE two_factor_auth_bindings (")
         _T("  user_id integer not null,")
         _T("  name varchar(63) not null,")
         _T("  configuration $SQL:TEXT null,")
         _T("PRIMARY KEY(user_id,name))")));

   if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
   {
      CHK_EXEC(SQLQuery(_T("UPDATE users SET system_access=system_access+140737488355328 WHERE (BITAND(system_access, 1) = 1)")));
      CHK_EXEC(SQLQuery(_T("UPDATE user_groups SET system_access=system_access+140737488355328 WHERE (BITAND(system_access, 1) = 1)")));
   }
   else
   {
      CHK_EXEC(SQLQuery(_T("UPDATE users SET system_access=system_access+140737488355328 WHERE ((system_access & 1) = 1)")));
      CHK_EXEC(SQLQuery(_T("UPDATE user_groups SET system_access=system_access+140737488355328 WHERE ((system_access & 1) = 1)")));
   }

   CHK_EXEC(SetMinorSchemaVersion(56));
   return true;
}

/**
 * Upgrade from 40.54 to 40.55
 */
static bool H_UpgradeFromV54()
{
   if (GetSchemaLevelForMajorVersion(38) < 17)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE alarms ADD rule_description varchar(255)")));
      DB_STATEMENT hStmt = DBPrepare(g_dbHandle,
         (g_dbSyntax == DB_SYNTAX_MSSQL) ?
            _T("UPDATE alarms SET rule_description=(SELECT REPLACE(REPLACE(REPLACE(REPLACE(CONVERT(varchar(255), comments), ?, ' '), ?, ' '), ?, ' '), '  ', ' ') FROM event_policy WHERE event_policy.rule_guid=alarms.rule_guid)") :
            _T("UPDATE alarms SET rule_description=(SELECT REPLACE(REPLACE(REPLACE(REPLACE(comments, ?, ' '), ?, ' '), ?, ' '), '  ', ' ') FROM event_policy WHERE event_policy.rule_guid=alarms.rule_guid)"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, _T("\n"), DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, _T("\r"), DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, _T("\t"), DB_BIND_STATIC);
         if (!SQLExecute(hStmt) && !g_ignoreErrors)
         {
            DBFreeStatement(hStmt);
            return false;
         }
         DBFreeStatement(hStmt);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }

      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 17));
   }

   CHK_EXEC(SetMinorSchemaVersion(55));
   return true;
}


/**
 * Upgrade from 40.53 to 40.54
 */
static bool H_UpgradeFromV53()
{
   if (GetSchemaLevelForMajorVersion(38) < 16)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_SETUP_ERROR, _T("SYS_TUNNEL_SETUP_ERROR"),
               EVENT_SEVERITY_MAJOR, EF_LOG, _T("50792874-0b2e-4eca-8c54-274b7d5e3aa2"),
               _T("Error setting up agent tunnel from %<ipAddress> (%<error>)"),
               _T("Generated on agent tunnel setup error.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Source IP address (ipAddress)\r\n")
               _T("   2) Error message (error)")
               ));

      int ruleId = NextFreeEPPruleID();

      TCHAR query[1024];
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
               _T("VALUES (%d,'bdd8f6b1-fe3a-4b20-bc0b-bb7507b264b2',7944,'Generate alarm on agent tunnel setup error','%%m',5,'TUNNEL_SETUP_ERROR_%%1','',0,%d)"),
               ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));

      _sntprintf(query, 256, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_TUNNEL_SETUP_ERROR);
      CHK_EXEC(SQLQuery(query));

      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 16));
   }
   CHK_EXEC(SetMinorSchemaVersion(54));
   return true;
}

/**
 * Upgrade from 40.52 to 40.53
 */
static bool H_UpgradeFromV52()
{
   if (GetSchemaLevelForMajorVersion(38) < 15)
   {
      static const TCHAR *batch =
         _T("UPDATE config SET data_type='C',need_server_restart=0 WHERE var_name='Objects.Nodes.ResolveDNSToIPOnStatusPoll'\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.ResolveDNSToIPOnStatusPoll','0','Never')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.ResolveDNSToIPOnStatusPoll','1','Always')\n")
         _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('Objects.Nodes.ResolveDNSToIPOnStatusPoll','2','On failure')\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(CreateConfigParam(_T("Objects.Nodes.ResolveDNSToIPOnStatusPoll.Interval"),
            _T("0"),
            _T("Number of status polls between resolving primary host name to IP, if Objects.Nodes.ResolveDNSToIPOnStatusPoll set to \"Always\"."),
            _T("polls"),
            'I',
            true,
            false,
            false,
            false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 15));
   }
   CHK_EXEC(SetMinorSchemaVersion(53));
   return true;
}

/**
 * Upgrade from 40.51 to 40.52
 */
static bool H_UpgradeFromV51()
{
   if (GetSchemaLevelForMajorVersion(38) < 14)
   {
      CHK_EXEC(CreateConfigParam(_T("Agent.RestartWaitTime"),
            _T("0"),
            _T("Period of time after agent restart for which agent will not be considered unreachable"),
            _T("seconds"),
            'I',
            true,
            false,
            false,
            false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 14));
   }
   CHK_EXEC(SetMinorSchemaVersion(52));
   return true;
}

/**
 * Upgrade from 40.50 to 40.51
 */
static bool H_UpgradeFromV50()
{
   if (GetSchemaLevelForMajorVersion(38) < 13)
   {
      int ruleId = NextFreeEPPruleID();

      TCHAR query[1024];
      _sntprintf(query, 1024, _T("INSERT INTO event_policy (rule_id,rule_guid,flags,comments,alarm_message,alarm_severity,alarm_key,script,alarm_timeout,alarm_timeout_event) ")
               _T("VALUES (%d,'b517ca11-cdf8-4813-87fa-9a2ace070564',7944,'Generate alarm on agent policy validation failure','%%m',5,'POLICY_ERROR_%%2_%%5','',0,%d)"),
               ruleId, EVENT_ALARM_TIMEOUT);
      CHK_EXEC(SQLQuery(query));

      _sntprintf(query, 256, _T("INSERT INTO policy_event_list (rule_id,event_code) VALUES (%d,%d)"), ruleId, EVENT_POLICY_VALIDATION_ERROR);
      CHK_EXEC(SQLQuery(query));

      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 13));
   }
   CHK_EXEC(SetMinorSchemaVersion(51));
   return true;
}

/**
 * Upgrade from 40.49 to 40.50
 */
static bool H_UpgradeFromV49()
{
   if (GetSchemaLevelForMajorVersion(38) < 12)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_POLICY_VALIDATION_ERROR, _T("SYS_POLICY_VALIDATION_ERROR"),
            SEVERITY_WARNING, EF_LOG, _T("7a0c3a71-8125-4692-985a-a7e94fbee570"),
            _T("Failed validation of %4 policy %3 in template %1 (%6)"),
            _T("Generated when agent policy within template fails validation.\r\n")
            _T("Parameters:\r\n")
            _T("   1) Template name\r\n")
            _T("   2) Template ID\r\n")
            _T("   3) Policy name\r\n")
            _T("   4) Policy type\r\n")
            _T("   5) Policy ID\r\n")
            _T("   6) Additional info")
            ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 12));
   }
   CHK_EXEC(SetMinorSchemaVersion(50));
   return true;
}

/**
 * Upgrade from 40.48 to 40.49
 */
static bool H_UpgradeFromV48()
{
   if (GetSchemaLevelForMajorVersion(38) < 11)
   {
      if ((g_dbSyntax == DB_SYNTAX_DB2) || (g_dbSyntax == DB_SYNTAX_INFORMIX) || (g_dbSyntax == DB_SYNTAX_ORACLE))
         CHK_EXEC(SQLQuery(_T("UPDATE nodes SET last_agent_comm_time=0 WHERE (BITAND(capabilities, 2) = 0) AND (last_agent_comm_time > 0)")));
      else
         CHK_EXEC(SQLQuery(_T("UPDATE nodes SET last_agent_comm_time=0 WHERE ((capabilities & 2) = 0) AND (last_agent_comm_time > 0)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 11));
   }
   CHK_EXEC(SetMinorSchemaVersion(49));
   return true;
}

/**
 * Upgrade from 40.47 to 40.48
 */
static bool H_UpgradeFromV47()
{
   CHK_EXEC(CreateConfigParam(_T("DataCollection.TemplateRemovalGracePeriod"),
      _T("0"),
      _T("Setting up grace period for deleting templates from DataCollection"),
      nullptr,
      'I',
      true,
      false,
      false,
      false));
   CHK_EXEC(SetMinorSchemaVersion(48));
   return true;
}

/**
 * Upgrade from 40.46 to 40.47
 */
static bool H_UpgradeFromV46()
{
   CHK_EXEC(SQLQuery(_T("UPDATE object_properties SET flags=65536 WHERE object_id IN (SELECT id FROM subnets WHERE synthetic_mask > 0)")));
   CHK_EXEC(DBDropColumn(g_dbHandle, _T("subnets"), _T("synthetic_mask")));
   CHK_EXEC(SetMinorSchemaVersion(47));
   return true;
}

/**
 * Upgrade from 40.45 to 40.46
 */
static bool H_UpgradeFromV45()
{
   if (GetSchemaLevelForMajorVersion(38) < 10)
   {
      static const TCHAR *batch =
         _T("ALTER TABLE interfaces ADD last_known_oper_state integer\n")
         _T("ALTER TABLE interfaces ADD last_known_admin_state integer\n")
         _T("UPDATE interfaces SET last_known_oper_state=0,last_known_admin_state=0\n")
         _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("last_known_oper_state")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("interfaces"), _T("last_known_admin_state")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 10));
   }
   CHK_EXEC(SetMinorSchemaVersion(46));
   return true;
}

/**
 * Upgrade from 40.44 to 40.45
 */
static bool H_UpgradeFromV44()
{
   CHK_EXEC(CreateConfigParam(_T("AuditLog.External.UseUTF8"),
         _T("0"),
         _T("Changes audit log encoding to UTF-8"),
         nullptr,
         'B',
         true,
         false,
         false,
         false));
   CHK_EXEC(SetMinorSchemaVersion(45));
   return true;
}

/**
 * Upgrade from 40.43 to 40.44
 */
static bool H_UpgradeFromV43()
{
   if (GetSchemaLevelForMajorVersion(38) < 9)
   {
      CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='JobHistoryRetentionTime'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 9));
   }
   CHK_EXEC(SetMinorSchemaVersion(44));
   return true;
}

/**
 * Upgrade from 40.42 to 40.43
 */
static bool H_UpgradeFromV42()
{
   if (GetSchemaLevelForMajorVersion(38) < 8)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE raw_dci_values ADD cache_timestamp integer")));
      CHK_EXEC(SQLQuery(_T("UPDATE raw_dci_values SET cache_timestamp=0")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("raw_dci_values"), _T("cache_timestamp")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 8));
   }
   CHK_EXEC(SetMinorSchemaVersion(43));
   return true;
}

/**
 * Upgrade from 40.41 to 40.42
 */
static bool H_UpgradeFromV41()
{
   if (GetSchemaLevelForMajorVersion(38) < 7)
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE job_history")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 7));
   }
   CHK_EXEC(SetMinorSchemaVersion(42));
   return true;
}

/**
 * Upgrade from 40.40 to 40.41
 */
static bool H_UpgradeFromV40()
{
   if (GetSchemaLevelForMajorVersion(38) < 6)
   {
      static const TCHAR *batch =
            _T("ALTER TABLE policy_action_list ADD snooze_time integer\n")
            _T("UPDATE policy_action_list SET snooze_time=0\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("policy_action_list"), _T("snooze_time")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 6));
   }

   CHK_EXEC(SetMinorSchemaVersion(41));
   return true;
}

/**
 * Upgrade from 40.39 to 40.40
 */
static bool H_UpgradeFromV39()
{
   if (GetSchemaLevelForMajorVersion(38) < 5)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_TUNNEL_HOST_DATA_MISMATCH, _T("SYS_TUNNEL_HOST_DATA_MISMATCH"),
               SEVERITY_WARNING, EF_LOG, _T("874aa4f3-51b9-49ad-a8df-fb4bb89d0f81"),
               _T("Host data mismatch on tunnel reconnect"),
               _T("Generated when new tunnel is replacing existing one and host data mismatch is detected.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Tunnel ID (tunnelId)\r\n")
               _T("   2) Old remote system IP address (oldIPAddress)\r\n")
               _T("   3) New remote system IP address (newIPAddress)\r\n")
               _T("   4) Old remote system name (oldSystemName)\r\n")
               _T("   5) New remote system name (newSystemName)\r\n")
               _T("   6) Old remote system FQDN (oldHostName)\r\n")
               _T("   7) New remote system FQDN (newHostName)\r\n")
               _T("   8) Old hardware ID (oldHardwareId)\r\n")
               _T("   9) New hardware ID (newHardwareId)")
               ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 5));
   }

   CHK_EXEC(SetMinorSchemaVersion(40));
   return true;
}

/**
 * Upgrade from 40.38 to 40.39
 */
static bool H_UpgradeFromV38()
{
   if (GetSchemaLevelForMajorVersion(38) < 4)
   {
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE ssh_keys (")
            _T("  id integer not null,")
            _T("  name varchar(255) not null,")
            _T("  public_key varchar(700) null,")
            _T("  private_key varchar(1710) null,")
            _T("PRIMARY KEY(id))")));

      static const TCHAR *batch =
            _T("ALTER TABLE nodes ADD ssh_key_id integer\n")
            _T("UPDATE nodes SET ssh_key_id=0\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("ssh_key_id")));

      // Update access rights for predefined "Admins" group
      DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
            access |= SYSTEM_ACCESS_SSH_KEY_CONFIGURATION;
            TCHAR query[256];
            _sntprintf(query, 256, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE id=1073741825"), access);
            CHK_EXEC(SQLQuery(query));
         }
         DBFreeResult(hResult);
      }
      else
      {
         if (!g_ignoreErrors)
            return false;
      }
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 4));
   }

   CHK_EXEC(SetMinorSchemaVersion(39));
   return true;
}

/**
 * Upgrade from 40.37 to 40.38
 */
static bool H_UpgradeFromV37()
{
   if (GetSchemaLevelForMajorVersion(38) < 3)
   {
      if (DBIsTableExist(g_dbHandle, _T("report_results")))
      {
         CHK_EXEC(SQLQuery(_T("DROP TABLE report_results")));
      }
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(38));
   return true;
}

/**
 * Upgrade from 40.36 to 40.37
 */
static bool H_UpgradeFromV36()
{
   if (GetSchemaLevelForMajorVersion(38) < 2)
   {
      if (DBIsTableExist(g_dbHandle, _T("report_notifications")))
      {
         CHK_EXEC(SQLQuery(_T("DROP TABLE report_notifications")));
      }
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(37));
   return true;
}

/**
 * Upgrade from 40.35 to 40.38
 * Upgrades straight to version 38, versions 36 and 37 skipped because in version 38 all reporting tables are already dropped.
 */
static bool H_UpgradeFromV35()
{
   if (GetSchemaLevelForMajorVersion(38) < 1)
   {
      static const TCHAR *deprecatedTables[] = {
         _T("qrtz_fired_triggers"),
         _T("qrtz_paused_trigger_grps"),
         _T("qrtz_scheduler_state"),
         _T("qrtz_locks"),
         _T("qrtz_simple_triggers"),
         _T("qrtz_cron_triggers"),
         _T("qrtz_simprop_triggers"),
         _T("qrtz_blob_triggers"),
         _T("qrtz_triggers"),
         _T("qrtz_job_details"),
         _T("qrtz_calendars"),
         _T("report_notification"),
         _T("reporting_results"),
         nullptr
      };
      for(int i = 0; deprecatedTables[i] != nullptr; i++)
      {
         if (DBIsTableExist(g_dbHandle, deprecatedTables[i]))
         {
            TCHAR query[256];
            _sntprintf(query, 256, _T("DROP TABLE %s"), deprecatedTables[i]);
            CHK_EXEC(SQLQuery(query));
         }
      }
      CHK_EXEC(SetSchemaLevelForMajorVersion(38, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(38));
   return true;
}

/**
 * Upgrade from 40.34 to 40.35
 */
static bool H_UpgradeFromV34()
{
   if (GetSchemaLevelForMajorVersion(37) < 6)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE users ADD email varchar(127)")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE users ADD phone_number varchar(63)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 6));
   }
   CHK_EXEC(SetMinorSchemaVersion(35));
   return true;
}

/**
 * Upgrade from 40.33 to 40.34
 */
static bool H_UpgradeFromV33()
{
   if (GetSchemaLevelForMajorVersion(37) < 5)
   {
      static const TCHAR *batch =
            _T("ALTER TABLE nodes ADD ssh_port integer\n")
            _T("UPDATE nodes SET ssh_port=22\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("ssh_port")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(34));
   return true;
}

/**
 * Upgrade from 40.32 to 40.33
 */
static bool H_UpgradeFromV32()
{
   if (GetSchemaLevelForMajorVersion(37) < 4)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Objects.Clusters.ContainerAutoBind' WHERE var_name='ClusterContainerAutoBind'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='Objects.Clusters.TemplateAutoApply' WHERE var_name='ClusterTemplateAutoApply'")));
      CHK_EXEC(CreateConfigParam(_T("Objects.AccessPoints.ContainerAutoBind"), _T("0"), _T("Enable/disable container auto binding for access points."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Objects.AccessPoints.TemplateAutoApply"), _T("0"), _T("Enable/disable template auto apply for access points."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Objects.MobileDevices.ContainerAutoBind"), _T("0"), _T("Enable/disable container auto binding for mobile devices."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Objects.MobileDevices.TemplateAutoApply"), _T("0"), _T("Enable/disable template auto apply for mobile devices."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Objects.Sensors.ContainerAutoBind"), _T("0"), _T("Enable/disable container auto binding for sensors."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("Objects.Sensors.TemplateAutoApply"), _T("0"), _T("Enable/disable template auto apply for sensors."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 4));
   }
   CHK_EXEC(SetMinorSchemaVersion(33));
   return true;
}

/**
 * Upgrade from 40.31 to 40.32
 */
static bool H_UpgradeFromV31()
{
   if (GetSchemaLevelForMajorVersion(37) < 3)
   {
      CHK_EXEC(CreateConfigParam(_T("Objects.Interfaces.Enable8021xStatusPoll"), _T("1"), _T("Globally enable or disable checking of 802.1x port state during status poll."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(32));
   return true;
}

/**
 * Upgrade from 40.30 to 40.31
 */
static bool H_UpgradeFromV30()
{
   if (GetSchemaLevelForMajorVersion(37) < 2)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='DataCollection.InstanceRetentionTime',default_value='7',need_server_restart=0 WHERE var_name='InstanceRetentionTime'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_value='7' WHERE var_name='DataCollection.InstanceRetentionTime' AND var_value='0'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(31));
   return true;
}

/**
 * Upgrade from 40.29 to 40.30
 */
static bool H_UpgradeFromV29()
{
   if (GetSchemaLevelForMajorVersion(37) < 1)
   {
      CHK_EXEC(CreateEventTemplate(EVENT_CLUSTER_AUTOADD, _T("SYS_CLUSTER_AUTOADD"),
               SEVERITY_NORMAL, EF_LOG, _T("308fce5f-69ec-450a-a42b-d1e5178512a5"),
               _T("Node %2 automatically added to cluster %4"),
               _T("Generated when node added to cluster object by autoadd rule.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Node ID\r\n")
               _T("   2) Node name\r\n")
               _T("   3) Cluster ID\r\n")
               _T("   4) Cluster name")
               ));

      CHK_EXEC(CreateEventTemplate(EVENT_CLUSTER_AUTOREMOVE, _T("SYS_CLUSTER_AUTOREMOVE"),
               SEVERITY_NORMAL, EF_LOG, _T("f2cdb47a-ae37-43d7-851d-f8f85e1e9f0c"),
               _T("Node %2 automatically removed from cluster %4"),
               _T("Generated when node removed from cluster object by autoadd rule.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Node ID\r\n")
               _T("   2) Node name\r\n")
               _T("   3) Cluster ID\r\n")
               _T("   4) Cluster name")
               ));
      CHK_EXEC(SetSchemaLevelForMajorVersion(37, 1));
   }
   CHK_EXEC(SetMinorSchemaVersion(30));
   return true;
}

/**
 * Upgrade from 40.28 to 40.29
 */
static bool H_UpgradeFromV28()
{
   if (GetSchemaLevelForMajorVersion(36) < 17)
   {
      CHK_EXEC(CreateConfigParam(_T("LDAP.NewUserAuthMethod"), _T("5"), _T("Authentication method to be set for user objects created by LDAP synchronization process."), nullptr, 'C', true, false, false, false));

      static const TCHAR *batch =
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','0','Local password')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','1','RADIUS password')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','2','Certificate')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','3','Certificate or local password')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','4','Certificate or RADIUS password')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('LDAP.NewUserAuthMethod','5','LDAP password')\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      DB_RESULT hResult = SQLSelect(_T("SELECT id,flags FROM users"));
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            if (DBGetFieldULong(hResult, i, 1) & UF_LDAP_USER)
            {
               TCHAR query[256];
               _sntprintf(query, 256, _T("UPDATE users SET auth_method=5 WHERE id=%u"), DBGetFieldULong(hResult, i, 0));
               CHK_EXEC(SQLQuery(query));
            }
         }
         DBFreeResult(hResult);
      }
      else if (!g_ignoreErrors)
      {
         return false;
      }

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 17));
   }
   CHK_EXEC(SetMinorSchemaVersion(29));
   return true;
}

/**
 * Upgrade from 40.27 to 40.28
 */
static bool H_UpgradeFromV27()
{
   if (GetSchemaLevelForMajorVersion(36) < 16)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='WindowsEvents.LogRetentionTime',description='Retention time in days for records in Windows event log. All records older than specified will be deleted by housekeeping process.' WHERE var_name='WinEventLogRetentionTime'")));
      CHK_EXEC(CreateConfigParam(_T("WindowsEvents.EnableStorage"), _T("1"), _T("Enable/disable local storage of received Windows events in NetXMS database."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 16));
   }
   CHK_EXEC(SetMinorSchemaVersion(28));
   return true;
}

/**
 * Upgrade from 40.26 to 40.27
 */
static bool H_UpgradeFromV26()
{
   if (GetSchemaLevelForMajorVersion(36) < 15)
   {
      static const TCHAR *batch =
            _T("UPDATE config SET var_name='Alarms.HistoryRetentionTime',units='days' WHERE var_name='AlarmHistoryRetentionTime'\n")
            _T("UPDATE config SET var_name='DBConnectionPool.CooldownTime',description='Inactivity time (in seconds) after which database connection will be closed.' WHERE var_name='DBConnectionPoolCooldownTime'\n")
            _T("UPDATE config SET var_name='DBConnectionPool.MaxLifetime',description='Maximum lifetime (in seconds) for a database connection.' WHERE var_name='DBConnectionPoolMaxLifetime'\n")
            _T("UPDATE config SET var_name='DBConnectionPool.BaseSize' WHERE var_name='DBConnectionPoolBaseSize'\n")
            _T("UPDATE config SET var_name='DBConnectionPool.MaxSize' WHERE var_name='DBConnectionPoolMaxSize'\n")
            _T("UPDATE config SET description='Name of discovery filter script from script library.' WHERE var_name='DiscoveryFilter'\n")
            _T("UPDATE config SET description='Discovery filter settings.' WHERE var_name='DiscoveryFilterFlags'\n")
            _T("UPDATE config SET description='Enable/disable reporting server.' WHERE var_name='EnableReportingServer'\n")
            _T("UPDATE config SET description='Enable/disable ability to acknowledge an alarm for a specific time.' WHERE var_name='EnableTimedAlarmAck'\n")
            _T("UPDATE config SET description='Enable/disable TAB and new line characters replacement by escape sequence in \"execute command on management server\" actions.' WHERE var_name='EscapeLocalCommands'\n")
            _T("UPDATE config SET description='Retention time in days for the records in event log. All records older than specified will be deleted by housekeeping process.' WHERE var_name='EventLogRetentionTime'\n")
            _T("UPDATE config SET description='Value for status propagation if StatusPropagationAlgorithm server configuration parameter is set to 2 (Fixed).' WHERE var_name='FixedStatusValue'\n")
            _T("UPDATE config SET description='Helpdesk driver name. If set to none, then no helpdesk driver is in use.' WHERE var_name='HelpDeskLink'\n")
            _T("UPDATE config SET description='Number of incorrect password attempts after which a user account is temporarily locked.' WHERE var_name='IntruderLockoutThreshold'\n")
            _T("UPDATE config SET description='Duration of user account temporarily lockout (in minutes) if allowed number of incorrect password attempts was exceeded.' WHERE var_name='IntruderLockoutTime'\n")
            _T("UPDATE config SET description='Jira issue type' WHERE var_name='JiraIssueType'\n")
            _T("UPDATE config SET description='Jira project component' WHERE var_name='JiraProjectComponent'\n")
            _T("UPDATE config SET description='Unique identifier for LDAP group object. If not set, LDAP users are identified by DN.' WHERE var_name='LDAP.GroupUniqueId'\n")
            _T("UPDATE config SET description='Unique identifier for LDAP user object. If not set, LDAP users are identified by DN.' WHERE var_name='LDAP.UserUniqueId'\n")
            _T("UPDATE config SET description='Listener port for connections from mobile agent.' WHERE var_name='MobileDeviceListenerPort'\n")
            _T("UPDATE config SET description='Time (in seconds) to wait for output of a local command object tool.' WHERE var_name='ServerCommandOutputTimeout'\n")
            _T("UPDATE config SET description='Default algorithm for calculation object status from it''s DCIs, alarms and child objects.' WHERE var_name='StatusCalculationAlgorithm'\n")
            _T("UPDATE config SET description='Status shift value for Relative propagation algorithm.' WHERE var_name='StatusShift'\n")
            _T("UPDATE config SET description='Threshold value for Single threshold status calculation algorithm.' WHERE var_name='StatusSingleThreshold'\n")
            _T("UPDATE config SET description='Threshold values for Multiple thresholds status calculation algorithm.' WHERE var_name='StatusThresholds'\n")
            _T("UPDATE config SET description='Values for Translated status propagation algorithm.' WHERE var_name='StatusTranslation'\n")
            _T("UPDATE config_values SET var_description='IP, then hostname' WHERE var_name='Syslog.NodeMatchingPolicy' AND var_value='0'\n")
            _T("DELETE FROM config_values WHERE var_name='StatusPropagationAlgorithm' AND var_value='0'\n")
            _T("UPDATE config SET data_type='C' WHERE var_name='StatusCalculationAlgorithm'\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('StatusCalculationAlgorithm','1','Most critical')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('StatusCalculationAlgorithm','2','Single threshold')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('StatusCalculationAlgorithm','3','Multiple thresholds')\n")
            _T("UPDATE config_values SET var_name='Objects.Interfaces.DefaultExpectedState' WHERE var_name='DefaultInterfaceExpectedState'\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(CreateConfigParam(_T("NXSL.EnableContainerFunctions"), _T("1"), _T("Enable/disable server-side NXSL functions for containers (such as CreateContainer, BindObject, etc.)."), nullptr, 'B', true, false, false, false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 15));
   }
   CHK_EXEC(SetMinorSchemaVersion(27));
   return true;
}

/**
 * Upgrade from 40.25 to 40.26
 */
static bool H_UpgradeFromV25()
{
   if (GetSchemaLevelForMajorVersion(36) < 14)
   {
      static const TCHAR *batch =
            _T("ALTER TABLE dc_targets ADD geolocation_ctrl_mode integer\n")
            _T("ALTER TABLE dc_targets ADD geo_areas varchar(2000)\n")
            _T("UPDATE dc_targets SET geolocation_ctrl_mode=0\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("dc_targets"), _T("geolocation_ctrl_mode")));

      // Update access rights for predefined "Admins" group
      DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
            access |= SYSTEM_ACCESS_MANAGE_GEO_AREAS;
            TCHAR query[256];
            _sntprintf(query, 256, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE id=1073741825"), access);
            CHK_EXEC(SQLQuery(query));
         }
         DBFreeResult(hResult);
      }
      else
      {
         if (!g_ignoreErrors)
            return false;
      }

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 14));
   }
   CHK_EXEC(SetMinorSchemaVersion(26));
   return true;
}

/**
 * Upgrade from 40.24 to 40.25
 */
static bool H_UpgradeFromV24()
{
   if (GetSchemaLevelForMajorVersion(36) < 13)
   {
      CHK_EXEC(CreateTable(
            _T("CREATE TABLE dc_targets (")
            _T("  id integer not null,")
            _T("  config_poll_timestamp integer not null,")
            _T("  instance_poll_timestamp integer not null,")
            _T("PRIMARY KEY(id))")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 13));
   }
   CHK_EXEC(SetMinorSchemaVersion(25));
   return true;
}

/**
 * Upgrade from 40.23 to 40.24
 */
static bool H_UpgradeFromV23()
{
   if (GetSchemaLevelForMajorVersion(36) < 12)
   {
      static const TCHAR *batch =
            _T("UPDATE event_cfg SET event_name='SYS_SNMP_TRAP_FLOOD_DETECTED' WHERE event_code=109\n")
            _T("UPDATE event_cfg SET event_name='SYS_SNMP_TRAP_FLOOD_ENDED' WHERE event_code=110\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(CreateEventTemplate(EVENT_GEOLOCATION_CHANGED, _T("SYS_GEOLOCATION_CHANGED"),
               SEVERITY_NORMAL, EF_LOG, _T("2c2f40d4-91d3-441e-92d4-62a32e98a341"),
               _T("Device geolocation changed to %3 %4 (was %7 %8)"),
               _T("Generated when device geolocation change is detected.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Latitude of new location in degrees\r\n")
               _T("   2) Longitude of new location in degrees\r\n")
               _T("   3) Latitude of new location in textual form\r\n")
               _T("   4) Longitude of new location in textual form\r\n")
               _T("   5) Latitude of previous location in degrees\r\n")
               _T("   6) Longitude of previous location in degrees\r\n")
               _T("   7) Latitude of previous location in textual form\r\n")
               _T("   8) Longitude of previous location in textual form")
               ));

      CHK_EXEC(CreateEventTemplate(EVENT_GEOLOCATION_INSIDE_RESTRICTED_AREA, _T("SYS_GEOLOCATION_INSIDE_RESTRICTED_AREA"),
               SEVERITY_WARNING, EF_LOG, _T("94d7a94b-a289-4cbe-8a83-a9c52b36c650"),
               _T("Device geolocation %3 %4 is within restricted area %5"),
               _T("Generated when new device geolocation is within restricted area.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Latitude of new location in degrees\r\n")
               _T("   2) Longitude of new location in degrees\r\n")
               _T("   3) Latitude of new location in textual form\r\n")
               _T("   4) Longitude of new location in textual form\r\n")
               _T("   5) Area name\r\n")
               _T("   6) Area ID")
               ));

      CHK_EXEC(CreateEventTemplate(EVENT_GEOLOCATION_OUTSIDE_ALLOWED_AREA, _T("SYS_GEOLOCATION_OUTSIDE_ALLOWED_AREA"),
               SEVERITY_WARNING, EF_LOG, _T("24dcb315-a5cc-41f5-a813-b0a5f8c31e7d"),
               _T("Device geolocation %3 %4 is outside allowed area"),
               _T("Generated when new device geolocation is within restricted area.\r\n")
               _T("Parameters:\r\n")
               _T("   1) Latitude of new location in degrees\r\n")
               _T("   2) Longitude of new location in degrees\r\n")
               _T("   3) Latitude of new location in textual form\r\n")
               _T("   4) Longitude of new location in textual form")
               ));

      CHK_EXEC(CreateTable(
            _T("CREATE TABLE geo_areas (")
            _T("   id integer not null,")
            _T("   name varchar(127) not null,")
            _T("   comments $SQL:TEXT null,")
            _T("   configuration $SQL:TEXT null,")
            _T("   PRIMARY KEY(id))")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 12));
   }
   CHK_EXEC(SetMinorSchemaVersion(24));
   return true;
}

/**
 * Upgrade from 40.22 to 40.23
 */
static bool H_UpgradeFromV22()
{
   if (GetSchemaLevelForMajorVersion(36) < 11)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD snmp_engine_id varchar(255)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 11));
   }
   CHK_EXEC(SetMinorSchemaVersion(23));
   return true;
}

/**
 * Upgrade from 40.21 to 40.22
 */
static bool H_UpgradeFromV21()
{
   if (GetSchemaLevelForMajorVersion(36) < 10)
   {
      static const TCHAR *batch =
            _T("ALTER TABLE object_properties ADD category integer\n")
            _T("UPDATE object_properties SET category=0\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("object_properties"), _T("category")));
      CHK_EXEC(DBRenameColumn(g_dbHandle, _T("object_properties"), _T("image"), _T("map_image")));

      CHK_EXEC(CreateTable(
            _T("CREATE TABLE object_categories (")
            _T("   id integer not null,")
            _T("   name varchar(63) not null,")
            _T("   icon varchar(36) null,")
            _T("   map_image varchar(36) null,")
            _T("   PRIMARY KEY(id))")));

      // Update access rights for predefined "Admins" group
      DB_RESULT hResult = SQLSelect(_T("SELECT system_access FROM user_groups WHERE id=1073741825"));
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
         {
            uint64_t access = DBGetFieldUInt64(hResult, 0, 0);
            access |= SYSTEM_ACCESS_OBJECT_CATEGORIES;
            TCHAR query[256];
            _sntprintf(query, 256, _T("UPDATE user_groups SET system_access=") UINT64_FMT _T(" WHERE id=1073741825"), access);
            CHK_EXEC(SQLQuery(query));
         }
         DBFreeResult(hResult);
      }
      else
      {
         if (!g_ignoreErrors)
            return false;
      }

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 10));
   }
   CHK_EXEC(SetMinorSchemaVersion(22));
   return true;
}

/**
 * Upgrade from 40.20 to 40.21
 */
static bool H_UpgradeFromV20()
{
   if (GetSchemaLevelForMajorVersion(36) < 9)
   {
      static const TCHAR *batch =
            _T("UPDATE config SET var_name='Syslog.IgnoreMessageTimestamp' WHERE var_name='SyslogIgnoreMessageTimestamp'\n")
            _T("UPDATE config SET var_name='Syslog.ListenPort' WHERE var_name='SyslogListenPort'\n")
            _T("UPDATE config_values SET var_name='Syslog.ListenPort' WHERE var_name='SyslogListenPort'\n")
            _T("UPDATE config SET var_name='Syslog.NodeMatchingPolicy' WHERE var_name='SyslogNodeMatchingPolicy'\n")
            _T("UPDATE config_values SET var_name='Syslog.NodeMatchingPolicy' WHERE var_name='SyslogNodeMatchingPolicy'\n")
            _T("UPDATE config SET var_name='Syslog.RetentionTime' WHERE var_name='SyslogRetentionTime'\n")
            _T("UPDATE config SET var_name='Syslog.EnableListener' WHERE var_name='EnableSyslogReceiver'\n")
            _T("UPDATE config SET description='Retention time in days for stored syslog messages. All messages older than specified will be deleted by housekeeping process.' WHERE var_name='Syslog.RetentionTime'\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(CreateConfigParam(_T("Syslog.EnableStorage"), _T("1"),
               _T("Enable/disable local storage of received syslog messages in NetXMS database."),
               nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 9));
   }
   CHK_EXEC(SetMinorSchemaVersion(21));
   return true;
}

/**
 * Upgrade from 40.19 to 40.20
 */
static bool H_UpgradeFromV19()
{
   if (GetSchemaLevelForMajorVersion(36) < 8)
   {
      static const TCHAR *batch =
            _T("ALTER TABLE mobile_devices ADD speed varchar(20)\n")
            _T("ALTER TABLE mobile_devices ADD direction integer\n")
            _T("ALTER TABLE mobile_devices ADD altitude integer\n")
            _T("UPDATE mobile_devices SET speed='-1',direction=-1,altitude=0\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("mobile_devices"), _T("speed")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("mobile_devices"), _T("direction")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("mobile_devices"), _T("altitude")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 8));
   }
   CHK_EXEC(SetMinorSchemaVersion(20));
   return true;
}

/**
 * Upgrade from 40.18 to 40.19
 */
static bool H_UpgradeFromV18()
{
   if (GetSchemaLevelForMajorVersion(36) < 7)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE mobile_devices ADD comm_protocol varchar(31)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 7));
   }
   CHK_EXEC(SetMinorSchemaVersion(19));
   return true;
}

/**
 * Upgrade from 40.17 to 40.18
 */
static bool H_UpgradeFromV17()
{
   if (GetSchemaLevelForMajorVersion(36) < 6)
   {
      CHK_EXEC(CreateConfigParam(_T("WinEventLogRetentionTime"), _T("90"), _T("Retention time in days for records in windows event log. All records older than specified will be deleted by housekeeping process."), _T("days"), 'I', true, false, false, false));

      if (g_dbSyntax == DB_SYNTAX_TSDB)
      {
         CHK_EXEC(CreateTable(
               _T("CREATE TABLE win_event_log (")
               _T("  id $SQL:INT64 not null,")
               _T("  event_timestamp timestamptz not null,")
               _T("  node_id integer not null,")
               _T("  zone_uin integer not null,")
               _T("  origin_timestamp integer not null,")
               _T("  log_name varchar(63) null,")
               _T("  event_source varchar(127) null,")
               _T("  event_severity integer not null,")
               _T("  event_code integer not null,")
               _T("  message varchar(2000) null,")
               _T("  raw_data $SQL:TEXT null,")
               _T("PRIMARY KEY(id,event_timestamp))")));
         CHK_EXEC(SQLQuery(_T("SELECT create_hypertable('win_event_log', 'event_timestamp', chunk_time_interval => interval '86400 seconds')")));
      }
      else
      {
         CHK_EXEC(CreateTable(
               _T("CREATE TABLE win_event_log (")
               _T("  id $SQL:INT64 not null,")
               _T("  event_timestamp integer not null,")
               _T("  node_id integer not null,")
               _T("  zone_uin integer not null,")
               _T("  origin_timestamp integer not null,")
               _T("  log_name varchar(63) null,")
               _T("  event_source varchar(127) null,")
               _T("  event_severity integer not null,")
               _T("  event_code integer not null,")
               _T("  message varchar(2000) null,")
               _T("  raw_data $SQL:TEXT null,")
               _T("PRIMARY KEY(id))")));
      }
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_win_event_log_timestamp ON win_event_log(event_timestamp)")));
      CHK_EXEC(SQLQuery(_T("CREATE INDEX idx_win_event_log_node ON win_event_log(node_id)")));

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 6));
   }
   CHK_EXEC(SetMinorSchemaVersion(18));
   return true;
}

/**
 * Upgrade form 40.16 to 40.17
 */
static bool H_UpgradeFromV16()
{
   if (GetSchemaLevelForMajorVersion(36) < 5)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='SNMP.Traps.LogAll',need_server_restart=0,description='Log all SNMP traps (even those received from addresses not belonging to any known node).' WHERE var_name='LogAllSNMPTraps'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET var_name='SNMP.Traps.AllowVarbindsConversion',need_server_restart=0 WHERE var_name='AllowTrapVarbindsConversion'")));
      CHK_EXEC(SQLQuery(_T("UPDATE config SET need_server_restart=0 WHERE var_name='SNMP.Traps.ProcessUnmanagedNodes'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 5));
   }
   CHK_EXEC(SetMinorSchemaVersion(17));
   return true;
}

/**
 * Upgrade form 40.15 to 40.16
 */
static bool H_UpgradeFromV15()
{
   if (GetSchemaLevelForMajorVersion(36) < 4)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD agent_cert_mapping_method char(1)")));
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD agent_cert_mapping_data varchar(500)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 4));
   }
   CHK_EXEC(SetMinorSchemaVersion(16));
   return true;
}

/**
 * Upgrade form 40.14 to 40.15
 */
static bool H_UpgradeFromV14()
{
   if (GetSchemaLevelForMajorVersion(36) < 3)
   {
      CHK_EXEC(SQLQuery(_T("DROP TABLE certificates")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 3));
   }
   CHK_EXEC(SetMinorSchemaVersion(15));
   return true;
}

/**
 * Upgrade form 40.13 to 40.14
 */
static bool H_UpgradeFromV13()
{
   if (GetSchemaLevelForMajorVersion(36) < 2)
   {
      CHK_EXEC(CreateConfigParam(_T("AgentTunnels.TLS.MinVersion"), _T("2"), _T("Minimal version of TLS protocol used on agent tunnel connection"), nullptr, 'C', true, false, false, false));

      static const TCHAR *batch =
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','0','1.0')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','1','1.1')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','2','1.2')\n")
            _T("INSERT INTO config_values (var_name,var_value,var_description) VALUES ('AgentTunnels.TLS.MinVersion','3','1.3')\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 2));
   }
   CHK_EXEC(SetMinorSchemaVersion(14));
   return true;
}

/**
 * Upgrade form 40.12 to 40.13
 */
static bool H_UpgradeFromV12()
{
   if (GetSchemaLevelForMajorVersion(36) < 1)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE object_properties ADD name_on_map varchar(63)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(36, 1));
   }
   CHK_EXEC(SetMinorSchemaVersion(13));
   return true;
}

/**
 * Upgrade form 40.11 to 40.12
 */
static bool H_UpgradeFromV11()
{
   if (GetSchemaLevelForMajorVersion(35) < 16)
   {
      CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET event_name='SNMP_TRAP_FLOOD_ENDED' WHERE event_code=110")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 16));
   }
   CHK_EXEC(SetMinorSchemaVersion(12));
   return true;
}

/**
 * Upgrade form 40.10 to 40.11
 */
static bool H_UpgradeFromV10()
{
   if (GetSchemaLevelForMajorVersion(35) < 15)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE object_properties ADD alias varchar(255)")));
      CHK_EXEC(SQLQuery(_T("UPDATE object_properties SET alias=(SELECT alias FROM interfaces WHERE interfaces.id=object_properties.object_id)")));
      CHK_EXEC(DBDropColumn(g_dbHandle, _T("interfaces"), _T("alias")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 15));
   }
   CHK_EXEC(SetMinorSchemaVersion(11));
   return true;
}

/**
 * Upgrade form 40.9 to 40.10
 */
static bool H_UpgradeFromV9()
{
   CHK_EXEC(SQLQuery(_T("UPDATE event_cfg SET event_name='SYS_NOTIFICATION_FAILURE',")
         _T("message='Unable to send notification using notification channel %1 to %2',description='")
         _T("Generated when server is unable to send notification.\r\n")
         _T("Parameters:\r\n")
         _T("   1) Notification channel name\r\n")
         _T("   2) Recipient address\r\n")
         _T("   3) Notification subject")
         _T("   4) Notification message")
         _T("' WHERE event_code=22")));

   CHK_EXEC(CreateConfigParam(_T("DefaultNotificationChannel.SMTP.Html"), _T("SMTP-HTML"), _T("Default notification channel for SMTP HTML formatted messages"), nullptr, 'S', true, false, false, false));
   CHK_EXEC(CreateConfigParam(_T("DefaultNotificationChannel.SMTP.Text"), _T("SMTP-Text"), _T("Default notification channel for SMTP Text formatted messages"), nullptr, 'S', true, false, false, false));

   TCHAR server[MAX_STRING_VALUE];
   TCHAR localHostName[MAX_STRING_VALUE];
   TCHAR fromName[MAX_STRING_VALUE];
   TCHAR fromAddr[MAX_STRING_VALUE];
   TCHAR mailEncoding[MAX_STRING_VALUE];
   DBMgrConfigReadStr(_T("SMTP.Server"), server, MAX_STRING_VALUE, _T("localhost"));
   DBMgrConfigReadStr(_T("SMTP.LocalHostName"), localHostName, MAX_STRING_VALUE, _T(""));
   DBMgrConfigReadStr(_T("SMTP.FromName"), fromName, MAX_STRING_VALUE, _T("NetXMS Server"));
   DBMgrConfigReadStr(_T("SMTP.FromAddr"), fromAddr, MAX_STRING_VALUE, _T("netxms@localhost"));
   DBMgrConfigReadStr(_T("MailEncoding"), mailEncoding, MAX_STRING_VALUE, _T("utf8"));
   uint32_t retryCount = DBMgrConfigReadUInt32(_T("SMTP.RetryCount"), 1);
   uint16_t port = static_cast<uint16_t>(DBMgrConfigReadUInt32(_T("SMTP.Port"), 25));

   const TCHAR *configTemplate = _T("Server=%s\r\n")
         _T("RetryCount=%u\r\n")
         _T("Port=%hu\r\n")
         _T("LocalHostName=%s\r\n")
         _T("FromName=%s\r\n")
         _T("FromAddr=%s\r\n")
         _T("MailEncoding=%s\r\n")
         _T("IsHTML=%s");

   DB_STATEMENT hStmt = DBPrepare(g_dbHandle, _T("INSERT INTO notification_channels (name,driver_name,description,configuration) VALUES (?,'SMTP',?,?)"), true);
   if (hStmt != NULL)
   {
      TCHAR tmpConfig[1024];
      _sntprintf(tmpConfig, 1024, configTemplate, server, retryCount, port, localHostName, fromName, fromAddr, mailEncoding, _T("no"));
     DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, _T("SMTP-Text"), DB_BIND_STATIC);
     DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, _T("Default SMTP text channel (automatically migrated)"), DB_BIND_STATIC);
     DBBind(hStmt, 3, DB_SQLTYPE_TEXT, tmpConfig, DB_BIND_STATIC);
     if (!SQLExecute(hStmt) && !g_ignoreErrors)
     {
        DBFreeStatement(hStmt);
        return false;
     }

     _sntprintf(tmpConfig, 1024, configTemplate, server, retryCount, port, localHostName, fromName, fromAddr, mailEncoding, _T("yes"));
    DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, _T("SMTP-HTML"), DB_BIND_STATIC);
    DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, _T("Default SMTP HTML channel (automatically migrated)"), DB_BIND_STATIC);
    DBBind(hStmt, 3, DB_SQLTYPE_TEXT, tmpConfig, DB_BIND_STATIC);
    if (!SQLExecute(hStmt) && !g_ignoreErrors)
    {
       DBFreeStatement(hStmt);
       return false;
    }

     DBFreeStatement(hStmt);
   }
   else if (!g_ignoreErrors)
     return false;

   //Delete old configuration parameters
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.Server'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.LocalHostName'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.FromName'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.FromAddr'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.RetryCount'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='SMTP.Port'")));
   CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='MailEncoding'")));

   CHK_EXEC(SQLQuery(_T("UPDATE actions SET action_type=3,channel_name='SMTP-Text' WHERE action_type=2")));

   CHK_EXEC(SetMinorSchemaVersion(10));
   return true;
}

/**
 * Upgrade form 40.8 to 40.9
 */
static bool H_UpgradeFromV8()
{
   if (GetSchemaLevelForMajorVersion(35) < 14)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE policy_action_list ADD blocking_timer_key varchar(127)")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 14));
   }
   CHK_EXEC(SetMinorSchemaVersion(9));
   return true;
}

/**
 * Upgrade form 40.7 to 40.8
 */
static bool H_UpgradeFromV7()
{
   if (GetSchemaLevelForMajorVersion(35) < 13)
   {
      CHK_EXEC(SQLQuery(_T("DELETE FROM config WHERE var_name='AllowDirectNotifications'")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 13));
   }
   CHK_EXEC(SetMinorSchemaVersion(8));
   return true;
}

/**
 * Upgrade form 40.6 to 40.7
 */
static bool H_UpgradeFromV6()
{
   if (GetSchemaLevelForMajorVersion(35) < 12)
   {
      CHK_EXEC(SQLQuery(_T("ALTER TABLE nodes ADD cip_vendor_code integer")));
      CHK_EXEC(SQLQuery(_T("UPDATE nodes SET cip_vendor_code=0")));
      CHK_EXEC(DBSetNotNullConstraint(g_dbHandle, _T("nodes"), _T("cip_vendor_code")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 12));
   }
   CHK_EXEC(SetMinorSchemaVersion(7));
   return true;
}

/**
 * Upgrade from 40.5 to 40.6
 */
static bool H_UpgradeFromV5()
{
   if (GetSchemaLevelForMajorVersion(35) < 11)
   {
      CHK_EXEC(CreateConfigParam(_T("Events.Processor.PoolSize"), _T("1"), _T("Number of threads for parallel event processing."), _T("threads"), 'I', true, true, false, false));
      CHK_EXEC(CreateConfigParam(_T("Events.Processor.QueueSelector"), _T("%z"), _T("Queue selector for parallel event processing."), nullptr, 'S', true, true, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 11));
   }
   CHK_EXEC(SetMinorSchemaVersion(6));
   return true;
}

/**
 * Update group ID in given table
 */
bool UpdateGroupId(const TCHAR *table, const TCHAR *column);

/**
 * Upgrade from 40.4 to 40.5
 */
static bool H_UpgradeFromV4()
{
   if (GetSchemaLevelForMajorVersion(35) < 10)
   {
      CHK_EXEC(UpdateGroupId(_T("user_groups"), _T("id")));
      CHK_EXEC(UpdateGroupId(_T("user_group_members"), _T("group_id")));
      CHK_EXEC(UpdateGroupId(_T("userdb_custom_attributes"), _T("object_id")));
      CHK_EXEC(UpdateGroupId(_T("acl"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("dci_access"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("alarm_category_acl"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("object_tools_acl"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("graph_acl"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("object_access_snapshot"), _T("user_id")));
      CHK_EXEC(UpdateGroupId(_T("responsible_users"), _T("user_id")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 10));
   }
   CHK_EXEC(SetMinorSchemaVersion(5));
   return true;
}

/**
 * Upgrade from 40.3 to 40.4
 */
static bool H_UpgradeFromV3()
{
   if (GetSchemaLevelForMajorVersion(35) < 9)
   {
      CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.RateLimit.Threshold"), _T("0"), _T("Threshold for number of SNMP traps per second that defines SNMP trap flood condition. Detection is disabled if 0 is set."), _T("seconds"), 'I', true, false, false, false));
      CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.RateLimit.Duration"), _T("15"), _T("Time period for SNMP traps per second to be above threshold that defines SNMP trap flood condition."), _T("seconds"), 'I', true, false, false, false));

      CHK_EXEC(CreateEventTemplate(EVENT_SNMP_TRAP_FLOOD_DETECTED, _T("SNMP_TRAP_FLOOD_DETECTED"),
               SEVERITY_MAJOR, EF_LOG, _T("6b2bb689-23b7-4e7c-9128-5102f658e450"),
               _T("SNMP trap flood detected (Traps per second: %1)"),
               _T("Generated when system detects an SNMP trap flood.\r\n")
               _T("Parameters:\r\n")
               _T("   1) SNMP traps per second\r\n")
               _T("   2) Duration\r\n")
               _T("   3) Threshold")
               ));
      CHK_EXEC(CreateEventTemplate(EVENT_SNMP_TRAP_FLOOD_ENDED, _T("SNMP_TRAP_FLOOD_ENDED"),
               SEVERITY_NORMAL, EF_LOG, _T("f2c41199-9338-4c9a-9528-d65835c6c271"),
               _T("SNMP trap flood ended"),
               _T("Generated after SNMP trap flood state is cleared.\r\n")
               _T("Parameters:\r\n")
               _T("   1) SNMP traps per second\r\n")
               _T("   2) Duration\r\n")
               _T("   3) Threshold")
               ));

      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 9));
   }

   CHK_EXEC(SetMinorSchemaVersion(4));
   return true;
}

/**
 * Upgrade from 40.2 to 40.3
 */
static bool H_UpgradeFromV2()
{
   if (GetSchemaLevelForMajorVersion(35) < 8)
   {
      static const TCHAR *batch =
            _T("UPDATE config SET description='A bitmask for encryption algorithms allowed in the server(sum the values to allow multiple algorithms at once): 1 = AES256, 2 = Blowfish-256, 4 = IDEA, 8 = 3DES, 16 = AES128, 32 = Blowfish-128.' WHERE var_name='AllowedCiphers'\n")
            _T("UPDATE config SET description='Comma-separated list of hosts to be used as beacons for checking NetXMS server network connectivity. Either DNS names or IP addresses can be used. This list is pinged by NetXMS server and if none of the hosts have responded, server considers that connection with network is lost and generates specific event.' WHERE var_name='BeaconHosts'\n")
            _T("UPDATE config SET description='The LdapConnectionString configuration parameter may be a comma- or whitespace-separated list of URIs containing only the schema, the host, and the port fields. Format: schema://host:port.' WHERE var_name='LDAP.ConnectionString'\n")
            _T("UPDATE config SET units='minutes',description='The synchronization interval (in minutes) between the NetXMS server and the LDAP server. If the parameter is set to 0, no synchronization will take place.' WHERE var_name='LDAP.SyncInterval'\n")
            _T("UPDATE config SET var_name='Client.MinViewRefreshInterval',default_value='300',units='milliseconds',description='Minimal interval between view refresh in milliseconds (hint for client).' WHERE var_name='MinViewRefreshInterval'\n")
            _T("UPDATE config SET var_name='Beacon.Hosts' WHERE var_name='BeaconHosts'\n")
            _T("UPDATE config SET var_name='Beacon.PollingInterval' WHERE var_name='BeaconPollingInterval'\n")
            _T("UPDATE config SET var_name='Beacon.Timeout' WHERE var_name='BeaconTimeout'\n")
            _T("UPDATE config SET var_name='SNMP.Traps.Enable' WHERE var_name='EnableSNMPTraps'\n")
            _T("UPDATE config SET var_name='SNMP.Traps.ListenerPort' WHERE var_name='SNMPTrapPort'\n")
            _T("UPDATE config_values SET var_name='SNMP.Traps.ListenerPort' WHERE var_name='SNMPTrapPort'\n")
            _T("UPDATE config SET var_name='SNMP.Traps.ProcessUnmanagedNodes',default_value='0',data_type='B',description='Enable/disable processing of SNMP traps received from unmanaged nodes.' WHERE var_name='ProcessTrapsFromUnmanagedNodes'\n")
            _T("DELETE FROM config WHERE var_name='SNMPPorts'\n")
            _T("DELETE FROM config_values WHERE var_name='SNMPPorts'\n")
            _T("<END>");
      CHK_EXEC(SQLBatch(batch));

      CHK_EXEC(CreateConfigParam(_T("SNMP.Traps.ProcessUnmanagedNodes"), _T("0"), _T("Enable/disable processing of SNMP traps received from unmanaged nodes."), nullptr, 'B', true, true, false, false));

      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 8));
   }
   CHK_EXEC(SetMinorSchemaVersion(3));
   return true;
}

/**
 * Upgrade from 40.1 to 40.2
 */
static bool H_UpgradeFromV1()
{
   if (GetSchemaLevelForMajorVersion(35) < 7)
   {
      CHK_EXEC(SQLQuery(
            _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
            _T("VALUES ('9c2dba59-493b-4645-9159-2ad7a28ea611',23,'Hook::OpenBoundTunnel','")
            _T("/* Available global variables:\r\n")
            _T(" *  $tunnel - incoming tunnel information (object of ''Tunnel'' class)\r\n")
            _T(" *\r\n")
            _T(" * Expected return value:\r\n")
            _T(" *  none - returned value is ignored\r\n */\r\n')")));
      CHK_EXEC(SQLQuery(
            _T("INSERT INTO script_library (guid,script_id,script_name,script_code) ")
            _T("VALUES ('64c90b92-27e9-4a96-98ea-d0e152d71262',24,'Hook::OpenUnboundTunnel','")
            _T("/* Available global variables:\r\n")
            _T(" *  $node - node this tunnel was bound to (object of ''Node'' class)\r\n")
            _T(" *  $tunnel - incoming tunnel information (object of ''Tunnel'' class)\r\n")
            _T(" *\r\n")
            _T(" * Expected return value:\r\n")
            _T(" *  none - returned value is ignored\r\n */\r\n')")));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 7));
   }

   CHK_EXEC(SetMinorSchemaVersion(2));
   return true;
}

/**
 * Upgrade from 40.0 to 40.1
 */
static bool H_UpgradeFromV0()
{
   if (GetSchemaLevelForMajorVersion(35) < 6)
   {
      CHK_EXEC(CreateConfigParam(_T("RoamingServer"), _T("0"), _T("Enable/disable roaming mode for server (when server can be disconnected from one network and connected to another or IP address of the server can change)."), nullptr, 'B', true, false, false, false));
      CHK_EXEC(SetSchemaLevelForMajorVersion(35, 6));
   }
   CHK_EXEC(SetMinorSchemaVersion(1));
   return true;
}

/**
 * Upgrade map
 */
static struct
{
   int version;
   int nextMajor;
   int nextMinor;
   bool (*upgradeProc)();
} s_dbUpgradeMap[] =
{
   { 62, 40, 63, H_UpgradeFromV62 },
   { 61, 40, 62, H_UpgradeFromV61 },
   { 60, 40, 61, H_UpgradeFromV60 },
   { 59, 40, 60, H_UpgradeFromV59 },
   { 58, 40, 59, H_UpgradeFromV58 },
   { 57, 40, 58, H_UpgradeFromV57 },
   { 56, 40, 57, H_UpgradeFromV56 },
   { 55, 40, 56, H_UpgradeFromV55 },
   { 54, 40, 55, H_UpgradeFromV54 },
   { 53, 40, 54, H_UpgradeFromV53 },
   { 52, 40, 53, H_UpgradeFromV52 },
   { 51, 40, 52, H_UpgradeFromV51 },
   { 50, 40, 51, H_UpgradeFromV50 },
   { 49, 40, 50, H_UpgradeFromV49 },
   { 48, 40, 49, H_UpgradeFromV48 },
   { 47, 40, 48, H_UpgradeFromV47 },
   { 46, 40, 47, H_UpgradeFromV46 },
   { 45, 40, 46, H_UpgradeFromV45 },
   { 44, 40, 45, H_UpgradeFromV44 },
   { 43, 40, 44, H_UpgradeFromV43 },
   { 42, 40, 43, H_UpgradeFromV42 },
   { 41, 40, 42, H_UpgradeFromV41 },
   { 40, 40, 41, H_UpgradeFromV40 },
   { 39, 40, 40, H_UpgradeFromV39 },
   { 38, 40, 39, H_UpgradeFromV38 },
   { 37, 40, 38, H_UpgradeFromV37 },
   { 36, 40, 37, H_UpgradeFromV36 },
   { 35, 40, 38, H_UpgradeFromV35 },
   { 34, 40, 35, H_UpgradeFromV34 },
   { 33, 40, 34, H_UpgradeFromV33 },
   { 32, 40, 33, H_UpgradeFromV32 },
   { 31, 40, 32, H_UpgradeFromV31 },
   { 30, 40, 31, H_UpgradeFromV30 },
   { 29, 40, 30, H_UpgradeFromV29 },
   { 28, 40, 29, H_UpgradeFromV28 },
   { 27, 40, 28, H_UpgradeFromV27 },
   { 26, 40, 27, H_UpgradeFromV26 },
   { 25, 40, 26, H_UpgradeFromV25 },
   { 24, 40, 25, H_UpgradeFromV24 },
   { 23, 40, 24, H_UpgradeFromV23 },
   { 22, 40, 23, H_UpgradeFromV22 },
   { 21, 40, 22, H_UpgradeFromV21 },
   { 20, 40, 21, H_UpgradeFromV20 },
   { 19, 40, 20, H_UpgradeFromV19 },
   { 18, 40, 19, H_UpgradeFromV18 },
   { 17, 40, 18, H_UpgradeFromV17 },
   { 16, 40, 17, H_UpgradeFromV16 },
   { 15, 40, 16, H_UpgradeFromV15 },
   { 14, 40, 15, H_UpgradeFromV14 },
   { 13, 40, 14, H_UpgradeFromV13 },
   { 12, 40, 13, H_UpgradeFromV12 },
   { 11, 40, 12, H_UpgradeFromV11 },
   { 10, 40, 11, H_UpgradeFromV10 },
   { 9,  40, 10, H_UpgradeFromV9  },
   { 8,  40, 9,  H_UpgradeFromV8  },
   { 7,  40, 8,  H_UpgradeFromV7  },
   { 6,  40, 7,  H_UpgradeFromV6  },
   { 5,  40, 6,  H_UpgradeFromV5  },
   { 4,  40, 5,  H_UpgradeFromV4  },
   { 3,  40, 4,  H_UpgradeFromV3  },
   { 2,  40, 3,  H_UpgradeFromV2  },
   { 1,  40, 2,  H_UpgradeFromV1  },
   { 0,  40, 1,  H_UpgradeFromV0  },
   { 0,  0,  0,  nullptr          }
};

/**
 * Upgrade database to new version
 */
bool MajorSchemaUpgrade_V40()
{
   int32_t major, minor;
   if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
      return false;

   while((major == 40) && (minor < DB_SCHEMA_VERSION_V40_MINOR))
   {
      // Find upgrade procedure
      int i;
      for(i = 0; s_dbUpgradeMap[i].upgradeProc != nullptr; i++)
         if (s_dbUpgradeMap[i].version == minor)
            break;
      if (s_dbUpgradeMap[i].upgradeProc == nullptr)
      {
         _tprintf(_T("Unable to find upgrade procedure for version 40.%d\n"), minor);
         return false;
      }
      _tprintf(_T("Upgrading from version 40.%d to %d.%d\n"), minor, s_dbUpgradeMap[i].nextMajor, s_dbUpgradeMap[i].nextMinor);
      DBBegin(g_dbHandle);
      if (s_dbUpgradeMap[i].upgradeProc())
      {
         DBCommit(g_dbHandle);
         if (!DBGetSchemaVersion(g_dbHandle, &major, &minor))
            return false;
      }
      else
      {
         _tprintf(_T("Rolling back last stage due to upgrade errors...\n"));
         DBRollback(g_dbHandle);
         return false;
      }
   }
   return true;
}
