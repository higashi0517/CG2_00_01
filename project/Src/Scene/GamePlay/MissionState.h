#pragma once

// 描画や入力から独立した、短い1ステージの進行ルール。
class MissionState {
public:
	enum class Phase { Title, Playing, Clear, GameOver };
	static constexpr int kTargetCount = 5;
	static constexpr int kMaxHealth = 3;
	static constexpr int kTimeLimitFrames = 90 * 60;

	void Start() { phase_ = Phase::Playing; health_ = kMaxHealth; defeated_ = 0; frames_ = 0; invincibleFrames_ = 0; }
	void ReturnToTitle() { phase_ = Phase::Title; }
	void Tick() {
		if (phase_ != Phase::Playing) { return; }
		++frames_;
		if (invincibleFrames_ > 0) { --invincibleFrames_; }
	}
	void Damage() {
		if (phase_ == Phase::Playing && invincibleFrames_ == 0 && health_ > 0) {
			--health_;
			invincibleFrames_ = 60;
		}
	}
	void DefeatEnemy() { if (phase_ == Phase::Playing) { ++defeated_; } }
	void Resolve() {
		if (phase_ != Phase::Playing) { return; }
		if (health_ <= 0) { phase_ = Phase::GameOver; }
		else if (defeated_ >= kTargetCount) { phase_ = Phase::Clear; }
		else if (frames_ >= kTimeLimitFrames) { phase_ = Phase::GameOver; }
	}
	Phase GetPhase() const { return phase_; }
	int GetHealth() const { return health_; }
	int GetDefeated() const { return defeated_; }
	int GetRemainingSeconds() const {
		const int remaining = kTimeLimitFrames - frames_;
		return remaining > 0 ? (remaining + 59) / 60 : 0;
	}
	bool IsInvincible() const { return invincibleFrames_ > 0; }
private:
	Phase phase_ = Phase::Title;
	int health_ = kMaxHealth;
	int defeated_ = 0;
	int frames_ = 0;
	int invincibleFrames_ = 0;
};
