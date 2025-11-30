# BaseOS

A simple 32-bit x86 kernel written in C and Assembly.

I started this project to learn more about low-level systems programming and how operating systems work under the hood. It's currently a work in progress but boots into a simple GUI-like menu system.

## Features

- **Bootloader**: Custom 16-bit real mode bootloader that switches to 32-bit protected mode.
- **Kernel**: Minimal C kernel.
- **Drivers**:
  - VGA Text Mode (80x25) with custom window rendering.
  - PS/2 Keyboard polling driver.
  - Serial Port (COM1) for debugging.
- **Interface**: Simple TUI (Text User Interface) with window management and menu navigation.

## Building and Running

You'll need a cross-compiler (`i686-elf-gcc` or `x86_64-elf-gcc`) and `nasm`. I use QEMU for emulation.

### Prerequisites (macOS)

```bash
brew install x86_64-elf-gcc nasm qemu
```

### Build & Run

To build the floppy image and run it in QEMU:

```bash
make run
```

To just build the image:

```bash
make
```

## Project Structure

- `boot.asm`: The bootloader. Loads the kernel from disk and jumps to protected mode.
- `kernel.c`: The main kernel source. Handles VGA, IO, and the UI logic.
- `kernel_entry.asm`: Assembly stub that calls the C `kmain` function.
- `Makefile`: Build script.

## License

MIT. Feel free to use this code for your own OSDev experiments!
