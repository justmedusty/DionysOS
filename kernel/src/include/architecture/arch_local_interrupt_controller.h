//
// Created by dustyn on 8/8/24.
//

#pragma once

#include "x86_64/madt.h"
#include "x86_64/acpi.h"
#include "include/types.h"
#ifdef __x86_64__


/*
 * Refer to Intel SDM Chapter 11
 */

#define IA32_APIC_BASE_MSR 0x1B
#define LAPIC_ID_REG 0x20
#define LAPIC_PPR 0xA0
#define LAPIC_ICRLO 0x300
#define LAPIC_ICRHI 0x310
#define LAPIC_ICINI 0x500
#define LAPIC_ICSTR 0x600
#define LAPIC_ICEDGE 0x0000
#define LAPIC_ICPEND 0x1000
#define LAPIC_ICPHYS 0x00000000
#define LAPIC_ICASSR 0x04000
#define LAPIC_ICSHRTHND 0x00000000
#define LAPIC_ICDESTSHIFT 24
#define LAPIC_ICRAIS 0x80000
#define LAPIC_ICRAES 0xc0000
#define LAPIC_TIMER_DIV 0x3E0
#define LAPIC_TIMER_INITCNT 0x380
#define LAPIC_TIMER_LVT 0x320
#define LAPIC_TIMER_DISABLE 0x10000
#define LAPIC_TIMER_CURCNT 0x390
#define LAPIC_TIMER_PERIODIC 0x20000
#define LAPIC_SPURIOUS 0xF0
#define LAPIC_EOI 0xB0

void lapic_init();
void lapic_timer_stop();
void lapic_timer_oneshot(uint8_t vec, uint64_t ms);
void lapic_calibrate_timer();
void lapic_write(uint32_t reg, uint32_t val);
uint32_t lapic_read(uint32_t reg);
void lapic_eoi();
void lapic_ipi(uint32_t id, uint8_t dat);
void lapic_broadcast_interrupt(uint32_t vec);
void lapic_send_int(uint32_t id, uint32_t vec);
void lapic_init_cpu(uint32_t id);
void lapic_start_cpu(uint32_t id, uint32_t vec);
uint32_t get_lapid_id();
#endif