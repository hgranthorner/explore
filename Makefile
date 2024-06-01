CC=clang

CFLAGS=-Wall -Werror -Wpedantic -std=c17

.PHONY: run
run: bin/explore
	./bin/explore

bin/explore: main.c
	$(CC) $(CFLAGS) -o $@ -Ofast -lraylib -Lraylib-5.0/build/raylib -g -Iraylib-5.0/build/raylib/include -framework Cocoa -framework IOKit main.c

.PHONY: raylib
raylib:
	curl -L https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz -o raylib.tar.gz && tar -xzf raylib.tar.gz && cmake -B raylib-5.0/build -S raylib-5.0 && make -C raylib-5.0/build
