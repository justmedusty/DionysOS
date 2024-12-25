# DionysOS. 
![image](https://github.com/user-attachments/assets/efe61f4f-42d5-4e71-9208-3ce22268f3e3)
[Link 1](https://www.worldhistory.org/Dionysos/) [Link 2](http://dionysia.org/greek/dionysos/thompson/dionysos.html)

# Why Dionysos?
As an enjoyer of greek myth, and Dionysos being associated with wine and insanity, writing an operating system seems to fit well with that theme. The themes of order & chaos are central to the story of Dionysos and such is also true for an operating system. Hardware is chaotic, users are chaotic, and the operating system stands between as an arbiter. It brings order to chaos and attempts to strike a balance between the inherent chaos of the surrounding environment and the order required for a coherent experience. The operating system provides a controlled environment for chaos, imposing structure on its internals and peripherals. The operating system may even succumb to the chaos on occasions such as an NMI (Non-Maskable Interrupt) or kernel panic, similar to the divine madness that afflicted Dionysos. His thyrsos, the staff capable of providing as well as destroying, also fits very well with this theme. The operating system can provide all of the resources a task requests, but at the same time it will destroy it when it steps out of line. Much like the thyrsos of Dionysos. 

In his travels around the world, Dionysos would gather followers, however those that did not partake in his proverbial song and dance were struck down while those that did were granted pleasures and gifts. The operating system tends to act in this way as well. The operating system demands compliance with rigid rules and protocols, much like Dionysos during his worldly travels. If the expectations of the operating system are not met, unruly processes will be struck down immediately, however those that partake in the song and dance will be granted the resources they request, even to unnecessary abundance. 

Parallels can even be drawn to the philosophical juxtaposition of Dionysian vs Apollonian thinking. The operating system software as a concept is like a battle pitting the chaotic Dionysian thought against Apollonian thought, finding a sort of balance somewhere in the middle.

# Short Intro
This is my take on my own operating system to further my understanding and improve my programming and system design skills. Also,  just to have some fun because let's face it, programming is like magic seeing how you can create almost anything by simply thinking it into existence. I will be combining some existing ideas and implementations and also mixing in some of my own thoughts and designs in as well.
I may add a posix compliant layer at some point but for the time being I think I will end up rolling my own userspace. I will probably use this project as an opportunity to learn about everything from rolling my own cryptographic functions to better acquaint myself with the mathematics behind algorithms such as RSA, AES, SHA, maybe even some simpler ciphers for fun. I will also be rolling my own data structures and accompanying algorithms to improve my understanding and actually implement these concepts in C.

Starting 10/26/2024 I am going to make the effort to comment my thoughts about every function, to save myself some headache down the line.

# Unique Features
Diosfs : I wrote a custom filesystem to be used for my operating system, it is non journaling and has similiarties to ext2 albeit a little bit simpler.

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

🟢Implement a ramdisk driver to facilitate the block filesystem. 

🟢Implement a simple file system

🟢Implement a scheduler to schedule threads in order make multitasking possible.

🟢Write a PCI driver.

🟡Implement a simple memory-only tmpfs filesystem

🟡Decide how to abstract devices. UNIX likes usually go for a /dev virtual filesystem containing device nodes and use ioctl() alongside standard FS calls to do operations on them.

🔴Write an NVMe driver.

🔴Write an AHCI driver.

🔴Get a userland going by loading executables from your VFS and running them in ring 3. Set up a way to perform system calls.

🔴Add support for a usage of the framebuffer in userspace

🔴Write a shell program

🔴Set up a userspace terminal with pre-compiled shell programs that can be executed



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
