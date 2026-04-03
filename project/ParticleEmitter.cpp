#include "ParticleEmitter.h"

void ParticleEmitter::Initialize(ParticleManager* particleManager, const std::string& particleName) {
	particleManager_ = particleManager;
	particleName_ = particleName;
	frameCounter_ = 0;
}

void ParticleEmitter::Update() {
	// 発生OFFの時は何もしない
	if (!isEmitting_) return;

	// タイマーを進める
	frameCounter_++;

	// 指定フレームに達したらパーティクルを発生させる
	if (frameCounter_ >= emitFrequency_) {
		Emit();
		frameCounter_ = 0; // タイマーをリセット
	}

	for (uint32_t i = 0; i < 10; i++) {
		particle[i].velocity = { 0.0f,0.1f,0.0f };
	}
}

void ParticleEmitter::Emit() {
	// マネージャーが設定されていれば発生させる
	if (particleManager_) {
		particleManager_->Emit(particleName_, position_, emitCount_);
	}
}