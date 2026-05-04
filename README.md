# Eron OS

Sistema operacional educacional em C e Assembly com kernel proprio e interface VGA.

## Caracteristicas

- Boot via GRUB Multiboot
- Kernel monolitico em C (i386, 32-bit)
- Interface VGA modo texto 80x25 com bordas e cores
- Shell interativo com comandos integrados
- Driver de teclado PS/2 com suporte a Shift e Caps Lock
- IDT (Interrupt Descriptor Table) e PIC (Programmable Interrupt Controller)

## Comandos do Shell

| Comando     | Descricao                        |
|-------------|----------------------------------|
| `help`      | Lista todos os comandos          |
| `clear`     | Limpa a tela                     |
| `info`      | Informacoes do sistema           |
| `version`   | Versao do Eron OS                |
| `uptime`    | Tempo desde o boot               |
| `mem`       | Informacoes de memoria           |
| `cpuid`     | Informacoes do processador       |
| `echo`      | Repete o texto digitado          |
| `reboot`    | Reinicia o sistema               |
| `halt`      | Desliga o sistema                |

## Compilacao

### Dependencias

```bash
sudo apt install build-essential gcc-multilib grub-pc-bin grub-common xorriso mtools
```

### Build

```bash
make        # Compila e gera a ISO
make clean  # Remove arquivos de build
```

### Executar no QEMU

```bash
make run
# ou
qemu-system-i386 -cdrom eron.iso -m 128M
```

## Estrutura do Projeto

```
eron/
├── src/
│   ├── boot/
│   │   ├── boot.S        # Ponto de entrada (Assembly)
│   │   ├── linker.ld     # Script do linker
│   │   └── grub.cfg      # Configuracao do GRUB
│   ├── kernel/
│   │   ├── kernel.c      # Funcao principal do kernel
│   │   ├── tty.c         # Driver de terminal VGA
│   │   ├── idt.c         # Tabela de interrupcoes
│   │   ├── teclado.c     # Driver de teclado PS/2
│   │   └── shell.c       # Shell interativo
│   └── include/
│       ├── vga.h         # Definicoes VGA
│       ├── io.h          # Funcoes de I/O (inb/outb)
│       ├── idt.h         # Estruturas da IDT
│       ├── teclado.h     # Header do teclado
│       └── config.h      # Configuracoes do sistema
├── scripts/
│   └── run.sh            # Script de build e execucao
├── Makefile
└── README.md
```

## Licenca

MIT License - veja [LICENSE](LICENSE) para detalhes.
