#include <stdio.h>
#include <string.h>
#include "parser.h"

int main() {
    FILE *fp = fopen("input.asm", "r");
    if (!fp) {
        printf("input.asm dosyasi acilamadi\n");
        return 1;
    }

    char line[100];
    ParsedLine pl;
    int LC = 0;

    while (fgets(line, sizeof(line), fp)) {
        int res = parseLine(line, &pl);
        if (res <= 0) continue;

        // START pseudo-op
        if (strcmp(pl.opcode, "START") == 0) {
            LC = 0;
            continue;
        }

        pl.lc = LC;

        // HOCANIN İSTEDİĞİ FORMATTA ÇIKTI
        printf("LC=%3d | Label: %-8s Opcode: %-8s Operand: %s\n",
               pl.lc,
               strlen(pl.label) ? pl.label : "-",
               pl.opcode,
               strlen(pl.operand) ? pl.operand : "-");

        // Pass 1'in diğer modülleri buradan devam edecek
        // updateSymbolTable(pl);
        // updateFRT(pl);
        // generatePartialCode(pl);

        LC += pl.size;
    }

    fclose(fp);
    return 0;
}
