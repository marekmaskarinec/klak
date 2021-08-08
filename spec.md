# klak

## introduction

Klak is a dynamically-typed, stack oriented language with manual memory management.
There is a stack (called the stack), values can be pushed to, poped from, or used as arguments to functions.
It is case insensitive and itentifiers are lowercased.
Everything is separated by 1 >= spaces.

## words

Words are bound to functions, or variables.
They can use any ASCII symbols except for the first character, which has to be a letter, or `:`.
The last character can't be any of the stack operators.
They can't be the same as any of the keywords.

## literals

Literals have value without being evaluated.

### numbers

Numbers are ints or real (decimal point) numbers.
Ints can be written as binary, decimal or hexadecimal.

### characters

Characters represent one ascii character or a hex number.
They support c's escape sequences and some other word representations.

Examples:
```
#a
#B
#\t
#space
#SPACE
##

'4b
```

### strings

Same as in other languages, strings literals are enclosed in ".
They can contain ascii characters, but also character literals embedded into them.
\# can be written by writing it two times like this `##`.

Examples:
```
"hello world!"

"hello#newlinenewline"
```

## stack operators

Some operators (not only the stack ones) can be prefixed with a number. If none is included, 1 is assumed.

### (n).

Pop the n items from left of the stack.

### (n)\<word/literal

Push value of the word to (nth place from) the left of the stack.
It is implicitly added.

```
5 2 1
```

Is same as

```
<5 <2 <1
```

### (n)>word/literal

Same as <, but to the right.

## comments

Comments start with `#`.
Block comments are done by adding a number after `#`.
That number marks, how many lines are commented.
Blocks **aren't** nested.

## data types

Data is stored in cells on the stack.
One cell is 8 bytes, but can be less on some systems.

### numbers

There are integers, decimals and fractions.
Ints are 64-bit (`long long int`), decimals are 64-bit too (`double`) and fractions are made of two 32-bit ints.

### arrays

Arrays are of fixed length (which can be determined at runtime).
They are passed by reference.

### strings

Strings are null terminated blocks of memory.
Same as arrays, they are passed by reference.

### lists

Linked lists.
They have similar operators to the stack, but prefixed with `l`.
They are passed by reference.

### tables

They can store any kind of value and use any kind of value as a key.
They are passed by reference.

### opaque

Data, user doesn't know how works like c's `FILE`.
They are passed by reference.

## declaration

Declaration is done by typing `:` before the variable name.
It doesn't assign any value.

```
:a
```

## assignement

Assignement is done by writing `.` before the variable name.
It pops and assigns the last value.

```
3 .a
```

## scopes

The stack is scope by making a copy of itself on enter, however you can propagate is using `^^`.
In functions, only first n elements of stack are copied.
Variables are scoped same as in c.

## built-in constants

```
t n
```

Zero is `n` and `n` is zero.
Every non-`n` value is `t`.

## control flow

### if

Similar to other languages.
The code between `if` and `then` is evaluated on test and if the top of the stack is `t`, block is executed.

Example:
```
if  5 4 gt  then
	"bigger#newline" n format
else then
	"smaller#newline" n format
fi
```

### loop

Loop evaluates everyting before `then` and if the stack-front is `t`, it executes the block.

```
0 .i
loop  10 i lt  then
	i "~a " n format
pool
"#newline" n format
```
Will print numbers from 0 to 9.

### functions

Functions are declared with the `mkw` (make word) keyword. It takes a name.

Example:
```
mkw add {
	+
}
```

Recursion is allowed.

#### prototypes

Prototypes are done by leaving out the body.

```
mkw foo
```

## multiple files

Files are included similarly to c.
There are source files and headers.
The implementation includes all headers only once and in the beggining.

Including is done using the `include` word.

```
"file.kh" include
"file2.kh" "file3.kh" include
```

## standard words

```
## arithmetics

+ # add
- # subtract
* # multiply
/ # divide
% # modulo


## comparation

=  # equals
>  # bigger
<  # smaller
<= # smaller or equal
>= # bigger or equal
!= # not equal


## bitwise operators

b&  # bitwise and
b|  # bitwise or
b<< # shift left
b>> # shift right
b~  # xor
!   # t => n, n => t


## functions

abs   # absolute function
neg   # -99 => 99
sin   # sine
cos   # cosine
tg    # tangens
round # round
trunc # round to zero


## stack operation

dup  # duplicates the value on the front
swap # swaps the front with the second element
rot  # rotates front 3 items
tuck # duplicate the top item below the second slot
over # duplicate the second item to the front 


## arrays

mka  # make array with size of front value
gnth # get nth element of an array [ array, index ]
snth # set nth element of an array [ array, index, value ]
len  # get length
atol # converts an array to a list
find # returns a list of indexes, where value can be found [ array, value ]


## strings

# gnth, snth and len are usable with strings
grow     # grows a string len by reallocating it (slow) [ string to-add ]
cat      # adds a string to string (even grows)
contains # returns t if string contains a substring
split    # splits a string into an array of strings [ string separator ]


## lists

# contains gnth, snth, len, find and stack operators, but prefixed with l
ltoa # converts a list to an array
car  # returns the lists car
cdr  # returns the lists cdr


## tables

mkt  # makes a table
tget # gets an element [ table key ]
tset # sets an element [ table key value ]
tcpy # copies a table even with its elements

## memory management
free # free (can free a list)
cpy  # copies a value (can copy a list)
```
