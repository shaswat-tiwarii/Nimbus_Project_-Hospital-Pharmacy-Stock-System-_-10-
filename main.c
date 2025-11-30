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

static void list_all_items() {
    if (!invCount) {
        puts("Nothing in storage.");
        return;
    }

    int d, m, y;
    today(&d, &m, &y);

    printf("%-6s %-20s %-10s %-8s %-8s %-12s\n",
           "ID","Name","Expires","Price","Stock","Status");

    for (int i = 0; i < invCount; i++) {
        Record *r = &invList[i];
        int cmp = compare_dates(r->dd, r->mm, r->yy, d, m, y);
        const char *info =
            (cmp < 0) ? "EXPIRED" :
            (cmp == 0 ? "Expires Today" : "Valid");

        printf("%-6d %-20s %02d-%02d-%04d %-8.2f %-8d %-12s\n",
               r->code, r->title, r->dd, r->mm, r->yy,
               r->unit_cost, r->onhand, info);
    }
}

static void show_expired_items() {
    int d, m, y;
    today(&d, &m, &y);

    int foundAny = 0;
    for (int i = 0; i < invCount; i++) {
        Record *r = &invList[i];
        if (compare_dates(r->dd, r->mm, r->yy, d, m, y) < 0) {
            if (!foundAny) {
                puts("Expired goods:");
                printf("%-6s %-20s %-12s %-6s\n",
                       "ID","Name","Expiry","Qty");
            }
            printf("%-6d %-20s %02d-%02d-%04d %-6d\n",
                   r->code, r->title, r->dd, r->mm, r->yy, r->onhand);
            foundAny = 1;
        }
    }

    if (!foundAny)
        puts("No expired records.");
}

static void add_stock() {
    char buf[128];
    puts("Item ID to restock:");
    get_line(buf, sizeof(buf));

    int code;
    if (!to_int(buf, &code)) {
        puts("Not valid.");
        return;
    }

    int idx = locate(code);
    if (idx < 0) {
        puts("ID not found.");
        return;
    }

    puts("Add how many?");
    get_line(buf, sizeof(buf));

    int amt;
    if (!to_int(buf, &amt) || amt <= 0) {
        puts("Bad amount.");
        return;
    }

    invList[idx].onhand += amt;
    printf("Stock updated: %d units now available.\n",
           invList[idx].onhand);
}

static void buy_item() {
    char buf[128];
    puts("ID to purchase:");
    get_line(buf, sizeof(buf));

    int code;
    if (!to_int(buf, &code)) {
        puts("Not a number.");
        return;
    }

    int idx = locate(code);
    if (idx < 0) {
        puts("No item with that ID.");
        return;
    }

    puts("Quantity:");
    get_line(buf, sizeof(buf));

    int q;
    if (!to_int(buf, &q) || q <= 0) {
        puts("Invalid qty.");
        return;
    }

    if (q > invList[idx].onhand) {
        printf("Only %d available.\n", invList[idx].onhand);
        return;
    }

    invList[idx].onhand -= q;
    double bill = q * invList[idx].unit_cost;

    printf("Total: %.2f. Remaining stock: %d\n",
           bill, invList[idx].onhand);
}

static void write_file() {
    FILE *fp = fopen(DATA_FILE, "wb");
    if (!fp) {
        perror("Cannot open file");
        return;
    }

    fwrite(&invCount, sizeof(invCount), 1, fp);

    for (int i = 0; i < invCount; i++)
        fwrite(&invList[i], sizeof(Record), 1, fp);

    fclose(fp);
    puts("Data saved.");
}

static void read_file() {
    FILE *fp = fopen(DATA_FILE, "rb");
    if (!fp) {
        puts("No saved file present.");
        return;
    }

    int n = 0;
    if (fread(&n, sizeof(n), 1, fp) != 1) {
        puts("Could not read header.");
        fclose(fp);
        return;
    }

    if (n < 0 || n > MAX_STOCK) {
        puts("Bad file data.");
        fclose(fp);
        return;
    }

    for (int i = 0; i < n; i++) {
        if (fread(&invList[i], sizeof(Record), 1, fp) != 1) {
            puts("Error reading records.");
            fclose(fp);
            return;
        }
    }

    invCount = n;
    fclose(fp);
    printf("Loaded %d item(s).\n", invCount);
}

static void load_demo() {
    Record a = {101,"Ibuprofen",10,10,2026,8.5,40};
    Record b = {102,"Cefuroxime",5,3,2025,120,15};
    Record c = {103,"Adrenaline",20,12,2027,250,5};

    invList[0] = a;
    invList[1] = b;
    invList[2] = c;

    invCount = 3;
    puts("Loaded sample items.");
}

static void menu() {
    puts("\n*** Pharmacy Stock Menu ***");
    puts("1. Add Item");
    puts("2. List Items");
    puts("3. Show Expired");
    puts("4. Delete Expired");
    puts("5. Restock");
    puts("6. Purchase");
    puts("7. Edit Record");
    puts("8. Save");
    puts("9. Load");
    puts("10. Demo Data");
    puts("0. Quit");
    printf("Pick> ");
}

int main() {
    char buf[64];
    int choice = -1;

    for (;;) {
        menu();
        get_line(buf, sizeof(buf));

        if (!to_int(buf, &choice)) {
            puts("Enter a number.");
            continue;
        }

        switch (choice) {
            case 1:  add_record(); break;
            case 2:  list_all_items(); break;
            case 3:  show_expired_items(); break;
            case 4:  delete_expired(); break;
            case 5:  add_stock(); break;
            case 6:  buy_item(); break;
            case 7:  edit_record(); break;
            case 8:  write_file(); break;
            case 9:  read_file(); break;
            case 10: load_demo(); break;
            case 0:  puts("Bye."); return 0;
            default: puts("Unknown option."); break;
        }
    }
}

//yash
