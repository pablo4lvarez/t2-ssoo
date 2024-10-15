#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "osrms_API.h"

// Definición de una variable global para almacenar la ruta de la memoria
static char* memory_file_path = NULL;

void os_mount(char* memory_path) {
    // Guardar la ruta de la memoria en la variable global
    if (memory_file_path != NULL) {
        free(memory_file_path); // Liberar memoria previa si ya se había montado antes
    }

    memory_file_path = (char*)malloc(strlen(memory_path) + 1);
    if (memory_file_path == NULL) {
        perror("Error al asignar memoria para la ruta del archivo");
        exit(EXIT_FAILURE);
    }

    strcpy(memory_file_path, memory_path);

    // Abrir el archivo de memoria para verificar que existe y es accesible
    FILE *mem_file = fopen(memory_file_path, "rb");
    if (mem_file == NULL) {
        perror("Error al abrir el archivo de memoria");
        exit(EXIT_FAILURE);
    }

    // Si el archivo se abrió correctamente, cerrarlo
    fclose(mem_file);

    printf("Memoria montada desde: %s\n", memory_file_path);
}