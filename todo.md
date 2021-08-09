
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
	- [ ] - switch
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
- [ ] - lists
- [ ] - hashmaps
- [ ] - arrays
- [ ] - file including
- [ ] - macros?

# standard functions

## arithmetics

- [ ] - + # add
- [ ] - - # subtract
- [ ] - * # multiply
- [ ] - / # divide
- [ ] - % # modulo


## comparation

- [ ] - =  # equals
- [ ] - >  # bigger
- [ ] - <  # smaller
- [ ] - <= # smaller or equal
- [ ] - >= # bigger or equal
- [ ] - != # not equal


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

- [ ] - dup  # duplicates the value on the front
- [ ] - swap # swaps the front with the second element
- [ ] - rot  # rotates front 3 items
- [ ] - tuck # duplicate the top item below the second slot
- [ ] - over # duplicate the second item to the front 


## arrays

- [ ] - mka  # make array with size of front value
- [ ] - gnth # get nth element of an array [ array, index ]
- [ ] - snth # set nth element of an array [ array, index, value ]
- [ ] - len  # get length
- [ ] - atol # converts an array to a list
- [ ] - find # returns a list of indexes, where value can be found [ array, value ]


## strings

- [ ] - gnth, snth and len are usable with strings
- [ ] - grow     # grows a string len by reallocating it (slow) [ string to-add ]
- [ ] - cat      # adds a string to string (even grows)
- [ ] - contains # returns t if string contains a substring
- [ ] - split    # splits a string into an array of strings [ string separator ]


## lists

- [ ] - contains gnth, snth, len, find and stack operators, but prefixed with l
- [ ] - ltoa # converts a list to an array
- [ ] - car  # returns the lists car
- [ ] - cdr  # returns the lists cdr


## tables

- [ ] - mkt  # makes a table
- [ ] - tget # gets an element [ table key ]
- [ ] - tset # sets an element [ table key value ]

## memory
- [ ] - cpy  # copies a value
- [ ] - rcpy # recursively copies a value
