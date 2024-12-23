all: build run

build:
	g++ ./main.cpp -o ./app.out

run:
	./app.out 4

test:
	./app.out 4 < test.txt

strace:
	strace ./app.out 4