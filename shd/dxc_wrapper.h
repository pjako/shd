#ifndef DXC_WRAPPER_H
#define DXC_WRAPPER_H

typedef struct dxc_Dll {
    os_Dl* dxcDll;
    void* createProc;
} dxc_Dll;

typedef struct dxc_CompileResult {
    Str8 spvCode;
    Str8 diagMessage;
} dxc_CompileResult;

bool dxc_loadLib(dxc_Dll* dll);

typedef struct dxc_Instance dxc_Instance;

typedef enum dxc_shaderType {
    dxc_shaderType_vertex,
    dxc_shaderType_pixel,
    dxc_shaderType_compute,
} dxc_shaderType;

typedef struct StrPair {
    Str8 left;
    Str8 right;
} StrPair;

dxc_Instance* dxc_create(Arena* arena, dxc_Dll* dll);
dxc_CompileResult dxc_compileHlslToSpv(Arena* arena, dxc_Instance* dxcInstance, dxc_shaderType type, Str8 source, Str8 inputFileName, Str8 entryPoint, StrPair* defines, u32 defineCount, Str8* params, u32 paramsCount);

#endif