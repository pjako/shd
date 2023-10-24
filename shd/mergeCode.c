
#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"

#include "os/os.h"
#include "os/os_helper.h"
//#include "os/os_app.h" 
//#include "os/os_log.h"

s32 os_main(s32 argc, char* argv[]) {
    Arena* arena = os_tempMemory();

    Str8 paths[] = {str8("engine/base"), str8("engine/os"), str8("engine/graphics")};

    for (u32 idx = 0; idx < countOf(paths)) {
        Str8 codeFile = os_fileRead(mem.arena, shaderFileName);

    }

    // create engine.h
    // create engine.c
    // create engine.m

    return 0;
}