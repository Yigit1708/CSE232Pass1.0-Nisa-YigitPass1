#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "pass1.h"

// Opcode tablosu (dokümandan)
typedef struct {
    char mnemonic[10];
    char opcode[3];
    int bytes;
    int isImmediate;
} OpcodeEntry;

OpcodeEntry opcodeMap[] = {
    {"ADD", "A1", 3, 0}, {"ADD", "A2", 2, 1},
    {"SUB", "A3", 3, 0}, {"SUB", "A4", 2, 1},
    {"LDA", "E1", 3, 0}, {"LDA", "E2", 2, 1},
    {"STA", "F1", 3, 0},
    {"CLL", "C1", 3, 0},
    {"JMP", "B4", 3, 0},
    {"BEQ", "B1", 3, 0},
    {"BGT", "B2", 3, 0},
    {"BLT", "B3", 3, 0},
    {"INC", "D2", 1, 0},
    {"DEC", "D1", 1, 0},
    {"RET", "C2", 1, 0},
    {"HLT", "FE", 1, 0}
};

int opcodeMapSize = sizeof(opcodeMap) / sizeof(OpcodeEntry);

// Relative addressing kullanan instruction'lar mı kontrol et
int isRelativeAddressing(const char *opcode) {
    return (strcmp(opcode, "BEQ") == 0 || 
            strcmp(opcode, "BGT") == 0 || 
            strcmp(opcode, "BLT") == 0);
}

// Opcode'u hex string'e çevir
void opcodeToHex(const char *opcode, const char *operand, char *hexOpcode) {
    int isImmediate = (operand && operand[0] == '#');
    
    for (int i = 0; i < opcodeMapSize; i++) {
        if (strcmp(opcodeMap[i].mnemonic, opcode) == 0) {
            if (opcodeMap[i].isImmediate == isImmediate || opcodeMap[i].bytes == 1) {
                strcpy(hexOpcode, opcodeMap[i].opcode);
                return;
            }
        }
    }
    strcpy(hexOpcode, "??"); // Bulunamadı
}

// Operand'ın sayı olup olmadığını kontrol et
int isNumeric(const char *str) {
    if (!str || strlen(str) == 0) return 0;
    if (str[0] == '#') {
        // Immediate: #5 gibi
        for (int i = 1; str[i]; i++) {
            if (!isdigit(str[i])) return 0;
        }
        return 1;
    }
    // Direkt sayı: 10, 70 gibi
    for (int i = 0; str[i]; i++) {
        if (!isdigit(str[i])) return 0;
    }
    return 1;
}

// Operand'dan sayıyı çıkar
int getNumericValue(const char *operand) {
    if (!operand) return 0;
    if (operand[0] == '#') {
        return atoi(operand + 1);
    }
    return atoi(operand);
}

// Symbol Table güncelleme
void updateSymbolTable(ParsedLine *pl, int LC) {
    // Label varsa Symbol Table'a ekle
    if (strlen(pl->label) > 0) {
        int existing = findInSymbolTable(pl->label);
        if (existing == -1) {
            // Yeni symbol
            addToSymbolTable(pl->label, LC);
        } else {
            // Symbol zaten var, hata olabilir
            printf("UYARI: Symbol '%s' zaten tanımlı (eski: %d, yeni: %d)\n", 
                   pl->label, existing, LC);
        }
    }
}

// Forward Reference Table güncelleme
void updateFRT(ParsedLine *pl, int LC) {
    // Operand bir symbol ise (sayı değilse) ve Symbol Table'da yoksa FRT'ye ekle
    if (strlen(pl->operand) > 0 && !isNumeric(pl->operand)) {
        // Immediate değilse (# ile başlamıyorsa)
        if (pl->operand[0] != '#') {
            // External reference değilse ve Symbol Table'da yoksa forward reference
            if (!isExternalReference(pl->operand)) {
                int found = findInSymbolTable(pl->operand);
                if (found == -1) {
                    // Symbol henüz tanımlı değil, forward reference
                    // 3 byte instruction'larda address LC+1 (opcode byte'ından sonra)
                    int addrOffset = (pl->size == 3) ? 1 : 0;
                    addToForwardRefTable(LC + addrOffset, pl->operand);
                }
            }
        }
    }
}

// Direct Address Table güncelleme
void updateDAT(ParsedLine *pl, int LC) {
    // Direct addressing kullanan instruction'lar için
    // Operand sayısal bir adres ise (örn: STA 70, STA 10)
    if (strlen(pl->operand) > 0 && isNumeric(pl->operand) && pl->operand[0] != '#') {
        int addr = getNumericValue(pl->operand);
        addToDirectAdrTable(addr);
    }
}

// HDRM Table güncelleme
void updateHDRM(ParsedLine *pl, int LC) {
    // EXTREF ve ENTRY için HDRM table'a ekleme yapılır
    // Bu kısım linker için önemli
    
    if (strcmp(pl->opcode, "EXTREF") == 0) {
        // EXTREF'teki her symbol için R (Reference) kodu
        char *operand_copy = strdup(pl->operand);
        char *token = strtok(operand_copy, ",");
        while (token != NULL) {
            // Baştaki boşlukları temizle
            while (*token == ' ' || *token == '\t') token++;
            addToHDRMTable('R', token, 0);
            token = strtok(NULL, ",");
        }
        free(operand_copy);
    }
    
    if (strcmp(pl->opcode, "ENTRY") == 0) {
        // ENTRY'deki her symbol için D (Definition) kodu
        char *operand_copy = strdup(pl->operand);
        char *token = strtok(operand_copy, ",");
        while (token != NULL) {
            while (*token == ' ' || *token == '\t') token++;
            // Symbol'ün address'ini bul (şimdilik 0, Pass 2'de güncellenecek)
            int addr = findInSymbolTable(token);
            if (addr == -1) addr = 0; // Henüz bilinmiyor
            addToHDRMTable('D', token, addr);
            token = strtok(NULL, ",");
        }
        free(operand_copy);
    }
    
    // M (Modify) kodu - Relocatable address'ler için
    // 3 byte instruction'larda operand bir symbol ise ve external reference veya forward reference ise
    if (pl->size == 3 && strlen(pl->operand) > 0) {
        // Operand numeric değilse ve immediate değilse
        if (!isNumeric(pl->operand) && pl->operand[0] != '#') {
            // M kodu ekle: LC + 1 (opcode byte'ından sonraki address kısmı)
            addToHDRMTable('M', pl->operand, LC + 1);
        }
    }
}

// Partial code generation
void generatePartialCode(ParsedLine *pl, int LC, FILE *sfp) {
    if (!sfp) return;
    
    // BYTE pseudo-op
    if (strcmp(pl->opcode, "BYTE") == 0) {
        int val = getNumericValue(pl->operand);
        fprintf(sfp, "%d %02d\n", LC, val);
        return;
    }
    
    // WORD pseudo-op
    if (strcmp(pl->opcode, "WORD") == 0) {
        int val = getNumericValue(pl->operand);
        fprintf(sfp, "%d %02d\n", LC, val);
        return;
    }
    
    char hexOpcode[3];
    opcodeToHex(pl->opcode, pl->operand, hexOpcode);
    
    // .s dosyasına yaz: LC ve partial code
    fprintf(sfp, "%d %s ", LC, hexOpcode);
    
    // Operand için partial code
    if (pl->size == 1) {
        // 1 byte instruction (HLT, RET, INC, DEC)
        fprintf(sfp, "\n");
    } else if (pl->size == 2) {
        // 2 byte instruction (immediate)
        if (pl->operand[0] == '#') {
            int val = getNumericValue(pl->operand);
            fprintf(sfp, "%02d\n", val);
        } else {
            fprintf(sfp, "??\n");
        }
    } else if (pl->size == 3) {
        // 3 byte instruction (direct/relative)
        if (isNumeric(pl->operand) && pl->operand[0] != '#') {
            // Direkt sayısal adres (2 byte: high, low)
            int addr = getNumericValue(pl->operand);
            
            // Relative addressing kontrolü (BEQ, BGT, BLT)
            if (isRelativeAddressing(pl->opcode)) {
                // Relative displacement = target - (LC + 3)
                int displacement = addr - (LC + 3);
                fprintf(sfp, "%02d %02d\n", (displacement >> 8) & 0xFF, displacement & 0xFF);
            } else {
                fprintf(sfp, "%02d %02d\n", (addr >> 8) & 0xFF, addr & 0xFF);
            }
        } else if (pl->operand[0] == '#') {
            // Bu durumda 2 byte olmalı, hata var
            fprintf(sfp, "?? ??\n");
        } else {
            // Symbol - Symbol Table'da varsa address'i yaz, yoksa forward reference
            int symbolAddr = findInSymbolTable(pl->operand);
            if (symbolAddr != -1) {
                // Symbol tanımlı
                if (isRelativeAddressing(pl->opcode)) {
                    // Relative addressing: target - (LC + 3)
                    int displacement = symbolAddr - (LC + 3);
                    fprintf(sfp, "%02d %02d\n", (displacement >> 8) & 0xFF, displacement & 0xFF);
                } else {
                    // Direct addressing
                    fprintf(sfp, "%02d %02d\n", (symbolAddr >> 8) & 0xFF, symbolAddr & 0xFF);
                }
            } else {
                // Symbol henüz bilinmiyor (forward reference)
                fprintf(sfp, "?? ??\n");
            }
        }
    } else {
        // Pseudo-op (START, END, EXTREF, ENTRY, PROG)
        fprintf(sfp, "\n");
    }
}

// Pass 1 ana işleme fonksiyonu
void processPass1(ParsedLine *pl, int LC, FILE *sfp) {
    // Pseudo-ops için özel işlem
    if (strcmp(pl->opcode, "EXTREF") == 0 || strcmp(pl->opcode, "ENTRY") == 0) {
        // Sadece HDRM table güncelle
        updateHDRM(pl, LC);
        // Partial code üretme (pseudo-op'lar için .s dosyasına yazma)
        // EXTREF ve ENTRY için .s dosyasına bir şey yazmıyoruz
        return;
    }
    
    // Normal instruction'lar için tüm tabloları güncelle
    updateSymbolTable(pl, LC);
    updateFRT(pl, LC);
    updateDAT(pl, LC);
    updateHDRM(pl, LC);
    
    // Partial code üret ve .s dosyasına yaz
    generatePartialCode(pl, LC, sfp);
}

