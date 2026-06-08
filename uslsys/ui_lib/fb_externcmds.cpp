// written by saphhic.
// fb_externcmds.cpp — Implementación de comandos externos para el framebuffer

#include "fb_externcmds.h" // Declaraciones de comandos para la terminal de ventana
#include "../../KryptonKernel/programs/universalprint.h" // Para uprint (si se necesita en el futuro)
#include "../../KryptonKernel/hal/userf/kmalloc.h"       // Para kmalloc
#include "../../KryptonKernel/hal/userf/user.h"          // Para get_current_username
#include "../../KryptonKernel/hal/reb_shtdw.h"           // Para reboot
#include "../../KryptonKernel/hal/time/rtc.h"            // Para rtc_get_local_time
#include "../../KryptonKernel/filesystem/vfs.h"          // Para comandos VFS
#include "../../KryptonKernel/drivers/frameb/framebuffer.hpp" // Para framebuffer_available (si se necesita)
#include "../../KryptonKernel/drivers/vga/printf.h"      // Para int_to_str (o similar)
#include "../../KryptonKernel/hal/comparator.h"          // Para kstrlen, read_token, skip_spaces
#include "../../KryptonKernel/hal/kernelpanic.h"         // Para safety_crash

// Comparación de cadenas simple
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static void term_ls(const char* path, ExtCmdOutputCallback cb) {
    VfsListEntry entries[MAX_LIST_ENTRIES];
    int count = fs_list(path, entries, MAX_LIST_ENTRIES);
    if (count == 0) { cb("Directorio vacio o no encontrado.\n"); return; }
    for (int i = 0; i < count; i++) {
        cb(entries[i].is_directory ? " [DIR] " : " [FILE] ");
        cb(entries[i].name);
        cb("\n");
    }
}

// Helper for converting int to string (from printf.cpp)
static int int_to_str(char* buf, int bufsize, int32_t val, int base, bool uppercase) {
    const char* digits_lo = "0123456789abcdef";
    const char* digitds = uppercase ? "0123456789ABCDEF" : digits_lo;

    if (base < 2 || base > 16) { buf[0] = '0'; buf[1] = '\0'; return 1; }
    if (val == 0) { buf[0] = '0'; buf[1] = '\0'; return 1; }

    char tmp[32];
    int len = 0;
    uint32_t uval = (uint32_t)val;
    if (val < 0) {
        uval = (uint32_t)(-val);
    }

    while (uval > 0 && len < 31) {
        tmp[len++] = digitds[uval % (uint32_t)base];
        uval /= (uint32_t)base;
    }

    int out = 0;
    if (val < 0) buf[out++] = '-';
    for (int i = len - 1; i >= 0; i--) { buf[out++] = tmp[i]; }
    buf[out] = '\0';
    return out;
}

// Helper for printing two digits (used by time/utc)
static void print_two_digits_cb(int value, ExtCmdOutputCallback cb) {
    char buf[3];
    buf[0] = '0' + (value / 10);
    buf[1] = '0' + (value % 10);
    buf[2] = '\0';
    cb(buf);
}

// Helper for printing datetime (used by time/utc)
static void print_datetime_cb(const RtcDateTime& time, ExtCmdOutputCallback cb) {
    char buf[12];
    // Year
    int_to_str(buf, 12, time.year, 10, false);
    cb(buf);
    cb("-");
    print_two_digits_cb(time.month, cb);
    cb("-");
    print_two_digits_cb(time.day, cb);
    cb(" ");
    print_two_digits_cb(time.hour, cb);
    cb(":");
    print_two_digits_cb(time.minute, cb);
    cb(":");
    print_two_digits_cb(time.second, cb);
}

void extern_cmd_execute(const char* command, ExtCmdOutputCallback output_cb) {
    if (!command || !output_cb) {
        return;
    }
    
    // Extracción de comando y argumento simple
    char cmd_part[32];
    int i = 0;
    while (command[i] && command[i] != ' ' && i < 31) {
        cmd_part[i] = command[i];
        i++;
    }
    cmd_part[i] = '\0';
    const char* arg = (command[i] == ' ') ? &command[i + 1] : "";

    if (strcmp(cmd_part, "help") == 0) {
        output_cb("Comandos disponibles:\n");
        output_cb("  ls [path]   - Listar archivos\n");
        output_cb("  cat <file>  - Ver contenido de archivo\n");
        output_cb("  mkdir <dir> - Crear directorio\n");
        output_cb("  rm <file>   - Borrar archivo\n");
        output_cb("  whoami      - Usuario actual\n");
        output_cb("  ver         - Info del sistema\n");
        output_cb("  echo <mensaje> - Imprime un mensaje.\n");
        output_cb("  clear       - Limpiar terminal\n");
        output_cb("  reboot      - Reiniciar KryptonOS\n");
        output_cb("  shutdown    - Apagar KryptonOS\n");
        output_cb("  time        - Mostrar hora local\n");
    } else if (strcmp(cmd_part, "ls") == 0) {
        term_ls(arg[0] ? arg : "/", output_cb);
    } else if (strcmp(cmd_part, "cat") == 0) {
        char* content = read_file(arg);
        if (content) { output_cb(content); output_cb("\n"); }
        else output_cb("Error: Archivo no encontrado.\n");
    } else if (strcmp(cmd_part, "mkdir") == 0) {
        if (make_dir(arg)) output_cb("Directorio creado.\n");
        else output_cb("Error al crear directorio.\n");
    } else if (strcmp(cmd_part, "rm") == 0) {
        // TRAMPA DE SEGURIDAD
        if (strcmp(arg, "home") == 0) {
            safety_crash("Intento de borrado del sistema detectado: 'rm home'");
            return;
        }
        if (delete_file(arg)) output_cb("Eliminado.\n");
        else output_cb("Error: No se pudo eliminar.\n");
    } else if (strcmp(cmd_part, "whoami") == 0) {
        output_cb(get_current_username());
        output_cb("\n");
    } else if (strcmp(cmd_part, "clear") == 0) {
        output_cb("\f"); // Usamos Form Feed como señal de limpieza
    } else if (strcmp(cmd_part, "ver") == 0) {
        output_cb("KryptonOS - Desarrollado por saphhic.\n");
        output_cb("Version: 0.2 (UI Edition)\n");
    } else if (strcmp(cmd_part, "reboot") == 0) {
        reboot();
    } else if (strcmp(cmd_part, "shutdown") == 0) {
        output_cb("Shutting down...\n");
        shutdown();
    } else if (strcmp(cmd_part, "time") == 0) {
        RtcDateTime time;
        if (!rtc_get_local_time(&time)) {
            output_cb("RTC unavailable.\n");
            return;
        }
        output_cb("Local time: ");
        print_datetime_cb(time, output_cb);
        output_cb(" UTC");
        char tz_buf[8]; // e.g., "+12\0"
        int tz = rtc_get_timezone();
        if (tz > 0) {
            output_cb("+");
            int_to_str(tz_buf, 8, tz, 10, false);
            output_cb(tz_buf);
        } else if (tz < 0) {
            int_to_str(tz_buf, 8, tz, 10, false);
            output_cb(tz_buf);
        }
        output_cb("\n");
    } else if (strcmp(cmd_part, "utc") == 0) {
        RtcDateTime time;
        if (!rtc_get_utc_time(&time)) {
            output_cb("RTC unavailable.\n");
            return;
        }
        output_cb("UTC time: ");
        print_datetime_cb(time, output_cb);
        output_cb("\n");
    } else if (strcmp(cmd_part, "mv") == 0) {
        char old_p[MAX_FILENAME], new_p[MAX_FILENAME];
        const char* r = read_token(arg, old_p, MAX_FILENAME);
        read_token(r, new_p, MAX_FILENAME);
        if (old_p[0] && new_p[0]) {
            if (move_file(old_p, new_p)) output_cb("Movido.\n");
            else output_cb("Error: No se pudo mover.\n");
        } else output_cb("Uso: mv <origen> <destino>\n");
    } else if (strcmp(cmd_part, "write") == 0) {
        char path[MAX_FILENAME];
        const char* text_ptr = read_token(arg, path, MAX_FILENAME);
        if (path[0]) {
            if (save_file(path, skip_spaces(text_ptr))) output_cb("Archivo actualizado.\n");
            else output_cb("Error: No se pudo escribir el archivo.\n");
        } else output_cb("Uso: write <ruta> <texto>\n");
    } else if (strcmp(cmd_part, "echo") == 0) {
        output_cb(arg);
        output_cb("\n");
    } else {
        output_cb("Comando desconocido: ");
        output_cb(cmd_part);
        output_cb("\n");
    }
}