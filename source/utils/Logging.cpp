/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "Logging.h"

/*
 * headers of spdlog library
 * spdlog repository:
 *   https://github.com/gabime/spdlog
 * License:
 *   MIT
 */
#include "spdlog/async.h"
#include "spdlog/common.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <ctime>
#include <iostream>
#include <filesystem>

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


/* псевдонимы для пространтв имён spdlog */
namespace lsinks = spdlog::sinks;
namespace llevel = spdlog::level;

logging_cfg::logging_cfg(const std::string &logger, io_loglvl ll,
    const std::string &file, size_t maxlen, uint16_t flush_rate,
    bool duplicate)
  : logger(logger), loglvl(ll), filepath(file), maxlen(maxlen),
    flush_rate_sec(flush_rate), cerr_duplicate(duplicate) {}

logging_cfg::logging_cfg(const std::string &logger, io_loglvl ll,
    const std::string &file, bool duplicate)
  : logging_cfg(logger, ll, file, DEFAULT_MAXLEN_LOGFILE,
    DEFAULT_FLUSH_RATE, duplicate) {}

logging_cfg &logging_cfg::operator=(const logging_cfg &lc) {
  if (this != &lc) {
    loglvl = lc.loglvl;
    filepath = lc.filepath;
    maxlen = lc.maxlen;
    cerr_duplicate = lc.cerr_duplicate;
  }
  return *this;
}

std::string logging_cfg::to_str() const {
  return "[" + logger + "]\n"
      "\tfile:" + filepath + "\n"
      "\tloglevel - " + io_loglvl_str(loglvl) + "\n"
  ;
}

std::ostream &operator<<(std::ostream &out, const logging_cfg &lc) {
  out << lc.to_str();
  return out;
}

logging_cfg Logging::li_ (
  "main_logger",
#ifdef _DEBUG
  io_loglvl::debug_logs,
  DEFAULT_LOGFILE,
  DEFAULT_MAXLEN_LOGFILE,
  DEFAULT_FLUSH_RATE,
  true
#else
  io_loglvl::err_logs,
  DEFAULT_LOGFILE,
  DEFAULT_MAXLEN_LOGFILE,
  DEFAULT_FLUSH_RATE,
  false
#endif  // _DEBUG
);
ErrorWrap Logging::error_;
Mutex Logging::logconfig_mutex_;
bool Logging::is_aval_ = false;

std::set<std::string> PrivateLogging::loggers_;
Mutex PrivateLogging::loggers_lock_;

/**
 * \brief Установить соответствие между уровнями
 *   логировния asp_db и spdlog
 * \todo Рассмотреть уникальный случай за switch
 * */
static llevel::level_enum set_loglevel(io_loglvl ll) {
  switch (ll) {
    case io_loglvl::debug_logs:
      return llevel::debug;
    case io_loglvl::info_logs:
      return llevel::info;
    case io_loglvl::warn_logs:
      return llevel::warn;
    case io_loglvl::err_logs:
      return llevel::err;
    case io_loglvl::no_log:
      return llevel::off;
  }
  return llevel::err;
}

/* Logging */
merror_t Logging::initInstance(const logging_cfg *li) {
  std::lock_guard<Mutex> init_lock(Logging::logconfig_mutex_);
  set_cfg(li);
  merror_t error = ERROR_SUCCESS_T;
  try {
    // отвязаться от всех зарегистрированного логгера
    if (spdlog::get(Logging::li_.logger).get())
      spdlog::drop(Logging::li_.logger);
    spdlog::init_thread_pool(8192, 1);
    std::shared_ptr<spdlog::async_logger> logger = nullptr;
    if (Logging::li_.cerr_duplicate) {
      // если необходимо дублировать вывод на консоль,
      //   регистрируем логгер с несколькими выходами
      // консольный вывод
      auto stdout_sink = std::make_shared<lsinks::stdout_color_sink_mt>();
      // stdout_sink->set_level(set_loglevel(Logging::li_.loglvl));
      // файловый ротируемый выход
      auto rotating_sink = std::make_shared<lsinks::rotating_file_sink_mt>(
          Logging::li_.filepath, Logging::li_.maxlen, 3);
      // rotating_sink->set_level(set_loglevel(Logging::li_.loglvl));
      std::vector<spdlog::sink_ptr> sinks {stdout_sink, rotating_sink};
      // связываем выводы
      logger = std::make_shared<spdlog::async_logger>(
          Logging::li_.logger, sinks.begin(), sinks.end(),
          spdlog::thread_pool(), spdlog::async_overflow_policy::block);
    } else {
      // если в консоль дублировать не нужно, используем обычный
      //   ротируемый асинхронный логгер
      auto rotating_sink = std::make_shared<lsinks::rotating_file_sink_mt>(
          Logging::li_.filepath, Logging::li_.maxlen, 3);
      logger = std::make_shared<spdlog::async_logger>(
          Logging::li_.logger, rotating_sink, spdlog::thread_pool(),
          spdlog::async_overflow_policy::block);
    }
    spdlog::register_logger(logger);
    spdlog::set_default_logger(logger);
    std::time_t tm = std::time(nullptr);
    logger->info("============================================================");
    logger->info("Logging by " + std::string(std::asctime(std::localtime(&tm))));
    logger->info("============================================================");
    logger->set_level(set_loglevel(Logging::li_.loglvl));
    spdlog::flush_every(std::chrono::seconds(Logging::li_.flush_rate_sec));
    Logging::is_aval_ = true;
  } catch (std::bad_alloc &e) {
    // ошибка bad_alloc фатальна
    std::cerr << "FATAL ERROR: BAD_ALLOCATION. "
        "LOGGING INSTANCE.\n\tError message:\n" << e.what();
    error = error_.SetError(ERROR_PAIR_DEFAULT(ERROR_FILE_LOGGING_ST));
  } catch (spdlog::spdlog_ex &e) {
    std::cerr << "FATAL ERROR: SPDLOG LIBRARY ERROR. "
        "LOGGING INSTANCE.\n\tError message:\n" << e.what();
    error = error_.SetError(ERROR_FILE_LOGGING_ST, "spdlog exception");
  } catch (std::exception &e) {
    std::cerr << "FATAL ERROR: UNDEFINED ERROR. "
        "LOGGING INSTANCE.\n\tError message:\n" << e.what();
    error = error_.SetError(ERROR_GENERAL_T, "");
  }
  if (error) {
    error_.SetError(error, "Open loggingfile ends with error.\n");
    std::cerr << "Main logger configuration:\n" << Logging::li_;
  }
  return error;
}

void Logging::append(io_loglvl ll, const std::string &msg) {
  if (Logging::is_aval_ && !msg.empty())
    spdlog::log(set_loglevel(ll), msg);
}

void Logging::append(const std::string &msg) {
  append(Logging::li_.loglvl, msg);
}

void Logging::set_cfg(const logging_cfg *li) {
  if (li)
    Logging::li_ = *li;
}

merror_t Logging::InitDefault() {
  return initInstance(nullptr);
}

Logging::~Logging() {
  spdlog::shutdown();
}

merror_t Logging::ResetInstance(const logging_cfg &li) {
  return initInstance(&li);
}

void Logging::ClearLogFile() {
  auto main_logger = spdlog::get(Logging::li_.logger);
  if (main_logger.get()) {
    // сбросить кэш
    main_logger->flush();
    // открыть файл на запись и закрыть
    std::fstream f(Logging::li_.filepath, std::ios_base::out);
    if (f.is_open()) {
      f.close();
    } else {
      Logging::is_aval_ = false;
      Logging::error_.SetError(ERROR_PAIR_DEFAULT(ERROR_FILE_LOGGING_ST));
    }
  }
}

void Logging::Flush() {
  auto main_logger = spdlog::get(Logging::li_.logger);
  if (main_logger.get())
    main_logger->flush();
}

merror_t Logging::GetErrorCode() {
  return Logging::error_.GetErrorCode();
}

io_loglvl Logging::GetLogLevel() {
  return Logging::li_.loglvl;
}

#if defined (_DEBUG)
void Logging::PrintCerr(const std::string &info) {
  std::cerr << info << std::endl;
}
#endif  // _DEBUG

void Logging::Append(const std::string &msg) {
  Logging::append(msg);
}

void Logging::Append(io_loglvl lvl, const std::string &msg) {
  Logging::append(lvl, msg);
}

void Logging::Append(merror_t error_code, const std::string &msg) {
  Logging::Append(Logging::li_.loglvl, error_code, msg);
}

void Logging::Append(io_loglvl lvl, merror_t error_code,
    const std::string &msg) {
  Logging::Append(lvl, "Error occurred.\n\tErr_msg: " +
      msg + "\n\tCode: " + hex2str(error_code));
}

void Logging::Append(const std::stringstream &sstr) {
  Logging::Append(sstr.str());
}

void Logging::Append(io_loglvl lvl, const std::stringstream &sstr) {
  Logging::Append(lvl, sstr.str());
}


/* PrivateLogging */
bool PrivateLogging::Register(const logging_cfg &cfg) {
  assert(0);
}

void PrivateLogging::Append(const std::string &msg) {

}

PrivateLogging::PrivateLogging(const logging_cfg &cfg) {

}
