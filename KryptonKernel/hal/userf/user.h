// written by saphhic.
#ifndef USER_H
#define USER_H

#include <stdint.h>
#include <stdbool.h>

#define MAX_USERS 16
#define MAX_USERNAME 24

#define MAX_PASSWORD 64
#define MIN_PASSWORD_LEN 3

enum UserRole : uint8_t {
    ROLE_ROOT = 0,
    ROLE_USER = 1,
    ROLE_GUEST = 2
};


struct User {
    char username[MAX_USERNAME];
    uint32_t password_hash;
    int uid;
    UserRole role;
    uint8_t failed_attempts;
    uint8_t login_count;
};

enum LoginResult {
    LOGIN_OK = 0,
    LOGIN_NO_USER = 1,
    LOGIN_BAD_PASS = 2,
    LOGIN_LOCKED = 3,
    LOGIN_ERROR = 4,
};

enum UserCreationResult {
    CREATE_OK = 0,
    CREATE_EXISTS = 1,
    CREATE_FULL = 2,
    CREATE_BAD_NAME = 3,
    CREATE_BAD_PASS = 4,
    CREATE_ERROR = 5,
};

extern struct User users[MAX_USERS];
extern int user_count;
extern int current_uid;

bool check_login();
bool check_priv();
LoginResult login_user(const char* username, const char* password);
bool delete_user(const char* username);
bool create_privuser(const char* username, const char* password);
UserCreationResult create_user(const char* username, const char* password, UserRole role);
UserCreationResult create_normal_user(const char* username, const char* password);
void init_default_user();
void list_users();

UserRole get_current_role();
const User* get_current_user();
const char* get_current_username();

void logout_user();
void reset_lockout();


#endif // USER_H

