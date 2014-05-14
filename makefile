project: src/main.o src/struktury.o src/parser.o src/wydruk.o
	cc -o drzewo src/main.o src/struktury.o src/parser.o src/wydruk.o

clean::
	-rm -f src/*~ src/*.o drzewo
