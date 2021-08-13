# klak

Klak is a stack oriented language with dynamic typing that compiles to c.
You can start by reading the [spec](https://github.com/marekmaskarinec/klak/blob/master/spec.md),
see the [todo list](https://github.com/marekmaskarinec/klak/blob/master/todo.md) for current progress.

## running klak programs

Get a copy of [umka](https://github.com/vtereshkov/umka-lang).
In the repo root, run `umka src/main.um`. This will translate `test.kk` to c.
You can paste that to `static/std.c` and compile using `cc static/std.c`.
