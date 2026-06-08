// written by saphhic.
// fb_externcmds.h — Declaraciones para comandos externos del framebuffer

#ifndef FB_EXTERNCMDS_H
#define FB_EXTERNCMDS_H

typedef void (*ExtCmdOutputCallback)(const char* str);
void extern_cmd_execute(const char* command, ExtCmdOutputCallback output_cb);

#endif // FB_EXTERNCMDS_H