#include "idt.h"
#include "config.h"
#include "memory/memory.h"
#include "kernel.h"
#include "../io/io.h"
struct idt_desc idt_descriptors[CastleOS_TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;


// Assembly entry points installed into the interrupt descriptor table.
extern void idt_load(struct idtr_desc* ptr);

extern void int21h();
extern void no_interrupt();

void no_interrupt_handler()
{
	// Acknowledge the PIC so it can deliver the next interrupt.
	outb(0x20, 0x20);
}

void int21h_handler()
{
	print("Keyboard pressed!\n", 15);
	// Notify the PIC that the keyboard interrupt has been handled.
	outb(0x20, 0x20);
}

void idt_zero()
{
	// Report the CPU's divide-error exception.
	print("Divide by zero error\n", 15);
}

// Fill one IDT gate with the handler's split 32-bit address and kernel attributes.
void idt_set(int interrupt_no, void* address)
{
	struct idt_desc* desc = &idt_descriptors[interrupt_no];
	desc->offset_1 = (uint32_t) address & 0xffff;
	desc->selector = KERNEL_CODE_SELECTOR;
	desc->zero = 0x00; 
	desc->type_attr = 0xEE;
	desc->offset_2 = (uint32_t) address >> 16;
}

void idt_init()
{
	// Start with empty entries, then point all vectors at the fallback handler.
	memset(idt_descriptors, 0, sizeof(idt_descriptors));
	idtr_descriptor.limit = sizeof(idt_descriptors) -1;
	idtr_descriptor.base = (uint32_t)idt_descriptors;
	
	for (int i = 0; i < CastleOS_TOTAL_INTERRUPTS; i++)
	{
    	idt_set(i, no_interrupt);
	}
	idt_set(0, idt_zero);
	idt_set(0x21, int21h);

	// Load the completed table into the processor.
	idt_load(&idtr_descriptor);

}
