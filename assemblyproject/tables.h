#ifndef TABLES_H
#define TABLES_H

#include "parser.h"

// Dokümandaki struct tanımları
struct SymbolTable {
    char symbol[10];
    int address;
};

struct ForwardRefTable {
    int address;
    char symbol[10];
};

struct DirectAdrTable {
    int address;
};

struct HDRMTable {
    char code;  // H, D, R, veya M
    char symbol[10];
    int address;
};

struct Memory {
    int address;
    char symbol[11]; // integers must be converted to string Ex: 132 ->"01" and "32"
};

// Global tablolar (dokümandaki boyutlarda)
#define MAX_ST 10
#define MAX_FRT 20
#define MAX_DAT 30
#define MAX_HDRM 20
#define MEMORY_SIZE 500  // Loader için memory array boyutu
#define MAX_OPCODE_LEN 10  // Opcode maksimum uzunluğu

extern struct SymbolTable ST[MAX_ST];
extern struct ForwardRefTable FRT[MAX_FRT];
extern struct DirectAdrTable DAT[MAX_DAT];
extern struct HDRMTable HDRMT[MAX_HDRM];
extern struct Memory M[MEMORY_SIZE]; // Loader için memory array

extern int ST_count;
extern int FRT_count;
extern int DAT_count;
extern int HDRM_count;

// Fonksiyon prototipleri
void initTables();
int addToSymbolTable(const char *symbol, int address);
int findInSymbolTable(const char *symbol);
int isExternalReference(const char *symbol); // EXTREF'te tanımlı mı kontrol et
int addToForwardRefTable(int address, const char *symbol);
int addToDirectAdrTable(int address);
int addToHDRMTable(char code, const char *symbol, int address);
void printAllTables();

#endif

