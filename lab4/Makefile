all: area sort main_compile main_dl link

link: 
	gcc area.o sort.o main_compile.o -o app_compile.out
	gcc main_dl.o -o app_dl.out

main_compile: main_compile.c
	gcc -c main_compile.c -o main_compile.o

main_dl: main_dl.c
	gcc -c main_dl.c -o main_dl.o

area: area.c
	gcc -c area.c -o area.o
	gcc -shared -rdynamic area.c -o area.so

sort: sort.c
	gcc -c sort.c -o sort.o
	gcc -shared -rdynamic sort.c -o sort.so

clean:
	rm *.out
	rm *.so
	rm *.o

strace:
	strace ./app_dl.out