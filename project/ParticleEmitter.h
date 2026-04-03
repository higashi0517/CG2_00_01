#pragma once
#include "ParticleManager.h"
#include "Matrix4x4.h" // Vector3 を使うため
#include <string>

class ParticleEmitter {
public:

    struct Particle {
		Transform transform;
		Vector3 velocity;
    };

    // 初期化
    void Initialize(ParticleManager* particleManager, const std::string& particleName);

    // 毎フレーム呼ぶ更新処理
    void Update();

    // 手動で発生させたい時用
    void Emit();

    // --- セッター（外部から設定を変更する関数） ---
    void SetPosition(const Vector3& pos) { position_ = pos; }
    void SetEmitFrequency(int frame) { emitFrequency_ = frame; } // 何フレームに1回発生させるか
    void SetEmitCount(uint32_t count) { emitCount_ = count; }    // 1回に発生させる数
    void SetIsEmitting(bool active) { isEmitting_ = active; }    // 発生のON/OFF

private:
    ParticleManager* particleManager_ = nullptr;
    std::string particleName_;

    Vector3 position_ = { 0.0f, 0.0f, 0.0f }; // エラーになっていた position_
    Particle particle[10];

    int emitFrequency_ = 10; // デフォルトは10フレームに1回
    int frameCounter_ = 0;   // フレーム計測用のタイマー
    uint32_t emitCount_ = 1; // デフォルトは1度に1個
    bool isEmitting_ = true; // 発生中かどうか
};