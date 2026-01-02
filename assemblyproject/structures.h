#ifndef STRUCTURES_H //Structları başka kod dosyalarında bu header dosyasını kullanarak çağıracağız
#define STRUCTURES_H //tanımlarını burada yapıyoruz,baştaki kontrolün sebebi aynı struct yapılarını farklı dosyalarda 
                     //bu dosya ile çağıracağımız için(#include "structures.h") tekrar tekrar tanımlanmasını engellemek
#define MAX_LABEL_LEN 10
#define MAX_OPERAND_LEN 20 // Daha sonra değiştirmek gerekirse rahat değiştirelim diye burada sabit olarak bulunduruyoruz
#define MAX_OPCODE_LEN 10

#define MEMORY_SIZE 500
#define MAX_ST 20
#define MAX_FRT 20
#define MAX_DAT 30
#define MAX_HDRM 20

struct SymbolTable {
    char symbol[MAX_LABEL_LEN];
    int address;
    };

struct ForwardRefTable {
    int address;
    char symbol[MAX_LABEL_LEN];
    };

struct DirectAdrTable {
    int address;
    };

struct HDRMTable {
    char code;
    char symbol[MAX_LABEL_LEN];
    int address;
    };

struct Memory {
    int address;
    char symbol[MAX_OPCODE_LEN+1]; // integers must be converted to string Ex: 132 ->“01” and “32”
    };

typedef struct ParsedLine {
    char label[MAX_LABEL_LEN];
    char opcode[MAX_OPCODE_LEN];
    char operand[MAX_OPERAND_LEN];
    int lc;
    int size;
} ParsedLine;

extern struct SymbolTable ST[MAX_ST]; //extern kodu bu structlardan oluşan bu listeler henüz yoksa bile 
extern struct ForwardRefTable FRT[MAX_FRT]; //ben daha sonra bunları main de tanımlayacağım o yüzden bu listleri main dışındaki
extern struct DirectAdrTable DAT[MAX_DAT]; //yerlerde de kullanmamda bir sakınca yok
extern struct HDRMTable HDRMT[MAX_HDRM  ];
extern struct Memory M[MEMORY_SIZE];

// Sayaçlar (Hangi indexte olduğumuzu bilmek için)
extern int ST_count;
extern int FRT_count;
extern int DAT_count;
extern int HDRM_count;

#endif