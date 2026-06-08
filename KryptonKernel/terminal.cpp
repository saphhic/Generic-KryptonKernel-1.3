// written by saphhic.
#include "drivers/vga/printf.h"
#include "drivers/vga/keymap.h"
#include "hal/reb_shtdw.h"
#include "hal/userf/user.h"
#include "hal/startup.h"
#include "hal/comparator.h"
#if defined(LAWAI_TARGET_BIOS)
#include "drivers/ethernet/ethernet.h"
#include "drivers/ethernet/netstack.h"
#endif
    
void terminal_main() {
    kb_set_framebuffer_echo(false);
    printf("vga shell started. type 'clear' to clear the screen, 'shutdown' to power off, 'reboot' to restart.\n");
    printf("\n\n");

    while (1) {
#if defined(LAWAI_TARGET_BIOS)
        netstack_poll();
#endif
        printf("home/> ");

        char* input = read_line();
        if (input == nullptr || input[0] == '\0') {
            continue;
        }

        if (compare(input, "clear") != false) {
            clear_screen();
            continue;
        }

        if (compare(input, "shutdown") != false) {
            printf("shutting down...\n");
            shutdown();
            continue;
        }

        if (compare(input, "reboot") != false) {
            printf("rebooting...\n");
            reboot();
            continue;
        }

        if (compare(input, "help") != false) {
            printf("available commands:\n");
            printf("  clear    - clear the screen\n");
            printf("  shutdown - power off the machine\n");
            printf("  reboot   - restart the machine\n");
            printf("  help     - show this message\n");
            printf("  dvga     - framebuffer handoff is unavailable in this build\n");
#if defined(LAWAI_TARGET_BIOS)
            printf("  ethinfo  - show ethernet driver status\n");
            printf("  ethpoll  - poll the NIC and print queued ethernet frames\n");
            printf("  netinfo  - show ARP/IPv4/ICMP stack status\n");
            printf("  netqemu  - load QEMU user-net IPv4 defaults\n");
            printf("  arpgw    - resolve the configured gateway with ARP\n");
            printf("  pinggw   - send ICMP echo to the configured gateway\n");
#endif
            continue;
        }

        if (compare(input, "dvga") != false) {
            printf("framebuffer handoff is unavailable in this build.\n");
            continue;
        }

#if defined(LAWAI_TARGET_BIOS)
        if (compare(input, "ethinfo")) {
            ethernet_debug_print_status();
            continue;
        }

        if (compare(input, "ethpoll")) {
            ethernet_debug_drain_frames();
            continue;
        }

        if (compare(input, "netinfo")) {
            netstack_debug_print_info();
            continue;
        }

        if (compare(input, "netqemu")) {
            netstack_debug_use_qemu_user_defaults();
            continue;
        }

        if (compare(input, "arpgw")) {
            netstack_debug_resolve_gateway();
            continue;
        }

        if (compare(input, "pinggw")) {
            netstack_debug_ping_gateway();
            continue;
        }
#endif

        printf(input);
        printf("\n");
    }
}
