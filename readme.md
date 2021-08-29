# klak

Klak is a stack oriented language with dynamic typing that compiles to c.
You can start by reading the [spec](https://github.com/marekmaskarinec/klak/blob/master/spec.md),
see the [todo list](https://github.com/marekmaskarinec/klak/blob/master/todo.md) for current progress.

## running klak programs

The interface for running klak programs is still wip.
Get a copy of [umka](https://github.com/vtereshkov/umka-lang).
In the repo root, run `umka src/main.um`. This will translate `test.kk` to c.
You can paste that to `static/std.c` and compile using `cc static/std.c`.

## examples

### hello world

```
"hello world" put
```

### recursive fibonacci

```
:fib ( n -- n' )
	if  dup 2 <  then
		ret
	fi

	dup  1 - fib
	swap 2 - fib
	+ ;

fib 10 put
```

Will print: `55`

### iterative fibonacci

```
$i 2 .i

loop  i 10 <  then
	tuck +

	i 1 + .i
pool

+ put
```

Will print: `55`

### arrays

```
2 mka ? creates an array with length 2
len put ? => 2

set 0 3 ? sets the first element from null to 3
get 0 put ? => 3

null 1 2 3 4 stoa ? converts all elements on stack until null to an array
+ ? + can append arrays
put ? => [ 3 null 1 2 3 4 ]
```
