#include "../osrms_API/osrms_API.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{

    // montar la memoria
    os_mount((char *)argv[1]);

    //resto de instrucciones
    
    //os_ls_processes();

    // int check1 = os_exists(105, "carrete.mp4");
    // int check2 = os_exists(228, "caramel.wav");
    // int check3 = os_exists(92, "popcorn.mkv");

    // printf("Check 1: %d\n", check1);
    // printf("Check 2: %d\n", check2);
    // printf("Check 3: %d\n", check3);

    //os_ls_files(228);

    //os_frame_bitmap();

    //os_tp_bitmap();

    // osrmsFile* pointer = os_open(228, "caramel.wav", 'r');
    // if (pointer == NULL) {
    //     printf("No se pudo abrir el archivo\n");
    // } else {
    //     printf("Archivo abierto correctamente\n");
    //     printf("ID del proceso: %d\n", pointer->process_id);
    //     printf("Nombre del archivo: %s\n", pointer->file_name);
    //     printf("Tamaño del archivo: %u bytes\n", pointer->file_size);
    //     printf("Modo de acceso: %c\n", pointer->mode);
    //     printf("Dirección virtual: %u\n", pointer->virtual_address);

    // }

    // return 0;

}