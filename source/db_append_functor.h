/**
 * asp_therm - implementation of real gas equations of state
 *
 * Copyright (c) 2020 Mishutinski Yurii
 *
 * This library is distributed under the MIT License.
 * See LICENSE file in the project root for full license information.
 */
#ifndef _DATABASE__DB_APPEND_FUNCTOR_H_
#define _DATABASE__DB_APPEND_FUNCTOR_H_

#include <functional>
#include <string>
#include <vector>

/**
 * \brief Шаблонный функтор на прокидывание аргумента
 * */
template <class T = void>
struct pass {
  T operator()(const T& t) const { return t; }
};
/**
 * \brief Шаблонный функтор на преобразование целочисленного значения
 *   к строковому виду
 * */
template <class T,
          typename =
              typename std::enable_if<std::is_arithmetic<T>::value, T>::type>
struct integer2str {
  std::string operator()(T t) const { return std::to_string(t); }
};

/**
 * \brief Шаблонный функтор заполнения контейнера данных
 * */
template <class T>
struct vector_wrapper {
  std::vector<T>& v;
  vector_wrapper(std::vector<T>& _v) : v(_v) {}
  void operator()(const T& t) const { v.push_back(t); }
};

/**
 * \brief Шаблонная функция
 * */
template <class T, template <class> class ContT = vector_wrapper>
void append_push(ContT<T>& src, const T& s) {
  src(s);
}
/**
 * \brief Функтор добавления элементов в контейнер
 * \tparam T Тип значений контейнера
 * \tparam ContT<T> Тип контейнера
 * \tparam AppendF Функция добавления данных к контейнеру
 * \tparam U Функция(функтор) преобразования данных к типу
 *   хранимому контейнером, см. использование
 * */
template <class T,
          template <class> class ContT = vector_wrapper,
          class AppendF = std::function<void(ContT<T>&, const T&)>,
          class U = pass<std::string>>
struct AppendOp {
  /// Ссылка на контейнер
  ContT<T>& src;
  /// Функтор добавления данных в контейнер
  AppendF ap_f;
  /// Оператор добавления данных в контейнер
  U to_type;
  /**
   * \brief Инициализация функтора
   * \param _src Контейнер заполняемых данных
   * \param _ap_f Функция добавления в контейнер
   * \param _to_type Функция преобразования к типу контейнера
   *
   * \warning Аргумент по умолчанию для шаблонного типа AppendF
   *   не отрабатывает, приходится явно прокидывать
   * */
  AppendOp(ContT<T>& _src, AppendF _ap_f = append_push<T>, U _to_type = U())
      : src(_src), ap_f(_ap_f), to_type(_to_type) {}

  /**
   * \brief Конвертировать параметр s к типу T и добавить
   *   результат к вектору `src`
   * \param s Параметр для добавления к контейнеру
   * */
  template <class S>
  void operator()(const S& s) {
    ap_f(src, to_type(s));
  }
};

#endif  // !_DATABASE__DB_APPEND_FUNCTOR_H_
