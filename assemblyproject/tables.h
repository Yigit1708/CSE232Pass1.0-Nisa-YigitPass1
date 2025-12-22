#ifndef TABLES_H
#define TABLES_H
#include "structures.h"
#include "parser.h"


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

