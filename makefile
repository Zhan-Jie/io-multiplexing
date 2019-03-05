build: test_select.c
	@if [ ! -d bin ]; then mkdir bin; fi;
	@gcc -o bin/test_select -std=c99 test_select.c

test: build
	@bin/test_select

clean:
	@rm -rf bin/
