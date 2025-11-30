/*
 * BaseOS Kernel
 * Main entry point and hardware abstraction layer.
 *
 * This is a simple kernel implementation for my personal OS project.
 * It handles basic VGA text mode rendering, keyboard input, and a simple GUI.
 */

// --- Types & Hardware Constants ---

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDR (uint8_t *)0xB8000

enum VgaColor {
  COLOR_BLACK = 0,
  COLOR_BLUE = 1,
  COLOR_GREEN = 2,
  COLOR_CYAN = 3,
  COLOR_RED = 4,
  COLOR_MAGENTA = 5,
  COLOR_BROWN = 6,
  COLOR_LIGHT_GREY = 7,
  COLOR_DARK_GREY = 8,
  COLOR_LIGHT_BLUE = 9,
  COLOR_LIGHT_GREEN = 10,
  COLOR_LIGHT_CYAN = 11,
  COLOR_LIGHT_RED = 12,
  COLOR_LIGHT_MAGENTA = 13,
  COLOR_YELLOW = 14,
  COLOR_WHITE = 15,
};

#define VGA_COLOR(fg, bg) ((bg << 4) | fg)

// I/O Ports
#define COM1_PORT 0x3f8
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_DATA_PORT 0x60
#define QEMU_SHUTDOWN_PORT 0x604
#define VBOX_SHUTDOWN_PORT 0xB004

// Keycodes
#define KEY_ESC 0x01
#define KEY_ENTER 0x1C
#define KEY_UP 0x48
#define KEY_DOWN 0x50

// --- Low Level I/O ---

static inline void outb(uint16_t port, uint8_t val) {
  asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline void outw(uint16_t port, uint16_t val) {
  asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

// --- Serial Debugging ---

void serial_init() {
  outb(COM1_PORT + 1, 0x00); // Disable interrupts
  outb(COM1_PORT + 3, 0x80); // Enable DLAB (set baud rate divisor)
  outb(COM1_PORT + 0, 0x03); // Set divisor to 3 (lo byte) 38400 baud
  outb(COM1_PORT + 1, 0x00); //                  (hi byte)
  outb(COM1_PORT + 3, 0x03); // 8 bits, no parity, one stop bit
  outb(COM1_PORT + 2, 0xC7); // Enable FIFO, clear them, with 14-byte threshold
  outb(COM1_PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

int serial_is_transmit_empty() { return inb(COM1_PORT + 5) & 0x20; }

void serial_write(char a) {
  while (serial_is_transmit_empty() == 0)
    ;
  outb(COM1_PORT, a);
}

void kprint_debug(const char *str) {
  while (*str) {
    serial_write(*str++);
  }
}

// --- String Utils ---

int kstrlen(const char *str) {
  int len = 0;
  while (str[len])
    len++;
  return len;
}

int kstrcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }
  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

// --- VGA Driver ---

void vga_clear(uint8_t color) {
  uint8_t *vid_mem = VGA_ADDR;
  for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
    vid_mem[i * 2] = ' ';
    vid_mem[i * 2 + 1] = color;
  }
}

void vga_print_at(const char *str, int col, int row, uint8_t color) {
  if (col >= VGA_WIDTH || row >= VGA_HEIGHT)
    return;

  uint8_t *vid_mem = VGA_ADDR + (row * VGA_WIDTH + col) * 2;
  while (*str) {
    if (col >= VGA_WIDTH)
      break;
    *vid_mem++ = *str++;
    *vid_mem++ = color;
    col++;
  }
}

void vga_print_centered(const char *str, int row, uint8_t color) {
  int len = kstrlen(str);
  int col = (VGA_WIDTH - len) / 2;
  if (col < 0)
    col = 0;
  vga_print_at(str, col, row, color);
}

/*
 * Draws a window with a title bar and a drop shadow.
 * Uses standard ASCII box drawing characters.
 */
void gui_draw_window(int x, int y, int w, int h, const char *title) {
  uint8_t border_color = VGA_COLOR(COLOR_BLACK, COLOR_WHITE);
  uint8_t content_color = VGA_COLOR(COLOR_BLACK, COLOR_WHITE);
  uint8_t shadow_color = VGA_COLOR(COLOR_BLACK, COLOR_BLACK);

  // Drop shadow
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      uint8_t *ptr = VGA_ADDR + ((y + 1 + i) * VGA_WIDTH + (x + 1 + j)) * 2;
      *ptr++ = ' ';
      *ptr++ = shadow_color;
    }
  }

  // Window background
  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      uint8_t *ptr = VGA_ADDR + ((y + i) * VGA_WIDTH + (x + j)) * 2;
      *ptr++ = ' ';
      *ptr++ = content_color;
    }
  }

  // Borders
  // Corners
  uint8_t *ptr = VGA_ADDR + (y * VGA_WIDTH + x) * 2;
  *ptr++ = 218;
  *ptr++ = border_color; // Top-Left

  ptr = VGA_ADDR + (y * VGA_WIDTH + (x + w - 1)) * 2;
  *ptr++ = 191;
  *ptr++ = border_color; // Top-Right

  ptr = VGA_ADDR + ((y + h - 1) * VGA_WIDTH + x) * 2;
  *ptr++ = 192;
  *ptr++ = border_color; // Bottom-Left

  ptr = VGA_ADDR + ((y + h - 1) * VGA_WIDTH + (x + w - 1)) * 2;
  *ptr++ = 217;
  *ptr++ = border_color; // Bottom-Right

  // Edges
  for (int i = 1; i < w - 1; i++) {
    ptr = VGA_ADDR + (y * VGA_WIDTH + (x + i)) * 2;
    *ptr++ = 196;
    *ptr++ = border_color; // Top

    ptr = VGA_ADDR + ((y + h - 1) * VGA_WIDTH + (x + i)) * 2;
    *ptr++ = 196;
    *ptr++ = border_color; // Bottom
  }

  for (int i = 1; i < h - 1; i++) {
    ptr = VGA_ADDR + ((y + i) * VGA_WIDTH + x) * 2;
    *ptr++ = 179;
    *ptr++ = border_color; // Left

    ptr = VGA_ADDR + ((y + i) * VGA_WIDTH + (x + w - 1)) * 2;
    *ptr++ = 179;
    *ptr++ = border_color; // Right
  }

  // Title bar
  if (title) {
    int title_len = kstrlen(title);
    int title_start = x + (w - title_len) / 2;
    vga_print_at(title, title_start, y + 1,
                 VGA_COLOR(COLOR_BLACK, COLOR_WHITE));

    // Separator line
    for (int i = 0; i < w; i++) {
      ptr = VGA_ADDR + ((y + 2) * VGA_WIDTH + (x + i)) * 2;
      if (i == 0) {
        *ptr++ = 195;
        *ptr++ = border_color; // Left T
      } else if (i == w - 1) {
        *ptr++ = 180;
        *ptr++ = border_color; // Right T
      } else {
        *ptr++ = 196;
        *ptr++ = border_color; // Horizontal
      }
    }
  }
}

// --- Keyboard Driver ---

uint8_t keyboard_read_scancode() {
  // Wait for buffer to be full
  while ((inb(KEYBOARD_STATUS_PORT) & 1) == 0)
    ;
  return inb(KEYBOARD_DATA_PORT);
}

// --- Main Kernel Logic ---

enum AppState { STATE_MENU = 0, STATE_HELLO, STATE_HELP, STATE_ABOUT };

const char *MENU_ITEMS[] = {"HELP", "HELLO", "ABOUT", "SHUTDOWN"};
const int MENU_COUNT = 4;

void draw_menu_items(int x, int y, int selected) {
  uint8_t color = VGA_COLOR(COLOR_BLACK, COLOR_WHITE);

  for (int i = 0; i < MENU_COUNT; i++) {
    int item_y = y + i;

    // Draw cursor
    if (i == selected) {
      vga_print_at(">", x, item_y, color);
    } else {
      vga_print_at(" ", x, item_y, color);
    }

    vga_print_at(MENU_ITEMS[i], x + 2, item_y, color);
  }
}

void kmain() {
  serial_init();
  kprint_debug("Kernel started.\n");

  int state = STATE_MENU;
  int selected_item = 0;
  int dirty = 1;       // Needs redraw?
  int full_redraw = 1; // Needs full background clear?

  while (1) {
    if (dirty) {
      if (full_redraw) {
        // Desktop background
        vga_clear(VGA_COLOR(COLOR_WHITE, COLOR_CYAN));
        full_redraw = 0;
      }

      if (state == STATE_MENU) {
        int w = 30, h = 10;
        int x = (VGA_WIDTH - w) / 2;
        int y = (VGA_HEIGHT - h) / 2;

        gui_draw_window(x, y, w, h, "Start");
        draw_menu_items(x + 4, y + 4, selected_item);

      } else if (state == STATE_HELLO) {
        int w = 40, h = 12;
        int x = (VGA_WIDTH - w) / 2;
        int y = (VGA_HEIGHT - h) / 2;

        gui_draw_window(x, y, w, h, "Hello");
        vga_print_centered("HELLO WORLD", y + 5,
                           VGA_COLOR(COLOR_BLACK, COLOR_WHITE));
        vga_print_centered("Press ESC to return", y + 8,
                           VGA_COLOR(COLOR_DARK_GREY, COLOR_WHITE));

      } else if (state == STATE_HELP) {
        int w = 50, h = 15;
        int x = (VGA_WIDTH - w) / 2;
        int y = (VGA_HEIGHT - h) / 2;

        gui_draw_window(x, y, w, h, "Help");

        int text_y = y + 4;
        vga_print_at("Use UP/DOWN arrows to navigate.", x + 4, text_y++,
                     VGA_COLOR(COLOR_BLACK, COLOR_WHITE));
        vga_print_at("Press ENTER to select.", x + 4, text_y++,
                     VGA_COLOR(COLOR_BLACK, COLOR_WHITE));
        vga_print_at("Press ESC to go back.", x + 4, text_y++,
                     VGA_COLOR(COLOR_BLACK, COLOR_WHITE));

      } else if (state == STATE_ABOUT) {
        int w = 40, h = 12;
        int x = (VGA_WIDTH - w) / 2;
        int y = (VGA_HEIGHT - h) / 2;

        gui_draw_window(x, y, w, h, "About");

        vga_print_centered("BaseOS Kernel", y + 4,
                           VGA_COLOR(COLOR_BLACK, COLOR_WHITE));
        vga_print_centered("Version 0.1.0", y + 5,
                           VGA_COLOR(COLOR_DARK_GREY, COLOR_WHITE));
        vga_print_centered("(c) 2025 CCG", y + 7,
                           VGA_COLOR(COLOR_BLACK, COLOR_WHITE));
        vga_print_centered("All Rights Reserved", y + 8,
                           VGA_COLOR(COLOR_BLACK, COLOR_WHITE));
      }
      dirty = 0;
    }

    uint8_t sc = keyboard_read_scancode();

    // Ignore key release events (top bit set)
    if (sc & 0x80)
      continue;

    if (state == STATE_MENU) {
      if (sc == KEY_UP) {
        selected_item--;
        if (selected_item < 0)
          selected_item = MENU_COUNT - 1;
        dirty = 1;
      } else if (sc == KEY_DOWN) {
        selected_item++;
        if (selected_item >= MENU_COUNT)
          selected_item = 0;
        dirty = 1;
      } else if (sc == KEY_ENTER) {
        if (selected_item == 0) { // HELP
          state = STATE_HELP;
          dirty = 1;
          full_redraw = 1;
        } else if (selected_item == 1) { // HELLO
          state = STATE_HELLO;
          dirty = 1;
          full_redraw = 1;
        } else if (selected_item == 2) { // ABOUT
          state = STATE_ABOUT;
          dirty = 1;
          full_redraw = 1;
        } else if (selected_item == 3) { // SHUTDOWN
          // Try QEMU then Bochs/VirtualBox shutdown
          outw(QEMU_SHUTDOWN_PORT, 0x2000);
          outw(VBOX_SHUTDOWN_PORT, 0x2000);

          // Fallback
          vga_clear(COLOR_BLACK);
          vga_print_centered("System Halted.", 12, COLOR_LIGHT_RED);
          asm volatile("hlt");
        }
      }
    } else {
      if (sc == KEY_ESC) {
        state = STATE_MENU;
        dirty = 1;
        full_redraw = 1;
      }
    }
  }
}