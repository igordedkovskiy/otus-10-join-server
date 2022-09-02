# otus homework #10 - join-server

# cpp_sqlite

```bash
./cpp_sqlite
INSERT A 0 lean
INSERT A 1 sweater
INSERT A 2 frank
.
.
.
exit
```

# join-server

## Protocol

| COMMAND | SQL analogue | Meaning |
| :--- | :--- | :--- |
| INSERT [table_name] [key_value] [name_value] | ``` INSERT INTO [table_name] VALUES([key_value], [name_value]); ``` |
| TRUNCATE [table_name] | ``` DELETE FROM [table_name]; ``` | clean table |
| INTERSECTION | ``` SELECT A.id, A.name, B.name FROM A INNER JOIN B WHERE A.id = B.id ORDER BY A.id ASC; ``` | get intersection of tables |
| SYMMETRIC_DIFFERENCE | ``` SELECT A.id, A.name FROM A WHERE A.id NOT IN (SELECT B.id FROM B) UNION ALL SELECT B.id, B.name FROM B WHERE B.id NOT IN (SELECT A.id FROM A) ORDER BY id ASC; ``` | get symmetric difference of tables |
| PRINT | ``` SELECT * FROM A ORDER BY id ASC; ``` | print table A |
| LIST |  ``` SELECT name FROM sqlite_master WHERE type='table'; ``` | print list of tables |

### Example

| COMMAND | SQL |
| :--- | :--- |
| ``` INSERT A 1 lean ``` | ``` INSERT INTO A VALUES(1, 'lean'); ``` |
| ``` TRUNCATE A ``` | ``` DELETE FROM A; ``` |

## Commands

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

### Symmetric difference (ordered)

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

