#include <stdint.h>

// GDT Entry structure
struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct gdt_entry gdt[3];
struct gdt_ptr gdt_ptr_val;

// IDT Entry structure
struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t zero;
    uint8_t type_attr;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct idt_entry idt[256];
struct idt_ptr idt_ptr_val;

volatile uint32_t timer_ticks = 0;

void init_gdt() {
    gdt[0].limit_low = 0;
    gdt[0].base_low = 0;
    gdt[0].base_mid = 0;
    gdt[0].access = 0;
    gdt[0].granularity = 0;
    gdt[0].base_high = 0;

    gdt[1].limit_low = 0xFFFF;
    gdt[1].base_low = 0;
    gdt[1].base_mid = 0;
    gdt[1].access = 0x9A;
    gdt[1].granularity = 0xCF;
    gdt[1].base_high = 0;

    gdt[2].limit_low = 0xFFFF;
    gdt[2].base_low = 0;
    gdt[2].base_mid = 0;
    gdt[2].access = 0x92;
    gdt[2].granularity = 0xCF;
    gdt[2].base_high = 0;

    gdt_ptr_val.limit = sizeof(gdt) - 1;
    gdt_ptr_val.base = (uint32_t)&gdt;
}

void set_idt_entry(int n, uint32_t handler) {
    idt[n].offset_low = handler & 0xFFFF;
    idt[n].selector = 0x08;
    idt[n].zero = 0;
    idt[n].type_attr = 0x8E;
    idt[n].offset_high = (handler >> 16) & 0xFFFF;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void init_pic() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFE);  // Unmask only IRQ0 (timer), mask rest
    outb(0xA1, 0xFF);
}

// Timer interrupt handler
void timer_handler() {
    timer_ticks++;

    // Send End-Of-Interrupt signal to PIC
    outb(0x20, 0x20);
}

// Assembly stub that calls our C handler
__attribute__((naked)) void timer_interrupt_stub() {
    __asm__ volatile (
        "pusha\n"
        "call timer_handler\n"
        "popa\n"
        "iret\n"
    );
}

void init_idt() {
    idt_ptr_val.limit = sizeof(idt) - 1;
    idt_ptr_val.base = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        set_idt_entry(i, 0);
    }

    // IRQ0 (timer) is now at interrupt number 32
    set_idt_entry(32, (uint32_t)timer_interrupt_stub);

    __asm__ volatile ("lidt (%0)" : : "r" (&idt_ptr_val));
}

void main() {
    init_gdt();
    init_idt();
    init_pic();

    __asm__ volatile ("sti");  // Enable interrupts

    while (1) {
        
    }
}

void _start() {
    main();
}
