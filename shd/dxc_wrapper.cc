#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <unknwn.h>
#include <windows.h>
#define ModuleHandle HMODULE
#undef max
#undef min
#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include "WinAdapter.h"
#include <dlfcn.h>
#define LoadLibraryA(name)   dlopen(name, RTLD_NOW)
#define GetProcAddress(h, n) dlsym(h, n)
#define FreeModule(h)        dlclose(h)
#define ModuleHandle         void*
#endif
#include "dxcapi.h"

#include <type_traits>

#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"

#include "os/os.h"
//#include "app/app.h" 
#include "log/log.h"

#include "com_ptr.hpp"

#include "dxc_wrapper.h"

bool dxc_loadLib(dxc_Dll* dll) {
    Arena* arena = NULL; // = os_tempMemory();
    Str8 execPath;
    mem_scoped(mem, arena) {
        execPath = s8(""); // os_execPath(mem.arena);
        
        Str8 paths[] = {
            str_join(mem.arena, execPath, s8("/"), s8("libdxcompiler")),
            str_join(mem.arena, execPath, s8("/"), s8("libdxcompiler"), s8(".dylib")),
            str_join(mem.arena, execPath, s8("/"), s8("dxcompiler"), s8(".dll")),
            str_join(mem.arena, execPath, s8("/"), s8("libdxcompiler"), s8(".so")),
            s8("libdxcompiler"),
            s8("libdxcompiler.dylib"),
            s8("libdxcompiler.so"),
            s8("dxcompiler.dll")
        };

        u32 count = countOf(paths);
        for (u32 idx = 0; idx < count; idx++) {
            dll->dxcDll = os_dlOpen(paths[idx]);
            if (dll->dxcDll != nullptr) break;
        }
    }

    if (!dll->dxcDll) {
        log_error(arena, s8("dxcompiler library not loaded (root dir was \""), execPath, s8("\").\n"));
        return false;
    }

    dll->createProc = (void*) os_dlSym(dll->dxcDll, "DxcCreateInstance"); //result.dxcompiler_dll_.get_proc_address("DxcCreateInstance");

    if (dll->createProc == nullptr) {
        log_error(arena, s8("failed to load DxcCreateInstance"));
        return false;
    }
    return true;
}

typedef struct dxc_Instance {
    DxcCreateInstanceProc       createProc;
    RXCComPtr<IDxcLibrary>        libraryInstance;
    RXCComPtr<IDxcCompiler>       compilerInstance;
    RXCComPtr<IDxcIncludeHandler> includeHandler;
} dxc_Instance;


dxc_Instance* dxc_create(Arena* arena, dxc_Dll* dll) {
    dxc_Instance* result = mem_arenaPushStruct(arena, dxc_Instance);
    mem_setZero(result, sizeof(dxc_Instance));
    
    // Look up the function for creating an instance of the library.
    auto createProc = (DxcCreateInstanceProc) dll->createProc; //result.dxcompiler_dll_.get_proc_address("DxcCreateInstance");
#if 1
    // Instantiate library, compiler and include handler.
    result->libraryInstance = RXCComPtr<IDxcLibrary>([&](auto ptr) {
        return createProc(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (LPVOID*)ptr);
    });

    result->compilerInstance = RXCComPtr<IDxcCompiler>([&](auto ptr) {
        return createProc(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (LPVOID*)ptr);
    });

    result->includeHandler = RXCComPtr<IDxcIncludeHandler>([&](auto ptr) {
        return result->libraryInstance->CreateIncludeHandler(ptr);
    });
#else 
    createProc(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (LPVOID*)&result->libraryInstance);
    createProc(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (LPVOID*)&result->compilerInstance);
    result->libraryInstance->CreateIncludeHandler(&result->includeHandler);
#endif
    return result;
}


dxc_CompileResult dxc_compileHlslToSpv(Arena* arena, dxc_Instance* dxcInstance, dxc_shaderType type, Str8 source, Str8 inputFileName, Str8 entryPoint, StrPair* defines, u32 defineCount, Str8* params, u32 paramsCount) {
    RXCComPtr<IDxcOperationResult> dxcResult{};
    mem_scoped(mem, arena) {

        const Str8 targetProfile = [&type]() {
            switch (type) {
                case dxc_shaderType_vertex:     return s8("vs_6_1");
                case dxc_shaderType_pixel:   return s8("ps_6_1");
                case dxc_shaderType_compute:    return s8("cs_6_1");
                default: s8("");
            }
            return s8("");
        }();
    

        wchar_t** wParams = mem_arenaPushArray(mem.arena, wchar_t*, paramsCount);
        for (u32 idx = 0; idx < paramsCount; idx++) {
            wParams[idx] = (wchar_t*) str_toStr16(mem.arena, params[idx]).content;
        }

        DxcDefine* dxcDefines = mem_arenaPushArray(mem.arena, DxcDefine, defineCount);
        for (u32 idx = 0; idx < defineCount; idx++) {
            dxcDefines[idx] = DxcDefine {(wchar_t*) str_toStr16(mem.arena, defines[idx].left).content, defines[idx].right.size == 0 ? nullptr : ((wchar_t*) str_toStr16(mem.arena, defines[idx].right).content)};
        }

        RXCComPtr<IDxcBlobEncoding> inputBlob = RXCComPtr<IDxcBlobEncoding>([&](auto ptr) {
            return dxcInstance->libraryInstance->CreateBlobWithEncodingFromPinned(source.content, (u32) source.size, 0, ptr);
        });

        Str16 str16InputFileName = str_toStr16(mem.arena, inputFileName);
        const wchar_t* wInputFileName = (const wchar_t*) str16InputFileName.content;
        const wchar_t* wEntryPoint = (const wchar_t*) str_toStr16(mem.arena, entryPoint).content;
        const wchar_t* wTargetPofile = (const wchar_t*) str_toStr16(mem.arena, targetProfile).content;

        auto pIncludeHandler = dxcInstance->includeHandler.get();
        dxcResult = RXCComPtr<IDxcOperationResult>([&](auto ptr) {
            return  dxcInstance->compilerInstance->Compile(
                inputBlob.get(),
                wInputFileName,
                wEntryPoint,
                wTargetPofile,
                (const wchar_t **)  wParams,
                paramsCount,
                dxcDefines,
                defineCount,
                pIncludeHandler,
                ptr);
        });
        /*
          virtual HRESULT STDMETHODCALLTYPE Compile(
    _In_ IDxcBlob *pSource,                       // Source text to compile
    _In_opt_z_ LPCWSTR pSourceName,               // Optional file name for pSource. Used in errors and include handlers.
    _In_opt_z_ LPCWSTR pEntryPoint,               // entry point name
    _In_z_ LPCWSTR pTargetProfile,                // shader profile to compile
    _In_opt_count_(argCount) LPCWSTR *pArguments, // Array of pointers to arguments
    _In_ UINT32 argCount,                         // Number of arguments
    _In_count_(defineCount)
      const DxcDefine *pDefines,                  // Array of defines
    _In_ UINT32 defineCount,                      // Number of defines
    _In_opt_ IDxcIncludeHandler *pIncludeHandler, // user-provided interface to handle #include directives (optional)
    _COM_Outptr_ IDxcOperationResult **ppResult   // Compiler output status, buffer, and errors
  ) = 0;
        */
    }

    dxc_CompileResult result = {};
    auto res = dxcResult.get();
    if (res != nullptr) {
        RXCComPtr<IDxcBlob> dxcSpirvBlob = RXCComPtr<IDxcBlob>([&](auto ptr) {
            return dxcResult->GetResult(ptr);
        });

        if (dxcSpirvBlob.get() != nullptr && dxcSpirvBlob->GetBufferSize() > 0) {
            const u8* dxcBlobStart = reinterpret_cast<u8*>(dxcSpirvBlob->GetBufferPointer());
            u64 size = dxcSpirvBlob->GetBufferSize();
            result.spvCode.content  = (u8*) mem_arenaPush(arena, size);
            result.spvCode.size     = size;
            // passing the blob would be cheaper (probably?) maybe but incompatible with C
            // spirv_blob(dxc_blob_start, dxc_blob_end);
            mem_copy(result.spvCode.content, dxcBlobStart, size);
            return result;
        }

        RXCComPtr<IDxcBlobEncoding> errmsgBlob = RXCComPtr<IDxcBlobEncoding>([&](auto ptr) { return dxcResult->GetErrorBuffer(ptr); });

        if (errmsgBlob.get() != nullptr && errmsgBlob->GetBufferSize() > 0) {
            Str8 error;
            error.content = (u8*) errmsgBlob->GetBufferPointer();
            error.size = errmsgBlob->GetBufferSize();
            result.diagMessage = error;
            return result;
        }
    }

    result.diagMessage = str_lit("Compilation failed for unknown reasons");
    return result;
}
