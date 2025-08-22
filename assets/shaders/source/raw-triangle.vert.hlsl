cbuffer TransformBuffer : register(b0, space1) {
    float4x4 mvp_matrix;
};

struct VertexInput {
    float4 position : POSITION;
    float4 color : COLOR;
};

struct VertexOutput {
    float4 color : TEXCOORD0;
    float4 position : SV_Position;
};

VertexOutput main(VertexInput input) {
    VertexOutput output;
    output.position = mul(mvp_matrix, input.position);
    output.color = float4(input.color);
    return output;
}
