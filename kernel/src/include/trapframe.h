//
// Created by dustyn on 6/17/24.
// Stolen from FreeBSD

#ifndef DIONYSOS_TRAPFRAME_H
#define DIONYSOS_TRAPFRAME_H
/*-
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2003 Peter Wemm.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*
 * System stack frames.
 */

#ifdef __i386__
/*
 * Exception/Trap Stack Frame
 */

struct trapframe {
	int	tf_fs;
	int	tf_es;
	int	tf_ds;
	int	tf_edi;
	int	tf_esi;
	int	tf_ebp;
	int	tf_isp;
	int	tf_ebx;
	int	tf_edx;
	int	tf_ecx;
	int	tf_eax;
	int	tf_trapno;
	/* below portion defined in 386 hardware */
	int	tf_err;
	int	tf_eip;
	int	tf_cs;
	int	tf_eflags;
	/* below only when crossing rings (user to kernel) */
	int	tf_esp;
	int	tf_ss;
};

/* Superset of trap frame, for traps from virtual-8086 mode */

struct trapframe_vm86 {
	int	tf_fs;
	int	tf_es;
	int	tf_ds;
	int	tf_edi;
	int	tf_esi;
	int	tf_ebp;
	int	tf_isp;
	int	tf_ebx;
	int	tf_edx;
	int	tf_ecx;
	int	tf_eax;
	int	tf_trapno;
	/* below portion defined in 386 hardware */
	int	tf_err;
	int	tf_eip;
	int	tf_cs;
	int	tf_eflags;
	/* below only when crossing rings (user (including vm86) to kernel) */
	int	tf_esp;
	int	tf_ss;
	/* below only when crossing from vm86 mode to kernel */
	int	tf_vm86_es;
	int	tf_vm86_ds;
	int	tf_vm86_fs;
	int	tf_vm86_gs;
};

/*
 * This alias for the MI TRAPF_USERMODE() should be used when we don't
 * care about user mode itself, but need to know if a frame has stack
 * registers.  The difference is only logical, but on i386 the logic
 * for using TRAPF_USERMODE() is complicated by sometimes treating vm86
 * bioscall mode (which is a special ring 3 user mode) as kernel mode.
 */
#define	TF_HAS_STACKREGS(tf)	TRAPF_USERMODE(tf)
#endif /* __i386__ */

#ifdef __amd64__
/*
 * Exception/Trap Stack Frame
 *
 * The ordering of this is specifically so that we can take first 6
 * the syscall arguments directly from the beginning of the frame.
 */

struct trapframe {
    uint64	tf_rdi;
    uint64	tf_rsi;
    uint64	tf_rdx;
    uint64	tf_rcx;
    uint64	tf_r8;
    uint64	tf_r9;
    uint64	tf_rax;
    uint64	tf_rbx;
    uint64	tf_rbp;
    uint64	tf_r10;
    uint64	tf_r11;
    uint64	tf_r12;
    uint64	tf_r13;
    uint64	tf_r14;
    uint64	tf_r15;
    uint32_t	tf_trapno;
    uint16_t	tf_fs;
    uint16_t	tf_gs;
    uint64	tf_addr;
    uint32_t	tf_flags;
    uint16_t	tf_es;
    uint16_t	tf_ds;
    /* below portion defined in hardware */
    uint64	tf_err;
    uint64	tf_rip;
    uint64	tf_cs;
    uint64	tf_rflags;
    /* the amd64 frame always has the stack registers */
    uint64	tf_rsp;
    uint64	tf_ss;
};

#define	TF_HASSEGS	0x1
#define	TF_HASBASES	0x2
#define	TF_HASFPXSTATE	0x4
#endif /* __amd64__ */
#endif //DIONYSOS_TRAPFRAME_H
