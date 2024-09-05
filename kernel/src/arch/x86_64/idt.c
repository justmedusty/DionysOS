//
// Created by dustyn on 6/20/24.
//

#include "gdt.h"
#include "include/arch/x86_64/asm_functions.h"
#include "include/types.h"
#include "idt.h"

#include <include/arch/arch_cpu.h>

#include "include/arch/arch_traps.h"
#include "include/uart.h"
#include "include/arch/arch_global_interrupt_controller.h"
#include "include/arch/arch_local_interrupt_controller.h"
#include "include/arch/arch_smp.h"

extern void isr_wrapper_0();
extern void isr_wrapper_1();
extern void isr_wrapper_2();
extern void isr_wrapper_3();
extern void isr_wrapper_4();
extern void isr_wrapper_5();
extern void isr_wrapper_6();
extern void isr_wrapper_7();
extern void isr_wrapper_8();

extern void isr_wrapper_10();
extern void isr_wrapper_11();
extern void isr_wrapper_12();
extern void isr_wrapper_13();
extern void isr_wrapper_14();

extern void isr_wrapper_16();
extern void isr_wrapper_17();
extern void isr_wrapper_18();
extern void isr_wrapper_19();

extern void irq_wrapper_0();
extern void irq_wrapper_1();
extern void irq_wrapper_2();
extern void irq_wrapper_3();
extern void irq_wrapper_4();
extern void irq_wrapper_5();
extern void irq_wrapper_6();
extern void irq_wrapper_7();
extern void irq_wrapper_8();
extern void irq_wrapper_9();
extern void irq_wrapper_10();
extern void irq_wrapper_11();
extern void irq_wrapper_12();
extern void irq_wrapper_13();
extern void irq_wrapper_14();
extern void irq_wrapper_15();
extern void irq_wrapper_16();
extern void irq_wrapper_17();
extern void irq_wrapper_18();
extern void irq_wrapper_19();
extern void irq_wrapper_20();
extern void irq_wrapper_21();
extern void irq_wrapper_22();
extern void irq_wrapper_23();
extern void irq_wrapper_24();
extern void irq_wrapper_25();
extern void irq_wrapper_26();
extern void irq_wrapper_27();
extern void irq_wrapper_28();
extern void irq_wrapper_29();
extern void irq_wrapper_30();
extern void irq_wrapper_31();
extern void irq_wrapper_32();
extern void irq_wrapper_33();
extern void irq_wrapper_34();
extern void irq_wrapper_35();
extern void irq_wrapper_36();
extern void irq_wrapper_37();
extern void irq_wrapper_38();
extern void irq_wrapper_39();
extern void irq_wrapper_40();
extern void irq_wrapper_41();
extern void irq_wrapper_42();
extern void irq_wrapper_43();
extern void irq_wrapper_44();
extern void irq_wrapper_45();
extern void irq_wrapper_46();
extern void irq_wrapper_47();
extern void irq_wrapper_48();
extern void irq_wrapper_49();
extern void irq_wrapper_50();
extern void irq_wrapper_51();
extern void irq_wrapper_52();
extern void irq_wrapper_53();
extern void irq_wrapper_54();
extern void irq_wrapper_55();
extern void irq_wrapper_56();
extern void irq_wrapper_57();
extern void irq_wrapper_58();
extern void irq_wrapper_59();
extern void irq_wrapper_60();
extern void irq_wrapper_61();
extern void irq_wrapper_62();
extern void irq_wrapper_63();
extern void irq_wrapper_64();
extern void irq_wrapper_65();
extern void irq_wrapper_66();
extern void irq_wrapper_67();
extern void irq_wrapper_68();
extern void irq_wrapper_69();
extern void irq_wrapper_70();
extern void irq_wrapper_71();
extern void irq_wrapper_72();
extern void irq_wrapper_73();
extern void irq_wrapper_74();
extern void irq_wrapper_75();
extern void irq_wrapper_76();
extern void irq_wrapper_77();
extern void irq_wrapper_78();
extern void irq_wrapper_79();
extern void irq_wrapper_80();
extern void irq_wrapper_81();
extern void irq_wrapper_82();
extern void irq_wrapper_83();
extern void irq_wrapper_84();
extern void irq_wrapper_85();
extern void irq_wrapper_86();
extern void irq_wrapper_87();
extern void irq_wrapper_88();
extern void irq_wrapper_89();
extern void irq_wrapper_90();
extern void irq_wrapper_91();
extern void irq_wrapper_92();
extern void irq_wrapper_93();
extern void irq_wrapper_94();
extern void irq_wrapper_95();
extern void irq_wrapper_96();
extern void irq_wrapper_97();
extern void irq_wrapper_98();
extern void irq_wrapper_99();
extern void irq_wrapper_100();
extern void irq_wrapper_101();
extern void irq_wrapper_102();
extern void irq_wrapper_103();
extern void irq_wrapper_104();
extern void irq_wrapper_105();
extern void irq_wrapper_106();
extern void irq_wrapper_107();
extern void irq_wrapper_108();
extern void irq_wrapper_109();
extern void irq_wrapper_110();
extern void irq_wrapper_111();
extern void irq_wrapper_112();
extern void irq_wrapper_113();
extern void irq_wrapper_114();
extern void irq_wrapper_115();
extern void irq_wrapper_116();
extern void irq_wrapper_117();
extern void irq_wrapper_118();
extern void irq_wrapper_119();
extern void irq_wrapper_120();
extern void irq_wrapper_121();
extern void irq_wrapper_122();
extern void irq_wrapper_123();
extern void irq_wrapper_124();
extern void irq_wrapper_125();
extern void irq_wrapper_126();
extern void irq_wrapper_127();
extern void irq_wrapper_128();
extern void irq_wrapper_129();
extern void irq_wrapper_130();
extern void irq_wrapper_131();
extern void irq_wrapper_132();
extern void irq_wrapper_133();
extern void irq_wrapper_134();
extern void irq_wrapper_135();
extern void irq_wrapper_136();
extern void irq_wrapper_137();
extern void irq_wrapper_138();
extern void irq_wrapper_139();
extern void irq_wrapper_140();
extern void irq_wrapper_141();
extern void irq_wrapper_142();
extern void irq_wrapper_143();
extern void irq_wrapper_144();
extern void irq_wrapper_145();
extern void irq_wrapper_146();
extern void irq_wrapper_147();
extern void irq_wrapper_148();
extern void irq_wrapper_149();
extern void irq_wrapper_150();
extern void irq_wrapper_151();
extern void irq_wrapper_152();
extern void irq_wrapper_153();
extern void irq_wrapper_154();
extern void irq_wrapper_155();
extern void irq_wrapper_156();
extern void irq_wrapper_157();
extern void irq_wrapper_158();
extern void irq_wrapper_159();
extern void irq_wrapper_160();
extern void irq_wrapper_161();
extern void irq_wrapper_162();
extern void irq_wrapper_163();
extern void irq_wrapper_164();
extern void irq_wrapper_165();
extern void irq_wrapper_166();
extern void irq_wrapper_167();
extern void irq_wrapper_168();
extern void irq_wrapper_169();
extern void irq_wrapper_170();
extern void irq_wrapper_171();
extern void irq_wrapper_172();
extern void irq_wrapper_173();
extern void irq_wrapper_174();
extern void irq_wrapper_175();
extern void irq_wrapper_176();
extern void irq_wrapper_177();
extern void irq_wrapper_178();
extern void irq_wrapper_179();
extern void irq_wrapper_180();
extern void irq_wrapper_181();
extern void irq_wrapper_182();
extern void irq_wrapper_183();
extern void irq_wrapper_184();
extern void irq_wrapper_185();
extern void irq_wrapper_186();
extern void irq_wrapper_187();
extern void irq_wrapper_188();
extern void irq_wrapper_189();
extern void irq_wrapper_190();
extern void irq_wrapper_191();
extern void irq_wrapper_192();
extern void irq_wrapper_193();
extern void irq_wrapper_194();
extern void irq_wrapper_195();
extern void irq_wrapper_196();
extern void irq_wrapper_197();
extern void irq_wrapper_198();
extern void irq_wrapper_199();
extern void irq_wrapper_200();
extern void irq_wrapper_201();
extern void irq_wrapper_202();
extern void irq_wrapper_203();
extern void irq_wrapper_204();
extern void irq_wrapper_205();
extern void irq_wrapper_206();
extern void irq_wrapper_207();
extern void irq_wrapper_208();
extern void irq_wrapper_209();
extern void irq_wrapper_210();
extern void irq_wrapper_211();
extern void irq_wrapper_212();
extern void irq_wrapper_213();
extern void irq_wrapper_214();
extern void irq_wrapper_215();
extern void irq_wrapper_216();
extern void irq_wrapper_217();
extern void irq_wrapper_218();
extern void irq_wrapper_219();
extern void irq_wrapper_220();
extern void irq_wrapper_221();
extern void irq_wrapper_222();
extern void irq_wrapper_223();
extern void irq_wrapper_224();
extern void irq_wrapper_225();
extern void irq_wrapper_226();
extern void irq_wrapper_227();
extern void irq_wrapper_228();
extern void irq_wrapper_229();
extern void irq_wrapper_230();
extern void irq_wrapper_231();
extern void irq_wrapper_232();
extern void irq_wrapper_233();
extern void irq_wrapper_234();
extern void irq_wrapper_235();
extern void irq_wrapper_236();
extern void irq_wrapper_237();
extern void irq_wrapper_238();
extern void irq_wrapper_239();
extern void irq_wrapper_240();
extern void irq_wrapper_241();
extern void irq_wrapper_242();
extern void irq_wrapper_243();
extern void irq_wrapper_244();
extern void irq_wrapper_245();
extern void irq_wrapper_246();
extern void irq_wrapper_247();
extern void irq_wrapper_248();
extern void irq_wrapper_249();
extern void irq_wrapper_250();
extern void irq_wrapper_251();
extern void irq_wrapper_252();
extern void irq_wrapper_253();
extern void irq_wrapper_254();
extern void irq_wrapper_255();

void (*irq_routines[256])() = {
};

uint8 idt_free_vec = 64;

void (*irq_wrappers[256])() = {
    irq_wrapper_0,
    irq_wrapper_1,
    irq_wrapper_2,
    irq_wrapper_3,
    irq_wrapper_4,
    irq_wrapper_5,
    irq_wrapper_6,
    irq_wrapper_7,
    irq_wrapper_8,
    irq_wrapper_9,
    irq_wrapper_10,
    irq_wrapper_11,
    irq_wrapper_12,
    irq_wrapper_13,
    irq_wrapper_14,
    irq_wrapper_15,
    irq_wrapper_16,
    irq_wrapper_17,
    irq_wrapper_18,
    irq_wrapper_19,
    irq_wrapper_20,
    irq_wrapper_21,
    irq_wrapper_22,
    irq_wrapper_23,
    irq_wrapper_24,
    irq_wrapper_25,
    irq_wrapper_26,
    irq_wrapper_27,
    irq_wrapper_28,
    irq_wrapper_29,
    irq_wrapper_30,
    irq_wrapper_31,
    irq_wrapper_32,
    irq_wrapper_33,
    irq_wrapper_34,
    irq_wrapper_35,
    irq_wrapper_36,
    irq_wrapper_37,
    irq_wrapper_38,
    irq_wrapper_39,
    irq_wrapper_40,
    irq_wrapper_41,
    irq_wrapper_42,
    irq_wrapper_43,
    irq_wrapper_44,
    irq_wrapper_45,
    irq_wrapper_46,
    irq_wrapper_47,
    irq_wrapper_48,
    irq_wrapper_49,
    irq_wrapper_50,
    irq_wrapper_51,
    irq_wrapper_52,
    irq_wrapper_53,
    irq_wrapper_54,
    irq_wrapper_55,
    irq_wrapper_56,
    irq_wrapper_57,
    irq_wrapper_58,
    irq_wrapper_59,
    irq_wrapper_60,
    irq_wrapper_61,
    irq_wrapper_62,
    irq_wrapper_63,
    irq_wrapper_64,
    irq_wrapper_65,
    irq_wrapper_66,
    irq_wrapper_67,
    irq_wrapper_68,
    irq_wrapper_69,
    irq_wrapper_70,
    irq_wrapper_71,
    irq_wrapper_72,
    irq_wrapper_73,
    irq_wrapper_74,
    irq_wrapper_75,
    irq_wrapper_76,
    irq_wrapper_77,
    irq_wrapper_78,
    irq_wrapper_79,
    irq_wrapper_80,
    irq_wrapper_81,
    irq_wrapper_82,
    irq_wrapper_83,
    irq_wrapper_84,
    irq_wrapper_85,
    irq_wrapper_86,
    irq_wrapper_87,
    irq_wrapper_88,
    irq_wrapper_89,
    irq_wrapper_90,
    irq_wrapper_91,
    irq_wrapper_92,
    irq_wrapper_93,
    irq_wrapper_94,
    irq_wrapper_95,
    irq_wrapper_96,
    irq_wrapper_97,
    irq_wrapper_98,
    irq_wrapper_99,
    irq_wrapper_100,
    irq_wrapper_101,
    irq_wrapper_102,
    irq_wrapper_103,
    irq_wrapper_104,
    irq_wrapper_105,
    irq_wrapper_106,
    irq_wrapper_107,
    irq_wrapper_108,
    irq_wrapper_109,
    irq_wrapper_110,
    irq_wrapper_111,
    irq_wrapper_112,
    irq_wrapper_113,
    irq_wrapper_114,
    irq_wrapper_115,
    irq_wrapper_116,
    irq_wrapper_117,
    irq_wrapper_118,
    irq_wrapper_119,
    irq_wrapper_120,
    irq_wrapper_121,
    irq_wrapper_122,
    irq_wrapper_123,
    irq_wrapper_124,
    irq_wrapper_125,
    irq_wrapper_126,
    irq_wrapper_127,
    irq_wrapper_128,
    irq_wrapper_129,
    irq_wrapper_130,
    irq_wrapper_131,
    irq_wrapper_132,
    irq_wrapper_133,
    irq_wrapper_134,
    irq_wrapper_135,
    irq_wrapper_136,
    irq_wrapper_137,
    irq_wrapper_138,
    irq_wrapper_139,
    irq_wrapper_140,
    irq_wrapper_141,
    irq_wrapper_142,
    irq_wrapper_143,
    irq_wrapper_144,
    irq_wrapper_145,
    irq_wrapper_146,
    irq_wrapper_147,
    irq_wrapper_148,
    irq_wrapper_149,
    irq_wrapper_150,
    irq_wrapper_151,
    irq_wrapper_152,
    irq_wrapper_153,
    irq_wrapper_154,
    irq_wrapper_155,
    irq_wrapper_156,
    irq_wrapper_157,
    irq_wrapper_158,
    irq_wrapper_159,
    irq_wrapper_160,
    irq_wrapper_161,
    irq_wrapper_162,
    irq_wrapper_163,
    irq_wrapper_164,
    irq_wrapper_165,
    irq_wrapper_166,
    irq_wrapper_167,
    irq_wrapper_168,
    irq_wrapper_169,
    irq_wrapper_170,
    irq_wrapper_171,
    irq_wrapper_172,
    irq_wrapper_173,
    irq_wrapper_174,
    irq_wrapper_175,
    irq_wrapper_176,
    irq_wrapper_177,
    irq_wrapper_178,
    irq_wrapper_179,
    irq_wrapper_180,
    irq_wrapper_181,
    irq_wrapper_182,
    irq_wrapper_183,
    irq_wrapper_184,
    irq_wrapper_185,
    irq_wrapper_186,
    irq_wrapper_187,
    irq_wrapper_188,
    irq_wrapper_189,
    irq_wrapper_190,
    irq_wrapper_191,
    irq_wrapper_192,
    irq_wrapper_193,
    irq_wrapper_194,
    irq_wrapper_195,
    irq_wrapper_196,
    irq_wrapper_197,
    irq_wrapper_198,
    irq_wrapper_199,
    irq_wrapper_200,
    irq_wrapper_201,
    irq_wrapper_202,
    irq_wrapper_203,
    irq_wrapper_204,
    irq_wrapper_205,
    irq_wrapper_206,
    irq_wrapper_207,
    irq_wrapper_208,
    irq_wrapper_209,
    irq_wrapper_210,
    irq_wrapper_211,
    irq_wrapper_212,
    irq_wrapper_213,
    irq_wrapper_214,
    irq_wrapper_215,
    irq_wrapper_216,
    irq_wrapper_217,
    irq_wrapper_218,
    irq_wrapper_219,
    irq_wrapper_220,
    irq_wrapper_221,
    irq_wrapper_222,
    irq_wrapper_223,
    irq_wrapper_224,
    irq_wrapper_225,
    irq_wrapper_226,
    irq_wrapper_227,
    irq_wrapper_228,
    irq_wrapper_229,
    irq_wrapper_230,
    irq_wrapper_231,
    irq_wrapper_232,
    irq_wrapper_233,
    irq_wrapper_234,
    irq_wrapper_235,
    irq_wrapper_236,
    irq_wrapper_237,
    irq_wrapper_238,
    irq_wrapper_239,
    irq_wrapper_240,
    irq_wrapper_241,
    irq_wrapper_242,
    irq_wrapper_243,
    irq_wrapper_244,
    irq_wrapper_245,
    irq_wrapper_246,
    irq_wrapper_247,
    irq_wrapper_248,
    irq_wrapper_249,
    irq_wrapper_250,
    irq_wrapper_251,
    irq_wrapper_252,
    irq_wrapper_253,
    irq_wrapper_254,
    irq_wrapper_255
};

//Going to index this array when setting up the idt
void (*isr_wrappers[32])() = {
    isr_wrapper_0,
    isr_wrapper_1,
    isr_wrapper_2,
    isr_wrapper_3,
    isr_wrapper_4,
    isr_wrapper_5,
    isr_wrapper_6,
    isr_wrapper_7,
    isr_wrapper_8,
    0,
    isr_wrapper_10,
    isr_wrapper_11,
    isr_wrapper_12,
    isr_wrapper_13,
    isr_wrapper_14,
    0,
    isr_wrapper_16,
    isr_wrapper_17,
    isr_wrapper_18,
    isr_wrapper_19,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0

};

struct gate_desc gates[288] = {};

struct idtr_desc idtr = {
    .off = (uint64_t)&gates,
    .sz = sizeof(gates) - 1,
};



void create_interrupt_gate(struct gate_desc* gate_desc, void* isr) {
    // Select 64-bit code segment of the GDT. See
    // https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md#x86_64.
    gate_desc->segment_selector = (struct segment_selector){
        .index = GDT_SEGMENT_RING0_CODE,
    };
    // Don't use IST.
    gate_desc->ist = 0;
    // ISR.
    gate_desc->gate_type = SSTYPE_INTERRUPT_GATE;
    // Run in ring 0.
    gate_desc->dpl = 0;
    // Present bit.
    gate_desc->p = 1;
    // Set offsets slices.
    gate_desc->off_1 = (size_t)isr;
    gate_desc->off_2 = ((size_t)isr >> 16) & 0xffff;
    gate_desc->off_3 = (size_t)isr >> 32;
}


void create_void_gate(struct gate_desc* gate_desc) {
    // Select 64-bit code segment of the GDT. See
    // https://github.com/limine-bootloader/limine/blob/trunk/PROTOCOL.md#x86_64.
    gate_desc->segment_selector = (struct segment_selector){
        .index = GDT_SEGMENT_RING0_CODE,
    };
    // Don't use IST.
    gate_desc->ist = 0;
    // ISR.
    gate_desc->gate_type = SSTYPE_INTERRUPT_GATE;
    // Run in ring 0.
    gate_desc->dpl = 0;
    // Present bit.
    gate_desc->p = 0;
    // Set offsets slices.
    gate_desc->off_1 = 0xff;
    gate_desc->off_2 = 0xff;
    gate_desc->off_3 = 0xffff;
}

void idt_init(void) {
    //just exceptions
    for (int i = 0; i < 32; i++) {
        if (exceptions[i] != T_NONE) {
            create_interrupt_gate(&gates[i], isr_wrappers[i]);
        }
        else {
            create_void_gate(&gates[i]);
        }
    }
    for (int i = 0; i < 256; i++) {
        create_interrupt_gate(&gates[i + 32], irq_wrappers[i - 32]);
    }
    load_idtr(&idtr);
    //enable interrupts (locally)
    sti();
    serial_printf("IDT Loaded, ISRs mapped\n");
}

void irq_handler(uint8 vec) {
    irq_routines[vec]();
}

void no_irq_handler(uint8 vec) {
    serial_printf("VECTOR %x.8  \n",vec);
    panic("IRQ NOT IMPLEMENTED");
}

void irq_handler_init() {
    for (int i = 0; i < 256; i++) {
        irq_routines[i] = (void *) no_irq_handler;
    }
}

void idt_reload() {
    idt_init();
}

uint8 idt_get_irq_vector() {
    if (idt_free_vec == 255) {
        panic("idt_get_irq_vector out of space");
    }
    return idt_free_vec++;
}


void irq_register(uint8 vec, void* handler) {
    irq_routines[vec] = handler;
    ioapic_redirect_irq(bootstrap_lapic_id, vec + 32, vec, 1);
    serial_printf("IRQ %x.8  loaded\n", vec);
}


void irq_unregister(uint8 vec) {
    irq_routines[vec] = (void *)no_irq_handler;
}
