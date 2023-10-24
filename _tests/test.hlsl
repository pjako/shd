[RenderProgram]
name = MyProgram
code = MyShaderCode
vsEntry = vsMain
psEntry = psMain
[Code]
name = MyShaderCode
resGroup1 = MyProgramResGroup1
dynResGroup0 = DynResGroup0
struct VSInput {
    [[vk::location(0)]] float3 pos  : position;
    [[vk::location(1)]] float3 col  : color;
    [[vk::location(2)]] float2 uv   : textcoords;
};

struct PSInput {
	float4 pos      : SV_POSITION;
    float4 color    : COLOR0;
    float2 uv       : TEXCOORD0;
};

[[rx::resGroup1()]] Texture2D fragTexture();
[[rx::resGroup1()]] SamplerState fragSampler();
[[rx::dynResGroup0()]] float2 transformOffset();

PSInput vsMain(VSInput input, uint VertexIndex : SV_VertexID) {
	PSInput output;
	output.color = float4(input.col, 1);
	output.pos = float4(input.pos.xyz + float3(transformOffset(), 0), 1.0);
	output.uv = input.uv;
	return output;
}

float4 psMain(PSInput input) : SV_TARGET {
    return input.color * fragTexture().Sample(fragSampler(), input.uv);
}


