#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "tables.h"
#include "pass1.h"
#include "pass2.h"
int main() {
    FILE *fp = fopen("test_main.asm", "r");
    if (!fp) {
        printf("input.asm dosyasi acilamadi\n");
        return 1;
    }

    // .s dosyasını aç (partial code için)
    FILE *sfp = fopen("output.s", "w");
    if (!sfp) {
        printf("output.s dosyasi olusturulamadi\n");
        fclose(fp);
        return 1;
    }

    // Tabloları başlat
    initTables();

    char line[100];
    ParsedLine pl;
    int LC = 0;
    int isStartFound = 0;

    printf("=== PASS 1 - PARSING VE TABLO OLUŞTURMA ===\n\n");

    while (fgets(line, sizeof(line), fp)) {
        // Boş satırları ve yorumları atla
        int len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[len-1] = '\0';
        
        int res = parseLine(line, &pl);
        if (res <= 0) continue;

        // PROG pseudo-op (LC'yi etkilemez)
        if (strcmp(pl.opcode, "PROG") == 0) {
            printf("PROG: %s\n", pl.operand);
            // HDRM Table'a H (Header) kodu ekle
            addToHDRMTable('H', pl.operand, 0);
            continue;
        }

        // START pseudo-op
        if (strcmp(pl.opcode, "START") == 0) {
            LC = 0;
            isStartFound = 1;
            printf("START\n");
            continue;
        }

        // END pseudo-op
        if (strcmp(pl.opcode, "END") == 0) {
            printf("END\n");
            break;
        }

        // EXTREF ve ENTRY pseudo-ops (LC'yi etkilemez)
        if (strcmp(pl.opcode, "EXTREF") == 0 || strcmp(pl.opcode, "ENTRY") == 0) {
            printf("%s: %s\n", pl.opcode, pl.operand);
            processPass1(&pl, LC, sfp);
            continue;
        }

        // START bulunmadan instruction işleme
        if (!isStartFound) {
            printf("UYARI: START bulunmadan instruction bulundu, atlaniyor\n");
            continue;
        }

        pl.lc = LC;

        // HOCANIN İSTEDİĞİ FORMATTA ÇIKTI
        printf("LC=%3d | Label: %-8s Opcode: %-8s Operand: %s\n",
               pl.lc,
               strlen(pl.label) ? pl.label : "-",
               pl.opcode,
               strlen(pl.operand) ? pl.operand : "-");

        // Pass 1 işlemleri: Tabloları güncelle ve partial code üret
        processPass1(&pl, LC, sfp);

        LC += pl.size;
    }

    fclose(fp);
    fclose(sfp);

    // Tabloları yazdır
    printAllTables();

    // DAT ve HDRM tablolarını .t dosyasına yaz (PDF'de istenen format)
    FILE *tfp = fopen("output.t", "w");
    if (tfp) {
        // 1. SYMBOL TABLE (Eksikti, ekliyoruz)
        fprintf(tfp, "=== SYMBOL TABLE ===\n");
        for (int i = 0; i < ST_count; i++) {
            fprintf(tfp, "Symbol: %-10s Address: %d\n", ST[i].symbol, ST[i].address);
        }

        // 2. FORWARD REFERENCE TABLE (Eksikti, ekliyoruz)
        fprintf(tfp, "\n=== FORWARD REFERENCE TABLE ===\n");
        for (int i = 0; i < FRT_count; i++) {
            fprintf(tfp, "Address: %d Symbol: %s\n", FRT[i].address, FRT[i].symbol);
        }
        // 3. DIRECT ADDRESS TABLE (DAT) ve HDRM TABLE
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
        printf("\nTablolar 'output.t' dosyasina yazildi.\n");
    } else {
        printf("\nUYARI: output.t dosyasi olusturulamadi!\n");
    }

    printf("\n=== PASS 1 TAMAMLANDI ===\n");
    printf("Partial code 'output.s' dosyasina yazildi.\n");

    runPass2();
    return 0;
}
