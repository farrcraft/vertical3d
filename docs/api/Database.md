# Databases

`api/database` (`v3dlib_database`, namespace `v3d::database`) is a thin layer over SQLite. It is
for documents that are saved incrementally, such as the editor's scene documents. No other
library or app includes SQLite: it is linked PRIVATE here, and no header of this library names
it.

## Connections

`Database` is one connection to one database.

- `open(path, mode)` opens it. `Mode::ReadOnly` and `Mode::ReadWrite` need a database to be
  there. `Mode::Create` makes an empty one when there is none. The path is UTF-8, and
  `":memory:"` opens a database that lasts only as long as the connection.
- Every connection turns foreign keys on, which SQLite otherwise leaves off.
- `execute(sql)` runs SQL that returns no rows. It may hold several statements separated by
  semicolons, and stops at the first that fails.
- `prepare(sql)` compiles the first statement of the text into a `Statement`, or returns an
  invalid one.
- `applicationId()` and `userVersion()` read and write the two numbers in a database's header.
  The application id says which program's format the database holds, and the user version is
  that format's version. A new database has zero in both.
- A connection is not shared between threads. Closing it, or destroying it, closes the database
  once every statement it made is gone.

A call that fails returns false, or an invalid statement, and `error()` says why. The message
stays until the next call that fails.

## Statements

`Statement` is one compiled statement.

- **Binding.** `bindInteger`, `bindReal`, `bindText`, `bindBlob` and `bindNull` set a parameter.
  Parameters are numbered from 1. A value is copied as it is bound. An empty blob binds as an
  empty blob, not as a null.
- **Running.** `step()` returns `Step::Row` while there is a row to read, `Step::Done` when the
  statement has finished, and `Step::Failed` with the reason in the statement's `error()`.
  `reset()` makes it ready to run again, and keeps the values bound.
- **Reading.** `integer`, `real`, `text`, `blob` and `isNull` read a column of the current row.
  Columns are numbered from 0. A text or blob read stays valid until the next step, reset or
  read of the same column. A value of another type is converted as SQLite converts it, so a table
  that must hold only its declared types is made `STRICT`.
- A statement can be moved but not copied, and must not outlive its connection. An invalid
  statement refuses every call and reads as empty.

## Binding and reading a row in order

`Binder` and `Columns` save counting parameters and columns by hand.

- **`Binder(&statement)`** resets the statement, then binds one parameter per call, from the
  first: `integer`, `real`, `text`, `blob` and `null`. `integer(std::optional)` binds a null
  for an empty value, and `reals({...})` binds one parameter for each value. The calls chain.
- **A failed bind is remembered.** The binds after it are skipped, `bound()` is false, and
  `run()` does not run the statement.
- **`run()`** steps the statement and is true only when every bind succeeded and the statement
  finished. It is for a statement that returns no rows.
- **`Columns(statement)`** reads the current row's columns in order, from the first:
  `integer`, `real`, `text` and `blob`. It holds the statement, so it is made for one row.

```cpp
database::Binder(&insert).integer(id).text(name).reals({ x, y, z }).integer(parent).run();

database::Columns row(select);
const std::int64_t id = row.integer();
const std::string name = row.text();
```

## Transactions

`Transaction` is a write transaction that rolls back unless it is committed.

- It begins with `BEGIN IMMEDIATE`, so the database's write lock is taken at the start. A second
  writer is refused there, rather than part way through its writes, and `active()` is false.
- `commit()` writes everything since the beginning. A commit that fails, such as one that a
  deferred constraint refuses, rolls back, and the database's `error()` says why.
- A transaction that is destroyed without a commit rolls back.

Background: [ADR-0101](../adr/0101-editor-a-scene-document-is-a-sqlite-database.md)
