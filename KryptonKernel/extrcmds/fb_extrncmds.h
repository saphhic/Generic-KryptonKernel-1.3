#ifndef FB_EXTRNCMDS_H
#define FB_EXTRNCMDS_H

void cmd_help();
void cmd_whoami();
void cmd_clear();
void cmd_reboot();
void cmd_shutdown();
void cmd_time();
void cmd_utc();

void cmd_ls_vfs(const char* path);
void cmd_cat_vfs(const char* path);
void cmd_mkdir_vfs(const char* path);
void cmd_rm_vfs(const char* path);
void cmd_mv_vfs(const char* old_path, const char* new_path);
void cmd_write_vfs(const char* path, const char* text, bool append);

#if defined(LAWAI_TARGET_BIOS)
    void cmd_netinfo();
    void cmd_ethinfo();
#endif

#endif // FB_EXTRNCMDS_H
