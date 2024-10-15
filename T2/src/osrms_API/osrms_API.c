#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "osrms_API.h"





// Definición de una variable global para almacenar la ruta de la memoria
static FILE* memory_file = NULL;

void os_mount(char* memory_path) {
    // Abrir el archivo de memoria
    memory_file = fopen(memory_path, "r+b"); // Abierto en modo lectura/escritura binaria
    if (memory_file == NULL) {
        perror("Error al abrir el archivo de memoria");
        exit(EXIT_FAILURE);
    }

    printf("Memoria montada desde: %s\n", memory_path);
}


#define PCB_TABLE_SIZE 32     // La tabla de PCBs tiene 32 entradas
#define PCB_ENTRY_SIZE 256    // Cada entrada en la tabla de PCBs ocupa 256 bytes
#define PCB_TABLE_OFFSET 0    // La tabla de PCBs empieza en el offset 0 de la memoria
#define FILE_TABLE_OFFSET 13    // Offset dentro del PCB para acceder a la Tabla de Archivos
#define FILE_ENTRY_SIZE 23        // Tamaño de cada entrada en la Tabla de Archivos
#define MAX_FILES_PER_PROCESS 5   // Máximo número de archivos por proceso


void os_ls_processes() {
    // Mover el puntero del archivo a la posición inicial de la tabla de PCBs
    fseek(memory_file, PCB_TABLE_OFFSET, SEEK_SET);

    printf("Procesos en ejecución:\n");
    printf("----------------------------\n");

    int found_running_process = 0;

    // Iterar sobre todas las entradas de la tabla de PCBs
    for (int i = 0; i < PCB_TABLE_SIZE; i++) {
        uint8_t state;         // 1 byte para el estado
        uint8_t process_id;    // 1 byte para el ID del proceso
        char process_name[12]; // 11 bytes para el nombre, más 1 byte para el '\0'

        // Leer el estado del proceso (1 byte)
        fread(&state, 1, 1, memory_file);

        // Leer el ID del proceso (1 byte)
        fread(&process_id, 1, 1, memory_file);

        // Leer el nombre del proceso (11 bytes)
        fread(process_name, 1, 11, memory_file);
        process_name[11] = '\0'; // Asegurar que el nombre esté terminado con '\0'

        // Saltar los 115 bytes de la Tabla de Archivos y 128 bytes de la Tabla de Páginas
        fseek(memory_file, 115 + 128, SEEK_CUR);

        // Verificar si el proceso está en ejecución (estado == 0x01)
        if (state == 0x01) {
            printf("ID: %d, Nombre: %s\n", process_id, process_name);
            found_running_process = 1;
        }
    }

    if (!found_running_process) {
        printf("No hay procesos en ejecución.\n");
    }
    printf("----------------------------\n");
}


int os_exists(int process_id, char* file_name) {
    // Mover el puntero del archivo a la tabla de PCBs
    fseek(memory_file, PCB_TABLE_OFFSET, SEEK_SET);

    // Iterar sobre todas las entradas de la tabla de PCBs para buscar el proceso con process_id
    for (int i = 0; i < PCB_TABLE_SIZE; i++) {
        uint8_t state;
        uint8_t pid;
        
        // Leer el estado del proceso (1 byte) y su ID (1 byte)
        fread(&state, 1, 1, memory_file);
        fread(&pid, 1, 1, memory_file);

        // Si encontramos el PCB del proceso con el ID correcto
        if (pid == process_id && state == 0x01) {
            // Saltar el resto de la entrada del PCB hasta la Tabla de Archivos
            fseek(memory_file, FILE_TABLE_OFFSET - 2, SEEK_CUR); // Saltar los bytes antes de la tabla de archivos

            // Ahora leemos la Tabla de Archivos de este proceso
            for (int j = 0; j < MAX_FILES_PER_PROCESS; j++) {
                uint8_t valid;
                char current_file_name[15];  // Correcta variable para el nombre del archivo
                uint32_t file_size;

                // Leer el byte de validez y el nombre del archivo (14 bytes)
                fread(&valid, 1, 1, memory_file);
                fread(current_file_name, 1, 14, memory_file);
                current_file_name[14] = '\0'; // Asegurar el null-terminator

                // Leer el tamaño del archivo (4 bytes)
                fread(&file_size, 4, 1, memory_file);

                // Saltar la dirección virtual (4 bytes)
                fseek(memory_file, 4, SEEK_CUR);

                // Depuración
                // printf("Entrada archivo %d: Validez: %d, Nombre: %s, Tamaño: %u\n", j, valid, current_file_name, file_size);

                // Verificar si el archivo es válido y si el nombre coincide
                if (valid == 0x01 && strcmp(current_file_name, file_name) == 0) {
                    return 1; // Archivo encontrado
                }
            }
            return 0; // Si recorremos toda la Tabla de Archivos y no lo encontramos
        }

        // Saltar al siguiente PCB si este no es el proceso que buscamos
        fseek(memory_file, PCB_ENTRY_SIZE - 2, SEEK_CUR); // Saltar al siguiente PCB
    }

    return 0; // Proceso no encontrado o archivo no encontrado
}


void os_ls_files(int process_id) {
    // Mover el puntero del archivo a la tabla de PCBs
    fseek(memory_file, PCB_TABLE_OFFSET, SEEK_SET);

    // Iterar sobre todas las entradas de la tabla de PCBs para buscar el proceso con process_id
    for (int i = 0; i < PCB_TABLE_SIZE; i++) {
        uint8_t state;
        uint8_t pid;

        // Leer el estado del proceso (1 byte) y su ID (1 byte)
        fread(&state, 1, 1, memory_file);
        fread(&pid, 1, 1, memory_file);

        // Depuración
        // printf("Leyendo entrada número %d, PID: %d, Estado: %d\n", i, pid, state);

        // Si el proceso está en ejecución y su ID coincide con process_id
        if (pid == process_id && state == 0x01) {
            printf("Archivos del proceso ID %d:\n", process_id);

            // Posicionar el puntero al inicio de la Tabla de Archivos dentro del PCB
            fseek(memory_file, FILE_TABLE_OFFSET - 2, SEEK_CUR);

            // Recorrer la Tabla de Archivos
            for (int j = 0; j < MAX_FILES_PER_PROCESS; j++) {
                uint8_t valid;
                char file_name[15];
                uint32_t file_size;

                // Leer el byte de validez y el nombre del archivo (14 bytes)
                fread(&valid, 1, 1, memory_file);
                fread(file_name, 1, 14, memory_file);
                file_name[14] = '\0'; // Asegurar el null-terminator

                // Leer el tamaño del archivo (4 bytes)
                fread(&file_size, 4, 1, memory_file);

                // Saltar la dirección virtual (4 bytes)
                fseek(memory_file, 4, SEEK_CUR);

                // Depuración
                //printf("Entrada archivo %d: Validez: %d, Nombre: %s, Tamaño: %u\n", j, valid, file_name, file_size);

                // Si el archivo es válido, imprimir su nombre y tamaño
                if (valid == 0x01) {
                    printf("Nombre: %s, Tamaño: %u bytes\n", file_name, file_size);
                }
            }
            return; // Terminar después de listar los archivos del proceso
        }

        // Saltar al siguiente PCB si este no es el proceso que buscamos
        fseek(memory_file, PCB_ENTRY_SIZE - 2, SEEK_CUR); // Saltar al siguiente PCB
    }

    // Si el proceso no está en ejecución o no se encuentra
    printf("No se encontraron archivos para el proceso ID %d.\n", process_id);
}




#define FRAME_BITMAP_OFFSET (8 * 1024 + 128 + 128 * 1024)  // Offset del Frame Bitmap en la memoria
#define FRAME_BITMAP_SIZE 8192    // 8 KB = 8192 Bytes = 65536 bits
#define TOTAL_FRAMES 65536        // 65536 frames en total
#define BITS_PER_BYTE 8           // 8 bits por cada byte


// Función para imprimir el estado del Frame Bitmap
void os_frame_bitmap() {
    uint8_t frame_bitmap[FRAME_BITMAP_SIZE];  // Buffer para leer el Frame Bitmap
    int occupied_count = 0;  // Contador de frames ocupados
    int free_count = 0;      // Contador de frames libres

    // Mover el puntero del archivo al inicio del Frame Bitmap
    fseek(memory_file, FRAME_BITMAP_OFFSET, SEEK_SET);

    // Leer los 8 KB del Frame Bitmap desde la memoria
    fread(frame_bitmap, 1, FRAME_BITMAP_SIZE, memory_file);

    printf("Estado actual del Frame Bitmap:\n");
    printf("-------------------------------\n");

    // Iterar sobre cada bit del Frame Bitmap y analizar su estado
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        // Calcular el índice del byte y el bit dentro del byte
        int byte_index = i / BITS_PER_BYTE;
        int bit_index = i % BITS_PER_BYTE;

        // Verificar si el bit está en 1 (ocupado) o en 0 (libre)
        if (frame_bitmap[byte_index] & (1 << bit_index)) {
            printf("Frame %d: Ocupado\n", i);
            occupied_count++;
        } else {
            printf("Frame %d: Libre\n", i);
            free_count++;
        }
    }

    // Imprimir el resumen final
    printf("-------------------------------\n");
    printf("Total de frames ocupados: %d\n", occupied_count);
    printf("Total de frames libres: %d\n", free_count);
}


#define TP_BITMAP_OFFSET (8 * 1024)    // Offset del Bitmap de Tablas de Páginas, después de los 8 KB de la Tabla de PCBs
#define TP_BITMAP_SIZE 128             // 128 bytes (1024 bits)
#define TOTAL_TP 1024                  // Total de Tablas de Páginas de Segundo Orden (1024)

void os_tp_bitmap() {
    uint8_t tp_bitmap[TP_BITMAP_SIZE];  // Buffer para leer el Bitmap de Tablas de Páginas
    int occupied_count = 0;  // Contador de tablas ocupadas
    int free_count = 0;      // Contador de tablas libres

    // Mover el puntero del archivo al inicio del Bitmap de Tablas de Páginas
    fseek(memory_file, TP_BITMAP_OFFSET, SEEK_SET);

    // Leer los 128 bytes del Bitmap de Tablas de Páginas desde la memoria
    fread(tp_bitmap, 1, TP_BITMAP_SIZE, memory_file);

    printf("Estado actual del Bitmap de Tablas de Páginas:\n");
    printf("--------------------------------------------\n");

    // Iterar sobre cada bit del Bitmap y analizar su estado
    for (int i = 0; i < TOTAL_TP; i++) {
        // Calcular el índice del byte y el bit dentro del byte
        int byte_index = i / BITS_PER_BYTE;
        int bit_index = i % BITS_PER_BYTE;

        // Verificar si el bit está en 1 (ocupado) o en 0 (libre)
        if (tp_bitmap[byte_index] & (1 << bit_index)) {
            printf("Tabla de Páginas %d: Ocupada\n", i);
            occupied_count++;
        } else {
            printf("Tabla de Páginas %d: Libre\n", i);
            free_count++;
        }
    }

    // Imprimir el resumen final
    printf("--------------------------------------------\n");
    printf("Total de tablas de páginas ocupadas: %d\n", occupied_count);
    printf("Total de tablas de páginas libres: %d\n", free_count);
}