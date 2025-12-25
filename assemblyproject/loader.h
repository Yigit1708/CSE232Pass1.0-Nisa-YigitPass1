#ifndef LOADER_H
#define LOADER_H

#include "tables.h"

// Loader ana fonksiyonu
// .exe dosyasını ve .t dosyasını okuyup memory array'e yükler
void runLoader(const char *exe_file, const char *t_file);

// Memory array'i ekrana yazdırır
void printMemoryArray(int start_addr, int end_addr);

// Integer'ı 2 karakterlik string'e çevirir (132 -> "01" ve "32")
void intToTwoCharString(int value, char *high, char *low);

#endif // LOADER_H

