# Scripting

`v3dlib_script` runs TypeScript. A host binds the functions a script may call, and runs
sources in one shared scope. The editor's console and scripts are built on it, as
[editor/Scripting.md](../editor/Scripting.md) describes.

## The runtime

`v3d::script::Runtime` owns one QuickJS-ng runtime and one context, and is used from one
thread.

- **`run(source, name)`** strips the source's types, as below, and runs the JavaScript left as
  a script in the global scope. Every run shares that scope, so a `let` or a function one run
  declares, the next can use. The `name` is what a problem calls the source.
- **The result.** `Result::value` is the value of the last statement run, as a person reads
  it. A string is in quotes, an object or an array is JSON, a function or a number is as the
  engine writes it, and `undefined` is nothing. `Result::problem` says why the run stopped, with
  the line and column in the source when they are known.
- **What a script can reach.** The language's built-ins, and the functions the host bound.
  There are no files, no network, no modules and no timers.
- **`bind(path, function)`** makes a `v3d::script::Function` callable by a path of names
  joined by dots, such as `timeline.set`. The objects along the path are made when they do not
  exist. A path that is empty, holds an empty name, or passes through something that is not an
  object binds nothing.
- **A call's arguments and value are JSON.** Each argument arrives as a `boost::json::value`,
  as `JSON.stringify` writes it. One JSON cannot hold, such as a function or `undefined`,
  arrives as null. The value the function returns is the call's value in the script, and a
  null returned is `undefined`, so a function with nothing to return gives nothing.
- **A refusal is an error in the script.** A function throws `v3d::script::Refused` to refuse
  a call, and the script sees an `Error` with its message, which it can catch. Any other
  exception arrives as an `InternalError`.
- **A problem names its place.** An error a script throws or the engine finds reading it is
  reported as its name and message, as `ReferenceError: missing is not defined`, at the first
  place in the source its stack names. A value thrown that is not an error is reported as
  `uncaught` and the value.

**Limits.** A run is stopped when it passes its time limit, a second unless `limit()` sets
another, and when it asks for more than 256 MB of memory or 256 kB of stack. The runtime
runs again after a run is stopped. The stack limit is measured from where each run begins.

## Stripping types

A script is TypeScript limited to syntax that can be erased. `v3d::script::strip()` takes the types
out and leaves JavaScript to run. The types are never checked.

- **What is erased.**
  - Annotations on variables, parameters, returns and fields.
  - The `?` of an optional parameter or member, and the `!` of a definite assignment.
  - Type parameters and type arguments.
  - `as`, `satisfies` and the non-null `!`.
  - Interfaces, type aliases, `declare` declarations, overloads and `implements` clauses.
  - The modifiers `public`, `private`, `protected`, `readonly` and `override`, abstract members
    and index signatures.
  - Type-only imports and exports.
- **Positions are kept.** Each run erased becomes spaces, and its line breaks stay, so an error
  the engine reports names the line and column of the TypeScript.
- **Two runs carry a character.** A return type that runs onto another line takes the `)` before
  it to its end, since `=>` may not follow a line break. An erased modifier that opens a member
  with a computed name or a generator's `*` leaves a `;`, so a field before it that has none
  does not run on into it.
- **What is refused.** An `enum`, a `namespace` or `module` that is not `declare`d, a parameter
  property, a type assertion written `<T>value`, `import =` and `export =`. Each needs code
  written for it, so none can be erased. A refusal names the line and column where the syntax
  begins.
- **What does not read.** A source that does not read as TypeScript is refused where reading
  stopped. A comment, string or template left open is refused at the place it opens, and a
  source nested more than 256 levels deep where it passes that depth.

The output is what Node's `module.stripTypeScriptTypes()` makes of the same source in its strip
mode, byte for byte, which the strip suite's cases are taken from.

**How it reads.** `api/script/internal/` holds a scanner and a parser that reads statements and
expressions just far enough to find where each type begins and ends, and keeps nothing else.
Where the source can be read two ways, it tries one and goes back for the other, as TypeScript
does. A parenthesis may open an arrow function's parameters or an expression, and a `<` may open
type arguments or be a comparison. Type arguments are taken only when what follows them could, as
TypeScript decides. A place where an arrow function did not read is remembered, so a nesting
read twice is not read again at every level.

Background: [ADR-0153](../adr/0153-editor-scripts-are-typescript-run-on-quickjs.md)
