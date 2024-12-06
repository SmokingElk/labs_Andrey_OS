all: prime_count gcf main_compile main_dl link

link: 
	gcc prime_count.o gcf.o main_compile.o -o app_compile.out
	gcc main_dl.o -o app_dl.out

main_compile: main_compile.c
	gcc -c main_compile.c -o main_compile.o

main_dl: main_dl.c
	gcc -c main_dl.c -o main_dl.o

prime_count: prime_count.c
	gcc -c prime_count.c -o prime_count.o
	gcc -shared -rdynamic prime_count.c -o prime_count.so

gcf: gcf.c
	gcc -c gcf.c -o gcf.o
	gcc -shared -rdynamic gcf.c -o gcf.so

clean:
	rm *.out
	rm *.so
	rm *.o