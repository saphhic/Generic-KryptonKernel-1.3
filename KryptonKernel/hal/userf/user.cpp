// written by saphhic.
#include "user.h"
#include "../../drivers/vga/printf.h"

User users[MAX_USERS];
int user_count = 0;
int current_uid = -1;

static int fail_count = 0;
static int next_uid = 0;
#define MAX_FAIL_ATTEMPTS 5

static int kstrlen(const char* s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

static bool kstreq(const char* a, const char* b) {
    int i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) return false;
        i++;
    }
    return a[i] == '\0' && b[i] == '\0';
}

static void kstrcopy(char* dst, const char* src, int maxlen) {
    int i = 0;
    while (i < maxlen - 1 && src && src[i]) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

static uint32_t djb2(const char* str) {
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + (uint8_t)(*str);
        str++;
    }
    return hash;
}

static uint32_t hash_password(const char* password, const char* salt) {
    char buf[MAX_PASSWORD + MAX_USERNAME + 2];
    int pi = 0, si = 0, bi = 0;
    while (password[pi] && bi < (int)sizeof(buf) - 2) {
        buf[bi++] = password[pi++];
    }
    while (salt[si] && bi < (int)sizeof(buf) - 1) {
        buf[bi++] = salt[si++];
    }
    buf[bi] = '\0';

    return djb2(buf);
}

static int find_user(const char* username) {
    for (int i = 0; i < user_count; i++) {
        if (kstreq(users[i].username, username)) {
            return i;
        }
    }
    return -1;
}

bool check_login() {
    return current_uid >= 0;
}

bool check_priv() {
    const User* user = get_current_user();
    return user != nullptr && user->role == ROLE_ROOT;
}

UserRole get_current_role() {
    if (current_uid < 0) return ROLE_GUEST;
    return users[current_uid].role;
}

const User* get_current_user() {
    if (current_uid < 0) return nullptr;
    for (int i = 0; i < user_count; i++) {
        if (users[i].uid == current_uid) {
            return &users[i];
        }
    }
    return nullptr;
}

const char* get_current_username() {
    const User* user = get_current_user();
    return user ? user->username : "unknown";
}

LoginResult login_user(const char* username, const char* password) {
    if (!username || !password) return LOGIN_ERROR;

    if (fail_count >= MAX_FAIL_ATTEMPTS) {
        return LOGIN_LOCKED;
    }

    if (kstrlen(password) < MIN_PASSWORD_LEN) {
        return LOGIN_BAD_PASS;
    }

    int idx = find_user(username);
    if (idx < 0) {
        fail_count++;
        return LOGIN_NO_USER;
    }

    uint32_t hash = hash_password(password, username);
    if (users[idx].password_hash != hash) {
        users[idx].failed_attempts++;
        fail_count++;
        return LOGIN_BAD_PASS;
    }

    current_uid = users[idx].uid;
    fail_count = 0;
    users[idx].failed_attempts = 0;
    users[idx].login_count++;
    return LOGIN_OK;
}

void logout_user() {
    current_uid = -1;
}

UserCreationResult create_user(const char* username, const char* password, UserRole role) {
    if (!username || !password) return CREATE_ERROR;
    if (user_count >= MAX_USERS) return CREATE_FULL;

    int ulen = kstrlen(username);
    int plen = kstrlen(password);

    if (ulen < 1 || ulen >= MAX_USERNAME) return CREATE_BAD_NAME;
    if (plen < MIN_PASSWORD_LEN) return CREATE_BAD_PASS;
    if (plen >= MAX_PASSWORD) return CREATE_BAD_PASS;

    for (int i = 0; i < ulen; i++) {
        char c = username[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) return CREATE_BAD_NAME;
    }

    if (find_user(username) >= 0) return CREATE_EXISTS;

    if (role == ROLE_ROOT && user_count > 0 && !check_priv()) {
        role = ROLE_USER; // Only root can create root users
    }

    User& u = users[user_count];
    kstrcopy(u.username, username, MAX_USERNAME);
    u.password_hash = hash_password(password, username);
    u.uid = next_uid++;
    u.role = role;
    u.failed_attempts = 0;
    u.login_count = 0;

    user_count++;
    return CREATE_OK;
    
}

bool create_privuser(const char* username, const char* password) {
    return create_user(username, password, ROLE_ROOT) == CREATE_OK;
}

UserCreationResult create_normal_user(const char* username, const char* password) {
    return create_user(username, password, ROLE_USER);
}

bool delete_user(const char* username) {
    if (!check_priv()) return false;
    int idx = find_user(username);
    if (idx < 0) return false;

    if (users[idx].uid == current_uid) {
        return false;
    }

    for (int i = idx; i < user_count - 1; i++) {
        users[i] = users[i + 1];
    }
    user_count--;
    return true;
}

void list_users() {
    printf("UID ROLE LOGINS NAME\n");
    printf("___ ____ _______ ____\n");
    for (int i = 0; i < user_count; i++) {
        char uid_s[4];
        uid_s[0] = '0' + (users[i].uid / 10);
        uid_s[1] = '0' + (users[i].uid % 10);
        uid_s[2] = ' '; uid_s[3] = '\0';
        printf(uid_s);
        printf("   ");

        switch (users[i].role) {
            case ROLE_ROOT: printf("ROOT   "); break;
            case ROLE_USER: printf("USER   "); break;
            case ROLE_GUEST: printf("GUEST  "); break;
        }
        printf("   ");

        char lc[4];
        lc[0] = '0' + (users[i].login_count / 10);
        lc[1] = '0' + (users[i].login_count % 10);
        lc[2] = ' '; lc[3] = '\0';
        printf(lc);
        printf("   ");

        printf(users[i].username);
        if (users[i].uid == current_uid) {
            printf(" (*)");
        }
        printf("\n");
    }
}

void init_default_user() {
    if (find_user("root") < 0) {
        create_user("root", "toor", ROLE_ROOT);
    }
}

void reset_lockout() {
    fail_count = 0;
}
