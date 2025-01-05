# DionysOS. 
![image](https://github.com/user-attachments/assets/efe61f4f-42d5-4e71-9208-3ce22268f3e3)
[Link 1](https://www.worldhistory.org/Dionysos/) [Link 2](http://dionysia.org/greek/dionysos/thompson/dionysos.html)

# Why Dionysos?
As an enjoyer of greek myth, and Dionysos being associated with wine and insanity, writing an operating system seems to fit well with that theme. The themes of order & chaos are central to the story of Dionysos and such is also true for an operating system. Hardware is chaotic, users are chaotic, and the operating system stands between as an arbiter. It brings order to chaos and attempts to strike a balance between the inherent chaos of the surrounding environment and the order required for a coherent experience. 

The themes of fertility surrounding Dionysos are very relevant in the context of an operating system. The operating system is the mother of all software. All of the software we use day to day is born on an operating system and lives in an operating system. The operating system both breeds and nurtures programs in a way that closely ties into the theme of fertility.

The operating system provides a controlled environment for chaos, imposing structure on its internals and peripherals. The operating system may even succumb to the chaos on occasions such as an NMI (Non-Maskable Interrupt) or kernel panic, similar to the divine madness that afflicted Dionysos. 

In his travels around the world, Dionysos would gather followers, however those that did not partake in his proverbial song and dance were struck down while those that did were granted pleasures and gifts. The operating system tends to act in this way as well. The operating system demands compliance with rigid rules and protocols, much like Dionysos during his worldly travels. If the expectations of the operating system are not met, unruly processes will be struck down immediately. Meanwhile, those that partake in his song and dance will be granted the resources they request, even to unnecessary abundance. 

Lastly, given Dionysos' creation as the son of a God and a mortal, there is a sense of "bridging the gap" with his story. His father was a god while his mother was a mortal, putting him in this state of entanglement between divinity and mortality. An operating system is also a sort of "gap bridger" between two realms, much like Dionysos. Hardware is the embodiment of chaos, dancing with the laws of physics as well as the laws of chemistry and chemical interactions. Software on the other hand has rigid structure, is predictable, and is bound by logic and reason. The operating system is the bridge between these two opposing realms. Much like Zeus and Semele procreating to spawn Dionysos.

# Short Intro
This is my take on my own operating system to further my understanding and improve my programming and system design skills. Also,  just to have some fun because let's face it, programming is like magic seeing how you can create almost anything by simply thinking it into existence. I will be combining some existing ideas and implementations and also mixing in some of my own thoughts and designs in as well.
I may add a posix compliant layer at some point but for the time being I think I will end up rolling my own userspace. I will probably use this project as an opportunity to learn about everything from rolling my own cryptographic functions to better acquaint myself with the mathematics behind algorithms such as RSA, AES, SHA, maybe even some simpler ciphers for fun. I will also be rolling my own data structures and accompanying algorithms to improve my understanding and actually implement these concepts in C.

Starting 10/26/2024 I am going to make the effort to comment my thoughts about every function, to save myself some headache down the line.

# Unique Features
Diosfs : I wrote a custom filesystem to be used for my operating system, it is non journaling and has similiarties to the original unix filesystem albeit a little bit simpler.

# LoC To Date (Excluding all bootloader files & Includng .c, .h, .asm)
~18,000 (The line_count script under the scripts directory can show the most current figure)

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

🟢Add support for a usage of the framebuffer

🟢Implement a simple memory-only tmpfs filesystem

🟡Decide how to abstract devices. UNIX likes usually go for a /dev virtual filesystem containing device nodes and use ioctl() alongside standard FS calls to do operations on them.

🔴Write an NVMe driver.

🔴Write an AHCI driver.

🔴Get a userland going by loading executables from your VFS and running them in ring 3. Set up a way to perform system calls.

🔴Write a shell program

🔴Set up a userspace terminal with pre-compiled shell programs that can be executed


# Screeshots
## The serial print messages are the most verbose, you can see these by selecting the serial0 output in your QEMU instance. This is where the majority of kernel output is cast to.
![image](https://github.com/user-attachments/assets/493fc7a2-b5f5-4b06-ba4c-41cc616053d7)
## The framebuffer has fewer print messages and is more high-level as can be seen here. The framebuffer printing will be reserved mostly for userspace with kernelspace just using it for panic messages and a few bootstrapping messages. The reasoning for this is mainly because I do not plan on implementing text buffering and scrolling support with my framebuffer functionality anytime soon and QEMU automatically handles this in the serial ouput. The framebuffer does wrap properly in the current implementation but once a line has shifted out of view, it is gone and cannot be read again.
![image](https://github.com/user-attachments/assets/6c96c449-6c14-4a21-9fd5-7538efd6e258)
## I have got it working on bare hardware on my main laptop as well as my chromebook. My main laptop required quite a bit more changes to get it working compared to the chromebook.
![20250105_101314](https://github.com/user-attachments/assets/3485e5f6-f889-4d91-b93a-a95312cf1438)
![20250105_073158](https://github.com/user-attachments/assets/7413d619-66cb-406f-ba10-0f67f4ccee50)

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
