#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "tables.h"
#include "pass1.h"
#include "pass2.h"

// Dosya adından uzantıyı kaldır ve yeni uzantı ekle
void getOutputFilename(const char *input_file, const char *extension, char *output_file, size_t size) {
    strncpy(output_file, input_file, size - 1);
    output_file[size - 1] = '\0';
    
    // .asm uzantısını bul ve kaldır
    char *dot = strrchr(output_file, '.');
    if (dot != NULL && strcmp(dot, ".asm") == 0) {
        *dot = '\0';
    }
    
    // Yeni uzantıyı ekle
    strncat(output_file, extension, size - strlen(output_file) - 1);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Kullanim: %s <asm_dosyasi>\n", argv[0]);
        printf("Ornek: %s test_main.asm\n", argv[0]);
        return 1;
    }
    
    const char *input_file = argv[1];
    FILE *fp = fopen(input_file, "r");
    if (!fp) {
        printf("HATA: %s dosyasi acilamadi\n", input_file);
        return 1;
    }

    // Çıktı dosya adlarını oluştur
    char s_file[256], o_file[256], t_file[256];
    getOutputFilename(input_file, ".s", s_file, sizeof(s_file));
    getOutputFilename(input_file, ".o", o_file, sizeof(o_file));
    getOutputFilename(input_file, ".t", t_file, sizeof(t_file));

    // .s dosyasını aç (partial code için)
    FILE *sfp = fopen(s_file, "w");
    if (!sfp) {
        printf("HATA: %s dosyasi olusturulamadi\n", s_file);
        fclose(fp);
        return 1;
    }

    // Tabloları başlat
    initTables();

    char line[100];
    ParsedLine pl;
    int LC = 0;
    int isStartFound = 0;


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
    // Not: Pass 2'de de .t dosyası yazılıyor, burada Pass 1 sonu için yazıyoruz
    // Pass 2'de güncel tablolar yazılacak

    // Pass 2'yi çalıştır (dosya adlarını parametre olarak geç)
    runPass2(s_file, o_file, t_file);
    return 0;
}
