/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Cohors Concurro library.

  This library is free software: you can redistribute
  it and/or modify it under the terms of the GNU Lesser General
  Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License
  for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/
#pragma once
//#pragma warning(disable: 4250 4251)

#include <log4cxx/logger.h>
#include <log4cxx/propertyconfigurator.h>
#include <string>

class Logging
{
private:
   // true if Init() was called
   // otherwise false
   static bool m_bConfigured;

   static log4cxx::LoggerPtr m_RootLogger;
   static log4cxx::LoggerPtr m_ThreadLogger;
   static log4cxx::LoggerPtr m_TrackLogger;
   static log4cxx::LoggerPtr m_ConcurrencyLogger;

   // Pointer to logger
   log4cxx::LoggerPtr m_pLogger;
   // Name for class logger
   std::string m_sName;
   // true if class used as AutoLogger
   bool m_bFunctionLogger;
public:
   // Constructor for class logger
   Logging(const char* szName);
   // Constructor for function logger
   Logging(const char* szName, const Logging& ParentLogger);
   virtual ~Logging();

   inline log4cxx::LoggerPtr GetLogger()
   {
      if (!m_bConfigured)
         Init();

      if (m_pLogger == NULL)
      {
         m_pLogger = log4cxx::Logger::getLogger(m_sName.c_str());
      }
      return m_pLogger;
   }
   inline std::string GetName() const { return m_sName; }

   // Reads settings for logger
   static void Init();

   static inline log4cxx::LoggerPtr Root() { return m_RootLogger ; };
   static inline log4cxx::LoggerPtr Thread() { return m_ThreadLogger ; };
   static inline log4cxx::LoggerPtr Concurrency() { return m_ConcurrencyLogger ; };
};

inline log4cxx::LoggerPtr GetLogger(const char * subname, const Logging& parent) {
	return log4cxx::Logger::getLogger(parent.GetName() + "." + subname);
}

inline log4cxx::LoggerPtr GetLogger(const char * subname, const log4cxx::LoggerPtr& parent) {
	std::string s;
	parent->getName(s);
	return log4cxx::Logger::getLogger(s + "." + subname);
}

// Define a custom log macros for put streams into log
#if !defined(LOG4CXX_THRESHOLD) || LOG4CXX_THRESHOLD <= 10000 
#define LOG4STRM_DEBUG(logger, stream_expr) { \
        if (LOG4CXX_UNLIKELY((logger)->isDebugEnabled())) {\
           ::log4cxx::helpers::MessageBuffer oss_; \
           { stream_expr ; } \
           (logger)->forcedLog(::log4cxx::Level::getDebug(), oss_.str(oss_), LOG4CXX_LOCATION); }}
#else
#define LOG4STRM_DEBUG(logger, message)
#endif

#if !defined(LOG4CXX_THRESHOLD) || LOG4CXX_THRESHOLD <= 5000 
#define LOG4STRM_TRACE(logger, stream_expr) { \
        if (LOG4CXX_UNLIKELY((logger)->isTraceEnabled())) {\
           ::log4cxx::helpers::MessageBuffer oss_; \
           { stream_expr ; } \
           (logger)->forcedLog(::log4cxx::Level::getTrace(), oss_.str(oss_), LOG4CXX_LOCATION); }}
#else
#define LOG4STRM_TRACE(logger, message)
#endif

#if !defined(LOG4CXX_THRESHOLD) || LOG4CXX_THRESHOLD <= 20000 
#define LOG4STRM_INFO(logger, stream_expr) { \
        if (LOG4CXX_UNLIKELY((logger)->isInfoEnabled())) {\
           ::log4cxx::helpers::MessageBuffer oss_; \
           { stream_expr ; } \
           (logger)->forcedLog(::log4cxx::Level::getInfo(), oss_.str(oss_), LOG4CXX_LOCATION); }}
#else
#define LOG4STRM_INFO(logger, message)
#endif

#if !defined(LOG4CXX_THRESHOLD) || LOG4CXX_THRESHOLD <= 30000 
#define LOG4STRM_WARN(logger, stream_expr) { \
        if (LOG4CXX_UNLIKELY((logger)->isWarnEnabled())) {\
           ::log4cxx::helpers::MessageBuffer oss_; \
           { stream_expr ; } \
           (logger)->forcedLog(::log4cxx::Level::getWarn(), oss_.str(oss_), LOG4CXX_LOCATION); }}
#else
#define LOG4STRM_WARN(logger, message)
#endif

#if !defined(LOG4CXX_THRESHOLD) || LOG4CXX_THRESHOLD <= 40000 
#define LOG4STRM_ERROR(logger, stream_expr) { \
        if (LOG4CXX_UNLIKELY((logger)->isErrorEnabled())) {\
           ::log4cxx::helpers::MessageBuffer oss_; \
           { stream_expr ; } \
           (logger)->forcedLog(::log4cxx::Level::getError(), oss_.str(oss_), LOG4CXX_LOCATION); }}
#else
#define LOG4STRM_ERROR(logger, message)
#endif

#if !defined(LOG4CXX_THRESHOLD) || LOG4CXX_THRESHOLD <= 50000 
#define LOG4STRM_FATAL(logger, stream_expr) { \
        if (LOG4CXX_UNLIKELY((logger)->isFatalEnabled())) {\
           ::log4cxx::helpers::MessageBuffer oss_; \
           { stream_expr ; } \
           (logger)->forcedLog(::log4cxx::Level::getFatal(), oss_.str(oss_), LOG4CXX_LOCATION); }}
#else
#define LOG4STRM_FATAL(logger, message)
#endif
