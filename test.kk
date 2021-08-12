
mkw foo {
	1 +
	ret
}

:var
1 .var

loop  1  then
	var
	foo
	.var var
	if  5 =  then
		break
	fi
pool
