# Eron OS

A 32-bit operating system kernel built from scratch in C and x86 Assembly.

## Architecture

```
src/
  boot/
    boot.S          Multiboot entry point, IRQ/syscall stubs
    linker.ld       Memory layout (loads at 0x100000)
  kernel/
    kernel.c        Kernel init, /proc readers, boot sequence
    tty.c           VGA text-mode terminal (80x25)
    idt.c           Interrupt Descriptor Table (256 gates)
    teclado.c       PS/2 keyboard driver (scancode set 1)
    shell.c         Shell (esh) with program dispatcher
    mm.c            Heap memory manager (kmalloc/kfree)
    timer.c         PIT timer at 100 Hz
    vfs.c           Virtual filesystem (dirs, files, devices)
    task.c          Program registration framework
    syscall.c       System call handler (INT 0x80)
    display.c       Display abstraction layer
  include/
    config.h        System constants
    vga.h           VGA color/entry macros
    io.h            Port I/O (inb/outb)
    idt.h           IDT structures
    teclado.h       Keyboard interface
    mm.h            Memory manager API
    timer.h         Timer API
    vfs.h           VFS API
    task.h          Program framework API
    syscall.h       Syscall numbers
    display.h       Display abstraction (text + future framebuffer)
```

## Subsystems

### Memory Manager (`mm.c`)
Heap allocator with 1 MB pool starting at `_kernel_end`. Supports `kmalloc(size)` and `kfree(ptr)` with 8-byte alignment and block headers with magic number validation.

### Virtual Filesystem (`vfs.c`)
Flat-array VFS with support for directories, files (static and dynamic), and device nodes. Populated at boot with `/etc`, `/proc`, `/dev`, `/home`, `/bin`, `/tmp`, `/var`. Dynamic `/proc` entries (`cpuinfo`, `meminfo`, `uptime`) generate content on read.

### Program Framework (`task.c`)
Programs are registered with `program_register(name, description, entry_fn)`. The shell dispatches commands by looking up the program table. Adding a new command requires only:

```c
static int my_program(int argc, char* argv[]) {
    terminal_writestring("Hello from my program\n");
    return 0;
}

// In shell_register_programs():
program_register("myprogram", "description", my_program);
```

### System Calls (`syscall.c`)
INT 0x80 handler with dispatch table. Current syscalls:
- `SYS_WRITE (1)` — write to terminal
- `SYS_MALLOC (5)` — allocate heap memory
- `SYS_FREE (6)` — free heap memory
- `SYS_TIME (9)` — get uptime in seconds
- `SYS_GETPID (4)` — get process ID

### Display Abstraction (`display.h`)
Current mode: VGA text (80x25, 16 colors, buffer at 0xB8000). The `display.h` header defines the interface for future framebuffer support via VESA LFB.

## Shell Commands (21 programs)

| Command | Description |
|---------|-------------|
| `help` | List all registered programs |
| `about` | System information |
| `clear` | Clear terminal |
| `uname [-a\|-r]` | Kernel identification |
| `uptime` | System uptime (H:MM:SS) |
| `date` | RTC date/time |
| `free` | Heap memory usage |
| `cpuid` | CPU vendor, family, model, flags |
| `whoami` | Current user |
| `hostname` | System hostname |
| `pwd` | Current directory |
| `ls [dir]` | List VFS directory contents |
| `cat <file>` | Read VFS file (supports /proc) |
| `echo <text>` | Print text |
| `ps` | Process list |
| `id` | User/group info |
| `arch` | CPU architecture |
| `display` | Display mode and buffer info |
| `memtest` | Test heap allocator |
| `reboot` | Restart system |
| `halt` | Power off |

## Building

```bash
# Requirements: gcc (multilib), binutils, grub-mkrescue, xorriso, qemu
sudo apt install gcc gcc-multilib binutils grub-pc-bin grub-common xorriso qemu-system-x86

make clean && make
```

## Running

```bash
# Direct kernel boot
qemu-system-i386 -kernel eron.bin -m 128M

# ISO boot (with GRUB)
qemu-system-i386 -cdrom eron.iso -m 128M
```

## Adding Programs

1. Write a program function in `shell.c`:
```c
static int prog_example(int argc, char* argv[]) {
    terminal_writestring("\nHello from example");
    return 0;
}
```

2. Register it in `shell_register_programs()`:
```c
program_register("example", "example program", prog_example);
```

3. The program appears automatically in `help` and can be called from the shell.

## Adding Filesystem Entries

```c
// In vfs_populate() in kernel.c:
vfs_mkdir("/mydir");
vfs_mkfile("/mydir/readme", "File content here");
vfs_mkdev("/dev/mydevice");
vfs_mkproc("/proc/mydata", my_read_function);
```

## Future: GUI Support

The display abstraction (`display.h`) is designed for future graphical mode:

1. Request VESA LFB in multiboot header
2. Implement `display_init_fb()` with framebuffer address
3. Add pixel primitives (`putpixel`, `fillrect`, `bitblt`)
4. Build a window manager on top
5. Switch `display_mode` to `DISPLAY_FRAMEBUFFER`

## License

MIT

## Author

Llucs
