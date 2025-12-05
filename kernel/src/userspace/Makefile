# === CONFIGURATION MACROS ===
define DEFAULT_VAR =
    ifeq ($(origin $1),default)
        override $(1) := $(2)
    endif
    ifeq ($(origin $1),undefined)
        override $(1) := $(2)
    endif
endef

override DEFAULT_KCC := cc
$(eval $(call DEFAULT_VAR,KCC,$(DEFAULT_KCC)))

override DEFAULT_KLD := ld
$(eval $(call DEFAULT_VAR,KLD,$(DEFAULT_KLD)))

override DEFAULT_KCFLAGS := -g -O2 -pipe
$(eval $(call DEFAULT_VAR,KCFLAGS,$(DEFAULT_KCFLAGS)))

override DEFAULT_KCPPFLAGS :=
$(eval $(call DEFAULT_VAR,KCPPFLAGS,$(DEFAULT_KCPPFLAGS)))

override DEFAULT_KNASMFLAGS := -F dwarf -g
$(eval $(call DEFAULT_VAR,KNASMFLAGS,$(DEFAULT_KNASMFLAGS)))

# === INTERNAL FLAGS ===
override KCFLAGS += \
    -Wall -Wextra -std=gnu11 -ffreestanding -fno-stack-protector \
    -fno-stack-check -fno-lto -fno-pie -m64 -march=x86-64 -mno-80387 \
    -mno-mmx -mno-red-zone -g -fanalyzer \
    -Wno-unused-variable -Wno-unused-function -no-pie -fno-builtin -fno-builtin-memcpy -fno-builtin-memset -nostdlib -nostartfiles -nodefaultlibs

override KNASMFLAGS += -Wall -f elf64

# Find C and NASM files
CFILES := $(wildcard *.c)
NASMFILES := $(shell find -L * -type f -name '*.asm')

# Compile only NASM files globally
ASM_OBJ := $(addprefix obj/,$(NASMFILES:.asm=.asm.o))

# Binaries from each C file
BINARIES := $(addprefix bin/,$(CFILES:.c=))

.PHONY: all
all: $(BINARIES)

# Rule: compile and link each C file with shared NASM objects
bin/%: %.c $(ASM_OBJ)
	mkdir -p "$$(dirname $@)"
	$(KCC) $(KCFLAGS) $(KCPPFLAGS) $< $(ASM_OBJ) -o $@

# Compile NASM files to object files
obj/%.asm.o: %.asm
	mkdir -p "$$(dirname $@)"
	nasm $(KNASMFLAGS) $< -o $@

# Clean rule
.PHONY: clean
clean:
	rm -rf bin obj