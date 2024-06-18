//
// Created by dustyn on 6/17/24.
//
typedef struct {
    uint32 rsv0;
    uint32 rsp0_lower;
    uint32 rsp0_upper;
    uint32 rsp1_lower;
    uint32 rsp1_upper;
    uint32 rsp2_lower;
    uint32 rsp2_upper;
    uint32 rsv1;
    uint32 rsv2;
    uint32 ist1_lower;
    uint32 ist1_upper;
    uint32 ist2_lower;
    uint32 ist2_upper;
    uint32 ist3_lower;
    uint32 ist3_upper;
    uint32 ist4_lower;
    uint32 ist4_upper;
    uint32 ist5_lower;
    uint32 ist5_upper;
    uint32 ist6_lower;
    uint32 ist6_upper;
    uint32 ist7_lower;
    uint32 ist7_upper;
    uint32 rsv3;
    uint32 rsv4;
    uint16 rsv5;
    uint16 iomap_base;
} __attribute__((packed)) tss;
