#include <stdio.h>
#include <string.h>

#define MAX_NAME 64

// Permission flags for a single category (owner, group, or others)
typedef struct {
    int read;
    int write;
    int execute;
} PermissionSet;

typedef struct {
    char filename[MAX_NAME];
    char owner[MAX_NAME];
    char group[MAX_NAME];
    PermissionSet owner_perms;
    PermissionSet group_perms;
    PermissionSet others_perms;
} SecureFile;

// Converts a PermissionSet into the familiar "rwx" style string, e.g. "rw-"
void perms_to_string(PermissionSet p, char* out) {
    out[0] = p.read    ? 'r' : '-';
    out[1] = p.write   ? 'w' : '-';
    out[2] = p.execute ? 'x' : '-';
    out[3] = '\0';
}

// Prints a file's full permission string, e.g. "-rwxr-xr--  alice  staff  report.txt"
void print_permissions(SecureFile* f) {
    char owner_str[4], group_str[4], others_str[4];
    perms_to_string(f->owner_perms, owner_str);
    perms_to_string(f->group_perms, group_str);
    perms_to_string(f->others_perms, others_str);

    printf("-%s%s%s  %-8s %-8s %s\n",
           owner_str, group_str, others_str, f->owner, f->group, f->filename);
}

// Determines which PermissionSet applies to a given requesting user.
// A user is "owner" if their username matches, "group" if their group
// matches (and they're not the owner), otherwise "others".
PermissionSet get_applicable_perms(SecureFile* f, const char* user, const char* user_group) {
    if (strcmp(user, f->owner) == 0) {
        return f->owner_perms;
    } else if (strcmp(user_group, f->group) == 0) {
        return f->group_perms;
    } else {
        return f->others_perms;
    }
}

// Checks whether a user is allowed to perform a given action ('r', 'w', or 'x')
int check_permission(SecureFile* f, const char* user, const char* user_group, char action) {
    PermissionSet p = get_applicable_perms(f, user, user_group);

    int allowed = 0;
    if (action == 'r') allowed = p.read;
    else if (action == 'w') allowed = p.write;
    else if (action == 'x') allowed = p.execute;

    const char* action_name = (action == 'r') ? "READ" : (action == 'w') ? "WRITE" : "EXECUTE";

    if (allowed) {
        printf("[%s] ALLOWED for '%s' on '%s'.\n", action_name, user, f->filename);
    } else {
        printf("[%s] DENIED  for '%s' on '%s' (insufficient permissions).\n",
               action_name, user, f->filename);
    }

    return allowed;
}

int main() {
    // A file owned by alice, group "staff":
    // owner:  rw-   (can read and write, not execute - it's a text file)
    // group:  r--   (staff members can only read)
    // others: ---   (no access at all for anyone else)
    SecureFile report = {
        .filename = "report.txt",
        .owner = "alice",
        .group = "staff",
        .owner_perms  = {1, 1, 0},
        .group_perms  = {1, 0, 0},
        .others_perms = {0, 0, 0}
    };

    // A shared script, group "staff":
    // owner:  rwx   (alice can do everything)
    // group:  r-x   (staff can read and run it, not modify)
    // others: r--   (anyone else can only read it)
    SecureFile script = {
        .filename = "backup.sh",
        .owner = "alice",
        .group = "staff",
        .owner_perms  = {1, 1, 1},
        .group_perms  = {1, 0, 1},
        .others_perms = {1, 0, 0}
    };

    printf("--- File permission listing (Unix ls -l style) ---\n");
    print_permissions(&report);
    print_permissions(&script);

    printf("\n--- Access checks on 'report.txt' (owner: alice, group: staff) ---\n");
    check_permission(&report, "alice", "staff", 'r');   // owner -> allowed
    check_permission(&report, "alice", "staff", 'w');   // owner -> allowed
    check_permission(&report, "bob",   "staff", 'r');   // same group -> allowed (read only)
    check_permission(&report, "bob",   "staff", 'w');   // same group -> denied (group has no write)
    check_permission(&report, "carol", "guests", 'r');  // others -> denied entirely

    printf("\n--- Access checks on 'backup.sh' (owner: alice, group: staff) ---\n");
    check_permission(&script, "bob",   "staff", 'x');   // group -> allowed (can execute)
    check_permission(&script, "bob",   "staff", 'w');   // group -> denied (no write)
    check_permission(&script, "carol", "guests", 'r');  // others -> allowed (read only)
    check_permission(&script, "carol", "guests", 'x');  // others -> denied (no execute)

    return 0;
}