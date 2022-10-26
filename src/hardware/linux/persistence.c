#include "../../pch.h"
#include "../../persistence.h"
#include "../../protocol.h"
#include "../../appio.h"
#include <unistd.h>
#include <limits.h>

/**
 * The RAM backed-up data for writings and readings
 */
PersistentData pers_data;

#define PERSISTENT_SIZE (sizeof(PersistentData))

static PersistentData s_persistentData = { 
    // Zero GUID by default, it means unassigned
    { 0, 0, 0, 0, 0 }, 

    // Used by bus secondary
    {
        UNASSIGNED_SUB_ADDRESS, 
        0xff
    },
    
#ifdef HAS_PERSISTENT_SINK_DATA
    PERSISTENT_SINK_DATA_DEFAULT_DATA
#endif
};

static FILE* pers_fopen(const char* mode) {
    char cwd[PATH_MAX];
    flog("Opening: %shome.mem", getcwd(cwd, PATH_MAX));
    return fopen("home.mem", mode);
}

void pers_load()
{
    FILE* file = pers_fopen("rb");
    if (file) {
        if (fread(&pers_data, PERSISTENT_SIZE, 1, file) == 1) {
            flog("Persistence file read");
        }
        fclose(file);
    } else {
        flog("Persistence file read err: %d", errno);
        pers_data = s_persistentData;
    }
}

void pers_save()
{
    FILE* file = pers_fopen("wb");
    if (file) {
        if (fwrite(&pers_data, PERSISTENT_SIZE, 1, file) == 1) {
            flog("Persistence file written");
        }
        fclose(file);
    } else {
        flog("Persistence file write err: %d", errno);
    }
}
