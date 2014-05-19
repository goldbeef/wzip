wzip:
	gcc -O2 *.cpp -o wzip  -L. -lds_ssort -pthread
clean:
	rm -f wzip
