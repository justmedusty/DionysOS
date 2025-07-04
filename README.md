# DionysOS. 
![image](https://github.com/user-attachments/assets/efe61f4f-42d5-4e71-9208-3ce22268f3e3)
[Link 1](https://www.worldhistory.org/Dionysos/) [Link 2](http://dionysia.org/greek/dionysos/thompson/dionysos.html)

# Why Dionysos?
As someone who appreciates Greek mythology, the choice of Dionysos as a symbolic reference for an operating system feels fitting. Dionysos, known for his associations with wine, chaos, and transformation, parallels the themes that underpin operating system design. An OS must reconcile chaos and order, hardware and users are inherently unpredictable, while the OS acts as a mediator that brings structure and consistency to their interactions.

The fertility often associated with Dionysos also connects well. Operating systems are foundational; all software depends on them to function. In a way, the OS “gives birth” to applications and manages their life cycles, nurturing them by providing resources and structure.

An OS creates a controlled environment where low-level hardware signals and peripheral events, potentially chaotic on their own, are organized and handled predictably. But even this control can break down under certain conditions, such as a non-maskable interrupt or a kernel panic, echoing moments of madness in Dionysos’s myth.

There's also a clear analogy in the way the OS enforces its rules. Like Dionysos rewarding those who followed his rites and punishing those who did not, the operating system allocates resources and executes processes that comply with its protocols, while terminating those that misbehave.

Finally, Dionysos’s dual heritage, born of a god and a mortal, mirrors the OS’s role as a bridge. It connects the physical, unpredictable world of hardware with the structured, logical realm of software. The OS mediates between these two layers, enabling them to coexist and function harmoniously.

# Short Intro
This is my take on my own operating system to further my understanding and improve my programming and system design skills. Also,  just to have some fun because let's face it, programming is like magic seeing how you can create almost anything by simply thinking it into existence. I will be combining some existing ideas and implementations and also mixing in some of my own thoughts and designs in as well.
I may add a posix compliant layer at some point but for the time being I think I will end up rolling my own userspace. I will probably use this project as an opportunity to learn about everything from rolling my own cryptographic functions to better acquaint myself with the mathematics behind algorithms such as RSA, AES, SHA, maybe even some simpler ciphers for fun. I will also be rolling my own data structures and accompanying algorithms to improve my understanding and actually implement these concepts in C.

# Unique Features
Diosfs : I wrote a custom filesystem to be used for my operating system, it is non journaling and has similiarties to the original unix filesystem albeit a little bit simpler.

# LoC To Date (Excluding all bootloader files & Includng .c, .h, .asm)
~26,500 (The line_count script under the scripts directory can show the most current figure)

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

🟢Write an NVMe driver.

🟢Set up a way to perform system calls.

🟢Write an AHCI driver.

🟡Decide how to abstract devices. UNIX likes usually go for a /dev virtual filesystem containing device nodes and use ioctl() alongside standard FS calls to do operations on them.

🟡Get a userland going by loading executables from your VFS and running them in ring 3. 

🔴Write a shell program

🔴Set up a userspace terminal with pre-compiled shell programs that can be executed


# Screeshots
## The serial print messages are the most verbose, you can see these by selecting the serial0 output in your QEMU instance. This is where the majority of kernel output is cast to.
![image](https://github.com/user-attachments/assets/493fc7a2-b5f5-4b06-ba4c-41cc616053d7)
## The framebuffer has fewer print messages and is more high-level as can be seen here. The framebuffer printing will be reserved mostly for userspace with kernelspace just using it for panic messages and a few bootstrapping messages. The reasoning for this is mainly because I do not plan on implementing text buffering and scrolling support with my framebuffer functionality anytime soon and QEMU automatically handles this in the serial ouput. The framebuffer does wrap properly in the current implementation but once a line has shifted out of view, it is gone and cannot be read again.
![image](https://github.com/user-attachments/assets/6c96c449-6c14-4a21-9fd5-7538efd6e258)
## I have got it working on bare hardware on my main laptop as well as my chromebook. My main laptop required quite a bit more changes to get it working compared to the chromebook. Notably on my main laptop an infinite loop was triggered in the MADT initialization because my break condition was length reaching a certain value but the final header on this laptop had a length of zero. The main issue I encountered was QEMU pages appear to be zerod by default and of course on real hardware you NEED to use your zalloc function for certain things or you will have a bad time.
![20250105_101314](https://github.com/user-attachments/assets/3485e5f6-f889-4d91-b93a-a95312cf1438)
![20250105_073158](https://github.com/user-attachments/assets/7413d619-66cb-406f-ba10-0f67f4ccee50)

# Makefile targets

Running `make all` will compile the kernel (from the `kernel/` directory) and then generate a bootable ISO image.

Running `make all-hdd` will compile the kernel and then generate a raw image suitable to be flashed onto a USB stick or hard drive/SSD.

Running `make run-x86` will build the kernel and a bootable ISO (equivalent to make all) and then run it using `qemu` (if installed).

Running `make run-hdd` will build the kernel and a raw HDD image (equivalent to make all-hdd) and then run it using `qemu` (if installed).

The `run-uefi` and `run-hdd-uefi` targets are equivalent to their non `-uefi` counterparts except that they boot `qemu` using a UEFI-compatible firmware.
