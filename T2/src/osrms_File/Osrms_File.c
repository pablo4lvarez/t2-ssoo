#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "Osrms_File.h"


// Definir el tamaño de la tabla de archivos
#define MAX_FILES_PER_PROCESS 5   // Máximo número de archivos por proceso
#define FILE_TABLE_OFFSET 13      // Offset dentro del PCB para acceder a la Tabla de Archivos
#define PCB_TABLE_OFFSET 0        // Offset inicial de la tabla de PCBs en la memoria
#define PCB_ENTRY_SIZE 256        // Tamaño de cada entrada en la tabla de PCBs
#define PCB_TABLE_SIZE 32         // Número máximo de procesos

// Definir la variable global para el archivo de memoria
// extern FILE* memory_file;


// // Función para abrir un archivo perteneciente a un proceso
// osrmsFile* os_open(int process_id, char* file_name, char mode) {
//     // Verificar que el modo sea válido ('r' o 'w')
//     if (mode != 'r' && mode != 'w') {
//         return NULL;
//     }

//     // Posicionar el puntero en la Tabla de Archivos del proceso
//     fseek(memory_file, PCB_TABLE_OFFSET, SEEK_SET);

//     // Iterar sobre todas las entradas de la tabla de PCBs para buscar el proceso con process_id
//     for (int i = 0; i < PCB_TABLE_SIZE; i++) {
//         uint8_t state;
//         uint8_t pid;

//         // Leer el estado del proceso (1 byte) y su ID (1 byte)
//         fread(&state, 1, 1, memory_file);
//         fread(&pid, 1, 1, memory_file);

//         // Si encontramos el PCB del proceso con el ID correcto
//         if (pid == process_id && state == 0x01) {
//             // Saltar hasta la Tabla de Archivos dentro del PCB
//             fseek(memory_file, FILE_TABLE_OFFSET - 2, SEEK_CUR); 

//             // Iterar sobre la Tabla de Archivos
//             for (int j = 0; j < MAX_FILES_PER_PROCESS; j++) {
//                 uint8_t valid;
//                 char current_file_name[15];
//                 uint32_t file_size;
//                 uint32_t virtual_address;

//                 // Leer el byte de validez y el nombre del archivo (14 bytes)
//                 fread(&valid, 1, 1, memory_file);
//                 fread(current_file_name, 1, 14, memory_file);
//                 current_file_name[14] = '\0';  // Asegurar el null-terminator

//                 // Leer el tamaño del archivo y la dirección virtual
//                 fread(&file_size, 4, 1, memory_file);
//                 fread(&virtual_address, 4, 1, memory_file);

//                 // Si estamos en modo lectura ('r') y encontramos el archivo, retornar un puntero al archivo
//                 if (mode == 'r' && valid == 0x01 && strcmp(current_file_name, file_name) == 0) {
//                     osrmsFile* file = (osrmsFile*)malloc(sizeof(osrmsFile));
//                     file->process_id = process_id;
//                     strcpy(file->file_name, current_file_name);
//                     file->file_size = file_size;
//                     file->current_offset = 0;  // Inicializar el offset en 0
//                     file->mode = mode;
//                     file->virtual_address = virtual_address;
//                     return file;
//                 }

//                 // Si estamos en modo escritura ('w') y encontramos el archivo, retornar NULL (ya existe)
//                 if (mode == 'w' && valid == 0x01 && strcmp(current_file_name, file_name) == 0) {
//                     return NULL;  // El archivo ya existe, no se puede crear
//                 }

//                 // Saltar a la siguiente entrada
//                 fseek(memory_file, 4, SEEK_CUR);  // Saltar la dirección virtual
//             }

//             // Si estamos en modo escritura ('w') y no encontramos el archivo, crear uno nuevo
//             if (mode == 'w') {
//                 osrmsFile* new_file = (osrmsFile*)malloc(sizeof(osrmsFile));
//                 new_file->process_id = process_id;
//                 strcpy(new_file->file_name, file_name);
//                 new_file->file_size = 0;  // Archivo recién creado, tamaño 0
//                 new_file->current_offset = 0;
//                 new_file->mode = mode;
//                 new_file->virtual_address = 0;  // Dirección virtual asignada posteriormente
//                 return new_file;
//             }
//         }

//         // Saltar al siguiente PCB si este no es el proceso que buscamos
//         fseek(memory_file, PCB_ENTRY_SIZE - 2, SEEK_CUR);
//     }

//     return NULL;  // Proceso no encontrado o error
// }