

//4 tane input dosyası alır -> MAIN.o MAIN.t DATA.o Data.t (isimler örnektir)
//output olarak exp.t ve exp.exe üretir
//(çalıştırmak için cmd ye : ".\linker.exe exp MAIN.o MAIN.t DATA.o DATA.t" yazınız

#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE 1024

// -------------------- small utilities --------------------
static void die(const char *msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(1);
}
static void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) die("malloc failed");
    return p;
}
static void *xcalloc(size_t n, size_t sz) {
    void *p = calloc(n, sz);
    if (!p) die("calloc failed");
    return p;
}
static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) die("realloc failed");
    return q;
}
static void trim(char *s) {
    int n = (int)strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || isspace((unsigned char)s[n-1]))) s[--n] = 0;
    int i = 0;
    while (s[i] && isspace((unsigned char)s[i])) i++;
    if (i > 0) memmove(s, s+i, strlen(s+i)+1);
}
static int is_hex_byte(const char *t) {
    return t && strlen(t) == 2 && isxdigit((unsigned char)t[0]) && isxdigit((unsigned char)t[1]);
}
static int parse_hex_byte(const char *t) {
    int v = 0; sscanf(t, "%x", &v);
    return v & 0xFF;
}

// -------------------- dynamic vector helpers --------------------
typedef struct { char **a; int n, cap; } StrVec;
static void sv_init(StrVec *v){ v->a=NULL; v->n=0; v->cap=0; }
static void sv_push(StrVec *v, const char *s){
    if (v->n == v->cap) {
        v->cap = v->cap ? v->cap*2 : 16;
        v->a = (char**)xrealloc(v->a, (size_t)v->cap * sizeof(char*));
    }
    v->a[v->n] = (char*)xmalloc(strlen(s)+1);
    strcpy(v->a[v->n], s);
    v->n++;
}
static int sv_contains(const StrVec *v, const char *s){
    for (int i=0;i<v->n;i++) if (strcmp(v->a[i], s)==0) return 1;
    return 0;
}
static void sv_free(StrVec *v){
    for (int i=0;i<v->n;i++) free(v->a[i]);
    free(v->a);
    v->a=NULL; v->n=0; v->cap=0;
}

typedef struct { int *a; int n, cap; } IntVec;
static void iv_init(IntVec *v){ v->a=NULL; v->n=0; v->cap=0; }
static void iv_push(IntVec *v, int x){
    if (v->n == v->cap) {
        v->cap = v->cap ? v->cap*2 : 32;
        v->a = (int*)xrealloc(v->a, (size_t)v->cap * sizeof(int));
    }
    v->a[v->n++] = x;
}
static int iv_contains(const IntVec *v, int x){
    for (int i=0;i<v->n;i++) if (v->a[i]==x) return 1;
    return 0;
}
static void iv_push_unique(IntVec *v, int x){
    if (!iv_contains(v, x)) iv_push(v, x);
}
static int cmp_int(const void *a, const void *b){
    int x=*(const int*)a, y=*(const int*)b;
    return (x>y) - (x<y);
}
static void iv_sort(IntVec *v){
    qsort(v->a, (size_t)v->n, sizeof(int), cmp_int);
}
static void iv_free(IntVec *v){
    free(v->a); v->a=NULL; v->n=0; v->cap=0;
}

typedef struct { char name[64]; int addr; } Sym;
typedef struct { Sym *a; int n, cap; } SymVec;
static void syv_init(SymVec *v){ v->a=NULL; v->n=0; v->cap=0; }
static void syv_push(SymVec *v, const char *name, int addr){
    if (v->n == v->cap) {
        v->cap = v->cap ? v->cap*2 : 32;
        v->a = (Sym*)xrealloc(v->a, (size_t)v->cap * sizeof(Sym));
    }
    strncpy(v->a[v->n].name, name, 63);
    v->a[v->n].name[63]=0;
    v->a[v->n].addr = addr;
    v->n++;
}
static int syv_find_addr(const SymVec *v, const char *name, int *out_addr){
    for (int i=0;i<v->n;i++){
        if (strcmp(v->a[i].name, name)==0){
            if (out_addr) *out_addr = v->a[i].addr;
            return 1;
        }
    }
    return 0;
}
static void syv_free(SymVec *v){
    free(v->a); v->a=NULL; v->n=0; v->cap=0;
}

// -------------------- module structures --------------------
typedef struct { char sym[64]; int local_addr; } ModRecord;
typedef struct { ModRecord *a; int n, cap; } ModVec;
static void mv_init(ModVec *v){ v->a=NULL; v->n=0; v->cap=0; }
static void mv_push(ModVec *v, const char *sym, int local_addr){
    if (v->n == v->cap) {
        v->cap = v->cap ? v->cap*2 : 32;
        v->a = (ModRecord*)xrealloc(v->a, (size_t)v->cap * sizeof(ModRecord));
    }
    strncpy(v->a[v->n].sym, sym, 63);
    v->a[v->n].sym[63]=0;
    v->a[v->n].local_addr = local_addr;
    v->n++;
}
static void mv_free(ModVec *v){
    free(v->a); v->a=NULL; v->n=0; v->cap=0;
}

typedef struct {
    char module_name[64];

    // .o bytes
    unsigned char *bytes;
    int length;

    // .t content
    SymVec symtab;     // local symbol table (all symbols)
    SymVec deflist;    // D records (exports) if present
    StrVec reflist;    // R records (imports)
    ModVec modlist;    // M records
    IntVec dat;        // DAT from module (if present)

    // placement
    int base;
} Module;

// -------------------- ESTAB --------------------
typedef struct { char name[64]; int abs_addr; } EstabEntry;
typedef struct { EstabEntry *a; int n, cap; } Estab;
static void estab_init(Estab *e){ e->a=NULL; e->n=0; e->cap=0; }
static int estab_find(const Estab *e, const char *name, int *out_abs){
    for (int i=0;i<e->n;i++){
        if (strcmp(e->a[i].name, name)==0){
            if (out_abs) *out_abs = e->a[i].abs_addr;
            return 1;
        }
    }
    return 0;
}
static void estab_add(Estab *e, const char *name, int abs_addr){
    int tmp;
    if (estab_find(e, name, &tmp)) {
        fprintf(stderr, "ERROR: duplicate external symbol definition: %s\n", name);
        exit(1);
    }
    if (e->n == e->cap) {
        e->cap = e->cap ? e->cap*2 : 64;
        e->a = (EstabEntry*)xrealloc(e->a, (size_t)e->cap * sizeof(EstabEntry));
    }
    strncpy(e->a[e->n].name, name, 63);
    e->a[e->n].name[63]=0;
    e->a[e->n].abs_addr = abs_addr;
    e->n++;
}
static void estab_free(Estab *e){
    free(e->a); e->a=NULL; e->n=0; e->cap=0;
}

// -------------------- parsing .o --------------------
static void read_object_o(Module *m, const char *o_path) {
    FILE *f = fopen(o_path, "r");
    if (!f) { perror(o_path); die("failed to open .o"); }

    unsigned char *tmp = NULL;
    int cap = 0, max_written = -1;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (!line[0]) continue;

        char *tok = strtok(line, " \t");
        if (!tok) continue;

        int lc = atoi(tok);
        int pos = lc;

        while ((tok = strtok(NULL, " \t")) != NULL) {
            if (pos + 1 > cap) {
                int newcap = cap ? cap : 64;
                while (newcap < pos + 1) newcap *= 2;
                tmp = (unsigned char*)xrealloc(tmp, (size_t)newcap);
                memset(tmp + cap, 0, (size_t)(newcap - cap));
                cap = newcap;
            }
            if (strcmp(tok, "??") == 0) {
                tmp[pos] = 0x00; // placeholder, will be patched by M
            } else if (is_hex_byte(tok)) {
                tmp[pos] = (unsigned char)parse_hex_byte(tok);
            } else {
                // ignore unexpected tokens
            }
            if (pos > max_written) max_written = pos;
            pos++;
        }
    }
    fclose(f);

    if (max_written < 0) die("empty .o");
    m->length = max_written + 1;
    m->bytes = (unsigned char*)xrealloc(tmp, (size_t)m->length);
}

// -------------------- parsing .t --------------------
static void read_table_t(Module *m, const char *t_path) {
    FILE *f = fopen(t_path, "r");
    if (!f) { perror(t_path); die("failed to open .t"); }

    syv_init(&m->symtab);
    syv_init(&m->deflist);
    sv_init(&m->reflist);
    mv_init(&m->modlist);
    iv_init(&m->dat);

    m->module_name[0] = 0;

    enum { SEC_NONE, SEC_SYM, SEC_DAT, SEC_HDRM } sec = SEC_NONE;

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        if (!line[0]) continue;

        if (strcmp(line, "=== SYMBOL TABLE ===") == 0) { sec = SEC_SYM; continue; }
        if (strcmp(line, "=== DIRECT ADDRESS TABLE (DAT) ===") == 0) { sec = SEC_DAT; continue; }
        if (strcmp(line, "=== HDRM TABLE ===") == 0) { sec = SEC_HDRM; continue; }

        if (sec == SEC_SYM) {
            // Symbol: LOOP       Address: 0
            char sym[64]; int addr;
            if (sscanf(line, "Symbol: %63s Address: %d", sym, &addr) == 2) {
                syv_push(&m->symtab, sym, addr);
            }
        } else if (sec == SEC_DAT) {
            // single integer per line
            int v;
            if (sscanf(line, "%d", &v) == 1) iv_push(&m->dat, v);
        } else if (sec == SEC_HDRM) {
            // Expected lines (per doc spirit):
            // H NAME 0
            // D SYM addr   (exports)   [may not exist in your current files]
            // R SYM 0      (imports)
            // M SYM addr   (patch at addr using SYM address)
            char code; char sym[64]; int addr;
            if (sscanf(line, "%c %63s %d", &code, sym, &addr) == 3) {
                if (code == 'H') {
                    strncpy(m->module_name, sym, 63);
                    m->module_name[63]=0;
                } else if (code == 'D') {
                    syv_push(&m->deflist, sym, addr);
                } else if (code == 'R') {
                    if (!sv_contains(&m->reflist, sym)) sv_push(&m->reflist, sym);
                } else if (code == 'M') {
                    mv_push(&m->modlist, sym, addr);
                } else {
                    // ignore
                }
            }
        }
    }
    fclose(f);

    if (!m->module_name[0]) strncpy(m->module_name, "NONAME", 63);
}

// -------------------- writing outputs --------------------
static void write_exe_listing(const char *exe_path, const unsigned char *linked, int linked_len) {
    FILE *f = fopen(exe_path, "w");
    if (!f) { perror(exe_path); die("failed to write exe"); }

    // one-line style is okay, but let's keep readable: 16 bytes per line
    const int per_line = 16;
    for (int i = 0; i < linked_len; i += per_line) {
        fprintf(f, "%d", i);
        int end = i + per_line;
        if (end > linked_len) end = linked_len;
        for (int j = i; j < end; j++) fprintf(f, " %02X", linked[j]);
        fprintf(f, "\n");
    }
    fclose(f);
}

static void write_dat_t(const char *t_path, IntVec *dat_out, Estab *estab, Module *mods, int modn) {
    iv_sort(dat_out);

    FILE *f = fopen(t_path, "w");
    if (!f) { perror(t_path); die("failed to write .t"); }

    // Write ESTAB first
    fprintf(f, "=== ESTAB ===\n");
    for (int i=0;i<estab->n;i++) {
        fprintf(f, "%s %d\n", estab->a[i].name, estab->a[i].abs_addr);
    }

    // Write M records (symbol + address pairs for relocation)
    fprintf(f, "=== M RECORDS ===\n");
    for (int i=0;i<modn;i++) {
        for (int k=0;k<mods[i].modlist.n;k++) {
            const char *sym = mods[i].modlist.a[k].sym;
            int field_abs = mods[i].base + mods[i].modlist.a[k].local_addr;
            fprintf(f, "%s %d\n", sym, field_abs);
        }
    }

    fprintf(f, "=== DIRECT ADDRESS TABLE (DAT) ===\n");
    for (int i=0;i<dat_out->n;i++) fprintf(f, "%d\n", dat_out->a[i]);

    fclose(f);
}

// -------------------- main linking logic --------------------
int main(int argc, char **argv) {
    if (argc < 4) {
        fprintf(stderr,
            "Usage:\n"
            "  %s <out_prefix> <mod1.o> <mod1.t> [<mod2.o> <mod2.t> ...]\n"
            "Example:\n"
            "  %s exp MAIN.o MAIN.t SBR1.o SBR1.t DT.o DT.t\n",
            argv[0], argv[0]);
        return 1;
    }
    if (((argc - 2) % 2) != 0) die("You must pass pairs of <.o> <.t>");

    const char *out_prefix = argv[1];
    int modn = (argc - 2) / 2;

    Module *mods = (Module*)xcalloc((size_t)modn, sizeof(Module));

    // 1) Read modules and assign bases (sequential placement; main first is recommended)
    int total_len = 0;
    for (int i=0;i<modn;i++) {
        const char *o_path = argv[2 + 2*i];
        const char *t_path = argv[2 + 2*i + 1];

        read_object_o(&mods[i], o_path);
        read_table_t(&mods[i], t_path);

        mods[i].base = total_len;
        total_len += mods[i].length;
    }

    // 2) Build global REQUIRED set from all R lists (imports)
    StrVec required;
    sv_init(&required);
    for (int i=0;i<modn;i++) {
        for (int r=0;r<mods[i].reflist.n;r++) {
            if (!sv_contains(&required, mods[i].reflist.a[r])) sv_push(&required, mods[i].reflist.a[r]);
        }
    }

    // 3) Build ESTAB (exports only)
    // Rule:
    //  - If module has D records: exports = D list
    //  - Else exports = (SYMBOL_TABLE symbols that are in global required set)
    Estab estab;
    estab_init(&estab);

    for (int i=0;i<modn;i++) {
        if (mods[i].deflist.n > 0) {
            for (int d=0; d<mods[i].deflist.n; d++) {
                int abs = mods[i].base + mods[i].deflist.a[d].addr;
                estab_add(&estab, mods[i].deflist.a[d].name, abs);
            }
        } else {
            for (int s=0; s<mods[i].symtab.n; s++) {
                const char *name = mods[i].symtab.a[s].name;
                if (sv_contains(&required, name)) {
                    int abs = mods[i].base + mods[i].symtab.a[s].addr;
                    estab_add(&estab, name, abs);
                }
            }
        }
    }

    // 4) Validate all R symbols are defined in ESTAB
    for (int i=0;i<modn;i++) {
        for (int r=0;r<mods[i].reflist.n;r++) {
            int tmp;
            if (!estab_find(&estab, mods[i].reflist.a[r], &tmp)) {
                fprintf(stderr, "ERROR: undefined external symbol '%s' referenced by module %s\n",
                        mods[i].reflist.a[r], mods[i].module_name);
                return 1;
            }
        }
    }

    // 5) Copy all bytes into linked memory
    unsigned char *linked = (unsigned char*)xcalloc((size_t)total_len, 1);
    for (int i=0;i<modn;i++) {
        memcpy(linked + mods[i].base, mods[i].bytes, (size_t)mods[i].length);
    }

    // 6) Build output DAT: start with all module DAT entries (shifted), then add patched M-field starts
    IntVec dat_out;
    iv_init(&dat_out);
    for (int i=0;i<modn;i++) {
        for (int d=0; d<mods[i].dat.n; d++) iv_push_unique(&dat_out, mods[i].base + mods[i].dat.a[d]);
    }

    // 7) Apply M records:
    //    For each M sym @ local_addr:
    //      - if sym in ESTAB => use that abs
    //      - else if sym is local (in module symtab) => use module-base + local symbol addr
    //      - else error
    for (int i=0;i<modn;i++) {
        for (int k=0;k<mods[i].modlist.n;k++) {
            const char *sym = mods[i].modlist.a[k].sym;
            int field_local = mods[i].modlist.a[k].local_addr;
            int field_abs = mods[i].base + field_local;

            if (field_abs < 0 || field_abs + 1 >= total_len) {
                fprintf(stderr, "ERROR: M record out of range in module %s: %s at %d\n",
                        mods[i].module_name, sym, field_abs);
                return 1;
            }

            int sym_abs;
            if (estab_find(&estab, sym, &sym_abs)) {
                // external
            } else {
                // try local symbol
                int local_addr;
                if (syv_find_addr(&mods[i].symtab, sym, &local_addr)) {
                    sym_abs = mods[i].base + local_addr;
                } else {
                    fprintf(stderr, "ERROR: unresolved symbol '%s' in M record (module %s)\n",
                            sym, mods[i].module_name);
                    return 1;
                }
            }

            // patch 16-bit big-endian address at field_abs
            linked[field_abs]     = (unsigned char)((sym_abs >> 8) & 0xFF);
            linked[field_abs + 1] = (unsigned char)(sym_abs & 0xFF);

            // loader relocation point
            iv_push_unique(&dat_out, field_abs);
        }
    }

    // 8) Write outputs
    char exe_path[256], t_path[256];
    snprintf(exe_path, sizeof(exe_path), "%s.exe", out_prefix);
    snprintf(t_path, sizeof(t_path), "%s.t", out_prefix);

    write_exe_listing(exe_path, linked, total_len);
    write_dat_t(t_path, &dat_out, &estab, mods, modn);

    // 9) Cleanup
    free(linked);
    iv_free(&dat_out);
    estab_free(&estab);
    sv_free(&required);

    for (int i=0;i<modn;i++) {
        free(mods[i].bytes);
        syv_free(&mods[i].symtab);
        syv_free(&mods[i].deflist);
        sv_free(&mods[i].reflist);
        mv_free(&mods[i].modlist);
        iv_free(&mods[i].dat);
    }
    free(mods);

    return 0;
}
