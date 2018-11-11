SamplerState basic_sampler : register(s0);
texture2D texture_to_sample : register(t0);

float4 main(in float4 position : SV_POSITION, in float2 tex : TEXCOORD) : SV_TARGET
{
    return texture_to_sample.Sample(basic_sampler, tex);
}