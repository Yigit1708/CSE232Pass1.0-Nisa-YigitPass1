#include <stdio.h>
#include "loader.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Kullanim: %s <exe_dosyasi> <t_dosyasi>\n", argv[0]);
        printf("Ornek: %s exp.exe exp.t\n", argv[0]);
        return 1;
    }
    
    const char *exe_file = argv[1];
    const char *t_file = argv[2];
    
    runLoader(exe_file, t_file);
    
    return 0;
}

