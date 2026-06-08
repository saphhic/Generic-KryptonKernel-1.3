#include "fb_extrncmds.h"
#include "../drivers/frameb/framebuffer.hpp"
#include "../hal/userf/user.h"
#include "../hal/userf/kmalloc.h"
#include "../hal/reb_shtdw.h"
#include "../hal/time/rtc.h"
#include "../filesystem/vfs.h"
#include "../programs/universalprint.h"

#if defined(LAWAI_TARGET_BIOS)
    #include "../drivers/ethernet/ethernet.h"
    #include "../drivers/ethernet/netstack.h"
#endif



void cmd_ls_vfs(const char* path) {
    VfsListEntry entries[MAX_LIST_ENTRIES];
    int count = fs_list(path, entries, MAX_LIST_ENTRIES);
    if (count == 0) { uprint("Empty or not found.\n"); return; }
    for (int i = 0; i < count; i++) {
        uprint(entries[i].is_directory ? "[D] " : "[F] ");
        uprint(entries[i].name);
        uprint("\n");
    }
}

void cmd_mkdir_vfs(const char* path) {
    if (make_dir(path)) uprint("Directory created.\n");
    else uprint("Could not create directory.\n");
}

void cmd_cat_vfs(const char* path) {
    char* content = read_file(path);
    if (content) uprint(content);
    else uprint("File not found.\n");
    uprint("\n");
}

void cmd_rm_vfs(const char* path) {
    if (delete_file(path)) uprint("Deleted.\n");
    else uprint("Could not delete path.\n");
}

void cmd_mv_vfs(const char* old_path, const char* new_path) {
    if (move_file(old_path, new_path)) uprint("Moved.\n");
    else uprint("Could not move path.\n");
}

void cmd_write_vfs(const char* path, const char* text, bool append) {
    bool ok = append ? append_file(path, text) : save_file(path, text);
    if (ok) uprint("File updated.\n");
    else uprint("Could not write file.\n");
}

void cmd_help() {
    printf_fb("Available commands:\n");
    printf_fb("  clear - Clear the screen\n");
    printf_fb("  help - Show this help message\n");
    printf_fb("  whoami - Display the current username\n");
    printf_fb("  time - Display local RTC time\n");
    printf_fb("  utc - Display UTC RTC time\n");
    printf_fb("  reboot - Reboot the system\n");
    printf_fb("  shutdown - Shutdown the system\n");
    printf_fb("  ls - List files in the current directory\n");
    printf_fb("  cat - Display the contents of a file\n");
    printf_fb("  mkdir - Create a new directory\n");
    printf_fb("  rm - Remove a file or directory\n");
#if defined(LAWAI_TARGET_BIOS)
    printf_fb("  netinfo - Display network information\n");
    printf_fb("  ethinfo - Display Ethernet information\n");
#endif
}



void cmd_whoami() {
    const char* username = get_current_username();
    printf_fb("You are logged in as: ");
    printf_fb(username);
    printf_fb("\n");
}

void cmd_clear() {
    fb_clear_screen();
}

void cmd_reboot() {
    printf_fb("Rebooting...\n");
    reboot();
}

void cmd_shutdown() {
    printf_fb("Shutting down...\n");
    shutdown();
}

static void print_two_digits(int value) {
    if (value < 10) {
        printf_fb("0");
    }
    printf_fb_d("%d", value);
}

static void print_datetime(const RtcDateTime& time) {
    printf_fb_d("%d", time.year);
    printf_fb("-");
    print_two_digits(time.month);
    printf_fb("-");
    print_two_digits(time.day);
    printf_fb(" ");
    print_two_digits(time.hour);
    printf_fb(":");
    print_two_digits(time.minute);
    printf_fb(":");
    print_two_digits(time.second);
}

void cmd_time() {
    RtcDateTime time;
    if (!rtc_get_local_time(&time)) {
        printf_fb("RTC unavailable.\n");
        return;
    }

    printf_fb("Local time: ");
    print_datetime(time);
    printf_fb(" UTC");
    int tz = rtc_get_timezone();
    if (tz > 0) {
        printf_fb("+");
        printf_fb_d("%d", tz);
    } else if (tz < 0) {
        printf_fb_d("%d", tz);
    }
    printf_fb("\n");
}

void cmd_utc() {
    RtcDateTime time;
    if (!rtc_get_utc_time(&time)) {
        printf_fb("RTC unavailable.\n");
        return;
    }

    printf_fb("UTC time: ");
    print_datetime(time);
    printf_fb("\n");
}

#if defined(LAWAI_TARGET_BIOS)
void cmd_netinfo() {
    printf_fb("Network Information:\n");
    netstack_debug_print_info();
}

void cmd_ethinfo() {
    printf_fb("Ethernet Information:\n");
    ethernet_debug_print_status();
}    
#endif
