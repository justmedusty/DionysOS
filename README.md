# DionysOS. 
![360_F_559263530_RS0cHCkD13iVZOnKILaPnJuYe4mZJBOQ-2027576610](https://github.com/user-attachments/assets/c7af6a1d-bdf4-410f-a3ba-5f04eb4d40c3)

# The Birth of Dionysos
![the-rise-of-the-god-dionysus-280226335](https://github.com/user-attachments/assets/41cbf10d-5ae5-446a-8672-13c407152561)

[History](https://www.worldhistory.org/Dionysos/)

# Why Dionysos?
As an enjoyer of greek myth, and Dionysos being associated with wine and insanity, writing an operating system seems to fit well with that theme. The theme of fertility also is pertinent since programming is an act of creation.

# Short Intro
This is my take on my own operating system to further my understanding and improve my programming and system design skills. Also,  just to have some fun because let's face it, programming is like magic seeing how you can create almost anything by simply thinking it into existence. I will be combining some existing ideas and implementations and also mixing in some of my own thoughts and designs in as well.
I may add a posix compliant layer at some point but for the time being I think I will end up rolling my own userspace. I will probably use this project as an opportunity to learn about everything from rolling my own cryptographic functions to better acquaint myself with the mathematics behind algorithms such as RSA, AES, SHA, maybe even some simpler ciphers for fun. I will also be rolling my own data structures and accompanying algorithms to improve my understanding and actually implement these concepts in C.

Starting 10/26/2024 I am going to make the effort to comment my thoughts about every function, to save myself some headache down the line.

# Overall Progress (🟢 : Done 🟡 : In progress 🔴 : Not yet started) 

🟢Serial

🟢Load a new GDT

🟢Load an IDT so that exceptions and interrupts can be handled.

🟢Write a physical memory allocator, a good starting point is a bitmap allocator.

🟢Write a virtual memory manager that can map, remap and unmap pages.

🟢Begin parsing ACPI tables, the most important one is the MADT since it contains information about the APIC.

🟢Start up the other CPUs. Limine provides a facility to make this less painful.

🟢Set up an interrupt controller such as the APIC.

🟢Configure a timer such as the Local APIC timer, the PIT, or the HPET.

🟢Design a virtual file system (VFS) and implement it. The traditional UNIX VFS works and saves headaches when porting software, but you can make your own thing too.

🟡Implement a scheduler to schedule threads in order make multitasking possible.

🟡Implement a simple virtual file system like a memory-only tmpfs to avoid crippling the design of your VFS too much while implementing it alongside real storage filesystems.

🟡Implement a ramdisk driver to facilitate the tempsfs filesystem. 

🔴Decide how to abstract devices. UNIX likes usually go for a /dev virtual filesystem containing device nodes and use ioctl() alongside standard FS calls to do operations on them.

🔴Get a userland going by loading executables from your VFS and running them in ring 3. Set up a way to perform system calls.

🔴Write a PCI driver.

🔴Add support for a storage medium, the easiest and most common ones are AHCI and NVMe

🔴Add support for a usage of the framebuffer in userspace

🔴Write a shell program

🔴Set up a userspace terminal with pre-compiled shell programs that can be executed


# Meta Progress (🟢 : Done 🟡 : In progress 🔴 : Not yet started) 

🟡Document all functions and explain ideas so that others can read and follow along better.

🟡Add locks in all areas where it will be needed.



# Ideas for arcane syscall names:

conjure (spawn)
bifurcate(fork)
obliterate(kill)
unbind,vanish,ascend(exit)
transmute(anything involving changing , such as changing file name or xattrs)
shroud (anything involving masking something)

# Makefile targets

Running `make all` will compile the kernel (from the `kernel/` directory) and then generate a bootable ISO image.

Running `make all-hdd` will compile the kernel and then generate a raw image suitable to be flashed onto a USB stick or hard drive/SSD.

Running `make run-x86` will build the kernel and a bootable ISO (equivalent to make all) and then run it using `qemu` (if installed).

Running `make run-hdd` will build the kernel and a raw HDD image (equivalent to make all-hdd) and then run it using `qemu` (if installed).

The `run-uefi` and `run-hdd-uefi` targets are equivalent to their non `-uefi` counterparts except that they boot `qemu` using a UEFI-compatible firmware.
