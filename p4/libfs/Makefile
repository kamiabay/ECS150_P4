
lib := libfs.a
objs := disk.o fs.o
CC      := gcc
CFLAGS  := -Wall -Wextra -Werror
CFLAGS  += -g

ifneq ($(V),1)
Q=@
endif

all: $(lib)

deps := $(patsubst %.o,%.d,$(objs))
-include $(deps)
DEPFLAGS = -MMD -MF $(@:.o=.d)

%.o: %.c
	$(Q) $(CC) $(CFLAGS) -c -o $@ $< $(DEPFLAGS)

$(lib): $(objs) $(other)
	ar rcs $(lib) $(objs)

clean:
	@echo "clean"
	$(Q) rm -f $(lib) $(objs) $(deps)
