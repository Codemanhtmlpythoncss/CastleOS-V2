CastleOS V2

A hobby operating system written from scratch in C and x86 Assembly.

CastleOS V2 is a learning project focused on understanding how operating systems work at a low level. The project is built without a standard library and runs directly on the hardware through a custom boot process.

Features

Boot System

* Custom bootloader
* Boots directly into CastleOS
* Runs in QEMU and on compatible x86 hardware

Terminal

* VGA text mode output
* Colored text support
* Custom printing functions
* Screen initialization and management

Interrupts

* Interrupt Descriptor Table (IDT)
* CPU exception handling
* Divide-by-zero exception testing

Mathematics

* Custom power function
* Custom square root implementation (ks_sqrt)
* Integer-to-string conversion
* Float-to-string conversion

Cryptography

* AES-256 key generation
* Entropy-based random number generation

Testing

Built-in self-tests verify:

* Random number generation
* Power function correctness
* Square root accuracy
* Exception handling
* AES key generation

Example boot output:

Initialised entropy [ OK ]
Initialised terminal [ OK ]
Initialising interrupt descriptor table [ OK ]
Testing IDT div by zero [ OK ]
Testing randomness [ OK ]
Testing power maths function [ OK ]
Testing SQRT function [ OK ]
AES-256 Key: ...

Project Goals

Current goals include:

* Keyboard driver
* Heap allocator
* Simple shell
* Filesystem support
* RTC (Real Time Clock)
* Memory management improvements
* Multitasking

Building

Requirements

* Docker
* Make
* QEMU
* i686-elf GCC cross compiler

Build and Run

make boot

This will:

1. Build the bootloader
2. Compile the kernel
3. Create a bootable disk image
4. Launch CastleOS in QEMU

Project Structure

src/
├── kernel.c
├── kernel.asm
├── maths.c
├── sqrt.c
├── cryptography.c
├── memory/
├── idt/
└── linker.ld

Why?

CastleOS exists primarily as a learning project to explore:

* Operating system development
* Computer architecture
* Memory management
* Interrupt handling
* Cryptography
* Low-level programming

License

This project is provided for educational purposes.
