build: main.c
	@if [ ! -d bin ]; then mkdir bin; fi;
	@gcc -o bin/main -std=c99 main.c

test: build
	@bin/main

clean:
	@rm -rf bin/
