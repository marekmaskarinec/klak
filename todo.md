
- [x] - char literal
	- [x] - normal
	- [x] - word
	- [x] - number
- [x] - string literal
- [x] - var assignement
	- [x] - c gen
	- [x] - errors
- [x] - word
- [ ] - push front
- [ ] - push back
- [x] - constant
- [ ] - keyword
	- [x] - mkw
	- [x] - if
	- [x] - then
	- [x] - fi
	- [x] - loop
	- [x] - pool
	- [x] - switch
	- [ ] - 4l's iter
- [x] - int
- [x] - hex int
- [x] - float
- [x] - eof
- [x] - var declaration
	- [x] - c gen
	- [x] - errors
- [x] - var pushing
	- [x] - c gen
	- [x] - errors
- [x] - pop
- [x] - {
- [x] - }
- [ ] - gc
	- [x] - reference counting
	- [x] - ref-- on out of scope
	- [ ] - proper testing
- [ ] - normal idents in errors
- [ ] - libtcc
- [x] - cons
- [ ] - hashmaps
- [ ] - arrays
- [ ] - file including
- [ ] - macros?

# standard functions

## arithmetics

- [x] - + # add
- [x] - - # subtract
- [x] - * # multiply
- [x] - / # divide
- [x] - % # modulo


## comparation

- [x] - =  # equals
- [x] - >  # bigger
- [x] - <  # smaller
- [x] - <= # smaller or equal
- [x] - >= # bigger or equal
- [x] - != # not equal


## bitwise operators

- [ ] - b&  # bitwise and
- [ ] - b|  # bitwise or
- [ ] - b<< # shift left
- [ ] - b>> # shift right
- [ ] - b~  # xor
- [ ] - !   # t => n, n => t


## functions

- [ ] - abs   # absolute function
- [ ] - sin   # sine
- [ ] - cos   # cosine
- [ ] - tg    # tangens
- [ ] - round # round
- [ ] - trunc # round to zero


## stack operation

- [x] - dup  # duplicates the value on the front
- [x] - swap # swaps the front with the second element
- [x] - rot  # rotates front 3 items
- [x] - tuck # duplicate the top item below the second slot
- [x] - over # duplicate the second item to the front 


## arrays

- [ ] - mka  # make array with size of front value
- [ ] - gnth # get nth element of an array ( array, index -- array, values-at-index )
- [ ] - snth # set nth element of an array ( array, index, value -- array )
- [ ] - len  # get length
- [ ] - atol # converts an array to a list
- [ ] - find # returns a list of indexes, where value can be found ( array, value -- list-of-indexes )


## strings

- [ ] - gnth, snth and len are usable with strings
- [ ] - grow     # grows a string len by reallocating it (slow) ( string to-add -- result )
- [ ] - contains # returns t if string contains a substring
- [ ] - split    # splits a string into an array of strings ( string separator -- result )


## cons

- [x] - cons   # ( car cdr -- cons )
- [ ] - contains gnth, snth, len, find and stack operators, but prefixed with l
- [ ] - ltoa   # ( cons -- array )
- [x] - car    # ( cons -- cons car )
- [x] - cdr    # ( cons -- cons cdr )
- [x] - uncons # ( cons -- car cdr )


## tables

- [ ] - mkt  # makes a table
- [ ] - tget # gets an element [ table key ]
- [ ] - tset # sets an element [ table key value ]

## memory
- [ ] - cpy  # copies a value
- [ ] - rcpy # recursively copies a value
