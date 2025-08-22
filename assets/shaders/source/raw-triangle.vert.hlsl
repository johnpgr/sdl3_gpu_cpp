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
    output.position = input.position;
    output.color = float4(input.color);
    return output;
}
