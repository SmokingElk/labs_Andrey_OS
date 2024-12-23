all: rm_last_out build run

run:
	./main.out

build: child main

main: main.c
	gcc main.c -o main.out

child: child.c
	gcc child.c -o child.out

rm_last_out:
	rm 2> /dev/null *.txt || true

strace:
	strace ./main.out