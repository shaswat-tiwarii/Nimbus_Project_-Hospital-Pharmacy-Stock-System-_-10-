#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_STOCK 200
#define NAME_LIMIT 64
#define DATA_FILE "store.bin"

typedef struct {
    int code;
    char title[NAME_LIMIT];
    int dd, mm, yy;
    double unit_cost;
    int onhand;
} Record;

static Record invList[MAX_STOCK];
static int invCount = 0;

static void get_line(char *dst, size_t cap) {
    if (fgets(dst, (int)cap, stdin) == NULL) {
        *dst = '\0';
        return;
    }
    size_t len = strlen(dst);
    if (len > 0 && dst[len - 1] == '\n')
        dst[len - 1] = '\0';
}

static int to_int(const char *s, int *out) {
    char junk;
    if (sscanf(s, "%d %c", out, &junk) == 1) return 1;
    return 0;
}

static int to_double(const char *s, double *out) {
    char junk;
    if (sscanf(s, "%lf %c", out, &junk) == 1) return 1;
    return 0;
}

static int parse_three_ints(const char *s, int *a, int *b, int *c) {
    char x;
    if (sscanf(s, "%d %d %d %c", a, b, c, &x) == 3) return 1;
    return 0;
}

static int compare_dates(int d1, int m1, int y1, int d2, int m2, int y2) {
    if (y1 != y2) return (y1 < y2) ? -1 : 1;
    if (m1 != m2) return (m1 < m2) ? -1 : 1;
    if (d1 != d2) return (d1 < d2) ? -1 : 1;
    return 0;
}

static void today(int *d, int *m, int *y) {
    time_t raw = time(NULL);
    struct tm *tp = localtime(&raw);
    *d = tp->tm_mday;
    *m = tp->tm_mon + 1;
    *y = tp->tm_year + 1900;
}

static int locate(int idCode) {
    for (int i = 0; i < invCount; i++)
        if (invList[i].code == idCode)
            return i;
    return -1;
}
static void add_record() {
    if (invCount >= MAX_STOCK) {
        puts("Inventory limit reached.");
        return;
    }

    char buf[128];
    Record tmp;
    memset(&tmp, 0, sizeof(tmp));

    puts("Enter new item ID:");
    get_line(buf, sizeof(buf));

    if (!to_int(buf, &tmp.code) || tmp.code <= 0) {
        puts("Bad ID.");
        return;
    }

    if (locate(tmp.code) != -1) {
        puts("ID already used by another product.");
        return;
    }

    puts("Enter item name:");
    get_line(tmp.title, sizeof(tmp.title));

    if (tmp.title[0] == '\0') {
        puts("Name can't be blank.");
        return;
    }

    puts("Expiry date (DD MM YYYY):");
    get_line(buf, sizeof(buf));
    if (!parse_three_ints(buf, &tmp.dd, &tmp.mm, &tmp.yy)) {
        puts("Invalid date format.");
        return;
    }

    puts("Unit price:");
    get_line(buf, sizeof(buf));
    if (!to_double(buf, &tmp.unit_cost) || tmp.unit_cost < 0) {
        puts("Bad price.");
        return;
    }

    puts("Quantity on hand:");
    get_line(buf, sizeof(buf));
    if (!to_int(buf, &tmp.onhand) || tmp.onhand < 0) {
        puts("Bad quantity.");
        return;
    }

    invList[invCount++] = tmp;
    puts("Item stored.");
}

static void edit_record() {
    char buf[128];
    puts("ID to edit:");
    get_line(buf, sizeof(buf));

    int code;
    if (!to_int(buf, &code)) {
        puts("Bad ID.");
        return;
    }

    int idx = locate(code);
    if (idx < 0) {
        puts("No such item.");
        return;
    }

    puts("1=Rename   2=Price   3=Expiry");
    get_line(buf, sizeof(buf));

    int opt;
    if (!to_int(buf, &opt)) {
        puts("Invalid choice.");
        return;
    }

    if (opt == 1) {
        puts("New name:");
        get_line(invList[idx].title, sizeof(invList[idx].title));
        puts("Name changed.");
    }
    else if (opt == 2) {
        puts("New price:");
        get_line(buf, sizeof(buf));
        double pr;
        if (!to_double(buf, &pr) || pr < 0) {
            puts("Bad price.");
            return;
        }
        invList[idx].unit_cost = pr;
        puts("Price updated.");
    }
    else if (opt == 3) {
        puts("New expiry (DD MM YYYY):");
        get_line(buf, sizeof(buf));
        int d, m, y;
        if (!parse_three_ints(buf, &d, &m, &y)) {
            puts("Invalid date.");
            return;
        }
        invList[idx].dd = d;
        invList[idx].mm = m;
        invList[idx].yy = y;
        puts("Expiry changed.");
    }
    else {
        puts("Unknown code.");
    }
}

static void delete_expired() {
    int d, m, y;
    today(&d, &m, &y);

    int writePos = 0;
    for (int i = 0; i < invCount; i++) {
        Record *r = &invList[i];
        if (compare_dates(r->dd, r->mm, r->yy, d, m, y) >= 0) {
            invList[writePos++] = invList[i];
        }
    }

    int removed = invCount - writePos;
    invCount = writePos;

    if (removed)
        printf("%d expired item(s) purged.\n", removed);
    else
        puts("Nothing to remove.");
}