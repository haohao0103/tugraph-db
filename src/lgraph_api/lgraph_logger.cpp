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

#include "tools/lgraph_logger.h"

namespace lgraph_api {

BOOST_LOG_GLOBAL_LOGGER_INIT(general_logger, src::severity_logger_mt< severity_level >) {
    src::severity_logger_mt< severity_level > lg;
    attrs::constant< std::string > general_type("general");
    lg.add_attribute("LogType", general_type);
    return lg;
}

BOOST_LOG_GLOBAL_LOGGER_INIT(debug_logger, src::severity_logger_mt< severity_level >) {
    src::severity_logger_mt< severity_level > lg;
    attrs::constant< std::string > debug_type("debug");
    lg.add_attribute("LogType", debug_type);

    // Init empty console log first if not inited
    if (!LoggerManager::GetInstance().IsInited()) {
        boost::shared_ptr< console_sink > empty_sink =
            boost::shared_ptr< console_sink > (new console_sink());
        empty_sink->locked_backend()->add_stream(
            boost::shared_ptr< std::ostream >(&std::clog, boost::null_deleter()));
        empty_sink->locked_backend()->auto_flush(true);
        logging::core::get()->add_sink(empty_sink);
    }

    return lg;
}

severity_level get_level(logging::value_ref< severity_level > const& level) {
    return level.get();
}

std::string level_to_string(logging::value_ref< severity_level > const& level) {
    switch (level.get()) {
    case TRACE:
        return "TRACE";
    case DEBUG:
        return "DEBUG";
    case INFO:
        return "INFO";
    case WARNING:
        return "WARNING";
    case ERROR:
        return "ERROR";
    case FATAL:
        return "FATAL";
    default:
        return "Invalid severity level";
    }
}

void general_formatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    strm << "[" << logging::extract< boost::posix_time::ptime >("TimeStamp", rec) << "] ";
    strm << "[" << level_to_string(logging::extract< severity_level >("Severity", rec)) << "] ";
    logging::value_ref< std::string > class_name = logging::extract< std::string >("Class", rec);
    if (class_name) strm << "[" << class_name << "] ";
    strm << "- "<< rec[expr::smessage];
}

void debug_formatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    strm << "[" << logging::extract< boost::posix_time::ptime >("TimeStamp", rec) << "] ";
    if (LoggerManager::GetInstance().GetLevel() <= severity_level::DEBUG) {
        strm << "[" << logging::extract< attrs::current_thread_id::value_type >("ThreadID", rec)
        << "] ";
    }
    strm << "[" << level_to_string(logging::extract< severity_level >("Severity", rec)) << "] ";
    logging::value_ref< std::string > class_name = logging::extract< std::string >("Class", rec);
    if (class_name) strm << "[" << class_name << "] ";
    if (LoggerManager::GetInstance().GetLevel() <= severity_level::DEBUG) {
        strm << "[" << logging::extract<std::string>("File", rec) << " : "
        << logging::extract<std::string>("Function", rec) << " : "
        << logging::extract<int>("Line", rec)
        << "] ";
    }
    strm << "- "<< rec[expr::smessage];
}

void console_formatter(logging::record_view const& rec, logging::formatting_ostream& strm) {
    logging::value_ref< std::string > log_type = logging::extract< std::string >("LogType", rec);
    if (log_type.get() == "debug") {
        debug_formatter(rec, strm);
    } else if (log_type.get() == "general") {
        general_formatter(rec, strm);
    }
}

void LoggerManager::Init(std::string log_dir, severity_level level) {
    // Set up log directory
    log_dir_ = log_dir;
    level_ = level;
    rotation_size_ = 5 * 1024 * 1024;
    rotation_target_dir_ = log_dir_ + "history_logs/";
    history_general_log_dir_ = rotation_target_dir_ + "general_logs/";
    history_debug_log_dir_ = rotation_target_dir_ + "debug_logs/";

    // Set up sink for general log
    general_sink_ = boost::shared_ptr< file_sink > (new file_sink(
        keywords::file_name = log_dir_ + "general.log",
        keywords::open_mode = std::ios_base::out | std::ios_base::app,
        keywords::enable_final_rotation = false,
        keywords::auto_flush = true,
        keywords::rotation_size = rotation_size_));
    general_sink_->locked_backend()->set_file_collector(sinks::file::make_collector(
        keywords::target = history_general_log_dir_));
    general_sink_->locked_backend()->scan_for_files();
    general_sink_->set_filter(log_type_attr == "general");
    general_sink_->set_formatter(&general_formatter);

    // Set up sink for debug log
    debug_sink_ = boost::shared_ptr< file_sink > (new file_sink(
        keywords::file_name = log_dir_ + "debug.log",
        keywords::open_mode = std::ios_base::out | std::ios_base::app,
        keywords::enable_final_rotation = false,
        keywords::auto_flush = true,
        keywords::rotation_size = rotation_size_));
    debug_sink_->locked_backend()->set_file_collector(sinks::file::make_collector(
        keywords::target = history_debug_log_dir_));
    debug_sink_->locked_backend()->scan_for_files();
    debug_sink_->set_filter(expr::attr< severity_level >("Severity") >= level_ &&
        log_type_attr == "debug");
    debug_sink_->set_formatter(&debug_formatter);

    // Set up sink for console log
    console_sink_ = boost::shared_ptr< console_sink > (new console_sink());
    console_sink_->locked_backend()->add_stream(
        boost::shared_ptr< std::ostream >(&std::clog, boost::null_deleter()));
    console_sink_->locked_backend()->auto_flush(true);
    console_sink_->set_filter(expr::attr< severity_level >("Severity") >= level_);
    console_sink_->set_formatter(&console_formatter);

    // Add sinks to log core
    logging::core::get()->remove_all_sinks();
    if (log_dir_ == "") {
        logging::core::get()->add_sink(console_sink_);
    } else {
        logging::core::get()->add_sink(general_sink_);
        logging::core::get()->add_sink(debug_sink_);
    }

    // Add some attributes too
    if (!global_inited_) {
        logging::core::get()->add_global_attribute("TimeStamp", attrs::local_clock());
        logging::core::get()->add_global_attribute("ThreadID", attrs::current_thread_id());
    }

    global_inited_ = true;
}

void LoggerManager::SetLevel(severity_level level) {
    level_ = level;
    general_sink_->set_filter(expr::attr< severity_level >("Severity") >= level_ &&
        log_type_attr == "general");
    debug_sink_->set_filter(expr::attr< severity_level >("Severity") >= level_ &&
        log_type_attr == "debug");
    console_sink_->set_filter(expr::attr< severity_level >("Severity") >= level_);
}

severity_level LoggerManager::GetLevel() {
    return level_;
}

bool LoggerManager::IsInited() {
    return global_inited_;
}

LoggerManager& LoggerManager::GetInstance() {
    static LoggerManager instance;
    return instance;
}

}  // namespace lgraph_api
