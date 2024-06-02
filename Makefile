CC=clang

CFLAGS=-Wall -Werror -Wpedantic -std=c17

bin/explore: main.c
	$(CC) $(CFLAGS) -o $@ -Ofast -lraylib -Lraylib-5.0/build/raylib -g -Iraylib-5.0/build/raylib/include -framework Cocoa -framework IOKit main.c

.PHONY: run
run: bin/explore
	./bin/explore

.PHONY: raylib
raylib:
	curl -L https://github.com/raysan5/raylib/archive/refs/tags/5.0.tar.gz -o raylib.tar.gz && tar -xzf raylib.tar.gz && cmake -B raylib-5.0/build -S raylib-5.0 && make -C raylib-5.0/build

.PHONY: stb_ds.h
stb_ds.h:
	curl https://raw.githubusercontent.com/nothings/stb/master/stb_ds.h -o stb_ds.h
