#include <stdio.h>
#include <string.h>
#include "tables.h"

// Global tablolar
struct SymbolTable ST[MAX_ST];
struct ForwardRefTable FRT[MAX_FRT];
struct DirectAdrTable DAT[MAX_DAT];
struct HDRMTable HDRMT[MAX_HDRM];

int ST_count = 0;
int FRT_count = 0;
int DAT_count = 0;
int HDRM_count = 0;

void initTables() {
    ST_count = 0;
    FRT_count = 0;
    DAT_count = 0;
    HDRM_count = 0;
}

int addToSymbolTable(const char *symbol, int address) {
    if (ST_count >= MAX_ST) {
        printf("HATA: Symbol Table dolu!\n");
        return -1;
    }
    
    // Aynı symbol var mı kontrol et
    for (int i = 0; i < ST_count; i++) {
        if (strcmp(ST[i].symbol, symbol) == 0) {
            printf("HATA: Symbol '%s' zaten tanımlı!\n", symbol);
            return -1;
        }
    }
    
    strcpy(ST[ST_count].symbol, symbol);
    ST[ST_count].address = address;
    ST_count++;
    return 0;
}

int findInSymbolTable(const char *symbol) {
    for (int i = 0; i < ST_count; i++) {
        if (strcmp(ST[i].symbol, symbol) == 0) {
            return ST[i].address;
        }
    }
    return -1; // Bulunamadı
}

int isExternalReference(const char *symbol) {
    // HDRM table'da R (Reference) kodlu symbol'leri kontrol et
    for (int i = 0; i < HDRM_count; i++) {
        if (HDRMT[i].code == 'R' && strcmp(HDRMT[i].symbol, symbol) == 0) {
            return 1; // External reference
        }
    }
    return 0; // External reference değil
}

int addToForwardRefTable(int address, const char *symbol) {
    if (FRT_count >= MAX_FRT) {
        printf("HATA: Forward Reference Table dolu!\n");
        return -1;
    }
    
    FRT[FRT_count].address = address;
    strcpy(FRT[FRT_count].symbol, symbol);
    FRT_count++;
    return 0;
}

int addToDirectAdrTable(int address) {
    if (DAT_count >= MAX_DAT) {
        printf("HATA: Direct Address Table dolu!\n");
        return -1;
    }
    
    // Aynı address var mı kontrol et
    for (int i = 0; i < DAT_count; i++) {
        if (DAT[i].address == address) {
            return 0; // Zaten var
        }
    }
    
    DAT[DAT_count].address = address;
    DAT_count++;
    return 0;
}

int addToHDRMTable(char code, const char *symbol, int address) {
    if (HDRM_count >= MAX_HDRM) {
        printf("HATA: HDRM Table dolu!\n");
        return -1;
    }
    
    HDRMT[HDRM_count].code = code;
    strcpy(HDRMT[HDRM_count].symbol, symbol);
    HDRMT[HDRM_count].address = address;
    HDRM_count++;
    return 0;
}

void printAllTables() {
    printf("\n=== SYMBOL TABLE ===\n");
    for (int i = 0; i < ST_count; i++) {
        printf("Symbol: %-10s Address: %d\n", ST[i].symbol, ST[i].address);
    }
    
    printf("\n=== FORWARD REFERENCE TABLE ===\n");
    for (int i = 0; i < FRT_count; i++) {
        printf("Address: %d Symbol: %s\n", FRT[i].address, FRT[i].symbol);
    }
    
    printf("\n=== DIRECT ADDRESS TABLE ===\n");
    for (int i = 0; i < DAT_count; i++) {
        printf("Address: %d\n", DAT[i].address);
    }
    
    printf("\n=== HDRM TABLE ===\n");
    for (int i = 0; i < HDRM_count; i++) {
        printf("Code: %c Symbol: %-10s Address: %d\n", 
               HDRMT[i].code, HDRMT[i].symbol, HDRMT[i].address);
    }
}

