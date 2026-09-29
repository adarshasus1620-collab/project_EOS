#include <stdint.h>

void main();

void _start() {
    main();
}

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

// VGA text mode screen (80 columns x 25 rows)
volatile uint16_t *vga_buffer = (volatile uint16_t *)0xB8000;
int cursor_x = 0;
int cursor_y = 0;

void clear_screen() {
    for (int i = 0; i < 80 * 25; i++) {
        vga_buffer[i] = (uint16_t)(0x0F00 | ' ');
    }
    cursor_x = 0;
    cursor_y = 0;
}

void set_cursor(int x, int y) {
    cursor_x = x;
    cursor_y = y;
}

void print_char(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
        return;
    }

    if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = 79;
        }
        vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t)(0x0F00 | ' ');
        return;
    }

    vga_buffer[cursor_y * 80 + cursor_x] = (uint16_t)(0x0F00 | (uint8_t)c);
    cursor_x++;
    if (cursor_x >= 80) {
        cursor_x = 0;
        cursor_y++;
    }
}

void print_string(const char *str) {
    for (int i = 0; str[i] != '\0'; i++) {
        print_char(str[i]);
    }
}

void print_number(uint32_t num) {
    char buffer[11];
    int i = 0;

    if (num == 0) {
        print_char('0');
        return;
    }

    while (num > 0) {
        buffer[i] = (char)('0' + (num % 10));
        num = num / 10;
        i++;
    }

    while (i > 0) {
        i--;
        print_char(buffer[i]);
    }
}

int str_equals(const char *a, const char *b) {
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return 0;
        }
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

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

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
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
    outb(0x21, 0xFC);
    outb(0xA1, 0xFF);
}

void timer_handler() {
    timer_ticks++;
    outb(0x20, 0x20);
}

__attribute__((naked)) void timer_interrupt_stub() {
    __asm__ volatile (
        "pusha\n"
        "call timer_handler\n"
        "popa\n"
        "iret\n"
    );
}

// Scan code to ASCII lookup tables (US QWERTY)
const char scancode_to_ascii[128] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0, 'a','s','d','f','g','h','j','k','l',';','\'','`',
    0, '\\', 'z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0, ' ', 0
};

const char scancode_to_ascii_shift[128] = {
    0, 27, '!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0, 'A','S','D','F','G','H','J','K','L',':','"','~',
    0, '|', 'Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0, ' ', 0
};

volatile int shift_pressed = 0;
volatile int caps_lock_on = 0;

// Command buffer for the shell
char command_buffer[128];
int command_length = 0;

void run_command() {
    command_buffer[command_length] = '\0';

    if (command_length == 0) {
        return;
    }

    if (str_equals(command_buffer, "help")) {
        print_string("Available commands:\n");
        print_string("  help  - show this list\n");
        print_string("  clear - clear the screen\n");
        print_string("  ticks - show timer tick count\n");
    } else if (str_equals(command_buffer, "clear")) {
        clear_screen();
    } else if (str_equals(command_buffer, "ticks")) {
        print_string("Timer ticks: ");
        print_number(timer_ticks);
        print_char('\n');
    } else {
        print_string("Unknown command: ");
        print_string(command_buffer);
        print_char('\n');
    }
}

// Convert a numpad scancode (0x47-0x53) to its character.
// This assumes Num Lock is on (digits mode, not arrow/navigation mode).
char numpad_to_ascii(uint8_t scancode) {
    switch (scancode) {
        case 0x47: return '7';
        case 0x48: return '8';
        case 0x49: return '9';
        case 0x4A: return '-';
        case 0x4B: return '4';
        case 0x4C: return '5';
        case 0x4D: return '6';
        case 0x4E: return '+';
        case 0x4F: return '1';
        case 0x50: return '2';
        case 0x51: return '3';
        case 0x52: return '0';
        case 0x53: return '.';
        default: return 0;
    }
}

int is_letter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

char apply_caps_lock(char c) {
    if (!is_letter(c)) {
        return c;
    }
    if (c >= 'a' && c <= 'z') {
        return (char)(c - 'a' + 'A');
    }
    return (char)(c - 'A' + 'a');
}

void keyboard_handler() {
    uint8_t scancode = inb(0x60);

    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
        outb(0x20, 0x20);
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_pressed = 0;
        outb(0x20, 0x20);
        return;
    }
    if (scancode == 0x3A) {
        // Caps Lock is a toggle - only react on key press, not release
        caps_lock_on = !caps_lock_on;
        outb(0x20, 0x20);
        return;
    }

    if (!(scancode & 0x80)) {
        char c;

        if (scancode >= 0x47 && scancode <= 0x53) {
            c = numpad_to_ascii(scancode);
        } else {
            c = shift_pressed ? scancode_to_ascii_shift[scancode] : scancode_to_ascii[scancode];
            if (caps_lock_on) {
                c = apply_caps_lock(c);
            }
        }

        if (c == '\n') {
            print_char('\n');
            run_command();
            command_length = 0;
            print_string("> ");
        } else if (c == '\b') {
            if (command_length > 0) {
                command_length--;
                print_char('\b');
            }
        } else if (c != 0 && command_length < 127) {
            command_buffer[command_length] = c;
            command_length++;
            print_char(c);
        }
    }

    outb(0x20, 0x20);
}

__attribute__((naked)) void keyboard_interrupt_stub() {
    __asm__ volatile (
        "pusha\n"
        "call keyboard_handler\n"
        "popa\n"
        "iret\n"
    );
}

// Default handler for any interrupt we don't specifically handle
__attribute__((naked)) void default_interrupt_stub() {
    __asm__ volatile (
        "pusha\n"
        "movb $0x20, %al\n"
        "outb %al, $0x20\n"
        "popa\n"
        "iret\n"
    );
}

void init_idt() {
    idt_ptr_val.limit = sizeof(idt) - 1;
    idt_ptr_val.base = (uint32_t)&idt;

    for (int i = 0; i < 256; i++) {
        set_idt_entry(i, (uint32_t)default_interrupt_stub);
    }

    set_idt_entry(32, (uint32_t)timer_interrupt_stub);
    set_idt_entry(33, (uint32_t)keyboard_interrupt_stub);

    __asm__ volatile ("lidt (%0)" : : "r" (&idt_ptr_val));
}

void main() {
    clear_screen();
    print_string("EOS kernel running in 32-bit protected mode\n");
    print_string("Type 'help' for a list of commands\n");
    print_string("> ");

    init_gdt();
    init_idt();
    init_pic();

    __asm__ volatile ("sti");

    while (1) {
        
    }
}
