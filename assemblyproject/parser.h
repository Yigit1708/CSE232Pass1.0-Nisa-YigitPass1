#ifndef PARSER_H
#define PARSER_H
#include "structures.h"

// Bir satırı parse eder
//  1  -> başarılı
//  0  -> boş satır
// -1  -> hata
int parseLine(char *line, ParsedLine *out);

// Opcode ve operand'a göre instruction size döndürür
int getInstructionSize(const char *opcode, const char *operand);

#endif
