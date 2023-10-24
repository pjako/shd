#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"
#include "base/base_debug.h"

#include "os/os.h"
#include "os/os_helper.h"
#include "os/os_app.h" 
#include "os/os_log.h"
#include "os/os_args_helper.h"

s32 os_main(s32 argc, char* argv[]) {
    arg_Opt options[] = {
        {str8("exclude"), 'e', arg_optType_string, arg_flag_required | arg_flag_requiredValue, STR_EMPTY, str8("exclude engine lib"), str8("define engine libs you want to exclude\nExample:\n--exclude audio,graphics,os")},
    };

    arg_Ctx argCtx = arg_makeCtx(&options[0], countOf(options), argv, argc);

    bx errorFound = false;
    bx showHelp = false;
    
    for (s32 opt = 0; (opt = arg_nextOpt(&argCtx)) != arg_end;) {
        switch (opt) {
            case arg_error_duplicate:
            case arg_error_missingArg:
            case arg_error_missingValue:
            case arg_error_invalidValue: {
                errorFound = true;
                showHelp = true;
            } break;
            case 'e': {
                // excluded libraries
            } break;
        }
    }

    return 0;
}