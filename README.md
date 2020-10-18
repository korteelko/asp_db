[![Build Status](https://travis-ci.org/korteelko/asp_db.svg?branch=master)](https://travis-ci.org/github/korteelko/asp_db) 

# asp\_db

Модуль взаимосвязи с БД проекта [asp_therm](https://github.com/korteelko/asp_therm). По ссылке можно найти информацию о конфигурировании подключения БД через xml файл, настройке БД и соответствующие сырцы, здесь их нет.   
 Обобщёный функционал - вывод ошибок, логирования, *чтения xml/json файлов конфигурации(почему-то нет, ридеры в основном проекте до сих пор болтаются)* вынесены в отдельную библиотеку - [asp_utils](https://github.com/korteelko/asp_utils).  
Интерфейс, который API, реализован только для postgres. Примеры его использования есть в директории `examples`.

