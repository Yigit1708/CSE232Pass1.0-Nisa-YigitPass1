#ifndef PARSER_H
#define PARSER_H

typedef struct {
    char label[10];
    char opcode[10];
    char operand[20];
    int lc;
    int size;
} ParsedLine;

// Bir satırı parse eder
//  1  -> başarılı
//  0  -> boş satır
// -1  -> hata
int parseLine(char *line, ParsedLine *out);

// Opcode'a göre instruction size döndürür
int getInstructionSize(const char *opcode);

#endif
