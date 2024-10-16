#include "../osrms_API/osrms_API.h"
#include "../osrms_File/Osrms_File.h"
#include <stdio.h>

int main(int argc, char const *argv[])
{

    // montar la memoria
    os_mount((char *)argv[1]);

    //resto de instrucciones



    // desmontar la memoria
    os_unmount();
    return 0;
}