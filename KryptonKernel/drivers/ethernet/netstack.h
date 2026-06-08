
//written by saphhic.

#ifndef LAWAI_NETSTACK_H
#define LAWAI_NETSTACK_H

void netstack_init();
void netstack_poll();
void netstack_debug_print_info();
void netstack_debug_use_qemu_user_defaults();
void netstack_debug_resolve_gateway();
void netstack_debug_ping_gateway();

#endif // LAWAI_NETSTACK_H
