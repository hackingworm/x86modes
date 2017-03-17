#
# arch/x86/boot/Makefile
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#
# Copyright (C) 1994 by Linus Torvalds
# Changed by many, many contributors over the years.
#

# If you want to preset the SVGA mode, uncomment the next line and
# set SVGA_MODE to whatever number you want.
# Set it to -DSVGA_MODE=NORMAL_VGA if you just want the EGA/VGA mode.
# The number is the same as you would ordinarily press at bootup.

SVGA_MODE	:= -DSVGA_MODE=NORMAL_VGA

setup-y		+= a20.o bioscall.o cmdline.o copy.o cpu.o cpuflags.o cpucheck.o
setup-y		+= early_serial_console.o header.o main.o memory.o
setup-y		+= pm.o pmjump.o printf.o regs.o string.o tty.o version.o video.o
setup-y		+= video-mode.o

# The link order of the video-*.o modules can matter.  In particular,
# video-vga.o *must* be listed first, followed by video-vesa.o.
# Hardware-specific drivers should follow in the order they should be
# probed, and video-bios.o should typically be last.
setup-y		+= video-vga.o
setup-y		+= video-vesa.o
setup-y		+= video-bios.o

setup-y		+= nonsense.o

# ---------------------------------------------------------------------------

src = src
inc = inc
obj = obj
dep = dep
bin = bin
tools = tools

SETUP_BINS = $(addprefix $(bin)/,bzImage setup.bin setup.elf)
SETUP_OBJS = $(addprefix $(obj)/,$(setup-y))
SETUP_DEPS = $(addsuffix .d,$(addprefix $(dep)/.,$(setup-y)))

CC = gcc
CFLAGS = -Wall -Wstrict-prototypes -march=i386 -mregparm=3 -ffreestanding

subdirs = $(tools)
clean.subs = $(addprefix clean.,$(subdirs))

.PHONY: all clean $(subdirs) $(clean.subs)

all: $(subdirs)

$(subdirs):
	$(MAKE) -w -C $@

all: $(bin)/bzImage

$(bin)/bzImage: $(bin)/setup.bin $(tools)/bin/build
	$(tools)/bin/build $< $@

$(bin)/setup.bin: $(bin)/setup.elf
	objcopy  -O binary $< $@

$(bin)/setup.elf: setup.ld $(SETUP_OBJS)
	ld -m elf_x86_64   -T setup.ld $(SETUP_OBJS) -o $@

$(obj)/%.o: $(src)/%.c
	$(CC) -Wp,-MQ,$@,-MMD,$(dep)/.$(notdir $@).d $(CFLAGS) -m16 -g -D_SETUP -Iinc -c -o $@ $<

$(obj)/%.o: $(src)/%.S
	$(CC) -Wp,-MQ,$@,-MMD,$(dep)/.$(notdir $@).d $(CFLAGS) -m16 -g -D__ASSEMBLY__ -Iinc -c -o $@ $<

clean: $(clean.subs)

$(clean.subs):
	$(MAKE) -w -C $(@:clean.%=%) clean

clean:
	rm -rf $(bin) $(obj) $(dep)

$(SETUP_BINS): | $(bin)
$(bin):
	mkdir $@

$(SETUP_OBJS): | $(obj)
$(obj):
	mkdir $@

$(SETUP_DEPS): | $(dep)
$(dep):
	mkdir $@

-include $(SETUP_DEPS)
