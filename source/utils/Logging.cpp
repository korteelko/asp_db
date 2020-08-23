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

#include <ctime>
#include <iostream>
#ifdef CXX17
#  include <filesystem>
#endif  // USE_FILESYSTEM

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#define DEFAULT_LOGFILE  "logs"
#define OLD_LOGFILE_SFX  "_prev"

logging_cfg::logging_cfg(io_loglvl ll, const std::string &file, bool duplicate)
  : loglvl(ll) ,cerr_duplicate(duplicate) {
  strncpy(filepath, file.c_str(), sizeof(filepath));
}

mlog_fostream Logging::output_;
logging_cfg Logging::li_ = {
#ifdef _DEBUG
  debug_logs,
  DEFAULT_LOGFILE,
  true
#else
  err_logs,
  DEFAULT_LOGFILE,
  false
#endif  // _DEBUG
};
ErrorWrap Logging::error_;
Mutex Logging::logfile_mutex_;
bool Logging::is_aval_ = false;

merror_t Logging::checkInstance() {
  // don't try call models_errors.h function
  //   in this function)
  merror_t error = ERROR_FILEIO_T;
  if (*Logging::li_.filepath != '\0') {
    if (Logging::output_.is_open()) {
  #if defined(NDEBUG)
      assert(0 && "logging module error");
  #else
      // todo: set error and error_msg
      //   push error message to stdio
      Logging::error_.SetError(ERROR_GENERAL_T, "error in programer DNA");
      Logging::output_.close();
  #endif
    }
    Logging::output_.open(Logging::li_.filepath,
        std::fstream::out | std::fstream::app);
    // if file open check size, if size > MAXSIZE_LOGFILE
    //   move logging file to 'logging file_old'
    if (Logging::output_.is_open()) {
      Logging::output_.seekp(0, Logging::output_.end);
      auto logfile_size = Logging::output_.tellp();
      // Logging::output_.seekp(0, Logging::output_.beg);
      Logging::output_.close();
      if (logfile_size > MAXSIZE_LOGFILE) {
        std::string logfile_copy(Logging::li_.filepath);
        logfile_copy += OLD_LOGFILE_SFX;
      #ifdef USE_FILESYSTEM
        std::filesystem::rename(
            Logging::li_.filepath, logfile_copy);
      #else
        rename(li_.filepath, logfile_copy.c_str());
      #endif  // USE_FILESYSTEM
      }
      error = ERROR_SUCCESS_T;
    } else {
      error = ERROR_FILE_OUT_ST;
      Logging::error_.SetError(error, "cannot open logging file");
    }
  }
  return error;
}

merror_t Logging::initInstance(const logging_cfg *li) {
  std::lock_guard<Mutex> init_lock(Logging::logfile_mutex_);
  set_cfg(li);
  merror_t error = checkInstance();
  if (error) {
    Logging::is_aval_ = false;
  } else {
    Logging::is_aval_ = true;
    // print start sequence
    Logging::output_.open(Logging::li_.filepath,
        std::fstream::out | std::fstream::app);
    if (Logging::output_.is_open()) {
      std::time_t tm = std::time(nullptr);
      Logging::output_ << "\n\n"
          << "==========================================================\n"
          << "Logging by " << std::asctime(std::localtime(&tm)) << "\n"
          << "==========================================================";
      Logging::output_ << "\n";
      Logging::output_.close();
    } else {
      Logging::is_aval_ = false;
      Logging::error_.SetError(ERROR_PAIR_DEFAULT(ERROR_FILE_LOGGING_ST));
    }
  }
  if (error)
    error_.SetError(error, "Open loggingfile end with error.\n");
  return error;
}

void Logging::append(const char *msg) {
  std::lock_guard<Mutex> init_lock(Logging::logfile_mutex_);
  Logging::output_.open(Logging::li_.filepath,
      std::fstream::out | std::fstream::app);
  if (Logging::output_.is_open()) {
    if (msg) {
      Logging::output_ << msg;
      if (Logging::li_.cerr_duplicate)
        std::cerr << msg << std::endl;
      if (!strrchr(msg, '\n'))
        Logging::output_.put('\n');
    }
    Logging::output_.close();
  } else {
    Logging::is_aval_ = false;
    Logging::error_.SetError(ERROR_PAIR_DEFAULT(ERROR_FILE_LOGGING_ST));
  }
}

void Logging::set_cfg(const logging_cfg *li) {
  if (li) {
    Logging::li_.loglvl = li->loglvl;
    memset(Logging::li_.filepath, 0, sizeof(Logging::li_.filepath));
    strncpy(Logging::li_.filepath, li->filepath, sizeof(Logging::li_.filepath));
  }
}

merror_t Logging::InitDefault() {
  return initInstance(nullptr);
}

merror_t Logging::ResetInstance(const logging_cfg &li) {
  return initInstance(&li);
}

merror_t Logging::GetErrorCode() {
  return Logging::error_.GetErrorCode();
}

io_loglvl Logging::GetLogLevel() {
  return Logging::li_.loglvl;
}

void Logging::ClearLogfile() {
  std::lock_guard<Mutex> lock(logfile_mutex_);
  Logging::output_.open(Logging::li_.filepath, std::fstream::out);
  if (Logging::output_.is_open()) {
    Logging::output_.close();
  } else {
    Logging::is_aval_ = false;
    Logging::error_.SetError(ERROR_PAIR_DEFAULT(ERROR_FILE_LOGGING_ST));
  }
  return;
}

#if defined (_DEBUG)
void Logging::PrintCerr(const std::string &info) {
  std::cerr << info << std::endl;
}
#endif  // _DEBUG

void Logging::Append(const std::string &msg) {
  if (Logging::is_aval_)
    if (!msg.empty() && (Logging::li_.loglvl != io_loglvl::no_log))
      Logging::append(msg.c_str());
}

void Logging::Append(io_loglvl lvl, const std::string &msg) {
  if (Logging::is_aval_)
    if (lvl <= Logging::li_.loglvl)
      if (!msg.empty() && (Logging::li_.loglvl != io_loglvl::no_log))
        Logging::append(msg.c_str());
}

void Logging::Append(merror_t error_code, const std::string &msg) {
  Logging::Append(io_loglvl::err_logs, error_code, msg);
}

void Logging::Append(io_loglvl lvl, merror_t error_code,
    const std::string &msg) {
  if (Logging::is_aval_)
    if (lvl <= Logging::li_.loglvl)
      Logging::Append(io_loglvl::err_logs, " Error occurred.\n  err_msg: " +
          msg + "\n  code: " + hex2str(error_code));
}

void Logging::Append(const std::stringstream &sstr) {
  Logging::Append(sstr.str());
}

void Logging::Append(io_loglvl lvl, const std::stringstream &sstr) {
  if (Logging::is_aval_)
    if (lvl <= Logging::li_.loglvl)
      Logging::Append(lvl, sstr.str());
}
