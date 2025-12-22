#ifndef PASS1_H
#define PASS1_H

#include "parser.h"
#include "tables.h"

// Partial code generation için
void processPass1(ParsedLine *pl, int LC, FILE *sfp);
void updateSymbolTable(ParsedLine *pl, int LC);
void updateFRT(ParsedLine *pl, int LC);
void updateDAT(ParsedLine *pl, int LC);
void updateHDRM(ParsedLine *pl, int LC);
void generatePartialCode(ParsedLine *pl, int LC, FILE *sfp);

// Opcode'u hex string'e çevir
void opcodeToHex(const char *opcode, const char *operand, char *hexOpcode);

#endif

