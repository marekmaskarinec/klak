# klak

## introduction

Klak is a dynamically-typed, stack oriented, garbage collected programming language.
It is case insensitive and itentifiers are lowercased.
Everything is separated by 1 >= spaces.

## words

Words are bound to functions, or variables.
They can use any ASCII symbols except for the first character, which should be a letter.
They can't be the same as any of the keywords.

## literals

Literals automatically push themselves to the stack.

### numbers

Numbers are floats numbers.
You can write them as hexadecimal.

### characters

Characters represent one ascii character or a hex number.
They also support word representations (newline, space and tab).

Examples:
```
#a
#B
#space
#SPACE
##

'4b
```

### strings

Same as in other languages, strings literals are enclosed in ".

Example:
```
"hello world!"
```

## stack operators

### (n).

Pop n items from left of the stack.

### (n)\<word/literal (WIP)

Push value of the word to (nth place from) the left of the stack.
It is implicitly added.

```
5 2 1
```

Is same as

```
<5 <2 <1
```

### (n)>word/literal (WIP)

Same as <, but to the right.

## comments

There are two types of comments. 
Line comments: `;` and block comments, which are enclosed in `()`.
Block comments are usually only used in word definitions.

## data types

Data is stored in cells on the stack.
One cell is 8 bytes, but can be less on some systems.

### numbers

There are only floats.
They are 64-bit (`double` in c).

### arrays

Arrays are of fixed length (which can be determined at runtime).
They are passed by reference.

### strings

Strings are null terminated blocks of memory.
Same as arrays, they are passed by reference.

### cons

Cons is a pair of cells like in lisp.

### tables (WIP)

They can store any kind of value and use any kind of value as a key.
They are passed by reference.

### opaque (WIP)

Data, user doesn't know how works like c's `FILE`.
They are passed by reference.

## declaration

Declaration is done by typing `:` before the variable name.
No value is assigned, but the variable is initialized with null.

```
:a
```

## assignement

Assignement is done by writing `.` before the variable name.
It pops and assigns the last value.

```
:a 3 .a ; a will now hold 3
```

## scopes

Variables are scoped same as in c.
The stack is global.

## control flow

### if

Similar to other languages.
The code between `if` and `then` is evaluated on test and if the top of the stack is not 0, block is executed.

Example:
```
if  5 4 gt  then
	"bigger#newline" n format
else
	"smaller#newline" n format
fi
```

### loop

Loop evaluates everyting before `then` and if the stack-front is not 0, it executes the block.

```
0 .i
loop  10 i lt  then
	i "~a " 0 format
	i 1 + .i
pool
"#newline" 0 format
```
Will print numbers from 0 to 9.

### switch

Code after `case` is evaluated and the top of the stack is saved.
Then code after `on` is evaluated and top is compared with the value saved before.
If none of the ons pass, else is executed.

```
case  5  then
	on 1 then
		"first case" 1 format
	no
	on 2 then
		"second case" 1 format
	no
	else
		"fallback case" 1 format
esac
```

### iter (WIP)

Iter like in 4l. You declare a variable and then push a variable.
Optionally, you can provide second declaration.
The first one is always index, the second one is value.

```
:my-array
10 mka .my-array

iter :i :v my-array then
	i v "~a: ~a" 1 format
reti
```

### functions

Functions are declared with the `mkw` (make word) keyword. It takes a name.
Function can't be used before declaration.

Example:
```
mkw square {
	dup *
}
```

Recursion is allowed.

#### prototypes

Prototypes are done by leaving out the body.

```
mkw foo
```

## preprocessor

Klak features a preprocessor, not dissimilar to c's one.
All preprocessor commands are prefixed with '@' and are located on the beginning of a line.

### @def

Defines a macro. It can, but doesn't have to have a value.

```
@def nip swap .
@def DEBUG
@def DEFAULT_LEN 256
```

### @udf

If a macro is defined, undefines it.

```
@def HELLO

@idf HELLO
	; I get compiled
@fid

@udf HELLO

@idf HELLO
	; I don't
@fid
```

### @idf

Skips everything before `@fid`, if argument is defined.

### @ind

Opposite of `@idf`

### @inc

Includes a file.

```
@inc "std.kh"
```

## standard words

## arithmetics

```
+  add
-  subtract
*  multiply
/  divide
%  modulo
```

## comparation

```
=   equals
>   bigger
<   smaller
<=  smaller or equal
>=  bigger or equal
!=  not equal
```

## IO

```
put     simplest way to print ( value -- )
format  like in common lisp ( args... format-string stream -- optional-output )
s>      prints the stack
open    open a file ( path mode -- FILE )
read    read all from file ( FILE -- string )
readl   read line from file ( FILE -- string )
readb   read byte from file ( FILE -- byte )
```

## bitwise operators

```
b&   bitwise and
b|   bitwise or
b<<  shift left
b>>  shift right
b~   xor
!    t => n, n => t
```

## functions

```
abs    absolute function
sin    sine
cos    cosine
tg     tangens
round  round
trunc  round to zero
```

## stack operation

```
dup   duplicates the value on the front
swap  swaps the front with the second element
rot   rotates front 3 items
tuck  duplicate the top item below the second slot
over  duplicate the second item to the front 
```

## arrays

```
mka   make array with size of front value ( length -- array )
get   get nth element of an array ( array index -- array element )
set   set nth element of an array ( array index values -- array )
len   get length ( array -- array length )
atol  converts an array to a list ( array -- list )
find  returns a list of indexes, where value can be found ( array value -- array list-of-values )
```

## strings

```
get, set and len are usable with strings
contains  returns t if string contains a substring ( string substring -- contains )
split     splits a string into an array of strings ( string separator -- list )
```

## lists

```
contains, get, set, len, find and stack operators, but prefixed with l
ltoa  converts a list to an array ( list -- array )
car   returns the lists car ( cons -- car )
cdr   returns the lists cdr ( cons -- cdr )
```

## tables

```
get, set, and contains is usable with tables
mkt    makes a table ( -- table )
ttol   returns a list of `key value cons` ( table -- list )
```

## memory

```
cpy   copies value ( val -- val1 val2 )
rcpy  recursively copies value ( val -- val1 val2 )
```
