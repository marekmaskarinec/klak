
@def right ptr 1 + .ptr
@def left ptr 1 - .ptr
@def inc get ptr 1 +  ptr swap set
@def dec get ptr 1 -  ptr swap set
@def print get ptr put

$ptr 0 .ptr
30000 mka

inc inc put
