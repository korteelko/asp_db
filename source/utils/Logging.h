/**
 * asp_therm - implementation of real gas equations of state
 *
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef UTILS__LOGGING_H
#define UTILS__LOGGING_H

#include "Common.h"
#include "ErrorWrap.h"

#include <fstream>
#include <set>
#include <sstream>


/**
 * \brief Дефолтное имя файла логов
 * */
#define DEFAULT_LOGFILE  "logs"
/**
 * \brief Максимальный размер основного файла логов
 * */
#define DEFAULT_MAXLEN_LOGFILE  128*1024  // 128 KiB
/**
 * \brief Частота скидывания буфферов логгера в выходы
 * */
#define DEFAULT_FLUSH_RATE 3 // every 3 sec


/**
 * \brief Структура инициализирующая конфигурацию логгера
 * */
struct logging_cfg {
  /**
   * \brief Собственное имя логгера
   * */
  const std::string logger;
  /**
   * \brief Уровень логирования
   * */
  io_loglvl loglvl = io_loglvl::err_logs;
  /**
   * \brief Путь к файлу
   * */
  std::string filepath = DEFAULT_LOGFILE;
  /**
   * \brief Максимальный размер файла(в байтах)
   * */
  size_t maxlen = DEFAULT_MAXLEN_LOGFILE;
  /**
   * \brief Частота скидывания логгов в выводы, в секундах
   * */
  uint16_t flush_rate_sec = DEFAULT_FLUSH_RATE;
  /**
   * \brief Флаг дублирования вывода в консоль
   * */
  bool cerr_duplicate = false;

public:
  /**
   * \brief Установки логирования
   * \param logger собственное имя логгера
   * \param ll уровень логирования
   * \param file файл для записи
   * \param maxlen Максимальный размер файла логирования, в байтах
   * \param flush_rate Частота обновления файла логгов, в секундах
   * \param duplicate дублировать логи в стандартный вывод ошибок
   * */
  logging_cfg(const std::string &logger, io_loglvl ll,
      const std::string &file, size_t maxlen, uint16_t flush_rate,
      bool duplicate);
  /**
   * \brief Установки логирования
   * \param logger собственное имя логгера
   * \param ll уровень логирования
   * \param file файл для записи
   * \param duplicate дублировать логи в стандартный вывод ошибок
   * */
  logging_cfg(const std::string &logger, io_loglvl ll,
      const std::string &file, bool duplicate);
  logging_cfg(const logging_cfg &lc) = default;
  /**
   * \brief Перегрузка конструктора присваивания, т.к. в структуре
   *   присутствуют неизменякмы члены.
   * \note Копируем всё кроме собственного имени логгера `logger`
   * */
  logging_cfg &operator= (const logging_cfg &lc);

  /**
   * \brief Конвертировать информацию о конфигурации логгера в строку
   * */
  std::string to_str() const;
};
/**
 * \brief Вывод данных структуры logging_cfg
 * */
std::ostream &operator<< (std::ostream &out, const logging_cfg &lc);


/**
 * \brief Класс системы управления логированием,
 *   обрабатывает глобальные сообщения.
 * \note Стандартная работа системы логирования проста -
 *   все реализованные функции добавления/обновления логов
 *   используя главный логгер пишут, информацию в основной файл.
 * \note Класс унифицирован для всех модулей ПО
 * */
class Logging {
  ADD_TEST_CLASS(LoggingProxy)

public:
  /**
   * \brief Инициализировать систему логирования структурой
   *   logging_cfg с параметрами по умолчанию:
   *   - logger = "main_logger"
   *   - filename = DEFAULT_LOGFILE
   *   - maxlen = DEFAULT_MAXLEN_LOGFILE
   * Отладка:
   *   - loglvl = io_loglvl::debug_logs
   *   - cerr_duplicate = true
   * Выпуск:
   *   - loglvl = io_loglvl::err_logs
   *   - cerr_duplicate = false
   * */
  static merror_t InitDefault();
  /**
   * \brief Освободить ресурсы, дропнуть логгеры
   * */
  ~Logging();
  /**
   * \brief Изменить параметры основного логгера
   * */
  static merror_t ResetInstance(const logging_cfg &li);
  /**
   * \brief Очистить файл логов
   * \todo Не нашёл функцию очистки файла в spdlog
   * */
  static void ClearLogFile();
  /**
   * \brief Скинуть буфферы выходов
   * */
  static void Flush();
  /**
   * \brief Код внутренней ошибки основного логгера
   * */
  static merror_t GetErrorCode();
  /**
   * \brief Уровень логирования
   * */
  static io_loglvl GetLogLevel();
#if defined (_DEBUG)
  /**
   * \brief Вывести строку в стандартный вывод ошибок
   * */
  static void PrintCerr(const std::string &info);
#endif  // _DEBUG
  /**
   * \brief Добавить сообщение `msg` к логу,
   *   если уровень логирования != io_loglvl::no_log
   * */
  static void Append(const std::string &msg);
  /**
   * \brief Добавить сообщение `msg` к логу,
   *   если уровень логирования соответствует переданному
   * */
  static void Append(io_loglvl lvl, const std::string &msg);
  /**
   * \brief Добавить информацию об ошибке и сообщение `msg` к логу
   * */
  static void Append(merror_t error_code, const std::string &msg);
  static void Append(io_loglvl lvl, merror_t error_code,
      const std::string &msg);
  static void Append(const std::stringstream &sstr);
  static void Append(io_loglvl lvl, const std::stringstream &sstr);
  // TODO: how about:
  // static void Append(ErrorWrap err, const std::string &msg); ???

private:
  /**
   * \brief check logfile exist, check length of file
   * */
  static merror_t checkInstance();
  /**
   * \brief check instance and set variables
   * */
  static merror_t initInstance(const logging_cfg *li);
  /**
   * \brief Собственно добавление сообщения
   *
   * \param ll Уровень логирования
   * \param msg Сообщение
   * */
  static void append(io_loglvl ll, const std::string &msg);
  /**
   * \brief Собственно добавление сообщения
   *
   * \param msg Сообщение
   * */
  static void append(const std::string &msg);
  /**
   * \brief reset logging configuration(filename, log level)
   * */
  static void set_cfg(const logging_cfg *li);

private:
  /**
   * \brief Конфигурация основного логгера
   * */
  static logging_cfg li_;
  /**
   * \brief Ошибка основного логгера
   * */
  static ErrorWrap error_;
  /**
   * \brief Мьютекс на изменение конфигурации логгера
   * */
  static Mutex logconfig_mutex_;
  /**
   * \brief Набор зарегистрированных логгеров
   * */
  static std::set<std::string> loggers_;
  /**
   * \brief Допустимость логирования
   * */
  static bool is_aval_;
};


/**
 * \brief Класс логирования
 * */
class PrivateLogging {
  ADD_TEST_CLASS(PrivateLoggingProxy)

public:
  /**
   * \brief Зарегистрировать логгер
   * \param cfg Конфигурация
   *
   * \return Результат регистрации
   *   true - зарегистрирован
   *   false - не зарегистрирован
   * */
  static bool Register(const logging_cfg &cfg);
  /**
   * \brief Соответствующий переданной конфигурации
   *   логгер уже заристрирован
   *
   * \return Результат проверки
   *   true - зарегистрирован
   *   false - не зарегистрирован
   * */
  // bool IsRegistered(const logging_cfg &cfg) const;
  /**
   * \brief Добавить информацию об ошибке и сообщение `msg` к логу
   * */
  void Append(const std::string &msg);

private:
  PrivateLogging(const logging_cfg &cfg);

private:
  /**
   * \brief Контейнер логгеров(ссылок на них)
   * */
  static std::set<std::string> loggers_;
  /**
   * \brief Мьютекс изменения контейнера логгеров
   * */
  static Mutex loggers_lock_;
};

#endif  // !UTILS__LOGGING_H
