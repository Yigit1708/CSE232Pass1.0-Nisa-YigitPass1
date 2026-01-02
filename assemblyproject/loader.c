#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "loader.h"
#include "tables.h"

// Memory array (global) - structures.h'da extern olarak tanımlı
extern struct Memory M[MEMORY_SIZE];
static int memory_count = 0; // Loader'a özel sayaç

// Integer'ı 2 karakterlik string'e çevirir
// Örnek: 132 -> high="01", low="32"
void intToTwoCharString(int value, char *high, char *low) {
    int high_byte = (value >> 8) & 0xFF;
    int low_byte = value & 0xFF;
    
    // 2 karakterlik string formatında (00-99)
    sprintf(high, "%02d", high_byte);
    sprintf(low, "%02d", low_byte);
}

// Memory array'e bir byte ekle
void addToMemory(int address, const char *byte_str) {
    if (memory_count >= MEMORY_SIZE) {
        printf("HATA: Memory array dolu!\n");
        return;
    }
    
    // Aynı address var mı kontrol et, varsa güncelle
    for (int i = 0; i < memory_count; i++) {
        if (M[i].address == address) {
            strncpy(M[i].symbol, byte_str, sizeof(M[i].symbol) - 1);
            M[i].symbol[sizeof(M[i].symbol) - 1] = '\0';
            return;
        }
    }
    
    // Yeni entry ekle
    M[memory_count].address = address;
    strncpy(M[memory_count].symbol, byte_str, sizeof(M[memory_count].symbol) - 1);
    M[memory_count].symbol[sizeof(M[memory_count].symbol) - 1] = '\0';
    memory_count++;
}

// .exe dosyasını oku ve memory array'e yükle
void loadExeFile(const char *exe_file, int loadpoint) {
    FILE *fp = fopen(exe_file, "r");
    if (!fp) {
        printf("HATA: %s dosyasi acilamadi!\n", exe_file);
        return;
    }
    
    char line[1024];
    int line_num = 0;
    
    printf("\n=== .exe DOSYASI OKUNUYOR ===\n");
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0; // Newline karakterlerini temizle
        
        if (strlen(line) < 1) continue;
        
        // Satır formatı: LC hex1 hex2 hex3 ... (16 byte per line)
        int lc;
        char *token = strtok(line, " \t");
        if (!token) continue;
        
        lc = atoi(token);
        int current_addr = loadpoint + lc;
        
        // Hex byte'ları oku
        while ((token = strtok(NULL, " \t")) != NULL) {
            // Hex byte'ı parse et (örn: "E1", "10", "33")
            int byte_val = 0;
            if (sscanf(token, "%x", &byte_val) == 1) {
                // Byte'ı hexadecimal string'e çevir (örn: E1, 0A, FF)
                char byte_str[4];
                snprintf(byte_str, sizeof(byte_str), "%02X", byte_val & 0xFF);
                
                // Memory array'e ekle
                addToMemory(current_addr, byte_str);
                
                current_addr++;
            }
        }
    }
    
    fclose(fp);
    printf("OK: .exe dosyasi yuklendi (loadpoint=%04X)\n", loadpoint);
}

// .t dosyasından DAT tablosunu oku
void readDATFromFile(const char *t_file, int *dat_addresses, int *dat_count) {
    FILE *fp = fopen(t_file, "r");
    if (!fp) {
        printf("HATA: %s dosyasi acilamadi!\n", t_file);
        return;
    }
    
    *dat_count = 0;
    int in_dat_section = 0;
    char line[256];
    
    printf("\n=== DAT TABLOSU OKUNUYOR ===\n");
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        
        // DAT section başlığını kontrol et
        if (strstr(line, "=== DIRECT ADDRESS TABLE (DAT) ===") != NULL) {
            in_dat_section = 1;
            continue;
        }
        
        // Başka bir section başlığı görürsek DAT section'ı bitir
        if (strstr(line, "===") != NULL && in_dat_section) {
            break;
        }
        
        if (in_dat_section) {
            // Boş satırları atla
            if (strlen(line) == 0) continue;
            
            // Address'i oku
            int addr = atoi(line);
            if (addr >= 0) {
                dat_addresses[*dat_count] = addr;
                (*dat_count)++;
                printf("DAT entry: %d\n", addr);
            }
        }
    }
    
    fclose(fp);
    printf("OK: DAT tablosu okundu (%d entry)\n", *dat_count);
}

// DAT'taki adresler için relocation uygula
void applyRelocation(int loadpoint, int *dat_addresses, int dat_count) {
    printf("\n=== RELOCATION UYGULANIYOR ===\n");
    
    for (int i = 0; i < dat_count; i++) {
        int reloc_addr = dat_addresses[i];
        int abs_addr = loadpoint + reloc_addr;
        
        // Bu adresteki 2 byte'ı oku (16-bit address)
        int byte1_addr = abs_addr;
        int byte2_addr = abs_addr + 1;
        
        // Memory'de bu adresler var mı kontrol et
        int byte1_val = -1, byte2_val = -1;
        
        for (int j = 0; j < memory_count; j++) {
            if (M[j].address == byte1_addr) {
                // Hexadecimal string'i parse et
                sscanf(M[j].symbol, "%x", &byte1_val);
            }
            if (M[j].address == byte2_addr) {
                // Hexadecimal string'i parse et
                sscanf(M[j].symbol, "%x", &byte2_val);
            }
        }
        
        if (byte1_val >= 0 && byte2_val >= 0) {
            // 16-bit address'i oluştur (big-endian)
            int old_addr = (byte1_val << 8) | byte2_val;
            
            // Loadpoint ekle (relocation)
            int new_addr = old_addr + loadpoint;
            
            // Yeni adresi 2 byte olarak hexadecimal string'e çevir
            char high_str[4], low_str[4];
            int high_byte = (new_addr >> 8) & 0xFF;
            int low_byte = new_addr & 0xFF;
            snprintf(high_str, sizeof(high_str), "%02X", high_byte);
            snprintf(low_str, sizeof(low_str), "%02X", low_byte);
            
            // Memory'yi güncelle
            addToMemory(byte1_addr, high_str);
            addToMemory(byte2_addr, low_str);
            
            printf("Relocation: LC=%04X -> %04X -> %04X (loadpoint=%04X)\n", 
                   reloc_addr, old_addr, new_addr, loadpoint);
        }
    }
}

// Memory array'i ekrana yazdır (Hexadecimal format)
void printMemoryArray(int start_addr, int end_addr) {
    printf("\n=== MEMORY ARRAY (M[]) ===\n");
    printf("address | M[]\n");
    printf("--------|-----\n");
    
    // Memory array'i address'e göre sırala (basit bubble sort)
    for (int i = 0; i < memory_count - 1; i++) {
        for (int j = 0; j < memory_count - i - 1; j++) {
            if (M[j].address > M[j + 1].address) {
                struct Memory temp = M[j];
                M[j] = M[j + 1];
                M[j + 1] = temp;
            }
        }
    }
    
    // Yazdır (Hexadecimal format)
    for (int i = 0; i < memory_count; i++) {
        if (start_addr == -1 && end_addr == -1) {
            // Tüm memory'yi yazdır
            printf("%7X | %s\n", M[i].address, M[i].symbol);
        } else if (M[i].address >= start_addr && M[i].address <= end_addr) {
            printf("%7X | %s\n", M[i].address, M[i].symbol);
        }
    }
}

// Loader ana fonksiyonu
void runLoader(const char *exe_file, const char *t_file) {
    // Memory array'i sıfırla
    memory_count = 0;
    memset(M, 0, sizeof(M));
    
    // Loadpoint'i terminalden al
    int loadpoint;
    printf("\n=== LOADER ===\n");
    printf("Loadpoint giriniz (decimal veya hex): ");
    
    char input[20];
    if (scanf("%s", input) != 1) {
        printf("HATA: Gecersiz loadpoint!\n");
        return;
    }
    
    // Hexadecimal veya decimal olarak parse et
    if (input[0] == '0' && (input[1] == 'x' || input[1] == 'X')) {
        sscanf(input, "%x", &loadpoint);
    } else {
        loadpoint = atoi(input);
    }
    
    printf("Loadpoint: %04X (hex) / %d (decimal)\n", loadpoint, loadpoint);
    
    // .exe dosyasını oku ve memory array'e yükle
    loadExeFile(exe_file, loadpoint);
    
    // .t dosyasından DAT tablosunu oku
    int dat_addresses[MAX_DAT];
    int dat_count = 0;
    readDATFromFile(t_file, dat_addresses, &dat_count);
    
    // DAT'taki adresler için relocation uygula
    if (dat_count > 0) {
        applyRelocation(loadpoint, dat_addresses, dat_count);
    }
    
    // Memory array'i ekrana yazdır
    printMemoryArray(-1, -1); // Tüm memory'yi yazdır
    
    printf("\n=== LOADER TAMAMLANDI ===\n");
}

