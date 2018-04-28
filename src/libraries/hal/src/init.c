#include <awoo.h>
#include <ali.h>
#include <dmm.h>
#include <ali/event.h>
#include <flail.h>
#include <hal.h>
#include "magic.h"
#include "panic.h"
#include <stddef.h>
#include <stdbool.h>
#include "basic_display.h"
#include "basic_uart.h"
#include "ports.h"
#include "exceptions.h"
#include "gdt.h"
#include "idt.h"
#include "multiboot.h"

static bool hal_initialized = false;

extern size_t kernel_start;
extern size_t kernel_end;

size_t hal_get_kernel_start()
{
    return (size_t)&kernel_start;
}

size_t hal_get_kernel_end()
{
    return (size_t)(&kernel_end) + 1;
}

void hal_init()
{
    hal_basic_display_init();
    hal_basic_uart_init();

    if (!hal_initialized) {
        hal_gdt_init();
        hal_idt_init();

        hal_exceptions_init();

        flail_init(AWOO_INFO, &kprint);
        dmm_init(&_flail_panic);
        hal_init_panic(&_flail_panic);

        // Get the Multiboot info struct
        MultibootInfo *multiboot_info = ((MultibootInfo*)hal_get_arg());

        // If bits 4 and 5 are set, Multiboot did something bad!
        // Bit 4 is set if a.out header is passed, bit 5 is set if ELF header
        // is passed - these should not be able to be set at the same time.
        // If they are, this is usually an indication of an error in the
        // bootloader.
        if (((multiboot_info->flags & (1 << 4)) != 0)
            && ((multiboot_info->flags & (1 << 5)) != 0)) {

            hal_panic("multiboot passed flags with bits 4 and 5 set");
        }

        // We don't want a.out stuff as we're compiled to ELF
        if ((multiboot_info->flags & (1 << 4)) != 0) {
            hal_panic("multiboot passed a.out header when we're expecting ELF");
        }

        // If we don't have ELF section header info, it'll be impossible to do
        // parsing of stabs later on
        if ((multiboot_info->flags & (1 << 5)) == 0) {
            hal_panic("multiboot didn't pass ELF section header info");
        }

        // We expect a memory map to initialize DMM
        if ((multiboot_info->flags & (1 << 6)) == 0) {
            hal_panic("multiboot didn't pass a memory map");
        }

        // Initialize DMM with our available memory regions (as told by multiboot)
        size_t mmap_addr = multiboot_info->mmap_addr;
        size_t mmap_count = ((size_t)multiboot_info->mmap_length) / sizeof(MultibootMemoryMapEntry);

        for (size_t i = 0; i < mmap_count; i++) {
            MultibootMemoryMapEntry *mmap_entry = ((MultibootMemoryMapEntry*)mmap_addr) + i;

            if (mmap_entry->type == MULTIBOOT_MEMORY_MAP_AVAILABLE) {
                // If we get an entry starting at 0x0, make it instead start at
                // 0x1 and decrease it's length by 1 byte.
                if ((size_t)mmap_entry->addr == 0) {
                    mmap_entry->addr += 1;
                    mmap_entry->length -= 1;
                }

                // If we get an entry that contains the kernel, just skip
                // it for the sake of simplicity.
                //
                // An alternative approach would be to split it into two
                // entries -- e.g.,
                //     <start of actual entry> through <kernel start -1>
                //     <kernel end + 1> through <end of actual entry>
                if (((size_t)mmap_entry->addr >= hal_get_kernel_start()) &&
                        ((size_t)mmap_entry->addr <= hal_get_kernel_end())) {
                    continue;
                }

                dmm_add_memory_region((void*)mmap_entry->addr, mmap_entry->length);
            }
        }

        ali_init(&dmm_malloc, &dmm_free, &dmm_realloc);

        event_trigger("HAL interrupts enable", NULL, 0);
    }

    hal_initialized = true;
}
