build_select: test_select.c
	@if [ ! -d bin ]; then mkdir bin; fi;
	@gcc -o bin/test_select -std=c99 test_select.c

test_select: build_select
	@bin/test_select

build_poll: test_poll.c
	@if [ ! -d bin ]; then mkdir bin; fi;
	@gcc -o bin/test_poll -std=c99 test_poll.c

test_poll: build_poll
	@bin/test_poll

build_epoll: test_epoll.c
	@if [ ! -d bin ]; then mkdir bin; fi;
	@gcc -o bin/test_epoll -std=c99 test_epoll.c

test_epoll: build_epoll
	@bin/test_epoll

clean:
	@rm -rf bin/
