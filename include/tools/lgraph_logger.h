/**
 * Copyright 2022 AntGroup CO., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 */

#pragma once

#include <stdexcept>
#include <string>
#include <iostream>
#include "fma-common/logger.h"
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/log/common.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/phoenix/bind/bind_function.hpp>
#include <boost/core/null_deleter.hpp>

namespace lgraph_api {

// Define log macro
#define GENERAL_LOG(LEVEL) BOOST_LOG_SEV(::lgraph_api::general_logger::get(), \
::lgraph_api::severity_level::LEVEL)

#define GENERAL_LOG_STREAM(LEVEL, CLASS) BOOST_LOG_SEV(::lgraph_api::general_logger::get(), \
  ::lgraph_api::severity_level::LEVEL) \
  << ::lgraph_api::logging::add_value("Class", CLASS)

#define DEBUG_LOG(LEVEL) BOOST_LOG_SEV(::lgraph_api::debug_logger::get(), \
  ::lgraph_api::severity_level::LEVEL) \
  << ::lgraph_api::logging::add_value("Line", __LINE__) \
  << ::lgraph_api::logging::add_value("File", __FILE__)       \
  << ::lgraph_api::logging::add_value("Function", __FUNCTION__) \

#define DEBUG_LOG_STREAM(LEVEL, CLASS) BOOST_LOG_SEV(::lgraph_api::debug_logger::get(), \
  ::lgraph_api::severity_level::LEVEL) \
  << ::lgraph_api::logging::add_value("Line", __LINE__) \
  << ::lgraph_api::logging::add_value("File", __FILE__)       \
  << ::lgraph_api::logging::add_value("Function", __FUNCTION__) \
  << ::lgraph_api::logging::add_value("Class", CLASS)

#define EXIT_ON_FATAL(SIGNAL) ::fma_common::_detail::PrintBacktraceAndExit(SIGNAL)

namespace logging = boost::log;
namespace attrs = boost::log::attributes;
namespace src = boost::log::sources;
namespace sinks = boost::log::sinks;
namespace expr = boost::log::expressions;
namespace keywords = boost::log::keywords;

using boost::shared_ptr;

typedef sinks::synchronous_sink< sinks::text_file_backend > file_sink;
typedef sinks::synchronous_sink< sinks::text_ostream_backend > console_sink;

enum severity_level {
    TRACE,
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    FATAL
};

class LoggerManager {
 private:
    std::string log_dir_;
    std::string rotation_target_dir_;
    std::string history_general_log_dir_;
    std::string history_debug_log_dir_;
    severity_level level_;
    int rotation_size_;
    boost::shared_ptr< file_sink > general_sink_;
    boost::shared_ptr< file_sink > debug_sink_;
    boost::shared_ptr< console_sink > console_sink_;
    bool global_inited_ = false;

 public:
    /**
     * @brief   Init LoggerManager. Set log directory, log filtering level.
     *
     * @param   log_dir   The log directory.
     * @param   level     The log filtering level.
     */
    void Init(std::string log_dir = "logs/", severity_level level = severity_level::INFO);

    /**
     * @brief   Set the log filtering level.
     *
     * @param   level     The log filtering level.
     */
    void SetLevel(severity_level level);

    /**
     * @brief   Get current log filtering level.
     *
     * @returns   current log filtering level.
     */
    severity_level GetLevel();

    bool IsInited();

    /**
     * @brief   Get a instance of LoggerManager class.
     *
     * @returns   a instance of LoggerManager class.
     */
    static LoggerManager& GetInstance();
};


// Global logger declaration
BOOST_LOG_GLOBAL_LOGGER(general_logger, src::severity_logger_mt< severity_level >)
BOOST_LOG_GLOBAL_LOGGER(debug_logger, src::severity_logger_mt< severity_level >)

BOOST_LOG_ATTRIBUTE_KEYWORD(log_type_attr, "LogType", std::string)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity_attr, "Severity", severity_level)

}  // namespace lgraph_api
