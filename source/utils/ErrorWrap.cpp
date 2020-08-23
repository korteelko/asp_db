/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#include "ErrorWrap.h"

#include "Logging.h"


ErrorWrap::ErrorWrap()
  : ErrorWrap(ERROR_SUCCESS_T, "") {}

ErrorWrap::ErrorWrap(merror_t error)
  : ErrorWrap(error, "") {}

ErrorWrap::ErrorWrap(merror_t error, const std::string &msg)
  : error_(error), msg_(msg), is_logged_(false) {}

merror_t ErrorWrap::SetError(merror_t error, const std::string &msg) {
  if (error_ && !is_logged_)
    LogIt();
  const std::lock_guard<Mutex> up_lock(update_mutex_);
  msg_ = msg;
  is_logged_ = false;
  return error_ = error;
}

void ErrorWrap::SetErrorMessage(const std::string &msg) {
  if (error_)
    msg_ = msg;
}

void ErrorWrap::LogIt(io_loglvl lvl) {
  const std::lock_guard<Mutex> up_lock(update_mutex_);
  if (error_ != ERROR_SUCCESS_T && !is_logged_) {
    if (!msg_.empty()) {
      Logging::Append(lvl, "Error occurred.\n  err_msg: " + msg_ +
          "\n  code: " + hex2str(error_));
    }
    is_logged_ = true;
  }
}

void ErrorWrap::LogIt() {
  LogIt(Logging::GetLogLevel());
}

merror_t ErrorWrap::GetErrorCode() const {
  return error_;
}

std::string ErrorWrap::GetMessage() const {
  return msg_;
}

bool ErrorWrap::IsLogged() const {
  return is_logged_;
}

void ErrorWrap::Reset() {
  const std::lock_guard<Mutex> lock(update_mutex_);
  error_ = ERROR_SUCCESS_T;
  msg_ = "";
  is_logged_ = false;
}
