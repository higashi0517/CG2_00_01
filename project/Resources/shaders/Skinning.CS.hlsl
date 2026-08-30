struct Well
{
    float32_t4x4 skeletonSpaceMatrix;
    float32_t4x4 skeletonSpaceInverseTransposeMatrix;
};

struct Vertex
{
    float32_t4 position;
    float32_t2 texcoord;
    float32_t3 normal;
};

struct VertexInfluence
{
    float32_t4 weight;
    uint32_t4 index;
};

struct SkinningInformation
{
    uint32_t numVertices;
};

// MatrixPalette
StructuredBuffer<Well> gMatrixPalette : register(t0);

// スキニング前の頂点
StructuredBuffer<Vertex> gInputVertices : register(t1);

// 頂点ごとのWeightとJointIndex
StructuredBuffer<VertexInfluence> gInfluences : register(t2);

// スキニング後の頂点
RWStructuredBuffer<Vertex> gOutputVertices : register(u0);

// 頂点数
ConstantBuffer<SkinningInformation> gSkinningInformation : register(b0);

[numthreads(1024, 1, 1)]
void main(
    uint32_t3 DTid : SV_DispatchThreadID)
{
    uint32_t vertexIndex = DTid.x;

    // 頂点数を超えたThreadでは処理しない
    if (vertexIndex >= gSkinningInformation.numVertices)
    {
        return;
    }

    Vertex input = gInputVertices[vertexIndex];

    VertexInfluence influence = gInfluences[vertexIndex];

    Vertex skinned;

    skinned.texcoord = input.texcoord;

    // 頂点位置
    skinned.position =
        mul(
            input.position,
            gMatrixPalette[influence.index.x]
                .skeletonSpaceMatrix
        ) * influence.weight.x;

    skinned.position +=
        mul(
            input.position,
            gMatrixPalette[influence.index.y]
                .skeletonSpaceMatrix
        ) * influence.weight.y;

    skinned.position +=
        mul(
            input.position,
            gMatrixPalette[influence.index.z]
                .skeletonSpaceMatrix
        ) * influence.weight.z;

    skinned.position +=
        mul(
            input.position,
            gMatrixPalette[influence.index.w]
                .skeletonSpaceMatrix
        ) * influence.weight.w;

    skinned.position.w = 1.0f;

    // 法線
    skinned.normal =
        mul(
            input.normal,
            (float32_t3x3)
            gMatrixPalette[influence.index.x]
                .skeletonSpaceInverseTransposeMatrix
        ) * influence.weight.x;

    skinned.normal +=
        mul(
            input.normal,
            (float32_t3x3)
            gMatrixPalette[influence.index.y]
                .skeletonSpaceInverseTransposeMatrix
        ) * influence.weight.y;

    skinned.normal +=
        mul(
            input.normal,
            (float32_t3x3)
            gMatrixPalette[influence.index.z]
                .skeletonSpaceInverseTransposeMatrix
        ) * influence.weight.z;

    skinned.normal +=
        mul(
            input.normal,
            (float32_t3x3)
            gMatrixPalette[influence.index.w]
                .skeletonSpaceInverseTransposeMatrix
        ) * influence.weight.w;

    skinned.normal = normalize(skinned.normal);

    // UAVへ書き込む
    gOutputVertices[vertexIndex] = skinned;
}