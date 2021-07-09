/**
 ** NetXMS - Network Management System
 ** Performance Data Storage Driver for InfluxDB
 ** Copyright (C) 2019 Sebastian YEPES FERNANDEZ & Julien DERIVIERE
 ** Copyright (C) 2021 Raden Solutions
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU Lesser General Public License as published by
 ** the Free Software Foundation; either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU Lesser General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **
 **              ,.-----__
 **          ,:::://///,:::-.
 **         /:''/////// ``:::`;/|/
 **        /'   ||||||     :://'`\
 **      .' ,   ||||||     `/(  e \
 **-===~__-'\__X_`````\_____/~`-._ `.
 **            ~~        ~~       `~-'
 ** Armadillo, The Metric Eater - https://www.nationalgeographic.com/animals/mammals/group/armadillos/
 **
 ** File: influxdb.h
 **/

#ifndef _influxdb_h_
#define _influxdb_h_

#include <nms_core.h>
#include <pdsdrv.h>
#include <nxqueue.h>

#if HAVE_LIBCURL
#include <curl/curl.h>
#endif

// debug pdsdrv.influxdb 1-8
#define DEBUG_TAG _T("pdsdrv.influxdb")

/**
 * Abstract sender
 */
class InfluxDBSender
{
protected:
   StringBuffer m_queue;
   uint32_t m_queueFlushThreshold;
   uint32_t m_queueSizeLimit;
   uint32_t m_maxCacheWaitTime;
   String m_hostname;
   uint16_t m_port;
   time_t m_lastConnect;
#ifdef _WIN32
   CRITICAL_SECTION m_mutex;
   CONDITION_VARIABLE m_condition;
#else
   pthread_mutex_t m_mutex;
   pthread_cond_t m_condition;
#endif
   bool m_shutdown;
   THREAD m_workerThread;

   void workerThread();

   virtual bool send(const char *data) = 0;

public:
   InfluxDBSender(const Config& config);
   virtual ~InfluxDBSender();

   void start();
   void stop();
   void enqueue(const TCHAR *data);
};

/**
 * UDP sender
 */
class UDPSender : public InfluxDBSender
{
protected:
   SOCKET m_socket;

   virtual bool send(const char *data) override;

   void createSocket();

public:
   UDPSender(const Config& config);
   virtual ~UDPSender();
};

#if HAVE_LIBCURL

/**
 * Generic API sender
 */
class APISender : public InfluxDBSender
{
private:
   CURL *m_curl;

protected:
   virtual bool send(const char *data) override;

   virtual void buildURL(char *url) = 0;
   virtual void addHeaders(curl_slist **headers);

public:
   APISender(const Config& config);
   virtual ~APISender();
};

/**
 * APIv1 sender
 */
class APIv1Sender : public APISender
{
protected:
   String m_db;
   String m_user;
   String m_password;

   virtual void buildURL(char *url) override;

public:
   APIv1Sender(const Config& config);
   virtual ~APIv1Sender();
};

/**
 * APIv2 sender
 */
class APIv2Sender : public APISender
{
protected:
   String m_organization;
   String m_bucket;
   char *m_token;

   virtual void buildURL(char *url) override;
   virtual void addHeaders(curl_slist **headers) override;

public:
   APIv2Sender(const Config& config);
   virtual ~APIv2Sender();
};

#endif

/**
 * Driver class definition
 */
class InfluxDBStorageDriver : public PerfDataStorageDriver
{
private:
   ObjectArray<InfluxDBSender> m_senders;
   bool m_enableUnsignedType;

public:
   InfluxDBStorageDriver();
   virtual ~InfluxDBStorageDriver();

   virtual const TCHAR *getName() override;
   virtual bool init(Config *config) override;
   virtual void shutdown() override;
   virtual bool saveDCItemValue(DCItem *dcObject, time_t timestamp, const TCHAR *value) override;
   virtual bool saveDCTableValue(DCTable *dcObject, time_t timestamp, Table *value) override;
};

#endif   /* _influxdb_h_ */
