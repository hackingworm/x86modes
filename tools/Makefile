src = src
inc = inc
dep = dep
bin = bin

CC = gcc
CFLAGS = -Wall -g

.PHONY: all clean

all: $(bin)/build

$(bin)/build: $(src)/build.c
	$(CC) -Wp,-MQ,$@,-MMD,$(dep)/.$(notdir $@).d $(CFLAGS) -Iinc -o $@ $<

clean:
	rm -rf $(bin) $(dep)

$(bin)/build: | $(bin)
$(bin):
	mkdir $@

$(dep)/.build.d: | $(dep)
$(dep):
	mkdir $@

-include $(dep)/.build.d
