#pragma once
#include <stdint.h>
#include <stdio.h> 
typedef struct {
    int process_id;          // ID del proceso al que pertenece el archivo
    char file_name[15];      // Nombre del archivo
    uint32_t file_size;      // Tamaño del archivo en bytes
    uint32_t current_offset; // Posición actual de lectura/escritura
    char mode;               // Modo de acceso ('r' o 'w')
    uint32_t virtual_address; // Dirección virtual del archivo
} osrmsFile;

extern FILE* memory_file;

osrmsFile* os_open(int process_id, char* file_name, char mode);
int os_read_file(osrmsFile* file_desc, char* dest);
int os_write_file(osrmsFile* file_desc, char* src);
void os_close(osrmsFile* file_desc);