CC = gcc
CFLAGS = -std=gnu18 -lm -g -Wall -Wextra -Wl,--disable-new-dtags,-rpath,./
TARGET = main freelist.so buddy.so

.PHONY: clean run build

build: ${TARGET}

main: main.c
	${CC} ${CFLAGS} -o $@ $<

freelist.so: freelist.c
	${CC} ${CFLAGS} -shared -o $@ $<

buddy.so: buddy.c
	${CC} ${CFLAGS} -shared -o $@ $<


clean:
	@rm -f ${TARGET}
	@rm -rf ./*.dSYM