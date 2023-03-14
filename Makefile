all:
	clang -std=c99 -Wall -pedantic -L. *.c -lruntime
