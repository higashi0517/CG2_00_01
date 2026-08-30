struct Particle
{
    float32_t3 translate;
    float32_t3 scale;
    float32_t lifeTime;
    float32_t3 velocity;
    float32_t currentTime;
    float32_t4 color;
};

static const uint32_t kMaxParticles = 1024;

RWStructuredBuffer<Particle>
    gParticles : register(u0);

[numthreads(1024, 1, 1)]
void main(
    uint32_t3 dispatchThreadId
        : SV_DispatchThreadID)
{
    uint32_t particleIndex =
        dispatchThreadId.x;

    if (particleIndex >= kMaxParticles)
    {
        return;
    }

    // いったん全Particleを0にする
    gParticles[particleIndex] =
        (Particle)0;

    // 表示確認用に先頭の1個だけ設定
    if (particleIndex == 0)
    {
        Particle particle =
            (Particle)0;

        particle.translate =
            float32_t3(0.0f, 0.0f, 0.0f);

        particle.scale =
            float32_t3(0.3f, 0.3f, 0.3f);

        particle.lifeTime = 1000.0f;

        particle.velocity =
            float32_t3(0.0f, 0.0f, 0.0f);

        particle.currentTime = 0.0f;

        particle.color =
            float32_t4(
                1.0f,
                1.0f,
                1.0f,
                1.0f
            );

        gParticles[particleIndex] =
            particle;
    }
}