#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "loader.h"
#include "tables.h"

// Memory array (global) - tables.h'da extern olarak tanımlı
extern struct Memory M[MEMORY_SIZE];
static int memory_count = 0; // Loader'a özel sayaç

// Opcode hex string'den instruction size bul
// Opcode tablosu (pass1.c'deki ile aynı)
typedef struct {
    char opcode[3];
    int bytes;
} OpcodeSize;

static OpcodeSize opcodeSizes[] = {
    {"A1", 3}, {"A2", 2}, {"A3", 3}, {"A4", 2},
    {"E1", 3}, {"E2", 2}, {"F1", 3},
    {"C1", 3}, {"B4", 3}, {"B1", 3}, {"B2", 3}, {"B3", 3},
    {"D2", 1}, {"D1", 1}, {"C2", 1}, {"FE", 1}
};

static int opcodeSizesCount = sizeof(opcodeSizes) / sizeof(OpcodeSize);

int getInstructionSizeFromOpcode(const char *opcode_hex) {
    for (int i = 0; i < opcodeSizesCount; i++) {
        if (strcmp(opcodeSizes[i].opcode, opcode_hex) == 0) {
            return opcodeSizes[i].bytes;
        }
    }
    return 1; // Default: 1 byte (BYTE directive veya bilinmeyen)
}

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
        return;
    }
    
    char line[1024];
    
    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = 0;
        
        if (strlen(line) < 1) continue;
        
        // Satır formatı: LC hex1 hex2 hex3 ... (16 byte per line)
        int lc;
        char *token = strtok(line, " \t");
        if (!token) continue;
        
        lc = atoi(token);
        int current_addr = loadpoint + lc;
        
        // Hex byte'ları oku
        while ((token = strtok(NULL, " \t")) != NULL) {
            int byte_val = 0;
            if (sscanf(token, "%x", &byte_val) == 1) {
                char byte_str[4];
                snprintf(byte_str, sizeof(byte_str), "%02X", byte_val & 0xFF);
                addToMemory(current_addr, byte_str);
                current_addr++;
            }
        }
    }
    
    fclose(fp);
}


// DAT'taki adresler için relocation uygula
// Her DAT address'i için operand byte'larını oku, symbol absolute address'ini al, loadpoint ekle
void applyRelocation(int loadpoint, int *dat_addresses, int dat_count) {
    for (int i = 0; i < dat_count; i++) {
        // DAT'taki address .exe dosyasındaki offset (LC)
        int exe_offset = dat_addresses[i];
        // Memory'deki absolute address
        int abs_addr = loadpoint + exe_offset;
        
        // Operand byte'larını oku (16-bit address)
        int byte1_addr = abs_addr;
        int byte2_addr = abs_addr + 1;
        
        int byte1_val = -1, byte2_val = -1;
        
        // Memory'den operand byte'larını oku
        for (int j = 0; j < memory_count; j++) {
            if (M[j].address == byte1_addr) {
                sscanf(M[j].symbol, "%x", &byte1_val);
            }
            if (M[j].address == byte2_addr) {
                sscanf(M[j].symbol, "%x", &byte2_val);
            }
        }
        
        if (byte1_val >= 0 && byte2_val >= 0) {
            // Operand'taki 16-bit address'i oku (symbol absolute address BEFORE loading)
            // Big-endian format: high_byte low_byte
            int symbol_abs_addr = (byte1_val << 8) | byte2_val;
            
            // final_address = loadpoint + symbol_absolute_address
            int final_addr = loadpoint + symbol_abs_addr;
            
            // Operand byte'larını güncelle (big-endian format)
            // final_addr'i 16-bit big-endian olarak yaz (DECIMAL format)
            char high_str[4], low_str[4];
            int high_byte = (final_addr >> 8) & 0xFF;
            int low_byte = final_addr & 0xFF;
            snprintf(high_str, sizeof(high_str), "%02d", high_byte);
            snprintf(low_str, sizeof(low_str), "%02d", low_byte);
            
            addToMemory(byte1_addr, high_str);
            addToMemory(byte2_addr, low_str);
        }
    }
}

// Memory array'de belirli bir address'teki byte'ı bul
const char* getByteAtAddress(int addr) {
    for (int i = 0; i < memory_count; i++) {
        if (M[i].address == addr) {
            return M[i].symbol;
        }
    }
    return NULL;
}

// Memory array'i ekrana yazdır (Instruction-based format)
void printMemoryArray(int start_addr, int end_addr) {
    // Memory array'i address'e göre sırala
    for (int i = 0; i < memory_count - 1; i++) {
        for (int j = 0; j < memory_count - i - 1; j++) {
            if (M[j].address > M[j + 1].address) {
                struct Memory temp = M[j];
                M[j] = M[j + 1];
                M[j + 1] = temp;
            }
        }
    }
    
    if (memory_count == 0) return;
    
    // Minimum ve maksimum address'leri bul
    int min_addr = M[0].address;
    int max_addr = M[memory_count - 1].address;
    
    // Instruction-based yazdır
    int current_addr = min_addr;
    int processed[1000] = {0}; // Processed byte'ları işaretle (basit yaklaşım)
    
    while (current_addr <= max_addr) {
        // Bu address'te byte var mı kontrol et
        const char *opcode_hex = getByteAtAddress(current_addr);
        if (!opcode_hex) {
            current_addr++;
            continue;
        }
        
        // Bu address zaten işlendi mi? (bir instruction'ın operand byte'ı mı?)
        int addr_index = current_addr - min_addr;
        if (addr_index >= 0 && addr_index < 1000 && processed[addr_index]) {
            current_addr++;
            continue;
        }
        
        // Instruction size'ı bul
        char opcode[4] = {0};
        strncpy(opcode, opcode_hex, 3);
        opcode[3] = '\0';
        int inst_size = getInstructionSizeFromOpcode(opcode);
        
        // Address kontrolü
        if (start_addr != -1 && end_addr != -1) {
            if (current_addr < start_addr || current_addr > end_addr) {
                current_addr += inst_size;
                continue;
            }
        }
        
        // Instruction'ı yazdır: ADDRESS OPCODE OPERAND_HIGH OPERAND_LOW
        printf("%d", current_addr);
        printf(" %s", opcode);
        
        // Operand byte'larını yazdır
        if (inst_size >= 2) {
            const char *byte2 = getByteAtAddress(current_addr + 1);
            if (byte2) {
                printf(" %s", byte2);
                int idx2 = (current_addr + 1) - min_addr;
                if (idx2 >= 0 && idx2 < 1000) processed[idx2] = 1;
            }
        }
        if (inst_size >= 3) {
            const char *byte3 = getByteAtAddress(current_addr + 2);
            if (byte3) {
                printf(" %s", byte3);
                int idx3 = (current_addr + 2) - min_addr;
                if (idx3 >= 0 && idx3 < 1000) processed[idx3] = 1;
            }
        }
        printf("\n");
        
        // Bu address'i işaretle
        if (addr_index >= 0 && addr_index < 1000) processed[addr_index] = 1;
        
        current_addr += inst_size;
    }
}

// Loader ana fonksiyonu
void runLoader(const char *exe_file, const char *t_file) {
    memory_count = 0;
    memset(M, 0, sizeof(M));
    
    int loadpoint;
    printf("Loadpoint giriniz (decimal veya hex): ");
    
    char input[20];
    if (scanf("%s", input) != 1) {
        return;
    }
    
    if (input[0] == '0' && (input[1] == 'x' || input[1] == 'X')) {
        sscanf(input, "%x", &loadpoint);
    } else {
        loadpoint = atoi(input);
    }
    
    loadExeFile(exe_file, loadpoint);
    
    // DAT'ı oku
    int dat_addresses[MAX_DAT];
    int dat_count = 0;
    
    FILE *fp = fopen(t_file, "r");
    if (fp) {
        int in_dat_section = 0;
        char line[256];
        
        while (fgets(line, sizeof(line), fp)) {
            line[strcspn(line, "\r\n")] = 0;
            
            if (strstr(line, "=== DIRECT ADDRESS TABLE (DAT) ===") != NULL) {
                in_dat_section = 1;
                continue;
            }
            
            if (strstr(line, "===") != NULL && in_dat_section) {
                break;
            }
            
            if (in_dat_section) {
                if (strlen(line) == 0) continue;
                int addr = atoi(line);
                if (addr >= 0) {
                    dat_addresses[dat_count] = addr;
                    dat_count++;
                }
            }
        }
        fclose(fp);
    }
    
    // DAT'taki adresler için relocation uygula
    if (dat_count > 0) {
        applyRelocation(loadpoint, dat_addresses, dat_count);
    }
    
    printMemoryArray(-1, -1);
}

