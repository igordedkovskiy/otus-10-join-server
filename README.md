# cpp_sqlite

Пример сборки проекта:
```bash
mkdir build&&cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

См. файлы `sqlite3.c` и `sqlite3.h`, которые будут распакованы в build-директорию `sqlite3-src` с помощью cmake-пакета FetchContent.

Пример использования:

```bash
./cpp_sqlite
# Usage: ./cpp_sqlite <SQL>

ls *.sqlite
# Нет такого файла или каталога
./cpp_sqlite "SELECT * FROM t;"
# cpp_sqlite_db.sqlite database opened successfully!
# Can't execute query: no such table: t

ls --size *.sqlite
# 0 cpp_sqlite_db.sqlite
```
Первый запуск неуспешный, потому файл с БД не будет создан. Второй запуск создаёт БД, но ввиду отсутствия таблицы t в БД, завершается с ошибкой.

Создаём таблицу, добавляем в неё данные и выводим её содержимое.

```bash
./cpp_sqlite "CREATE TABLE t (a int, b text);"
# cpp_sqlite_db.sqlite database opened successfully!
ls --size *.sqlite
# 8 cpp_sqlite_db.sqlite

./cpp_sqlite "INSERT INTO t VALUES(100, 'Hello');"
# cpp_sqlite_db.sqlite database opened successfully!
./cpp_sqlite "INSERT INTO t VALUES(200, 'World\!');"
# cpp_sqlite_db.sqlite database opened successfully!
ls --size *.sqlite
# 8 cpp_sqlite_db.sqlite

./cpp_sqlite "SELECT * FROM t;"
# cpp_sqlite_db.sqlite database opened successfully!
# a = 100
# b = Hello
#
# a = 200
# b = World\!
```

Можно воспользоваться viewer-ом для SQLite, я предпочитаю пользоваться DBeaver. Подобных программ много, можно пользоваться теми, которые нравятся больше... но DBeawer всеядный и им удобно пользоваться при работе с различными СУБД.

```bash
wget https://dbeaver.io/files/dbeaver-ce_latest_amd64.deb
sudo dpkg -i dbeaver-ce_latest_amd64.deb
dbeaver
```

### Insert

```bash
INSERT A 0 lean
INSERT A 1 sweater
INSERT A 2 frank
INSERT A 3 violation
INSERT A 4 quality
INSERT A 5 precision

INSERT B 3 proposal
INSERT B 4 example
INSERT B 5 lake
INSERT B 6 flour
INSERT B 7 wonder
INSERT B 8 selection
```

### Intersection (ordered)

```bash
SELECT A.id, A.name, B.name FROM A
INNER JOIN B
WHERE A.id = B.id
ORDER BY A.id ASC;
```

### Symmetric difference

```bash
SELECT * FROM A
WHERE A.id NOT IN (SELECT B.id FROM B)
UNION ALL
SELECT * FROM B
WHERE B.id NOT IN (SELECT A.id FROM A)
ORDER BY id ASC;

SELECT A.id, A.name FROM A
WHERE A.id NOT IN (SELECT B.id FROM B)
UNION ALL
SELECT B.id, B.name FROM B
WHERE B.id NOT IN (SELECT A.id FROM A)
ORDER BY id ASC;
```


### List of tables names

```bash
SELECT name
FROM sqlite_master
WHERE type='table';
```


## Protocol

| COMMAND | SQL analogue | Meaning |
| :--- | :--- | :--- |
| INSERT [table_name] [key_value] [name_value] | ``` INSERT INTO [table_name] VALUES([key_value], [name_value]); ``` |
| TRUNCATE [table_name] | ``` DELETE FROM [table_name]; ``` | clean table |
| INTERSECTION | ``` SELECT A.id, A.name, B.name FROM A INNER JOIN B WHERE A.id = B.id ORDER BY A.id ASC; ``` | get intersection of tables |
| SYMMETRIC_DIFFERENCE | ``` SELECT A.id, A.name FROM A WHERE A.id NOT IN (SELECT B.id FROM B) UNION ALL SELECT B.id, B.name FROM B WHERE B.id NOT IN (SELECT A.id FROM A) ORDER BY id ASC; ``` | get symmetric difference of tables |

### Example

| COMMAND | SQL |
| :--- | :--- |
| ``` INSERT A 1 lean ``` | ``` INSERT INTO A VALUES(1, 'lean'); ``` |
| ``` TRUNCATE A ``` | ``` DELETE FROM A; ``` |
