#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>  // Para tipos enteros de tamaño fijo
#include "Osrms_File.h"

// Definir el tamaño de la tabla de archivos
#define MAX_FILES_PER_PROCESS 5   // Máximo número de archivos por proceso
#define FILE_TABLE_OFFSET 13      // Offset dentro del PCB para acceder a la Tabla de Archivos
#define PCB_TABLE_OFFSET 0        // Offset inicial de la tabla de PCBs en la memoria
#define PCB_ENTRY_SIZE 256        // Tamaño de cada entrada en la tabla de PCBs
#define PCB_TABLE_SIZE 32         // Número máximo de procesos

#define FRAME_SIZE 32000       // Tamaño de cada frame (por ejemplo, 256 bytes)
#define NUM_FRAMES 65536       // Número de frames en la memoria

// Definir la variable global para el archivo de memoria
extern FILE* memory_file;

// Función para encontrar el primer frame libre en la memoria virtual
uint32_t find_first_free_frame() {
    uint8_t frame_state;

    // Iterar sobre los frames en memoria para encontrar uno libre (estado 0)
    for (uint32_t i = 0; i < NUM_FRAMES; i++) {
        fseek(memory_file, i * FRAME_SIZE, SEEK_SET); // Posicionar en el inicio del frame
        fread(&frame_state, 1, 1, memory_file); // Leer el estado del frame

        if (frame_state == 0x00) {  // Si el frame está libre
            return i * FRAME_SIZE;  // Retornar la dirección virtual de ese frame
        }
    }
    return UINT32_MAX;  // Retornar un valor grande si no hay espacio disponible
}

// Función para abrir un archivo perteneciente a un proceso
osrmsFile* os_open(int process_id, char* file_name, char mode) {
    if (mode != 'r' && mode != 'w') {
        return NULL;  // Modo inválido
    }

    fseek(memory_file, PCB_TABLE_OFFSET, SEEK_SET);  // Ir al inicio de la tabla de PCBs

    // Iterar sobre los PCBs para buscar el proceso con el ID proporcionado
    for (int i = 0; i < PCB_TABLE_SIZE; i++) {
        uint8_t state, pid;
        fread(&state, 1, 1, memory_file);  // Leer estado
        fread(&pid, 1, 1, memory_file);    // Leer ID del proceso

        if (pid == process_id && state == 0x01) {  // Si encontramos el proceso activo
            fseek(memory_file, FILE_TABLE_OFFSET - 2, SEEK_CUR);  // Ir a la Tabla de Archivos

            int used_files = 0;  // Contador de archivos usados

            // Iterar sobre la tabla de archivos del proceso
            for (int j = 0; j < MAX_FILES_PER_PROCESS; j++) {
                uint8_t valid;
                char current_file_name[15];
                uint32_t file_size, virtual_address;

                fread(&valid, 1, 1, memory_file);  // Leer validez
                fread(current_file_name, 1, 14, memory_file);  // Leer nombre
                current_file_name[14] = '\0';  // Null-terminar

                fread(&file_size, 4, 1, memory_file);  // Leer tamaño
                fread(&virtual_address, 4, 1, memory_file);  // Leer dirección virtual

                if (valid == 0x01) {
                    used_files++;  // Incrementar si el archivo es válido
                }

                // Modo lectura: Si el archivo existe, retornar su descriptor
                if (mode == 'r' && valid == 0x01 && strcmp(current_file_name, file_name) == 0) {
                    osrmsFile* file = (osrmsFile*)malloc(sizeof(osrmsFile));
                    file->process_id = process_id;
                    strcpy(file->file_name, current_file_name);
                    file->file_size = file_size;
                    file->current_offset = 0;
                    file->mode = mode;
                    file->virtual_address = virtual_address;
                    return file;
                }

                // Modo escritura: Si ya existe, retornar NULL
                if (mode == 'w' && valid == 0x01 && strcmp(current_file_name, file_name) == 0) {
                    return NULL;  // Archivo ya existe
                }
            }

            // Verificar si la tabla de archivos ya está llena
            if (mode == 'w' && used_files >= MAX_FILES_PER_PROCESS) {
                printf("Error: La tabla de archivos del proceso %d está llena.\n", process_id);
                return NULL;  // No hay espacio para más archivos
            }

            // Crear un nuevo archivo si hay espacio
            uint32_t free_address = find_first_free_frame();  // Buscar primer frame libre
            if (free_address == UINT32_MAX) {
                return NULL;  // No hay espacio disponible
            }

            osrmsFile* new_file = (osrmsFile*)malloc(sizeof(osrmsFile));
            new_file->process_id = process_id;
            strcpy(new_file->file_name, file_name);
            new_file->file_size = 0;
            new_file->current_offset = 0;
            new_file->mode = mode;
            new_file->virtual_address = free_address;

            // Escribir la nueva entrada en la Tabla de Archivos
            fseek(memory_file, -FILE_TABLE_OFFSET, SEEK_CUR);  // Regresar al inicio de la tabla
            uint8_t valid = 0x01;
            fwrite(&valid, 1, 1, memory_file);
            fwrite(new_file->file_name, 1, 14, memory_file);
            fwrite(&new_file->file_size, 4, 1, memory_file);
            fwrite(&new_file->virtual_address, 4, 1, memory_file);

            return new_file;
        }
        fseek(memory_file, PCB_ENTRY_SIZE - 2, SEEK_CUR);  // Ir al siguiente PCB
    }
    return NULL;  // No se encontró el proceso
}


// Función para leer el contenido del archivo desde memoria virtual
int os_read_file(osrmsFile* file_desc, char* dest) {
    if (!file_desc || !dest) {
        return -1;  // Error si el descriptor o la ruta de destino no es válida
    }

    FILE* output_file = fopen(dest, "wb");
    if (!output_file) {
        return -1;  // Error al abrir el archivo de salida
    }

    uint32_t virtual_address = file_desc->virtual_address;
    uint32_t file_size = file_desc->file_size;
    uint32_t bytes_read = 0;
    uint32_t current_frame = virtual_address / FRAME_SIZE;
    uint32_t offset_in_frame = virtual_address % FRAME_SIZE;

    uint8_t buffer[FRAME_SIZE];

    while (bytes_read < file_size) {
        fseek(memory_file, current_frame * FRAME_SIZE, SEEK_SET);
        fread(buffer, 1, FRAME_SIZE, memory_file);

        uint32_t bytes_to_copy = FRAME_SIZE - offset_in_frame;
        if (bytes_to_copy > file_size - bytes_read) {
            bytes_to_copy = file_size - bytes_read;
        }

        fwrite(buffer + offset_in_frame, 1, bytes_to_copy, output_file);

        bytes_read += bytes_to_copy;
        current_frame++;
        offset_in_frame = 0;
    }

    fclose(output_file);
    return bytes_read;
}


int os_write_file(osrmsFile* file_desc, char* src) {
    if (!file_desc || !src) {
        return -1;  // Error: descriptor o ruta fuente inválidos
    }

    // Abrir el archivo de origen en modo lectura binaria
    FILE* input_file = fopen(src, "rb");
    if (!input_file) {
        printf("Error: No se pudo abrir el archivo de origen %s.\n", src);
        return -1;
    }

    // Determinar tamaño del archivo fuente
    fseek(input_file, 0, SEEK_END);
    uint32_t src_size = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);

    uint32_t bytes_written = 0;
    uint32_t current_frame = file_desc->virtual_address / FRAME_SIZE;
    uint32_t offset_in_frame = file_desc->virtual_address % FRAME_SIZE;

    uint8_t buffer[FRAME_SIZE];  // Búfer temporal para lectura

    while (bytes_written < src_size) {
        // Determinar cuántos bytes quedan en el frame actual
        uint32_t available_space_in_frame = FRAME_SIZE - offset_in_frame;
        uint32_t bytes_to_write = available_space_in_frame;

        // Ajustar si el archivo es más pequeño que el espacio disponible
        if (src_size - bytes_written < available_space_in_frame) {
            bytes_to_write = src_size - bytes_written;
        }

        // Leer datos del archivo de entrada al buffer
        fread(buffer + offset_in_frame, 1, bytes_to_write, input_file);

        // Posicionar el puntero al inicio del frame actual
        fseek(memory_file, current_frame * FRAME_SIZE, SEEK_SET);
        fwrite(buffer, 1, offset_in_frame + bytes_to_write, memory_file);

        bytes_written += bytes_to_write;
        offset_in_frame = 0;  // Reiniciar el offset para el siguiente frame

        // Si terminamos de escribir en un frame, pasar al siguiente
        current_frame++;

        // Verificar si hemos agotado los frames disponibles
        if (current_frame >= NUM_FRAMES && bytes_written < src_size) {
            printf("Error: No hay más frames disponibles en la memoria.\n");
            fclose(input_file);
            return -1;
        }
    }

    // Cerrar el archivo de entrada
    fclose(input_file);

    // Actualizar los atributos del descriptor del archivo
    file_desc->file_size = bytes_written;

    return bytes_written;  // Retornar el total de bytes escritos
}
