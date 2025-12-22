#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "parser.h"

typedef struct {
    char opcode[10];
    int size;
} OpcodeInfo;

// SMPL opcode tablosu
OpcodeInfo opcodeTable[] = {
    {"ADD", 3}, {"SUB", 3}, {"LDA", 3}, {"STA", 3},
    {"CLL", 3}, {"JMP", 3}, {"BEQ", 3}, {"BGT", 3}, {"BLT", 3},
    {"INC", 1}, {"DEC", 1}, {"RET", 1}, {"HLT", 1},
    {"BYTE", 1}, {"WORD", 1},
    {"START", 0}, {"END", 0}, {"EXTREF", 0}, {"ENTRY", 0}, {"PROG", 0}
};

int opcodeCount = sizeof(opcodeTable) / sizeof(OpcodeInfo);

// Operand'a göre instruction size döndürür
// Immediate addressing (#) varsa 2 byte, yoksa 3 byte (direct)
int getInstructionSize(const char *opcode, const char *operand) {
    // Immediate addressing kontrolü
    int isImmediate = (operand && operand[0] == '#');
    
    // ADD, SUB, LDA için immediate/direct ayrımı
    if (strcmp(opcode, "ADD") == 0) {
        return isImmediate ? 2 : 3;
    }
    if (strcmp(opcode, "SUB") == 0) {
        return isImmediate ? 2 : 3;
    }
    if (strcmp(opcode, "LDA") == 0) {
        return isImmediate ? 2 : 3;
    }
    
    // Diğer opcode'lar için tablo kontrolü
    for (int i = 0; i < opcodeCount; i++) {
        if (strcmp(opcode, opcodeTable[i].opcode) == 0)
            return opcodeTable[i].size;
    }
    return -1;
}

int parseLine(char *line, ParsedLine *out) {
    char temp[100];
    strcpy(temp, line);

    // Default değerler
    out->label[0] = '\0';
    out->opcode[0] = '\0';
    out->operand[0] = '\0';

    char *token;
    char *rest = temp;

    // İlk token
    token = strtok_r(rest, " \t\r\n", &rest); // \r kısmı windows uyumluluğu için eklendi aksi halde windows üzerinde 
                                              // hata veriyor proje sonunda sadece linuxa uyumlu olması açısından kaldırılabilir 
    if (!token) return 0; // boş satır

    // Label kontrolü
    if (strchr(token, ':')) {
        token[strlen(token) - 1] = '\0'; // ':' sil
        strcpy(out->label, token);

        token = strtok_r(NULL, " \t\r\n", &rest); //60. satırdaki yorum aynı şekilde geçerlidir
        if (!token) {
            printf("HATA: Opcode eksik\n");
            return -1;
        }
    }

    // Opcode
    strcpy(out->opcode, token);

    // Operand (varsa)
    token = strtok_r(NULL, "\r\n", &rest); //60. satırdaki yorum aynı şekilde geçerlidir
    if (token) {
        while (*token == ' ' || *token == '\t') token++;
        strcpy(out->operand, token);
    }

    // Instruction size (operand'a göre hesaplanır)
    out->size = getInstructionSize(out->opcode, out->operand);
    if (out->size == -1) {
        printf("HATA: Bilinmeyen opcode -> %s\n", out->opcode);
        return -1;
    }

    return 1;
}
