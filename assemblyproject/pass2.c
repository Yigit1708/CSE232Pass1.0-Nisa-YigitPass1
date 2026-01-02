#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pass2.h"
#include "tables.h" 

// HDRM tablosunda External Reference (M kaydı) olup olmadığını kontrol eder
char* findInHDRM_M_Record(int address) {
    for (int i = 0; i < HDRM_count; i++) {
        if (HDRMT[i].code == 'M' && HDRMT[i].address == address) {
            return HDRMT[i].symbol;
        }
    }
    return NULL;
}

void runPass2(const char *s_file, const char *o_file, const char *t_file) {
    // Girdi (.s) ve çıktı (.o) dosyalarını aç
    FILE *inFile = fopen(s_file, "r");
    FILE *outFile = fopen(o_file, "w");
    
    if (!inFile || !outFile) {
        printf("HATA: Pass 2 dosyalari acilamadi!\n");
        return;
    }


    char line[128];
    
    // Satır satır okuma ve işleme döngüsü
    while (fgets(line, sizeof(line), inFile)) {
        line[strcspn(line, "\r\n")] = 0; // Temizlik
        if (strlen(line) < 2) continue;

        int lc;
        char opcode[5], op1[5], op2[5];
        int tokenCount = 0;

        // Değişkenleri sıfırla ve satırı parse et
        opcode[0] = '\0'; op1[0] = '\0'; op2[0] = '\0';
        tokenCount = sscanf(line, "%d %s %s %s", &lc, opcode, op1, op2);

        if (tokenCount < 2) continue; 

        // LC ve Opcode'u dosyaya yaz
        fprintf(outFile, "%d %s ", lc, opcode);

        // Operand yoksa (sadece opcode varsa) satırı bitir
        if (tokenCount == 2) {
            fprintf(outFile, "\n");
            continue;
        }

        // Eğer operand '??' ise adres çözümlemesi yap (Pass 2'nin ana görevi)
        if (strcmp(op1, "??") == 0) {
            int lookupAddr = lc + 1;
            char *symbolName = NULL;
            int isExternal = 0;

            // Önce FRT'ye, bulunamazsa HDRM'ye (External) bak
            for (int i = 0; i < FRT_count; i++) {
                if (FRT[i].address == lookupAddr) {
                    symbolName = FRT[i].symbol;
                    break;
                }
            }
            if (symbolName == NULL) {
                symbolName = findInHDRM_M_Record(lookupAddr);
                if (symbolName != NULL) isExternal = 1;
            }

            // Sembol bulunduysa uygun formatta yaz
            if (symbolName != NULL) {
                if (isExternal) {
                    // External referanslar ?? ?? olarak kalır
                    fprintf(outFile, "?? ??\n");
                }
                else {
                    // İç referans: Symbol Table'dan adresi alıp yaz
                    int symbolAddr = findInSymbolTable(symbolName);
                    if (symbolAddr != -1) {
                        fprintf(outFile, "%02d %02d\n", (symbolAddr >> 8) & 0xFF, symbolAddr & 0xFF);
                    } else {
                        fprintf(outFile, "?? ??\n"); // Hata: Sembol tabloda yok
                    }
                }
            } else {
                fprintf(outFile, "?? ??\n"); // Hata: Referans bulunamadı
            }
        } 
        else {
            // Zaten çözülmüş operandı (veya sayıyı) olduğu gibi yaz
            fprintf(outFile, "%s", op1);
            if (tokenCount > 3) fprintf(outFile, " %s", op2);
            fprintf(outFile, "\n");
        }
    }

    fclose(inFile);
    fclose(outFile);
    
    // PDF'de istenen: Pass 2 sonunda DAT ve HDRM tablolarını .t dosyasına yaz
    FILE *tfp = fopen(t_file, "w");
    if (tfp) {
        // 1. SYMBOL TABLE
        fprintf(tfp, "=== SYMBOL TABLE ===\n");
        for (int i = 0; i < ST_count; i++) {
            fprintf(tfp, "Symbol: %-10s Address: %d\n", ST[i].symbol, ST[i].address);
        }
        
        // 2. FORWARD REFERENCE TABLE
        fprintf(tfp, "\n=== FORWARD REFERENCE TABLE ===\n");
        for (int i = 0; i < FRT_count; i++) {
            fprintf(tfp, "Address: %d Symbol: %s\n", FRT[i].address, FRT[i].symbol);
        }
        
        // 3. DIRECT ADDRESS TABLE (DAT)
        fprintf(tfp, "=== DIRECT ADDRESS TABLE (DAT) ===\n");
        for (int i = 0; i < DAT_count; i++) {
            fprintf(tfp, "%d\n", DAT[i].address);
        }
        
        // 4. HDRM TABLE
        fprintf(tfp, "\n=== HDRM TABLE ===\n");
        for (int i = 0; i < HDRM_count; i++) {
            fprintf(tfp, "%c %s %d\n", HDRMT[i].code, HDRMT[i].symbol, HDRMT[i].address);
        }
        fclose(tfp);
    }
}