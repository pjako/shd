#include "base/base.h"
#include "base/base_types.h"
#include "base/base_mem.h"
#include "base/base_str.h"
#include "base/base_args.h"

#include "os/os.h"
//#include "app/app.h" 
#include "log/log.h"
#include "parser/tokenizer.h"
#include "parser/scf.h"

#include "dxc_wrapper.h"

#include "spirv_cross/spirv_parser.hpp"
#include "spirv_cross/spirv_reflect.hpp"

#include <string_view>

//#define typeOf decltype

using namespace spirv_cross;

#define arrDef(TYPE) struct {TYPE* elements; u32 count; u32 capacity;}
#define arrTypeDef(TYPE) typedef struct TYPE##Array {TYPE* elements; u32 count; u32 capacity;} TYPE##Array
#define arrVarDef(TYPE) TYPE##Array  
#define arrInit(ARENA, ARR, CAPACITY) (ARR)->elements = (typeOf((ARR)->elements)) mem_arenaPush((ARENA), CAPACITY * sizeof((ARR)->elements[0])); (ARR)->capacity = (CAPACITY), (ARR)->count = 0
#define arrPushGet(arena, arr) (((arr)->count + 1) < (arr)->capacity ? ((arr)->elements + ((arr)->count++)) : &( (typeOf((arr)->elements)) ((arr)->elements = (typeOf((arr)->elements)) mem_copy(mem_arenaPush((arena), sizeof((arr)->elements[0]) * (arr)->capacity * 2), (arr)->elements, ((arr)->capacity * 2) * sizeof((arr)->elements[0]))))[(arr)->count++])
#define arrFor(ARR, IDXNAME) for(u32 IDXNAME = 0; IDXNAME < (ARR)->count; IDXNAME++)

typedef struct shd_TokenRef {
    Str8 file;
    u32 line;
    Str8 token;
} shd_TokenRef;

typedef struct shd_Error {
    Str8 fileName;
    u32 line;
    Str8 errorMsg;
} shd_Error;

typedef struct shd_MatProperty {
    Str8 name;
    Str8 value;
} shd_MatProperty;

typedef struct shd_Material {
    Str8 name;
    Str8 program;
    arrDef(shd_MatProperty) properties;
} shd_Material;

typedef struct shd_SamplerState {
    Str8    name;
    Str8    addressModeU;
    Str8    addressModeV;
    Str8    addressModeW;
    Str8    magFilter;
    Str8    minFilter;
    Str8    mipmapMode;
    f32     lodMinClamp;
    f32     lodMaxClamp;
    Str8    compare;
    u32     maxAnisotropy;
} shd_SamplerState;


enum resourceType {
    resourceType_constant,
    resourceType_texture,
    resourceType_sampler,
    resourceType__count,
    resourceType_invalid = -1,
};

enum constType {
    constType_f32,
    constType_f32x2,
    constType_f32x3,
    constType_f32x4,
    constType_u32,
    constType__count,
    constType_invalid = -1,
};

typedef struct Str8Pair {
    Str8 left;
    Str8 right;
} Str8Pair;

arrTypeDef(Str8Pair);

static const Str8 constTypeList[constType__count] = {
    str_lit("float"),
    str_lit("float2"),
    str_lit("float3"),
    str_lit("float4"),
    str_lit("uint"),
};

constType getConstType(Str8 str) {
    for (u32 idx = 0; idx < countOf(constTypeList); idx++) {
        if (str_isEqual(str, constTypeList[idx])) {
            return (constType) idx;
        }
    }
    return constType_invalid;
}

static const u32 constTypeByteSize[constType__count] = {
    4,
    4 * 2,
    4 * 3,
    4 * 4,
    4
};

enum textureType {
    textureType_tex2d,
    textureType__count,
};

static const Str8 textureTypeList[textureType__count] = {
    str_lit("Texture2D"),
};

static const u32 textureTypeByteSize[textureType__count] = {
    2,
};

// fnNameToken.text, fnTypeToken.text, indexToken.text, resTypeToken.text, resGroupToken.text
struct ResInfo {
    Str8 name;
    Str8 typeName;
    u32  type;
    u32  byteSize;
};

struct ResArr {
    ResInfo* elements;
    u32 count;
    u32 capacity;
};

struct ResGroupInfo {
    ResArr resTypes[resourceType__count];
    u32 count;
};

typedef struct shd_ResGroup {
    Str8 name;
    u32 constantsSize;
    ResGroupInfo info;
} shd_ResGroup;

typedef struct shd_RasterState {
    Str8 name;
    Str8 cullMode;
    Str8 faceWinding;
    f32 depthBias;
    f32 depthBiasSlopeScale;
    f32 depthBiasClamp;
} shd_RasterState;

typedef struct shd_DepthStencil {
    Str8 name;
    bx disabled;
    Str8 format;
    bx depthWriteEnabled;
    bx stencilEnabled;
    u32 stencilReadMask;
    u32 stencilWriteMask;
    u32 stencilRef;
    Str8 stencilFrontFailOp;
    Str8 stencilFrontDepthFailOp;
    Str8 stencilFrontPassOp;
    Str8 stencilFrontCompareFunc;
    Str8 stencilBackFailOp;
    Str8 stencilBackDepthFailOp;
    Str8 stencilBackPassOp;
    Str8 stencilBackCompareFunc;
} shd_DepthStencil;

// HEADER CODEGEN

void shd_recordHeaderCode(Arena* recordArena, Str8 prefix) {

#if 0
typedef struct ShaderCode {
    u8* code;
    u32 size;
} ShaderCode;
#endif
    str_fmt(recordArena, str8("typedef struct {}ShaderCode {{"), prefix);
    str_join(recordArena, str8("    u8* code;"));
    str_join(recordArena, str8("    u32 size;"));
    str_fmt(recordArena, str8("}} {}ShaderCode;"), prefix);

#if 0
typedef struct ShaderDesc {
    u32 codeIndex;
    Str8 entry;
} ShaderDesc;
#endif
    str_fmt(recordArena, str8("typedef struct {}ShaderDesc {{"), prefix);
    str_join(recordArena, str8("    u32 codeIndex;"));
    str_join(recordArena, str8("    u32 entry;"));
    str_fmt(recordArena, str8("}} {}ShaderDesc;"), prefix);

#if 0
typedef struct RenderProgramDesc {
    Str8 name;
    ShaderDesc vs;
    ShaderDesc ps;
} RenderProgramDesc;
#endif
    str_fmt(recordArena, str8("typedef struct {}RenderProgramDesc {{"), prefix);
    str_join(recordArena, str8("    Str8 name;"));
    str_join(recordArena, str8("    ShaderDesc vs;"));
    str_join(recordArena, str8("    ShaderDesc ps;"));
    str_fmt(recordArena, str8("}} {}RenderProgramDesc;"), prefix);

/*
static ShaderCode shaderCode[] = {{}};

static RenderProgramDesc renderPrograms[] = {{}};
*/
}

void shd_recordCodeGen(Arena* recordArena, Str8 prefix, Str8 fileName) {
    str_join(recordArena, str8("#ifndef _RX_SHADERS_"));
    str_join(recordArena, str8("#define _RX_SHADERS_"));
    shd_recordHeaderCode(recordArena, prefix);

    

    str_join(recordArena, str8("#endif /*_RX_SHADERS_*/"));
}

// PARSING SHADER SOURCE

enum shd_entryPointType {
    shd_entryPointType_vertex,
    shd_entryPointType_pixel,
    shd_entryPointType_compute,
};

typedef struct shd_ShaderEntryPoint {
    Str8 name;
    shd_entryPointType type;
} shd_ShaderEntryPoint;

typedef struct shd_ShaderText {
    struct {
        shd_ShaderEntryPoint* elements;
        uint32_t count;
    } entryPoints;
    Str8 source;
} shd_TextShader;

typedef struct shd_ShaderFileContent {
    shd_ShaderText shader;

} shd_ShaderFileContent;


enum resGroup {
    resGroup0,
    resGroup1,
    resGroup2,
    resGroup3,
    dynGroup0,
    dynGroup1,
    dynGroup__count,
};

struct MetaDataReplaceBlock {
    tn_Token start;
    tn_Token end;
};

struct CodeInfo {
    Str8 name;
    Str8 code;
    ResGroupInfo resGroups[dynGroup__count];

    // blocks that get put into comments, for example:
    // /*[[rx:texture(ResGroup0, 1)]]*/
    struct {
        MetaDataReplaceBlock* elements;
        u32 count;
        u32 capacity;
    } replaceBlocks;

    Str8 resGroupNames[dynGroup__count];
};

typedef enum shd_shaderParamType {
    shd_shaderParamType_f32,
    shd_shaderParamType_f32x2,
    shd_shaderParamType_f32x3,
    shd_shaderParamType_f32x4,
    shd_shaderParamType__count,
    shd_shaderParamType__invalid = - 1,
} shd_shaderParamType;

typedef struct shd_ShaderParam {
    shd_shaderParamType type;
    Str8 name;
    Str8 shortName;
    u32 size;
} shd_ShaderParam;

typedef struct shd_ShaderStorageBuffer {
    u32 flags;
    u32 slot;
    Str8 name;
} shd_ShaderStorageBuffer;

typedef struct shd_Shader {
    Str8 entry;
    shd_entryPointType type;
    Str8 source;
    arrDef(shd_ShaderParam) inputs;
    arrDef(shd_ShaderParam) outputs;
    arrDef(shd_ShaderStorageBuffer) storageBuffers;
    u32 pushSize;
} shd_Shader;

typedef struct RenderProgram {
    Str8 name;
    shd_Shader vs;
    shd_Shader ps;
    //Str8 vsEntry;
    //Str8 psEntry;
    //Str8 vsSpirv;
    //Str8 psSpirv;
} RenderProgram;

void shd_parseSpirvShaderDetails(Arena* arena, shd_Shader* shader, CompilerReflection& compiler) {
    ShaderResources shdResources = compiler.get_shader_resources();

    bx entryPointFound = false;

    const auto entryPoints = compiler.get_entry_points_and_stages();

    std::string_view entryName((char*) shader->entry.content, shader->entry.size);
    // ExecutionModelFragment // ExecutionModelVertex
    for (const auto& item: entryPoints) {
        if (compiler.get_execution_model() == item.execution_model && item.name == entryName) {
            entryPointFound = true;
            break;
        }
    }

    switch (compiler.get_execution_model()) {
        case spv::ExecutionModelVertex:    shader->type = shd_entryPointType_vertex; break;
        case spv::ExecutionModelFragment:  shader->type = shd_entryPointType_pixel; break;
        case spv::ExecutionModelGLCompute: shader->type = shd_entryPointType_compute; break;
        default: ASSERT(!"Unknown execution model"); break;
    }

    arrInit(arena, &shader->inputs, shdResources.stage_inputs.size());
    for (const Resource& resAttr: shdResources.stage_inputs) {
        //attr_t refl_attr;
        uint32_t decoration = compiler.get_decoration(resAttr.id, spv::DecorationLocation);
        shd_ShaderParam* input = &shader->inputs.elements[decoration];
        input->name = str_makeSized(arena, (u8*)resAttr.name.c_str(), resAttr.name.size());
        u32 startIdxShort = str_lastIndexOfChar(input->name, u8'.') + 1;
        input->shortName = str_subStr(input->name, startIdxShort, input->name.size);
        auto type = compiler.get_type(resAttr.type_id);
        auto decorationStr = compiler.get_decoration_string(resAttr.id, spv::DecorationLocation);

        switch (type.basetype) {
            case SPIRType::Float: {
                input->type = (shd_shaderParamType) (((u32)shd_shaderParamType_f32) + type.vecsize - 1);

            } break;
            default: break; // error unknowntype
        }
    }
    shader->inputs.count = shader->inputs.capacity;

    arrInit(arena, &shader->outputs, shdResources.stage_outputs.size());
    for (const Resource& resAttr: shdResources.stage_outputs) {
        //attr_t refl_attr;
        uint32_t decoration = compiler.get_decoration(resAttr.id, spv::DecorationLocation);
        shd_ShaderParam* outputs = &shader->outputs.elements[decoration];
        outputs->name = str_makeSized(arena, (u8*)resAttr.name.c_str(), resAttr.name.size());
        auto type = compiler.get_type(resAttr.type_id);
        auto decorationStr = compiler.get_decoration_string(resAttr.id, spv::DecorationLocation);

        switch (type.basetype) {
            case SPIRType::Float: {
                outputs->type = (shd_shaderParamType) (((u32)shd_shaderParamType_f32) + type.vecsize - 1);

            } break;
            default: break; // error unknowntype
        }
    }
    shader->outputs.count = shader->outputs.capacity;

    // TODO: Handle non bindless case for uniforms/textures/samplers
#if 0
    size_t uniformBuffers = shdResources.uniform_buffers.size();
    for (const Resource& uniformRes: shdResources.uniform_buffers) {
        std::string name = compiler.get_name(uniformRes.id);
        log_trace(str8("fooBar"));
    }
#endif

    arrInit(arena, &shader->storageBuffers, shdResources.storage_buffers.size());
    for (const Resource& storageRes: shdResources.storage_buffers) {
        uint32_t decoration = compiler.get_decoration(storageRes.id, spv::DecorationLocation);
        std::string name = compiler.get_name(storageRes.id);
        
        shd_ShaderStorageBuffer* buffer = arrPushGet(arena, &shader->storageBuffers);
        auto type = compiler.get_type(storageRes.type_id);
        buffer->name = str_makeSized(arena, (u8*)name.c_str(), name.size());
        buffer->slot = decoration;
    }

    if (shdResources.push_constant_buffers.size() > 0) {
        const Resource& pushRes = shdResources.push_constant_buffers[0];
        //std::string name = compiler.get_name(pushRes.id);
        auto type = compiler.get_type(pushRes.type_id);
        shader->pushSize = compiler.get_declared_struct_size(type);
    } else {
        shader->pushSize = 0;
    }
#if 0
    // uniform blocks
    for (const Resource& ub_res: shd_resources.uniform_buffers) {
        std::string n = compiler.get_name(ub_res.id);
        uniform_block_t refl_ub;
        const SPIRType& ub_type = compiler.get_type(ub_res.base_type_id);
        refl_ub.slot = compiler.get_decoration(ub_res.id, spv::DecorationBinding);
        // shift fragment shader uniform blocks binding back to
        if (is_vulkan && (refl_ub.slot >= (int)vk_fs_ub_binding_offset)) {
            refl_ub.slot -= 4;
        }
        refl_ub.size = (int) compiler.get_declared_struct_size(ub_type);
        refl_ub.struct_name = ub_res.name;
        refl_ub.inst_name = compiler.get_name(ub_res.id);
        if (refl_ub.inst_name.empty()) {
            refl_ub.inst_name = compiler.get_fallback_name(ub_res.id);
        }
        refl_ub.flattened = can_flatten_uniform_block(compiler, ub_res);
        for (int m_index = 0; m_index < (int)ub_type.member_types.size(); m_index++) {
            uniform_t refl_uniform;
            refl_uniform.name = compiler.get_member_name(ub_res.base_type_id, m_index);
            const SPIRType& m_type = compiler.get_type(ub_type.member_types[m_index]);
            refl_uniform.type = spirtype_to_uniform_type(m_type);
            if (m_type.array.size() > 0) {
                refl_uniform.array_count = m_type.array[0];
            }
            refl_uniform.offset = compiler.type_struct_member_offset(ub_type, m_index);
            refl_ub.uniforms.push_back(refl_uniform);
        }
        refl.uniform_blocks.push_back(refl_ub);
    }
#endif
}

Str8 shd_getVertexFormatStr(shd_shaderParamType type) {
    switch (type) {
#if 0
        case : return str8("rx_vertexFormat_u8x2");
        case : return str8("rx_vertexFormat_u8x4");
        case : return str8("rx_vertexFormat_s8x2");
        case : return str8("rx_vertexFormat_s8x4");
        case : return str8("rx_vertexFormat_u8x2n");
        case : return str8("rx_vertexFormat_u8x4n");
        case : return str8("rx_vertexFormat_s8x2n");
        case : return str8("rx_vertexFormat_s8x4n");
        case : return str8("rx_vertexFormat_u16x2");
        case : return str8("rx_vertexFormat_u16x4");
        case : return str8("rx_vertexFormat_s16x2");
        case : return str8("rx_vertexFormat_s16x4");
        case : return str8("rx_vertexFormat_u16x2n");
        case : return str8("rx_vertexFormat_u16x4n");
        case : return str8("rx_vertexFormat_s16x2n");
        case : return str8("rx_vertexFormat_s16x4n");
        case : return str8("rx_vertexFormat_f16x2");
        case : return str8("rx_vertexFormat_f16x4");
        case : return str8("rx_vertexFormat_u32x1");
        case : return str8("rx_vertexFormat_u32x2");
        case : return str8("rx_vertexFormat_u32x3");
        case : return str8("rx_vertexFormat_u32x4");
        case : return str8("rx_vertexFormat_s32x1");
        case : return str8("rx_vertexFormat_s32x2");
        case : return str8("rx_vertexFormat_s32x3");
        case : return str8("rx_vertexFormat_s32x4");
#endif
        case shd_shaderParamType_f32: return str8("rx_vertexFormat_f32x1");
        case shd_shaderParamType_f32x2: return str8("rx_vertexFormat_f32x2");
        case shd_shaderParamType_f32x3: return str8("rx_vertexFormat_f32x3");
        case shd_shaderParamType_f32x4: return str8("rx_vertexFormat_f32x4");
        default: break;
    }
    return str8("UNKNOWNTYPE");
}

typedef struct ShaderFileInfo {
    arrDef(RenderProgram) renderPrograms;
    arrDef(CodeInfo) codeInfos;
    arrDef(shd_DepthStencil) depthStencils;
    arrDef(shd_RasterState) rasterStates;
    arrDef(shd_ResGroup) resGroups;
    arrDef(shd_SamplerState) samplerStates;
    arrDef(shd_Material) materials;
} ShaderFileInfo;

Str8 shd_generateHeaderShader(Arena* arena, shd_Shader* shader, Str8 name, Str8 prefix) {
    Str8 tab = str8("    ");
#if 0
typedef struct shd_Shader {
    Str8 entry;
    shd_entryPointType type;
    Str8 source;
    arrDef(shd_ShaderParam) inputs;
    arrDef(shd_ShaderParam) outputs;
    arrDef(shd_ShaderStorageBuffer) storageBuffers;
    u32 pushSize;
} shd_Shader;
#endif

    Str8 shaderShort = str8("Vs");
    switch (shader->type) {
        case shd_entryPointType_vertex:  shaderShort = str8("Vs"); break;
        case shd_entryPointType_pixel:   shaderShort = str8("Ps"); break;
        case shd_entryPointType_compute: shaderShort = str8("Cs"); break;
    }


    Str8 generatedCode = {0};
    str_record(generatedCode, arena) {
        // write spirv code

        str_fmt(arena, str8("static const u32 {0}shader{1}Code_{2}[] = {{\n"), prefix, shaderShort, name);
        {
            u32* spirv = (u32*) shader->source.content;
            u32  size  = shader->source.size / 4;
            ASSERT(size * 4 == shader->source.size);
            for (u32 idx = 0;;) {
                str_join(arena, tab);
                for (u32 j = minVal(idx + 8, size);;) {
                    u32 val = spirv[idx];
                    Str8 hexVal = str_u32ToHex(arena, val);
                    idx++;
                    if (idx >= j) break;
                    str_join(arena, str8(", "));
                }
                if (idx >= size) break;
                str_join(arena, str8(",\n"));
            }
        }
        str_join(arena, str8("\n};\n\n"));

        // in
        if (shader->inputs.count > 0) {
            str_fmt(arena, str8("static {0}ShaderInOutValue {0}shader{1}In_{2}[] = {{\n"), prefix, shaderShort, name);
            arrFor(&shader->inputs, idx) {
                shd_ShaderParam* param = shader->inputs.elements + idx;
                Str8 type = shd_getVertexFormatStr(param->type);
                str_join(arena, tab, str8("{"), type, str8(", {(u8*)\""), param->name,str8("\", "), param->name.size, str8("}},\n"));
            }
            str_join(arena, str8("};\n\n"));
        }

        // out
        if (shader->outputs.count > 0) {
            str_fmt(arena, str8("static {0}ShaderInOutValue {0}shader{1}Out_{2}[] = {{\n"), prefix, shaderShort, name);
            arrFor(&shader->outputs, idx) {
                shd_ShaderParam* param = shader->outputs.elements + idx;
                Str8 type = shd_getVertexFormatStr(param->type);
                str_join(arena, tab, str8("{"), type, str8(", {(u8*) \""), param->name,str8("\", "), param->name.size, str8("}},\n"));
            }
            str_join(arena, str8("};\n\n"));
        }

        // storage buffer
        if (shader->storageBuffers.count > 0) {
            str_fmt(arena, str8("static {0}StorageBuffer {0}shader{1}StorageBuffers_{2}[] = {{\n"), prefix, shaderShort, name);
            arrFor(&shader->storageBuffers, idx) {
                shd_ShaderStorageBuffer* param = shader->storageBuffers.elements + idx;
                str_join(arena, tab, str8("{"), param->slot, str8(", {(u8*) \""), param->name, str8("\", "), param->name.size, str8("}},\n"));
            }
            str_join(arena, str8("};\n\n"));
        }


        // write shader struct
        str_fmt(arena, str8("static {0}Shader {0}shader{1}_{2} = {{\n"), prefix, shaderShort, name);

        // entry
        str_join(arena, tab, str8("{(u8*) \""), shader->entry, str8("\", "), shader->entry.size, str8("},\n"));

        // in
        if (shader->inputs.count == 0) {
            str_join(arena, str8("    {NULL, 0},\n"));
        } else {
            str_fmt(arena,  str8("    {{&{0}shader{1}In_{2}[0], {3}}},\n"), prefix, shaderShort, name, shader->inputs.count);
        }

        // out
        if (shader->outputs.count == 0) {
            str_join(arena, str8("    {NULL, 0},\n"));
        } else {
            str_fmt(arena,  str8("    {{&{0}shader{1}Out_{2}[0], {3}}},\n"), prefix, shaderShort, name, shader->outputs.count);
        }

        // storage buffer
        if (shader->storageBuffers.count == 0) {
            str_join(arena, str8("    {NULL, 0},\n"));
        } else {
            str_fmt(arena, str8("    {{&{0}shader{1}StorageBuffers_{2}[0], {3}}},\n"), prefix, shaderShort, name, shader->storageBuffers.count);
        }

        // push size
        str_fmt(arena, str8("    {0},\n"), shader->pushSize);
        // push size
        str_fmt(arena, str8("    {{(u8*) &{0}shader{1}Code_{2}[0], {3}}},\n"), prefix, shaderShort, name, shader->source.size);

        // spirv code
        str_join(arena, str8("};\n\n"));
    }
    return generatedCode;
}

Str8 shd_generateHeader(Arena* arena, ShaderFileInfo* fileInfo, Str8 prefix, CodeInfo* codeInfo, Str8PairArray* typeMap) {

    // build enum

    Str8 generatedCode = {0};
    str_record(generatedCode, arena) {
        Str8 tab = str8("    ");

        // generate default struct that holds generic shader informations


#if 0
        typedef enum generatedShaderType {
            generatedShaderType_bindless,
            generatedShaderType_bindGroups,
        } generatedShaderType;

        typedef struct PREFIX_SHADERNAME_ResGroup0 {

        } PREFIX_SHADERNAME_ResGroup0;

        typedef struct ShaderInOutValue {
            rx_vertexFormat type;
            Str8 name;
        } ShaderInOutValue;

        typedef struct StorageBuffer {
            u32 slot;
            Str8 name;
        } StorageBuffer;

        typedef struct Shader {
            Str8 entry;
            struct {
                ShaderInOutValue* elements;
                uint32_t count;
            } in;
            struct {
                ShaderInOutValue* elements;
                uint32_t count;
            } out;
            struct {
                StorageBuffer* elements;
                uint32_t count;
            } storageBuffers;
            u32 pushSize;
            Str8 code;
        } Shader;

        typedef struct ResBlocks {
            u32 constantSize;
            u32 textureCount;
            u32 samplerCount;
        } ResBlocks;

        typedef struct RenderProgram {
            Str8 name;
            Shader vs;
            Shader ps;
            ResBlocks[6];
        } RenderProgram;
#endif

        str_fmt(arena, str8("typedef struct {}StorageBuffer {{\n"), prefix);
        str_join(arena, str8("    u32 slot;\n"));
        str_join(arena, str8("    Str8 name;\n"));
        str_fmt(arena, str8("}} {}StorageBuffer;\n"), prefix);
        
        str_join(arena, str8("\n"));

        str_fmt(arena, str8("typedef enum {}generatedShaderType {{\n"), prefix);
        str_fmt(arena, str8("    {}generatedShaderType_bindless,\n"), prefix);
        str_fmt(arena, str8("    {}generatedShaderType_bindGroups,\n"), prefix);
        str_fmt(arena, str8("}} {}generatedShaderType;\n"), prefix);
        
        str_join(arena, str8("\n"));

        str_fmt(arena, str8("typedef struct {}ShaderInOutValue {{\n"), prefix);
        str_join(arena, str8("    rx_vertexFormat type;\n"));
        str_join(arena, str8("    Str8 name;\n"));
        str_fmt(arena, str8("}} {}ShaderInOutValue;\n"), prefix);
        
        str_join(arena, str8("\n"));

        str_fmt(arena, str8("typedef struct {}Shader {{\n"), prefix);
        str_join(arena, str8("    Str8 entry;\n"));
        str_join(arena, str8("    struct {\n"));
        str_fmt(arena, str8("        {}ShaderInOutValue* elements;\n"), prefix);
        str_join(arena, str8("        uint32_t count;\n"));
        str_join(arena, str8("    } in;\n"));
        str_join(arena, str8("    struct {\n"));
        str_fmt(arena, str8("        {}ShaderInOutValue* elements;\n"), prefix);
        str_join(arena, str8("        uint32_t count;\n"));
        str_join(arena, str8("    } out;\n"));
        str_join(arena, str8("    struct {\n"));
        str_fmt(arena,  str8("        {}StorageBuffer* elements;\n"), prefix);
        str_join(arena, str8("        u32 count;\n"));
        str_join(arena, str8("    } storageBuffers;\n"));
        str_join(arena, str8("    u32 pushSize;\n"));
        str_join(arena, str8("    Str8 code;\n"));
        str_fmt(arena, str8("}} {}Shader;\n"), prefix);
        
        str_join(arena, str8("\n"));

        str_fmt(arena, str8("typedef struct {}ResBlocks {{\n"), prefix);
        str_join(arena, str8("    u32 constantSize;\n"));
        str_join(arena, str8("    u32 textureCount;\n"));
        str_join(arena, str8("    u32 samplerCount;\n"));
        str_fmt(arena, str8("}} {}ResBlocks;\n"), prefix);
        
        str_join(arena, str8("\n"));

        str_fmt(arena, str8("typedef struct {}RenderProgram {{\n"), prefix);
        str_join(arena, str8("    Str8 name;\n"));
        str_fmt(arena, str8("    {}Shader* vs;\n"), prefix);
        str_fmt(arena, str8("    {}Shader* ps;\n"), prefix);
        str_fmt(arena, str8("    {0}ResBlocks resBlocks[{1}];\n"), prefix, dynGroup__count);
        str_fmt(arena, str8("}} {}RenderProgram;\n"), prefix);
        
        str_join(arena, str8("\n\n"));

        // generate ResBlock structs

        
        str_join(arena, str8("// Global ResBlocks\n"));

        for (u32 idx = 0; idx < fileInfo->resGroups.count; idx++) {
            shd_ResGroup* resGroup = fileInfo->resGroups.elements + idx;
            str_fmt(arena, str8("typedef struct {0}{1} {{\n"), prefix, resGroup->name);
            for (u32 resTypeIdx = 0; resTypeIdx < resourceType__count; resTypeIdx++) {
                ResArr* resArr = &resGroup->info.resTypes[resTypeIdx];
                for (u32 idx = 0; idx < resArr->count; idx++) {
                    ResInfo* resInfo = resArr->elements + idx;
                    Str8 mapName = {};
                    // get type mapped type name
                    for (u32 i = 0; i < typeMap->count; i++) {
                        if (str_isEqual(typeMap->elements[i].left, resInfo->typeName)) {
                            mapName = typeMap->elements[i].right;
                        }
                    }
                    ASSERT(mapName.size > 0 && "Undefined type mapping");
                    str_fmt(arena, str8("    {0} {1};\n"), mapName, resInfo->name);
                }
            }
            str_fmt(arena, str8("}} {0}{1};\n\n"), prefix, resGroup->name);
        }

        str_join(arena, str8("\n"));

        Str8 resDefaultNames[] = {str8("_ResGroup0"), str8("_ResGroup1"), str8("_ResGroup2"), str8("_ResGroup3"), str8("_DynGroup0"), str8("_DynGroup1")};
        for (u32 idx = 0; idx < fileInfo->codeInfos.count; idx++) {
            str_fmt(arena, str8("// Code {} ResGroups\n"), codeInfo->name);
            for (u32 resGroupIdx = 0; resGroupIdx < countOf(codeInfo->resGroups); resGroupIdx++) {
                ResGroupInfo* resGroupInfo = &codeInfo->resGroups[resGroupIdx];
                Str8 groupPrefix = str8("");
                Str8 resBlockName = codeInfo->resGroupNames[resGroupIdx];
                if (resBlockName.size > 0) {
                    bx globalResGroup = false;
                    for (u32 idx = 0; idx < fileInfo->resGroups.count; idx++) {
                        shd_ResGroup* resGroup = fileInfo->resGroups.elements + idx;
                        if (str_isEqual(resBlockName, resGroup->name)) {
                            globalResGroup = true;
                            break;
                        }
                    }
                    if (globalResGroup) {
                        str_fmt(arena, str8("typedef {0}{1} {0}{2}{3};\n\n"), prefix, resBlockName, codeInfo->name, resDefaultNames[resGroupIdx]);
                        continue;
                    }
                } else {
                    groupPrefix = codeInfo->name;
                    resBlockName = resDefaultNames[resGroupIdx];
                }
                // Resgroup is local to the shader, write it out
                bx hasValues = false;
                
                for (u32 resTypeIdx = 0; resTypeIdx < resourceType__count; resTypeIdx++) {
                    ResArr* resArr = &resGroupInfo->resTypes[resTypeIdx];
                    if (resArr->count > 0) {
                        hasValues = true;
                        break;
                    }
                }

                if (hasValues) {
                    str_fmt(arena, str8("typedef struct {0}{1}{2} {{\n"), prefix, groupPrefix, resBlockName);

                    for (u32 resTypeIdx = 0; resTypeIdx < resourceType__count; resTypeIdx++) {
                        ResArr* resArr = &resGroupInfo->resTypes[resTypeIdx];
                        for (u32 idx = 0; idx < resArr->count; idx++) {
                            ResInfo* resInfo = resArr->elements + idx;
                            Str8 mapName = {};
                            for (u32 i = 0; i < typeMap->count; i++) {
                                if (str_isEqual(typeMap->elements[i].left, resInfo->typeName)) {
                                    mapName = typeMap->elements[i].right;
                                }
                            }
                            ASSERT(mapName.size > 0 && "Undefined type mapping");
                            str_fmt(arena, str8("    {0} {1};\n"), mapName, resInfo->name);
                        }
                    }

                    str_fmt(arena, str8("}} {0}{1}{2};\n\n"), prefix, groupPrefix, resBlockName);
                }
            }
        }
        
        str_join(arena, str8("\n"));

        // build enums with render programs
        str_join(arena, str8("// Render Programs\n"));

        str_join(arena, str8("typedef enum "), prefix, str8("renderProgram {\n"));

        for (u32 idx = 0; idx < fileInfo->renderPrograms.count; idx++) {
            RenderProgram* renderProgram = fileInfo->renderPrograms.elements + idx;
            str_join(arena, tab, prefix, str8("renderProgram_"), renderProgram->name, str8(",\n"));
        }

        str_join(arena, tab, prefix, str8("renderProgram__count,\n"));
        str_join(arena, str8("} "), prefix, str8("renderProgram;\n"));

        str_join(arena, str8("\n\n"));

        for (u32 idx = 0; idx < fileInfo->renderPrograms.count; idx++) {
            RenderProgram* renderProgram = fileInfo->renderPrograms.elements + idx;
            str_join(arena, str8("// RenderProgram: "), renderProgram->name, str8("\n\n"));

            // write (spirv) code

            str_fmt(arena, str8("typedef struct {0}{1}Vertex {{\n"), prefix, renderProgram->name, renderProgram->vs.entry);

            for (u32 inputIdx = 0; inputIdx < renderProgram->vs.inputs.count; inputIdx++) {
                shd_ShaderParam* input = renderProgram->vs.inputs.elements + inputIdx;
                static Str8 typeNames[] = {
                    str_lit("f32"),
                    str_lit("Vec2"),
                    str_lit("Vec3"),
                    str_lit("Vec4"),
                };
                Str8 typeName = typeNames[input->type];
                str_fmt(arena, str8("   {0}{1} {2};\n"), prefix, typeName, input->shortName);

                //input->
                //shd_shaderParamType
                //input->stortName
            }

            str_fmt(arena, str8("}} {0}{1}Vertex;\n"), prefix, renderProgram->name, renderProgram->vs.entry);

            str_join(arena, str8("\n"));

            shd_generateHeaderShader(arena, &renderProgram->vs, renderProgram->name, prefix);
            shd_generateHeaderShader(arena, &renderProgram->ps, renderProgram->name, prefix);

            // writer ResGroups (Uniforms)
            /*
            typedef struct RenderProgram {
                Str8 name;
                Shader vs;
                Shader ps;
                ResBlocks resBlocks[6];
            } RenderProgram;*/
            str_fmt(arena, str8("static {0}RenderProgram {0}{1} = {{\n"), prefix, renderProgram->name);
            str_fmt(arena, str8("    {{(u8*) \"{1}\", {2}}}, \n"), prefix, renderProgram->name, renderProgram->name.size);
            str_fmt(arena, str8("    &{0}shaderVs_{1},\n"), prefix, renderProgram->name);
            str_fmt(arena, str8("    &{0}shaderPs_{1},\n"), prefix, renderProgram->name);
            str_fmt(arena, str8("    {{\n"), prefix, renderProgram->name);
            for (u32 idx = 0; idx < countOf(codeInfo->resGroups); idx++) {
                ResGroupInfo* info = codeInfo->resGroups + idx;

                Str8 resBlockName = codeInfo->resGroupNames[idx];
                for (u32 idx = 0; idx < fileInfo->resGroups.count; idx++) {
                    shd_ResGroup* resGroup = fileInfo->resGroups.elements + idx;
                    if (str_isEqual(resGroup->name, resBlockName)) {
                        // use global ResGroup
                        info = &resGroup->info;
                        break;
                    }
                }
                
                ResArr* constants = info->resTypes + resourceType_constant;
                ResArr* textures  = info->resTypes + resourceType_texture;
                ResArr* samplers  = info->resTypes + resourceType_sampler;

                str_fmt(arena, str8("        {{{0}, {1}, {2}}},\n"), constants->count, textures->count, samplers->count);
            }
            str_fmt(arena, str8("    }}\n"), prefix, renderProgram->name);
            str_join(arena, str8("};\n"));
        }


        str_fmt(arena, str8("static {}RenderProgram* {}renderPrograms[] = {{\n"), prefix);
        for (u32 idx = 0; idx < fileInfo->renderPrograms.count; idx++) {
            RenderProgram* renderProgram = fileInfo->renderPrograms.elements + idx;
            str_fmt(arena, str8("    &{0}{1},\n"), prefix, renderProgram->name);
        }

        str_join(arena, str8("};\n"));


        
#if 0
        str_fmt(arena, str8("{}RenderProgram* {}getRenderProgram({}renderProgram program) {{\n"), prefix);
        str_join(arena, tab, str8("switch (program) {\n"));
        for (u32 idx = 0; idx < count; idx++) {
            RenderProgram* renderProgram = renderPrograms + idx;
            str_join(arena, tab, tab, str8("case "), prefix, str8("renderProgram_"), renderProgram->name, str8(": {\n"));



            str_join(arena, tab, tab, str8("} break;"));
        }
#endif
    }
    //log_trace("-- header --\n", generatedCode);
    return generatedCode;
}

bx shd_parseFromFile(Arena* arena, ShaderFileInfo* outFileInfo, Str8 shaderFileName, Str8 shaderFile) {
    //Arena* arena = os_tempMemory();
    log_trace(arena, s8("Compile..."));
    
    scf_Global scf = scf_parse(shaderFile, shaderFileName);

    //arrDef(RenderProgram) renderPrograms;
    arrInit(arena, &outFileInfo->renderPrograms, 20);
    arrInit(arena, &outFileInfo->codeInfos, 20);
    arrInit(arena, &outFileInfo->depthStencils, 20);
    arrInit(arena, &outFileInfo->rasterStates, 20);
    arrInit(arena, &outFileInfo->resGroups, 20);
    arrInit(arena, &outFileInfo->samplerStates, 20);
    arrInit(arena, &outFileInfo->materials, 20);

    //CodeInfo* codeInfo = NULL;
    for (bx running = scf_next(&scf); running;) {
        //log_warn(str8("category: \""), scf.category, "\"");
        if (str_isEqual(scf.category, str8("RenderProgram"))) {
            // record RenderProgram
            Str8 name = {};
            Str8 vsEntry = {};
            Str8 psEntry = {};
            for (running = scf_next(&scf); running && scf.valueType != scf_type_category; running = scf_next(&scf)) {
                if (str_isEqual(scf.key, str8("name"))) {
                    name = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("vsEntry"))) {
                    vsEntry = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("psEntry"))) {
                    psEntry = scf.valueStr;
                }
            }
            //log_traceFmt("RenderProgram: {0} vsEtry: {1} psEntry: {2}", name, vsEntry, psEntry);
            RenderProgram* newProgram = arrPushGet(arena, &outFileInfo->renderPrograms);
            newProgram->name = name;
            newProgram->vs.entry = vsEntry;
            newProgram->ps.entry = psEntry;
            continue;
        } else if (str_isEqual(scf.category, str8("Code"))) {
            CodeInfo* codeInfo = arrPushGet(arena, &outFileInfo->codeInfos);
            for (u32 resGroupIdx = 0; resGroupIdx < countOf(codeInfo->resGroups); resGroupIdx++) {
                for (u32 resTypeIdx = 0; resTypeIdx < countOf(codeInfo->resGroups[0].resTypes); resTypeIdx++) {
                    arrInit(arena, &codeInfo->resGroups[resGroupIdx].resTypes[resTypeIdx], 20);
                }
            }
            arrInit(arena, &codeInfo->replaceBlocks, 20);

            Str8 cname = {};
            Str8 cresGroup0 = {};
            Str8 cresGroup1 = {};
            Str8 cresGroup2 = {};
            Str8 cresGroup3 = {};
            Str8 cdynResGroup0 = {};
            Str8 cdynResGroup1 = {};

            // run ini parser till we run into code
            for (running = scf_next(&scf); running && scf.valueType != scf_type_category; running = scf_next(&scf)) {
                if (str_isEqual(scf.key, str8("name"))) {
                    cname = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("resGroup0"))) {
                    cresGroup0 = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("resGroup1"))) {
                    cresGroup1 = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("resGroup2"))) {
                    cresGroup2 = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("resGroup3"))) {
                    cresGroup3 = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("dynResGroup0"))) {
                    cdynResGroup0 = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("dynResGroup1"))) {
                    cdynResGroup1 = scf.valueStr;
                }
            }
            codeInfo->name = cname;
            codeInfo->resGroupNames[0] = cresGroup0;
            codeInfo->resGroupNames[1] = cresGroup1;
            codeInfo->resGroupNames[2] = cresGroup2;
            codeInfo->resGroupNames[3] = cresGroup3;
            codeInfo->resGroupNames[4] = cdynResGroup0;
            codeInfo->resGroupNames[5] = cdynResGroup1;

            // Parse shader code metadata
            log_trace(arena, s8("Parse code..."));
            Str8 code = str_subStr(scf.str, scf.needle, scf.str.size);
            codeInfo->code = code;
            tn_Tokenizer tokenizer = tn_createTokenize(code, scf.fileName);
            
            u32 depth = 0;
            bx firstLineSymbol = true;
            for (bx running = tn_parsing(&tokenizer); running;) {
                tn_Token token = tn_getToken(&tokenizer);

                if (depth == 0 && token.type == tn_tokenType_openBracket) {
                    tn_Tokenizer tkn = tokenizer;
                    if (!tn_parsing(&tkn)) break;
                    tn_Token nextToken = tn_getToken(&tkn);
                    if (nextToken.type != tn_tokenType_openBracket) {
                        // continue regular scf parsing...
                        scf.needle =  ((u64) token.text.content) - ((u64) scf.str.content);
                        codeInfo->code.size = ((u64) token.text.content) - ((u64) codeInfo->code.content);
                        break; // return
                    }
                    // found a metadata bracket [[
                    // parse content till we get ]]
                    if (!tn_parsing(&tkn)) break;
                    nextToken = tn_getToken(&tkn);
                    // parse metadata rx::
                    if (nextToken.type != tn_tokenType_identifier || !str_isEqual(nextToken.text, str8("rx"))) {
                        break; // return
                    }
                    // parse ::
                    if (!tn_parsing(&tkn)) break;
                    nextToken = tn_getToken(&tkn);
                    if (nextToken.type != tn_tokenType_colon) {
                        break;
                    }

                    if (!tn_parsing(&tkn)) break;
                    nextToken = tn_getToken(&tkn);
                    if (nextToken.type != tn_tokenType_colon) {
                        break;
                    }
                    // looking for texture, sampler, constant
                    if (!tn_parsing(&tkn)) break;
                    tn_Token resTypeToken = tn_getToken(&tkn);
                    if (resTypeToken.type != tn_tokenType_identifier) {
                        break;
                    }

                    resourceType resType = resourceType_invalid;

                    resGroup group = resGroup0;
                    // ResGroup0-3, DynResGroup0-1
                    if (str_isEqual(resTypeToken.text, str8("resGroup0"))) {
                        group = resGroup0;
                    } else if (str_isEqual(resTypeToken.text, str8("resGroup1"))) {
                        group = resGroup1;
                    } else if (str_isEqual(resTypeToken.text, str8("resGroup2"))) {
                        group = resGroup2;
                    } else if (str_isEqual(resTypeToken.text, str8("resGroup3"))) {
                        group = resGroup3;
                    } else if (str_isEqual(resTypeToken.text, str8("dynResGroup0"))) {
                        group = dynGroup0;
                    } else if (str_isEqual(resTypeToken.text, str8("dynResGroup1"))) {
                        group = dynGroup1;
                    } else {
                        break; // error unknown resource type name
                    }

                    if (!tn_parsing(&tkn)) break;
                    nextToken = tn_getToken(&tkn);
                    if (nextToken.type != tn_tokenType_openParen) {
                        break;
                    }
                    #if 0
                    if (!tn_parsing(&tkn)) break;
                    tn_Token resGroupToken = tn_getToken(&tkn);
                    if (resGroupToken.type != tn_tokenType_identifier) {
                        break;
                    }
                    #endif

                    #if 0
                    if (!tn_parsing(&tkn)) break;
                    nextToken = tn_getToken(&tkn);
                    if (nextToken.type != tn_tokenType_comma) {
                        break;
                    }

                    if (!tn_parsing(&tkn)) break;
                    tn_Token indexToken = tn_getToken(&tkn);
                    if (indexToken.type != tn_tokenType_number || indexToken.s32 >= 20 || indexToken.s32 < 0) {
                        break;
                    }
                    #endif
                    // parse )]]

                    if (!tn_parsing(&tkn)) break;
                    nextToken = tn_getToken(&tkn);
                    if (nextToken.type != tn_tokenType_closeParen) {
                        break;
                    }

                    if (!tn_parsing(&tkn)) break;
                    nextToken = tn_getToken(&tkn);
                    if (nextToken.type != tn_tokenType_closeBracket) {
                        break; // return
                    }

                    if (!tn_parsing(&tkn)) break;
                    nextToken = tn_getToken(&tkn);
                    if (nextToken.type != tn_tokenType_closeBracket) {
                        break; // return
                    }

                    tn_Token endMetaBlockToken = nextToken;

                    // parse TYPE funcName();

                    if (!tn_parsing(&tkn)) break;
                    tn_Token fnTypeToken = tn_getToken(&tkn);
                    if (fnTypeToken.type != tn_tokenType_identifier) {
                        break; // expected TYPE
                    }

                    // todo properly parse all supported types
                    if (str_isEqual(fnTypeToken.text, str8("Texture2D"))) {
                        resType = resourceType_texture;
                    } else if (str_isEqual(fnTypeToken.text, str8("SamplerState"))) {
                        resType = resourceType_sampler;
                    } else {
                        resType = resourceType_constant;
                    }

                    if (!tn_parsing(&tkn)) break;
                    tn_Token fnNameToken = tn_getToken(&tkn);
                    if (fnNameToken.type != tn_tokenType_identifier) {
                        break; // expected funcName
                    }

                    if (!tn_parsing(&tkn)) break;
                    nextToken = tn_getToken(&tkn);
                    if (nextToken.type != tn_tokenType_openParen) {
                        break; // expected empty parameter body ()
                    }

                    if (!tn_parsing(&tkn)) break;
                    nextToken = tn_getToken(&tkn);
                    if (nextToken.type != tn_tokenType_closeParen) {
                        break; // expected empty parameter body ()
                    }

                    if (!tn_parsing(&tkn)) break;
                    nextToken = tn_getToken(&tkn);
                    if (nextToken.type != tn_tokenType_semicolon) {
                        break; // expected no function body (will be generated)
                    }

                    // res metadata and function prototyp is valid
                    // record it down
                    codeInfo->resGroups[group].count++;
                    
                    ResInfo* resInfo = &codeInfo->resGroups[group].resTypes[resType].elements[codeInfo->resGroups[group].resTypes[resType].count++];
                    if (resInfo->name.size > 0) {
                        break; // res info already exist
                    }
                    //codeInfo->resGroups[group].resTypes[resType].count = maxVal(codeInfo->resGroups[group].resTypes[resType].count, indexToken.s32 + 1);
                    resInfo->name     = fnNameToken.text;
                    resInfo->typeName = fnTypeToken.text;

                    switch (resType) {
                        case resourceType_texture: {
                            resInfo->byteSize = 4;
                        } break;
                        case resourceType_sampler: {
                            resInfo->byteSize = 4;
                        } break;
                        case resourceType_constant: {
                            constType type = getConstType(fnTypeToken.text);
                            if (type != constType_invalid) {
                                resInfo->byteSize += constTypeByteSize[type];
                            } else {
                                // error unknown const type
                            }
                        } break;
                        default: break;// error
                    }

                    //MetaDataReplaceBlock* metaBlock = (((&codeInfo->replaceBlocks)->count + 1) < (&codeInfo->replaceBlocks)->capacity ? ((&codeInfo->replaceBlocks)->elements + ((&codeInfo->replaceBlocks)->count++)) : &( (typeOf((&codeInfo->replaceBlocks)->elements[0]))* ((&codeInfo->replaceBlocks)->elements = mem_copy(mem_arenaPush((arena), sizeof((&codeInfo->replaceBlocks)->elements[0]) * (&codeInfo->replaceBlocks)->capacity * 2), (&codeInfo->replaceBlocks)->elements, ((&codeInfo->replaceBlocks)->capacity * 2) * sizeof((&codeInfo->replaceBlocks)->elements[0]))))[(&codeInfo->replaceBlocks)->count++]);
                    MetaDataReplaceBlock* metaBlock = arrPushGet(arena, &codeInfo->replaceBlocks);
                    metaBlock->start = token;
                    metaBlock->end = endMetaBlockToken;

                    //log_traceFmt(str8("ResInputDef: {0} {1} {2}"), fnNameToken.text, fnTypeToken.text, resTypeToken.text);

                    //log_trace("parse metadata [[rx::");
                    tokenizer = tkn;
                    running = tn_parsing(&tokenizer);
                    continue;
                }
                
                switch (token.type) {
                    case tn_tokenType_openParen:
                    case tn_tokenType_openBrace:
                    case tn_tokenType_openBracket: {
                        depth++;
                    } break;
                    case tn_tokenType_closeParen:
                    case tn_tokenType_closeBrace:
                    case tn_tokenType_closeBracket: {
                        depth--;
                    } break;
                    default: break;
                }
                firstLineSymbol = token.type == tn_tokenType_endOfLine ? true : false;
                running = tn_parsing(&tokenizer);
            }
        } else if (str_isEqual(scf.category, str8("DepthStencil"))) {
            shd_DepthStencil* depthStencil = arrPushGet(arena, &outFileInfo->depthStencils);
            mem_setZero(depthStencil, sizeof(shd_DepthStencil));
            for (running = scf_next(&scf); running && scf.valueType != scf_type_category; running = scf_next(&scf)) {
                if (str_isEqual(scf.key, str8("name"))) {
                    depthStencil->name = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("disabled"))) {
                    depthStencil->disabled = str_isEqual(scf.valueStr, str8("true")) ? true : false;
                } else if (str_isEqual(scf.key, str8("format"))) {
                    depthStencil->format = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("depthWriteEnabled"))) {
                    depthStencil->depthWriteEnabled = str_isEqual(scf.valueStr, str8("true")) ? true : false;
                } else if (str_isEqual(scf.key, str8("stencilEnabled"))) {
                    depthStencil->stencilEnabled = str_isEqual(scf.valueStr, str8("true")) ? true : false;
                } else if (str_isEqual(scf.key, str8("stencilReadMask"))) {
                    depthStencil->stencilReadMask = str_parseU32(scf.valueStr);
                } else if (str_isEqual(scf.key, str8("stencilWriteMask"))) {
                    depthStencil->stencilWriteMask = str_parseU32(scf.valueStr);
                } else if (str_isEqual(scf.key, str8("stencilRef"))) {
                    depthStencil->stencilRef = str_parseU32(scf.valueStr);
                } else if (str_isEqual(scf.key, str8("stencilFrontFailOp"))) {
                    depthStencil->stencilFrontFailOp = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("stencilFrontDepthFailOp"))) {
                    depthStencil->stencilFrontDepthFailOp = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("stencilFrontPassOp"))) {
                    depthStencil->stencilFrontPassOp = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("stencilFrontCompareFunc"))) {
                    depthStencil->stencilFrontCompareFunc = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("stencilBackFailOp"))) {
                    depthStencil->stencilBackFailOp = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("stencilBackDepthFailOp"))) {
                    depthStencil->stencilBackDepthFailOp = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("stencilBackPassOp"))) {
                    depthStencil->stencilBackPassOp = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("stencilBackCompareFunc"))) {
                    depthStencil->stencilBackCompareFunc = scf.valueStr;
                } else {
                    // unknown value
                }
            }
            //log_traceFmt("DepthStencil: {0}", depthStencil->name);
            continue;
        } else if (str_isEqual(scf.category, str8("RasterState"))) {
            shd_RasterState* rasterState = arrPushGet(arena, &outFileInfo->rasterStates);
            mem_setZero(rasterState, sizeof(shd_RasterState));
            for (running = scf_next(&scf); running && scf.valueType != scf_type_category; running = scf_next(&scf)) {
                if (str_isEqual(scf.key, str8("name"))) {
                    rasterState->name = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("cullMode"))) {
                    rasterState->cullMode = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("faceWinding"))) {
                    rasterState->faceWinding = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("depthBias"))) {
                    rasterState->depthBias = str_parseF32(scf.valueStr);
                } else if (str_isEqual(scf.key, str8("depthBiasSlopeScale"))) {
                    rasterState->depthBiasSlopeScale = str_parseF32(scf.valueStr);
                } else if (str_isEqual(scf.key, str8("depthBiasClamp"))) {
                    rasterState->depthBiasClamp = str_parseF32(scf.valueStr);
                }
            }
            //log_traceFmt("RasterState: {0}", rasterState->name);
            continue;
        } else if (str_isEqual(scf.category, str8("ResGroup"))) {
            shd_ResGroup* resGroup = arrPushGet(arena, &outFileInfo->resGroups);
            mem_setZero(resGroup, sizeof(shd_ResGroup));
            arrInit(arena, &resGroup->info.resTypes[resourceType_constant], 8);
            arrInit(arena, &resGroup->info.resTypes[resourceType_texture], 8);
            arrInit(arena, &resGroup->info.resTypes[resourceType_sampler], 8);

            for (running = scf_next(&scf); running && scf.valueType != scf_type_category; running = scf_next(&scf)) {
                if (str_isEqual(scf.key, str8("name"))) {
                    resGroup->name = scf.valueStr;
                } else {
                    resourceType resType;
                    if (str_isEqual(scf.valueStr, str8("Texture2D"))) {
                        resType = resourceType_texture;
                    } else if (str_isEqual(scf.valueStr, str8("SamplerState"))) {
                        resType = resourceType_sampler;
                    } else {
                        resType = resourceType_constant;
                    }
                    ResArr* resArr = &resGroup->info.resTypes[resType];
                    ResInfo* resInfo = arrPushGet(arena, resArr);
                    resInfo->name = scf.key;
                    resInfo->typeName = scf.valueStr;
                    switch (resType) {
                        case resourceType_constant: {
                            constType type = getConstType(scf.valueStr);
                            ASSERT(type != constType_invalid && "Unknown const type");
                            resInfo->type = type;
                            resInfo->byteSize = constTypeByteSize[type];
                        } break;
                        default: {
                            resInfo->byteSize = 4;
                            
                        } break;
                    }

#if 0
                    shd_ResDef* resDef = NULL;
                    if (str_isEqual(scf.valueStr, str8("Texture2D"))) {
                        resDef = arrPushGet(arena, &resGroup->resources);
                        resDef->type = shd_resType_texture2d;
                    } else if (str_isEqual(scf.valueStr, str8("SamplerState"))) {
                        resDef = arrPushGet(arena, &resGroup->resources);
                        resDef->type = shd_resType_sampler;
                    } else {
                        
                    }
#endif

                }
            }
            //log_traceFmt("ResGroup: {0}", resGroup->name);
            continue;
        } else if (str_isEqual(scf.category, str8("Material"))) {
            shd_Material* material = arrPushGet(arena, &outFileInfo->materials);
            arrInit(arena, &material->properties, 8);

            mem_setZero(material, sizeof(shd_Material));
            for (running = scf_next(&scf); running && scf.valueType != scf_type_category; running = scf_next(&scf)) {
                if (str_isEqual(scf.key, str8("name"))) {
                    material->name = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("program"))) {
                    material->program = scf.valueStr;
                } else {
                    shd_MatProperty* resDef = arrPushGet(arena, &material->properties);
                    resDef->name = scf.key;
                    resDef->value = scf.valueStr;
                }
            }
            //log_traceFmt("Material: {0}", material->name);
            continue;
        }  else if (str_isEqual(scf.category, str8("SamplerState"))) {
            shd_SamplerState* samplerState = arrPushGet(arena, &outFileInfo->samplerStates);

            mem_setZero(samplerState, sizeof(shd_SamplerState));
            for (running = scf_next(&scf); running && scf.valueType != scf_type_category; running = scf_next(&scf)) {
                if (str_isEqual(scf.key, str8("name"))) {
                    samplerState->name = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("addressModeU"))) {
                    samplerState->addressModeU = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("addressModeV"))) {
                    samplerState->addressModeV = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("addressModeW"))) {
                    samplerState->addressModeW = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("magFilter"))) {
                    samplerState->magFilter = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("minFilter"))) {
                    samplerState->minFilter = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("mipmapMode"))) {
                    samplerState->mipmapMode = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("lodMinClamp"))) {
                    samplerState->lodMinClamp = str_parseF32(scf.valueStr);
                } else if (str_isEqual(scf.key, str8("lodMaxClamp"))) {
                    samplerState->lodMaxClamp = str_parseF32(scf.valueStr);
                } else if (str_isEqual(scf.key, str8("compare"))) {
                    samplerState->compare = scf.valueStr;
                } else if (str_isEqual(scf.key, str8("maxAnisotropy"))) {
                    samplerState->maxAnisotropy = str_parseF32(scf.valueStr);
                }
            }
            //log_traceFmt("SamplerState: {0}", samplerState->name);
            continue;
        } else {
            log_warn(arena, s8("Unknown category: \""), scf.category, s8("\""));
        }
        running = scf_next(&scf);
        //log_debug("category:", scf.category, " key: ", scf.key, " val: ", scf.valueStr);
    }

    return true;
}

bx shd_generateShaders(Arena* arena, dxc_Instance* dxcInstance, ShaderFileInfo* shaderFileInfo, CodeInfo* codeInfo) {
    Str8 generatedCode = {0};
    str_record(generatedCode, arena) {
        // Generate code for
        u64 lastOffset = ((u64)codeInfo->code.content);
        u8* lastPointer = codeInfo->code.content;
        for (u32 idx = 0; codeInfo->replaceBlocks.count > idx; idx++) {
            MetaDataReplaceBlock* block = codeInfo->replaceBlocks.elements + idx;
            u64 currOffset = ((u64)block->start.text.content);
            u64 length = currOffset - lastOffset;
            mem_copy(mem_arenaPush(arena, length), lastPointer, length);
            str_join(arena, str8("/*"));
            lastOffset = ((u64) block->end.text.content) + block->end.text.size;
            lastPointer = block->end.text.content + block->end.text.size;
            length =  lastOffset - currOffset;
            mem_copy(mem_arenaPush(arena, length), block->start.text.content, length);
            str_join(arena, str8("*/"));
        }
        u64 lastLength = (((u64)codeInfo->code.content) + codeInfo->code.size) - lastOffset;
        mem_copy(mem_arenaPush(arena, lastLength), lastPointer, lastLength);

        str_join(arena, str8("\n\n"));
        str_join(arena, str8("//******************************************************************//"), str8("\n"));
        str_join(arena, str8("//******************************************************************//"), str8("\n"));
        str_join(arena, str8("//************************ GENERATED CODE **************************//"), str8("\n"));
        str_join(arena, str8("//******************************************************************//"), str8("\n"));
        str_join(arena, str8("//******************************************************************//"), str8("\n"));
        str_join(arena, str8("\n\n"));


        str_join(arena, str8("[[vk::push_constant]]"), str8("\n"));
        str_join(arena, str8("struct rx_pushConstants {"), str8("\n"));
        str_join(arena, str8("	uint instanceRefIndicies[6];"), str8("\n"));
        str_join(arena, str8("} rx_pushConstants;"), str8("\n\n"));


        str_join(arena, str8("[[vk::binding(0, 0)]] ByteAddressBuffer   rx_staticResBlocks;"), str8("\n"));
        str_join(arena, str8("[[vk::binding(1, 0)]] ByteAddressBuffer   rx_dynamicResBlocks;"), str8("\n"));
        str_join(arena, str8("[[vk::binding(2, 0)]] Texture2D			rx_textures[];"), str8("\n"));
        str_join(arena, str8("[[vk::binding(3, 0)]] SamplerState		rx_samplers[];"), str8("\n\n"));
        // this maybe only needed for compute shader probably...
        //str_join(arena, str8("[[vk::binding(3, 0)]] RWByteAddressBuffer		rx_buffer[];"), str8("\n"));

        for (u32 resGroupIdx = 0; resGroupIdx < countOf(codeInfo->resGroups); resGroupIdx++) {
            ResGroupInfo* resGroupInfo = &codeInfo->resGroups[resGroupIdx];
            Str8 resGroupName = codeInfo->resGroupNames[resGroupIdx];
            Str8 resBlockName = resGroupIdx >= dynGroup0 ? str8("rx_dynamicResBlocks") : str8("rx_staticResBlocks");
            
            // referenced group
            ResGroupInfo* namedResGroup = resGroupInfo;
            for (u32 idx = 0; idx < shaderFileInfo->resGroups.count; idx++) {
                if (str_isEqual(resGroupName, shaderFileInfo->resGroups.elements[idx].name)) {
                    namedResGroup = &shaderFileInfo->resGroups.elements[idx].info;
                }
            }

            u32 byteOffset = 0;
            for (u32 resTypeIdx = resourceType_constant; resTypeIdx < resourceType__count; resTypeIdx++) {
                ResArr* resArr = &resGroupInfo->resTypes[resTypeIdx];
                ResArr* namedResArr = &namedResGroup->resTypes[resTypeIdx];
                for (u32 idx = 0; idx < namedResArr->count; idx++) {
                    ResInfo* namedResInfo = namedResArr->elements + idx;

                    // check which properties from the block were used and generate a getter for it
                    for (u32 i = 0; i < resArr->count; i++) {
                        ResInfo* resInfo = resArr->elements + i;
                        if (str_isEqual(namedResInfo->name, resInfo->name)) {
                            if (!str_isEqual(namedResInfo->typeName, resInfo->typeName)) {
                                ASSERT(!"Error type of named res group must be the same");
                            }
                            //u32 size = constTypeByteSize[namedResInfo->type];;
                            switch ((resourceType)resTypeIdx) {
                                case resourceType_constant: {
                                    str_join(arena, namedResInfo->typeName, str8(" "), namedResInfo->name, str8("() {\n"));
                                    str_fmt(arena, str8("	return {0}.Load<{1}>(rx_pushConstants.instanceRefIndicies[{2}] * sizeof(uint) + sizeof(uint) * {3});\n"), resBlockName, namedResInfo->typeName, resGroupIdx, byteOffset);   
                                    str_join(arena, str8("}\n"));
                                    byteOffset += namedResInfo->byteSize / 4;
                                } break;
                                case resourceType_texture: {
                                    str_join(arena, namedResInfo->typeName, str8(" "), namedResInfo->name, str8("() {\n"));
                                    str_fmt(arena,  str8("    uint index = {0}.Load(rx_pushConstants.instanceRefIndicies[{2}] * sizeof(uint) + sizeof(uint) * {3});\n"), resBlockName, namedResInfo->typeName, resGroupIdx, byteOffset);   
                                    str_join(arena, str8("    return rx_textures[index];\n")); 
                                    str_join(arena, str8("}\n"));  
                                    byteOffset += namedResInfo->byteSize / 4;
                                }; break;
                                case resourceType_sampler: {
                                    str_join(arena, namedResInfo->typeName, str8(" "), namedResInfo->name, str8("() {\n"));
                                    str_fmt(arena,  str8("    uint index = {0}.Load(rx_pushConstants.instanceRefIndicies[{2}] * sizeof(uint) + sizeof(uint) * {3});\n"), resBlockName, namedResInfo->typeName, resGroupIdx, byteOffset);     
                                    str_join(arena, str8("    return rx_samplers[index];\n")); 
                                    str_join(arena, str8("}\n"));  
                                    byteOffset += namedResInfo->byteSize / 4;
                                }; break;
                                default: break;
                            }
                        }
                    }
                }
            }
        }
        
        #if 0
        for (u32 resGroupIdx = 0; resGroupIdx < countOf(codeInfo->resGroups); resGroupIdx++) {
            if (codeInfo->resGroups[resGroupIdx].count == 0) continue;
            str_join(arena, str8("\n"));
            str_join(arena, resGroupIdx >= dynGroup0 ? str8("// DynResGroup") : str8("// ResGroup"), resGroupIdx >= dynGroup0 ? (resGroupIdx - dynGroup0) : resGroupIdx,str8("\n"));
            Str8 resBlockName = resGroupIdx >= dynGroup0 ? str8("rx_dynamicResBlocks") : str8("rx_staticResBlocks");
            uint32_t byteOffset = 0;
            // constants
            {
                ResArr* constantsArr = &codeInfo->resGroups[resGroupIdx].resTypes[resourceType_constant];

                for (u32 idx = 0; idx < constantsArr->count; idx++) {
                    ResInfo* info = constantsArr->elements + idx;
                    
                    str_join(arena, info->typeName, str8(" "), info->name, str8("() {\n"));
                    str_fmt(arena, str8("	return {0}.Load<{1}>(rx_pushConstants.instanceRefIndicies[{2}] + {3});\n"), resBlockName, info->typeName, resGroupIdx, byteOffset);   
                    str_join(arena, str8("}\n"));
                    byteOffset += info->byteSize;
                }

            }
            // textures
            {
                ResArr* texturesArr = &codeInfo->resGroups[resGroupIdx].resTypes[resourceType_texture];

                for (u32 idx = 0; idx < texturesArr->count; idx++) {
                    ResInfo* info = texturesArr->elements + idx;
                    str_join(arena, info->typeName, str8(" "), info->name, str8("() {\n"));
                    str_fmt(arena,  str8("    uint index = {0}.Load(rx_pushConstants.instanceRefIndicies[{2}] + {3});\n"), resBlockName, info->typeName, resGroupIdx, byteOffset);   
                    str_join(arena, str8("    return rx_textures[index];\n")); 
                    str_join(arena, str8("}\n"));  
                    byteOffset += info->byteSize;
                }

            }
            // samplers
            {
                ResArr* samplersArr = &codeInfo->resGroups[resGroupIdx].resTypes[resourceType_sampler];

                for (u32 idx = 0; idx < samplersArr->count; idx++) {
                    ResInfo* info = samplersArr->elements + idx;
                    str_join(arena, info->typeName, str8(" "), info->name, str8("() {\n"));
                    str_fmt(arena,  str8("    uint index = {0}.Load(rx_pushConstants.instanceRefIndicies[{2}] + {3});\n"), resBlockName, info->typeName, resGroupIdx, byteOffset);     
                    str_join(arena, str8("    return rx_samplers[index];\n")); 
                    str_join(arena, str8("}\n"));  
                    byteOffset += info->byteSize;
                }

            }
            /*
                resourceType_constant,
                resourceType_texture,
                resourceType_sampler,
            */
        }
        #endif
        str_join(arena, str8("\0"));
    }
    //codeInfo->programs
    //log_trace(str8("\ngenerated code:\n"), generatedCode, str8("\n gen done"));
    //log_trace("call dxc...\n");
    StrPair defs = {};
    Str8 params = str_lit("-spirv");
    // compile code with dxc

    // https://github.com/floooh/sokol-tools/blob/master/src/shdc/spirvcross.cc


    // gather shader informations about locations & sizes in input/output
    arrFor(&shaderFileInfo->renderPrograms, idx) {
        RenderProgram* program = shaderFileInfo->renderPrograms.elements + idx;
        dxc_CompileResult vsResult = dxc_compileHlslToSpv(arena, dxcInstance, dxc_shaderType_vertex, generatedCode, codeInfo->name, program->vs.entry, &defs, 0, &params, 1);
        if (vsResult.diagMessage.size > 0) log_error(arena, vsResult.diagMessage);
        program->vs.source = vsResult.spvCode;
        dxc_CompileResult psResult = dxc_compileHlslToSpv(arena, dxcInstance, dxc_shaderType_pixel, generatedCode, codeInfo->name, program->ps.entry, &defs, 0, &params, 1);
        if (psResult.diagMessage.size > 0) log_error(arena, psResult.diagMessage);
        program->ps.source = psResult.spvCode;

        // parse vertex shader
        Parser vsSpirvParser((u32*) program->vs.source.content, program->vs.source.size / 4);
        vsSpirvParser.parse();
        CompilerReflection vsCompiler(std::move(vsSpirvParser.get_parsed_ir()));
        ASSERT(program->vs.type == shd_entryPointType_vertex && "Expected vertex shader");
        shd_parseSpirvShaderDetails(arena, &program->vs, vsCompiler);

        // parse fragment shader
        Parser psSpirvParser((u32*) program->ps.source.content, program->ps.source.size / 4);
        psSpirvParser.parse();
        CompilerReflection psCompiler(std::move(psSpirvParser.get_parsed_ir()));
        shd_parseSpirvShaderDetails(arena, &program->ps, psCompiler);
        ASSERT(program->ps.type == shd_entryPointType_pixel && "Expected pixel shader");
    }

    return true;
}


i32 main(i32 argc, char* argv[]) {

// enable for debug
#if 0
    char* debugArgV[] = {
        (char*) "-s",
        (char*) PROJECT_ROOT "/engine_test/ui_shaders.hlsl",
        (char*) "-h",
        (char*) PROJECT_ROOT "/engine_test/ui_shaders.hlsl.h"
    };
    i32 debugArgc = countOf(debugArgV);

    argv = (char**) debugArgV;
    argc = debugArgc;
#endif


    arg_Opt options[] = {
        {str8("shader"), 's', arg_optType_string, arg_flag_required | arg_flag_requiredValue, STR_EMPTY, str8("SHD shader file path."), str8("Absolute path to the shader file.")},
        {str8("header"), 'h', arg_optType_string, arg_flag_none, STR_EMPTY, str8("generated header output file path."), str8("Absolute path where the generated header should be created.")},
        {str8("prefix"), 'p', arg_optType_string, arg_flag_none, str_lit("shd_"), str8("prefix used in generated header"), str8("Add user defined prefix to variables and functions in the generated header.\nThe default shader is \"shd_\"")},
    };

    arg_Ctx argCtx = arg_makeCtx(&options[0], countOf(options), argv, argc);
    
    Str8 headerPrefix = str8("");
    Str8 headerTargetPath = STR_EMPTY;
    Str8 shaderFilePath = STR_EMPTY;
    // Str8 shaderFileContent = STR_EMPTY;
    Str8 shaderFileName = STR_EMPTY;

    {
        // scan arguments
        bx errorFound = false;
        bx showHelp = false;

        i32 opt = 0;
        while ( (opt = arg_nextOpt(&argCtx)) != arg_end) {
            switch (opt) {
                case arg_error_duplicate:
                case arg_error_missingArg:
                case arg_error_missingValue:
                case arg_error_invalidValue: {
                    errorFound = true;
                    showHelp = true;
                } break;
                case 's': {
                    // shader file
                    shaderFilePath = argCtx.foundValue;
                } break;
                case 'h': {
                    // header target path
                    headerTargetPath = argCtx.foundValue;
                } break;
                case 'p': {
                    // header content prefix
                    headerPrefix = argCtx.foundValue;
                } break;
            }
        }

        if (showHelp) {
            // show help
        }

        if (errorFound) {
            return 1;
        }
    }

    if (shaderFilePath.size == 0) {
        return 1;
    }

    // parse shader file(s)...

    BaseMemory baseMem = os_getBaseMemory();
    Arena* arena = mem_makeArena(&baseMem, MEGABYTE(20));

    dxc_Dll dll;

    if (!dxc_loadLib(&dll)) {
        return 1;
    }

    dxc_Instance* dxcInstance = dxc_create(arena, &dll);

    Str8 shaderFileContent = os_fileRead(arena, shaderFilePath);

    if (shaderFileContent.size == 0) {
        return 1;
    }

    ShaderFileInfo fileInfo = {};

    if (!shd_parseFromFile(arena, &fileInfo, shaderFileName, shaderFileContent)) {
        return 1; // error!
    }

    if (!shd_generateShaders(arena, dxcInstance, &fileInfo, &fileInfo.codeInfos.elements[0])) {
        return 1; // error!
    }

    if (!str_isEmpty(headerTargetPath)) {
        
        Str8Pair constTypeList[] = {
            {str8("float"), str8("f32")},
            {str8("float2"), str8("Vec2")},
            {str8("float3"), str8("Vec3")},
            {str8("float4"), str8("Vec4")},
            {str8("uint"), str8("u32")},
            {str8("uint"), str8("u32")},
            {str8("Texture2D"), str8("rx_texture")},
            {str8("SamplerState"), str8("rx_sampler")},
        };
        Str8PairArray typeMap = {};
        typeMap.elements = &constTypeList[0];
        typeMap.count = countOf(constTypeList);


        // generate header containing meta information & shader
        Str8 generatedHeader = shd_generateHeader(arena, &fileInfo, headerPrefix, &fileInfo.codeInfos.elements[0], &typeMap);


        //for (u32 idx = 0; idx < fileInfo.renderPrograms.count; ++idx) {
        //    RenderProgram* program = &fileInfo.renderPrograms.elements[idx];
        //    os_fileWrite(str_join(arena, shaderFilePath, str8("."), program->name, str8(".spirv.vs")), program->vs.source);
        //    os_fileWrite(str_join(arena, shaderFilePath, str8("."), program->name, str8(".spirv.ps")), program->ps.source);
        //}

        if (!os_fileWrite(headerTargetPath, generatedHeader)) {
            return 1; // error
        }
    }

    return 0;
}
