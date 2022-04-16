CC=gcc

build:
	$(CC) -c -Wall -Werror -fpic matelib.c && \
	$(CC) -shared matelib.o -o libmate.so && \
	rm matelib.o

clean:
	rm libmate.so 