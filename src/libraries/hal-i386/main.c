#include <kernel.h>
#include "basic_display.h"
#include "basic_uart.h"
#include "ports.h"
#include <stddef.h>

static uint32_t magic;
static void *arg;

extern size_t *kernel_end;

size_t *hal_badmalloc_start_address()
{
    return kernel_end;
}

uint32_t hal_get_magic()
{
    return magic;
}

void hal_store_magic(uint32_t magic_, void *arg_)
{
    magic = magic_;
    arg   = arg_;
}

void hal_init()
{
    hal_basic_display_init();
    hal_basic_uart_init();
}

void hal_shutdown()
{
    kprint("\r\n");
    kprint("TODO: Implement an ACPI-based hal_shutdown().");
}

void hal_disable_interrupts()
{
	__asm__ volatile ("cli");
}

void hal_hard_shutdown()
{
	kprint("\r\n");
	kprint("Doing a hard shutdown.\r\n");

	hal_disable_interrupts();

	while ((hal_inb(0x64) & 2) != 0) {
		// Wait until condition is true before continuing.
	}

	hal_outb(0x64, 0xD1);

	while ((hal_inb(0x64) & 2) != 0) {
		// Wait until condition is true before continuing.
	}

	hal_outb(0x60, 0xFE); // Keyboard RESET.
}

// Used to shut down after test failures.
void hal_test_fail_shutdown()
{
    // Try qemu-specific shutdown that returns a nonzero exit code.
    hal_outb(0xf4, 0x00);
    // Then fall through to a typical hard shutdown, if that didn't work.
    hal_hard_shutdown();
}

void kprint(const char *string)
{
    hal_basic_display_print(string);
    hal_basic_uart_print(string);
}
