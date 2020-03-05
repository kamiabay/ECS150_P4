
lib := libfs.a
objs := disk.o fs.o
CC      := gcc
CFLAGS  := -Wall -Wextra -Werror
CFLAGS  += -g

all: $(lib)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(lib): $(objs) $(other)
	ar rcs $(lib) $(objs)

clean:
	@echo "clean"
	rm -f $(lib) $(objs)
