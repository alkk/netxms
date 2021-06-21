/*
** NetXMS - Network Management System
** Copyright (C) 2019-2021 Raden Solutions
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
** File: notification_channel.cpp
**
**/

#include "nxcore.h"
#include <ncdrv.h>

#define DEBUG_TAG _T("nc")
#define NC_THREAD_KEY _T("NotificationChannel")

/**
 * Static data
 */
static uint64_t s_notificationId = 1;  // Next available notification ID

/**
 * Class to store in queued notification message
 */
class NotificationMessage
{
   TCHAR *m_recipient;
   TCHAR *m_subject;
   TCHAR *m_body;

public:
   NotificationMessage(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body);
   ~NotificationMessage();

   const TCHAR *getRecipient() const { return m_recipient; }
   const TCHAR *getSubject() const { return m_subject; }
   const TCHAR *getBody() const { return m_body; }
};

/**
 * Storage manager for driver
 */
class NCDriverServerStorageManager : public NCDriverStorageManager
{
private:
   TCHAR m_channelName[MAX_OBJECT_NAME];

public:
   NCDriverServerStorageManager(const TCHAR *channelName) : NCDriverStorageManager() { _tcslcpy(m_channelName, channelName, MAX_OBJECT_NAME); }
   virtual ~NCDriverServerStorageManager() { }

   virtual TCHAR *get(const TCHAR *key) override;
   virtual StringMap *getAll() override;
   virtual void set(const TCHAR *key, const TCHAR *value) override;
   virtual void clear(const TCHAR *key) override;
};

/**
 * Get value for given key
 */
TCHAR *NCDriverServerStorageManager::get(const TCHAR *key)
{
   TCHAR *value = nullptr;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT entry_value FROM nc_persistent_storage WHERE channel_name=? AND entry_name=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_channelName, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         if (DBGetNumRows(hResult) > 0)
            value = DBGetField(hResult, 0, 0, nullptr, 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return value;
}

/**
 * Get all keys for this channel
 */
StringMap *NCDriverServerStorageManager::getAll()
{
   StringMap *values = new StringMap();
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT entry_name,entry_value FROM nc_persistent_storage WHERE channel_name=?"));
   if (hStmt != NULL)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_channelName, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         int count = DBGetNumRows(hResult);
         for(int i = 0; i < count; i++)
         {
            TCHAR *key = DBGetField(hResult, i, 0, nullptr, 0);
            TCHAR *value = DBGetField(hResult, i, 1, nullptr, 0);
            if ((key != nullptr) && (value != nullptr))
            {
               values->setPreallocated(key, value);
            }
            else
            {
               MemFree(key);
               MemFree(value);
            }
         }
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
   return values;
}

/**
 * Set value for given key
 */
void NCDriverServerStorageManager::set(const TCHAR *key, const TCHAR *value)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("SELECT entry_name FROM nc_persistent_storage WHERE channel_name=? AND entry_name=?"));
   if (hStmt != nullptr)
   {
      bool update = false;
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_channelName, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
      DB_RESULT hResult = DBSelectPrepared(hStmt);
      if (hResult != nullptr)
      {
         update = (DBGetNumRows(hResult) > 0);
         DBFreeResult(hResult);
      }
      DBFreeStatement(hStmt);

      hStmt = DBPrepare(hdb,
               update ?
                        _T("UPDATE nc_persistent_storage SET entry_value=? WHERE channel_name=? AND entry_name=?") :
                        _T("INSERT INTO nc_persistent_storage (entry_value,channel_name,entry_name) VALUES (?,?,?)"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, value, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_channelName, DB_BIND_STATIC);
         DBBind(hStmt, 3, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
         DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Delete given key
 */
void NCDriverServerStorageManager::clear(const TCHAR *key)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM nc_persistent_storage WHERE channel_name=? AND entry_name=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_channelName, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, key, DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Last known status for message sending
 */
enum class NCSendStatus
{
   UNKNOWN = 0,
   SUCCESS = 1,
   FAILED = 2
};

/**
 * Configured notification channel
 */
class NotificationChannel
{
private:
   NCDriver *m_driver;
   TCHAR m_name[MAX_OBJECT_NAME];
   TCHAR m_description[MAX_NC_DESCRIPTION];
   MUTEX m_driverLock;
   THREAD m_workerThread;
   ObjectQueue<NotificationMessage> m_notificationQueue;
   TCHAR m_driverName[MAX_OBJECT_NAME];
   char *m_configuration;
   const NCConfigurationTemplate *m_confTemplate;
   TCHAR m_errorMessage[MAX_NC_ERROR_MESSAGE];
   NCDriverServerStorageManager *m_storageManager;
   NCSendStatus m_lastStatus;

   void setError(const TCHAR *message)
   {
      m_lastStatus = NCSendStatus::FAILED;
      _tcslcpy(m_errorMessage, message, MAX_NC_ERROR_MESSAGE);
   }

   void clearError()
   {
      m_lastStatus = NCSendStatus::SUCCESS;
      m_errorMessage[0] = 0;
   }

   void workerThread();
   void writeNotificationLog(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body, bool success);

public:
   NotificationChannel(NCDriver *driver, NCDriverServerStorageManager *storageManager, const TCHAR *name,
            const TCHAR *description, const TCHAR *driverName, char *config, const NCConfigurationTemplate *confTemplate, const TCHAR *errorMessage);
   ~NotificationChannel();

   void send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body);
   void fillMessage(NXCPMessage *msg, uint32_t base);
   const TCHAR *getName() const { return m_name; }
   const char *getConfiguration() const { return m_configuration; }

   void update(const TCHAR *description, const TCHAR *driverName, const char *config);
   void updateName(const TCHAR *newName) { _tcslcpy(m_name, newName, MAX_OBJECT_NAME); }
   void saveToDatabase();
};

/**
 * Notification channel driver descriptor
 */
struct NCDriverDescriptor
{
   NCDriver *(*instanceFactory)(Config *, NCDriverStorageManager *);
   const NCConfigurationTemplate *confTemplate;
   TCHAR name[MAX_OBJECT_NAME];
};

static StringObjectMap<NCDriverDescriptor> s_driverList(Ownership::True);
static StringObjectMap<NotificationChannel> s_channelList(Ownership::True);
static Mutex s_channelListLock;

/**
 * Notification message constructor
 */
NotificationMessage::NotificationMessage(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   m_recipient = MemCopyString(recipient);
   m_subject = MemCopyString(subject);
   m_body = MemCopyString(body);
}

/**
 * Notification message destructor
 */
NotificationMessage::~NotificationMessage()
{
   MemFree(m_recipient);
   MemFree(m_subject);
   MemFree(m_body);
}

/**
 * Notification channel constructor
 */
NotificationChannel::NotificationChannel(NCDriver *driver, NCDriverServerStorageManager *storageManager, const TCHAR *name,
         const TCHAR *description, const TCHAR *driverName, char *config, const NCConfigurationTemplate *confTemplate, const TCHAR *errorMessage) :
                  m_notificationQueue(64, Ownership::True)
{
   m_driver = driver;
   m_storageManager = storageManager;
   m_confTemplate = confTemplate;
   _tcslcpy(m_name, name, MAX_OBJECT_NAME);
   _tcslcpy(m_description, description, MAX_NC_DESCRIPTION);
   _tcslcpy(m_driverName, driverName, MAX_OBJECT_NAME);
   m_configuration = config;
   m_driverLock = MutexCreate();
   _tcslcpy(m_errorMessage, errorMessage, MAX_NC_ERROR_MESSAGE);
   m_lastStatus = NCSendStatus::UNKNOWN;
   m_workerThread = ThreadCreateEx(this, &NotificationChannel::workerThread);
}

/**
 * Notification channel destructor
 */
NotificationChannel::~NotificationChannel()
{
   m_notificationQueue.setShutdownMode();
   ThreadJoin(m_workerThread);
   MutexDestroy(m_driverLock);
   delete m_driver;
   delete m_storageManager;
   MemFree(m_configuration);
}

/**
 * Notification sending thread
 */
void NotificationChannel::workerThread()
{
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Worker thread for channel %s started"), m_name);
   while(true)
   {
      NotificationMessage *notification = m_notificationQueue.getOrBlock();
      if (notification == INVALID_POINTER_VALUE)
         break;

      MutexLock(m_driverLock);
      if (m_driver != nullptr)
      {
         bool success = m_driver->send(notification->getRecipient(), notification->getSubject(), notification->getBody());
         if (success)
         {
            clearError();
         }
         else
         {
            PostSystemEvent(EVENT_NOTIFICATION_FAILURE, g_dwMgmtNode, "ssss", m_name,
                  notification->getRecipient(), notification->getSubject(), notification->getBody());
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Driver error for channel %s, message dropped"), m_name);
            setError(_T("Driver error"));
         }
         writeNotificationLog(notification->getRecipient(), notification->getSubject(), notification->getBody(), success);
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 4, _T("No driver for channel %s, message dropped"), m_name);
         setError(_T("Driver not initialized"));
      }
      MutexUnlock(m_driverLock);
      delete notification;
   }
   nxlog_debug_tag(DEBUG_TAG, 2, _T("Worker thread for channel %s stopped"), m_name);
}

/**
 * Public method to send notification. It adds notification to the queue.
 */
void NotificationChannel::send(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body)
{
   m_notificationQueue.put(new NotificationMessage(recipient, subject, body));
}

/**
 * Fill NXCPMessage with notification channel data
 */
void NotificationChannel::fillMessage(NXCPMessage *msg, UINT32 base)
{
   msg->setField(base, m_name);
   msg->setField(base + 1, m_description);
   msg->setField(base + 2, m_driverName);
   msg->setFieldFromMBString(base + 3, m_configuration);
   msg->setField(base + 4, m_driver != nullptr);
   if (m_confTemplate != nullptr)
   {
      msg->setField(base + 5, m_confTemplate->needSubject);
      msg->setField(base + 6, m_confTemplate->needRecipient);
   }
   else
   {
      msg->setField(base + 5, false);
      msg->setField(base + 6, true);
   }
   msg->setField(base + 7, m_errorMessage);
   msg->setField(base + 8, static_cast<int16_t>(m_lastStatus));
}

/**
 * Update notification channel
 */
void NotificationChannel::update(const TCHAR *description, const TCHAR *driverName, const char *config)
{
   _tcslcpy(m_description, description, MAX_NC_DESCRIPTION);

   if (_tcscmp(m_driverName, driverName) || strcmp(m_configuration, config))
   {
      NCDriverDescriptor *dd = s_driverList.get(driverName);
      NCDriver *driver = nullptr;
      const NCConfigurationTemplate *confTemplate = nullptr;
      if (dd != nullptr)
      {
         Config cfg(false);
         if (cfg.loadConfigFromMemory(config, (int)strlen(config), driverName, nullptr, true, false))
         {
            driver = dd->instanceFactory(&cfg, m_storageManager);
            if (driver != nullptr)
            {
               confTemplate = dd->confTemplate;
            }
            else
            {
               nxlog_debug_tag(DEBUG_TAG, 1, _T("Cannot parse driver %s configuration: %hs"), m_name, config);
            }
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 1, _T("Cannot parse driver %s configuration: %hs"), m_name, config);
         }
      }
      MutexLock(m_driverLock);
      delete m_driver;
      m_driver = driver;
      MutexUnlock(m_driverLock);
      m_confTemplate = confTemplate;
      _tcslcpy(m_driverName, driverName, MAX_OBJECT_NAME);
      MemFree(m_configuration);
      m_configuration = MemCopyStringA(config);
   }
   saveToDatabase();
}

/**
 * Save channel configuration to database
 */
void NotificationChannel::saveToDatabase()
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt;
   if (IsDatabaseRecordExist(hdb, _T("notification_channels"), _T("name"), m_name))
   {
      hStmt = DBPrepare(hdb,
               _T("UPDATE notification_channels SET driver_name=?,description=?,configuration=? WHERE name=?"));
   }
   else
   {
      hStmt = DBPrepare(hdb,
               _T("INSERT INTO notification_channels (driver_name,description,configuration,name) VALUES (?,?,?,?)"));
   }
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, m_driverName, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_description, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_TEXT, DB_CTYPE_UTF8_STRING, m_configuration, DB_BIND_STATIC);
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Save notification log to database
 */
void NotificationChannel::writeNotificationLog(const TCHAR *recipient, const TCHAR *subject, const TCHAR *body, bool success)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt;

   hStmt = DBPrepare(hdb,
               _T("INSERT INTO notification_log (id,notification_channel,notification_timestamp,recipient,subject,message,success) VALUES (?,?,?,?,?,?,?)"));

   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_INTEGER, s_notificationId++);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, m_name, DB_BIND_STATIC);
      DBBind(hStmt, 3, DB_SQLTYPE_INTEGER, (uint32_t)time(nullptr));
      DBBind(hStmt, 4, DB_SQLTYPE_VARCHAR, recipient, DB_BIND_STATIC);
      DBBind(hStmt, 5, DB_SQLTYPE_VARCHAR, subject, DB_BIND_STATIC);
      DBBind(hStmt, 6, DB_SQLTYPE_VARCHAR, body, DB_BIND_STATIC);
      DBBind(hStmt, 7, DB_SQLTYPE_INTEGER, success ? 1 : 0);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   DBConnectionPoolReleaseConnection(hdb);

   nxlog_debug_tag(DEBUG_TAG, 5, _T("NotificationLog: id %u, channel %s, timestamp %u, recipient %s, subject %s, message %s, success %s"),
    s_notificationId - 1, m_name, time(nullptr), recipient, subject, body, success ? _T("True") : _T("False"));
}

/**
 * Delete method called in separate thread
 */
static void DeleteNotificationChannelInternal(TCHAR *name)
{
   s_channelListLock.lock();
   NotificationChannel *nc = s_channelList.unlink(name);
   s_channelListLock.unlock();

   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_STATEMENT hStmt = DBPrepare(hdb, _T("DELETE FROM notification_channels WHERE name=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, nc->getName(), DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   hStmt = DBPrepare(hdb, _T("DELETE FROM nc_persistent_storage WHERE channel_name=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, nc->getName(), DB_BIND_STATIC);
      DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }
   DBConnectionPoolReleaseConnection(hdb);

   delete nc;
   MemFree(name);
}

/**
 * Delete notification channel
 */
bool DeleteNotificationChannel(const TCHAR *name)
{
   s_channelListLock.lock();
   bool contains = s_channelList.contains(name);
   s_channelListLock.unlock();
   if (contains)
   {
      ThreadPoolExecuteSerialized(g_mainThreadPool, NC_THREAD_KEY, DeleteNotificationChannelInternal, MemCopyString(name));
   }
   return contains;
}

/**
 * Check if notification channel with given name exists
 */
bool IsNotificationChannelExists(const TCHAR *name)
{
   s_channelListLock.lock();
   bool exist = s_channelList.contains(name);
   s_channelListLock.unlock();
   return exist;
}

/**
 * Create new notification channel
 */
static NotificationChannel *CreateNotificationChannel(const TCHAR *name, const TCHAR *description, const TCHAR *driverName, char *configuration)
{
   NCDriverDescriptor *dd = s_driverList.get(driverName);
   NCDriver *driver = nullptr;
   NCDriverServerStorageManager *storageManager = new NCDriverServerStorageManager(name);
   const NCConfigurationTemplate *confTemplate = nullptr;
   StringBuffer errorMessage;
   if (dd != nullptr)
   {
      Config config(false);
      if (config.loadConfigFromMemory(configuration, strlen(configuration), driverName, NULL, true, false))
      {
         driver = dd->instanceFactory(&config, storageManager);
         if (driver != nullptr)
         {
            confTemplate = dd->confTemplate;
         }
         else
         {
            errorMessage.append(_T("Unable to create instance of driver "));
            errorMessage.append(driverName);
            nxlog_debug_tag(DEBUG_TAG, 1, _T("Unable to create instance of driver %s"), driverName);
            nxlog_debug_tag(DEBUG_TAG, 1, _T("   Configuration: %hs"), configuration);
         }
      }
      else
      {
         errorMessage.append(_T("Cannot parse configuration for channel "));
         errorMessage.append(name);
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Cannot parse configuration for channel %s"), name);
         nxlog_debug_tag(DEBUG_TAG, 1, _T("   Configuration: %hs"), configuration);
      }
   }
   else
   {
      errorMessage.append(_T("Cannot find driver "));
      errorMessage.append(driverName);
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Cannot find driver %s"), driverName);
   }

   return new NotificationChannel(driver, storageManager, name, description, driverName, configuration, confTemplate, errorMessage);
}

/**
 * Create notification channel and save
 */
void CreateNotificationChannelAndSave(const TCHAR *name, const TCHAR *description, const TCHAR *driverName, char *configuration)
{
   NotificationChannel *nc = CreateNotificationChannel(name, description, driverName, configuration);
   s_channelListLock.lock();
   s_channelList.set(name, nc);
   nc->saveToDatabase();
   s_channelListLock.unlock();
}

/**
 * Update new notification channel
 */
void UpdateNotificationChannel(const TCHAR *name, const TCHAR *description, const TCHAR *driverName, char *configuration)
{
   s_channelListLock.lock();
   NotificationChannel *nc = s_channelList.get(name);
   if(nc != nullptr)
      nc->update(description, driverName, configuration);
   s_channelListLock.unlock();
}

/**
 * Rename notification channel and notification name in action table
 * first - old name
 * second - new name
 */
static void RenameNotificationChannelInDB(std::pair<TCHAR *, TCHAR *> *names)
{
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   bool success = false;
   DBBegin(hdb);

   DB_STATEMENT hStmt = DBPrepare(hdb, _T("UPDATE notification_channels SET name=? WHERE name=?"));
   if (hStmt != nullptr)
   {
      DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, names->second, DB_BIND_STATIC);
      DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, names->first, DB_BIND_STATIC);
      success = DBExecute(hStmt);
      DBFreeStatement(hStmt);
   }

   if (success)
   {
      hStmt = DBPrepare(hdb, _T("UPDATE actions SET channel_name=? WHERE channel_name=? AND action_type=3"));
      if (hStmt != nullptr)
      {
         DBBind(hStmt, 1, DB_SQLTYPE_VARCHAR, names->second, DB_BIND_STATIC);
         DBBind(hStmt, 2, DB_SQLTYPE_VARCHAR, names->first, DB_BIND_STATIC);
         success = DBExecute(hStmt);
         DBFreeStatement(hStmt);
      }
      else
      {
         success = false;
      }
   }

   if (success)
   {
      DBCommit(hdb);
   }
   else
   {
      DBRollback(hdb);
   }
   DBConnectionPoolReleaseConnection(hdb);

   MemFree(names->first);
   MemFree(names->second);
   delete names;
}

/**
 * Rename notification channel
 */
void RenameNotificationChannel(TCHAR *name, TCHAR *newName)
{
   s_channelListLock.lock();
   NotificationChannel *nc = s_channelList.unlink(name);
   if (nc != nullptr)
   {
      nc->updateName(newName);
      s_channelList.set(newName, nc);
      auto pair = new std::pair<TCHAR *, TCHAR *>(name, newName);
      UpdateChannelNameInActions(pair);
      ThreadPoolExecuteSerialized(g_mainThreadPool, NC_THREAD_KEY, RenameNotificationChannelInDB, pair);
   }
   else
   {
      MemFree(name);
      MemFree(newName);
   }
   s_channelListLock.unlock();
}

/**
 * Get notification channel list
 */
void GetNotificationChannels(NXCPMessage *msg)
{
   s_channelListLock.lock();
   uint32_t base = VID_ELEMENT_LIST_BASE;
   Iterator<std::pair<const TCHAR*, NotificationChannel*>> *it = s_channelList.iterator();
   msg->setField(VID_CHANNEL_COUNT, s_channelList.size());
   while(it->hasNext())
   {
      NotificationChannel *nc = it->next()->second;
      nc->fillMessage(msg, base);
      base += 20;
   }
   delete it;
   s_channelListLock.unlock();
}

/**
 * Get notification driver list
 */
void GetNotificationDrivers(NXCPMessage *msg)
{
   uint32_t base = VID_ELEMENT_LIST_BASE;
   StringList *keyList = s_driverList.keys();
   for(int i = 0; i < keyList->size(); i++)
   {
      msg->setField(base, keyList->get(i));
      base++;
   }
   delete keyList;
   msg->setField(VID_DRIVER_COUNT, s_driverList.size());
}

/**
 * Get configuration of given communication channel. Returned string is dynamically allocated and should be freed by caller.
 */
char *GetNotificationChannelConfiguration(const TCHAR *name)
{
   s_channelListLock.lock();
   NotificationChannel *nc = s_channelList.get(name);
   char *configuration = (nc != nullptr) ? MemCopyStringA(nc->getConfiguration()) : nullptr;
   s_channelListLock.unlock();
   return configuration;
}

/**
 * Send notification
 */
void SendNotification(const TCHAR *name, TCHAR *recipient, const TCHAR *subject, const TCHAR *message)
{
   s_channelListLock.lock();
   NotificationChannel *nc = s_channelList.get(name);
   if (nc != nullptr)
   {
      if (recipient != nullptr)
      {
         TCHAR *curr = recipient, *next;
         do
         {
            next = _tcschr(curr, _T(';'));
            if (next != nullptr)
               *next = 0;
            Trim(curr);
            nc->send(curr, subject, message);
            curr = next + 1;
         } while(next != nullptr);
      }
      else
      {
         nc->send(recipient, subject, message);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification channel \"%s\" not found"), name);
   }
   s_channelListLock.unlock();
}

/**
 * Load notification channel driver
 *
 * @param file Driver's file name
 */
static void LoadDriver(const TCHAR *file)
{
   TCHAR errorText[256];

   HMODULE hModule = DLOpen(file, errorText);
   if (hModule != nullptr)
   {
      int *apiVersion = reinterpret_cast<int *>(DLGetSymbolAddr(hModule, "NcdAPIVersion", errorText));
      const char **name = reinterpret_cast<const char **>(DLGetSymbolAddr(hModule, "NcdName", errorText));
      NCDriver *(*InstanceFactory)(Config *, NCDriverStorageManager *) = (NCDriver *(*)(Config *, NCDriverStorageManager *))DLGetSymbolAddr(hModule, "NcdCreateInstance", errorText);
      NCConfigurationTemplate *(*GetConfigTemplate)() = (NCConfigurationTemplate *(*)())DLGetSymbolAddr(hModule, "NcdGetConfigurationTemplate", errorText);

      if ((apiVersion != nullptr) && (InstanceFactory != nullptr) && (name != nullptr) && (GetConfigTemplate != nullptr))
      {
         if (*apiVersion == NCDRV_API_VERSION)
         {
            NCDriverDescriptor *ncDriverDescriptor = new NCDriverDescriptor();
            ncDriverDescriptor->instanceFactory = InstanceFactory;
            ncDriverDescriptor->confTemplate = GetConfigTemplate();
#ifdef UNICODE
            TCHAR *tmp = WideStringFromMBString(*name);
            _tcslcpy(ncDriverDescriptor->name, tmp, MAX_OBJECT_NAME);
            MemFree(tmp);
#else
            _tcslcpy(ncDriverDescriptor->name, *name, MAX_OBJECT_NAME);
#endif
            s_driverList.set(ncDriverDescriptor->name, ncDriverDescriptor);
            nxlog_debug_tag(DEBUG_TAG, 4, _T("Notification channel driver %s loaded successfully"), ncDriverDescriptor->name);
         }
         else
         {
            nxlog_debug_tag(DEBUG_TAG, 1, _T("Notification channel driver \"%s\" cannot be loaded because of API version mismatch (driver: %d; server: %d)"),
                     file, *apiVersion, NCDRV_API_VERSION);
            DLClose(hModule);
         }
      }
      else
      {
         nxlog_debug_tag(DEBUG_TAG, 1, _T("Unable to find entry point in notification channel driver \"%s\""), file);
         DLClose(hModule);
      }
   }
   else
   {
      nxlog_debug_tag(DEBUG_TAG, 1, _T("Unable to load module \"%s\" (%s)"), file, errorText);
   }
}

/**
 * Load configuration channels on server start
 */
void LoadNotificationChannelDrivers()
{
   TCHAR path[MAX_PATH];
   _tcscpy(path, g_netxmsdLibDir);
   _tcscat(path, LDIR_NCD);

   nxlog_debug_tag(DEBUG_TAG, 1, _T("Loading notification channel drivers from %s"), path);
#ifdef _WIN32
   SetDllDirectory(path);
#endif
   _TDIR *dir = _topendir(path);
   if (dir != nullptr)
   {
      _tcscat(path, FS_PATH_SEPARATOR);
      int insPos = (int)_tcslen(path);

      struct _tdirent *f;
      while((f = _treaddir(dir)) != nullptr)
      {
         if (MatchString(_T("*.ncd"), f->d_name, false))
         {
            _tcscpy(&path[insPos], f->d_name);
            LoadDriver(path);
         }
      }
      _tclosedir(dir);
   }
#ifdef _WIN32
   SetDllDirectory(NULL);
#endif
   nxlog_debug_tag(DEBUG_TAG, 1, _T("%d notification channel drivers loaded"), s_driverList.size());
}

/**
 * Load notification channel configuration form database
 */
void LoadNotificationChannels()
{
   int numberOfAddedDrivers = 0;
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT result = DBSelect(hdb, _T("SELECT name,driver_name,description,configuration FROM notification_channels"));
   if (result != nullptr)
   {
      int numRows = DBGetNumRows(result);
      for (int i = 0; i < numRows; i++)
      {
         TCHAR name[MAX_OBJECT_NAME];
         TCHAR driverName[MAX_OBJECT_NAME];
         TCHAR description[MAX_NC_DESCRIPTION];
         DBGetField(result, i, 0, name, MAX_OBJECT_NAME);
         DBGetField(result, i, 1, driverName, MAX_OBJECT_NAME);
         DBGetField(result, i, 2, description, MAX_NC_DESCRIPTION);
         char *configuration = DBGetFieldA(result, i, 3, nullptr, 0);
         NotificationChannel *nc = CreateNotificationChannel(name, description, driverName, configuration);
         s_channelListLock.lock();
         s_channelList.set(name, nc);
         s_channelListLock.unlock();
         numberOfAddedDrivers++;
         nxlog_debug_tag(DEBUG_TAG, 4, _T("Notification channel %s successfully created"), name);
      }
      DBFreeResult(result);
   }
   nxlog_debug_tag(DEBUG_TAG, 1, _T("%d notification channels added"), numberOfAddedDrivers);
   DBConnectionPoolReleaseConnection(hdb);
}

/**
 * Shutdown all notification channels
 */
void ShutdownNotificationChannels()
{
   s_channelListLock.lock();
   s_channelList.clear();  // This will delete all channels and destructors will handle correct shutdown
   s_channelListLock.unlock();
}

/**
 * Notification logging initialisation
 */
void InitNotificationLogs()
{
   // Determine first available event id
   DB_HANDLE hdb = DBConnectionPoolAcquireConnection();
   DB_RESULT hResult = DBSelect(hdb, _T("SELECT max(id) FROM notification_log"));
   if (hResult != nullptr)
   {
      if (DBGetNumRows(hResult) > 0)
      {
         s_notificationId = DBGetFieldUInt64(hResult, 0, 0) + 1;
      }
      DBFreeResult(hResult);
   }
   DBConnectionPoolReleaseConnection(hdb);
}
