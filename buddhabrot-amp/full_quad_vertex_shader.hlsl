// Generate a triangle that covers full display area. Expects 3 vertices.
// 0 -> tex(0, 0), position(-1, 1, 0, 1)
// 1 -> tex(2, 0), position(3, 1, 0, 1)
// 2 -> tex(0, 2), position(-1, -3, 0, 1)
void main(in uint vertex_id : SV_VERTEXID, out float4 position : SV_POSITION, out float2 tex : TEXCOORD)
{
    tex = float2(uint2(vertex_id << 1, vertex_id) & 2);
    position = float4(lerp(float2(-1, 1), float2(1, -1), tex), 0, 1);

    //position = float4((float)(vertex_id / 2) * 4.0 - 1.0, (float)(vertex_id % 2) * 4.0 - 1.0, 0.0, 1.0);
    //tex = float2((float)(vertex_id / 2) * 2.0, 1.0 - (float)(vertex_id % 2) * 2.0);
}
