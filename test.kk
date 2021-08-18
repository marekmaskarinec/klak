
:fib ( n -- n' )
	if  dup 2 <  then  ret  fi

	dup  1 - fib
	swap 2 - fib +
;

10 fib put
