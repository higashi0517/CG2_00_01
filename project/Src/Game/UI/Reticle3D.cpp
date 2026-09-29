#include "Reticle3D.h"

#include "SpriteManager.h"
#include "WinApp.h"
#include "TextureManager.h"
#include <algorithm>
#include <cassert>
#include <cmath>

#ifdef _DEBUG
#include "imgui.h"
#endif

namespace
{
	constexpr float kScreenWidth = 1280.0f;
	constexpr float kScreenHeight = 720.0f;
}

void Reticle3D::Initialize(
	SpriteManager* spriteManager,
	WinApp* winApp,
	const std::string& textureFilePath)
{
	assert(spriteManager != nullptr);
	assert(winApp != nullptr);
	winApp_ = winApp;

	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize(spriteManager, textureFilePath);
	sprite_->SetAnchorPoint({ 0.5f, 0.5f });
	sprite_->SetSize({ 32.0f, 32.0f });
	sprite_->SetColor({ 0.2f, 1.0f, 0.3f, 0.9f });
	sprite_->Update();

	TextureManager::GetInstance()->LoadTexture("Resources/debugfont.png");
	auto makeSolid = [&]() {
		auto part = std::make_unique<Sprite>();
		part->Initialize(spriteManager, "Resources/white1x1.png");
		part->SetTextureSize({ 1.0f, 1.0f });
		part->SetAnchorPoint({ 0.5f, 0.5f });
		return part;
	};
	for (size_t i = 0; i < lockCorners_.size(); ++i) {
		lockCorners_[i] = makeSolid();
		lockCornerOutlines_[i] = makeSolid();
	}
	lockLabelBackground_ = makeSolid();
	lockLabelBackground_->SetColor({ 0.02f, 0.02f, 0.03f, 0.9f });
	lockLabelBackground_->SetSize({ 146.0f, 38.0f });
	constexpr char label[] = "LOCK ON";
	for (size_t i = 0; i < lockLabel_.size(); ++i) {
		auto& letter = lockLabel_[i];
		letter = std::make_unique<Sprite>();
		letter->Initialize(spriteManager, "Resources/debugfont.png");
		// 既存フォントはASCII 32から、9x18ピクセル・横14文字で並ぶ。
		const int glyph = label[i] - ' ';
		letter->SetTextureLeftTop({ float(glyph % 14 * 9), float(glyph / 14 * 18) });
		letter->SetTextureSize({ 9.0f, 18.0f });
		letter->SetSize({ 18.0f, 32.0f });
		letter->SetColor({ 1.0f, 0.75f, 0.2f, 1.0f });
	}
}

void Reticle3D::Update(const Camera& camera)
{
	POINT mousePosition{};
	if (!GetCursorPos(&mousePosition) ||
		!ScreenToClient(winApp_->GetHwnd(), &mousePosition)) {
		isVisible_ = false;
		return;
	}

	screenPosition_ = {
		static_cast<float>(mousePosition.x),
		static_cast<float>(mousePosition.y)
	};
	isVisible_ =
		screenPosition_.x >= 0.0f && screenPosition_.x <= kScreenWidth &&
		screenPosition_.y >= 0.0f && screenPosition_.y <= kScreenHeight;

	const float ndcX = screenPosition_.x / kScreenWidth * 2.0f - 1.0f;
	const float ndcY = 1.0f - screenPosition_.y / kScreenHeight * 2.0f;
	const Matrix4x4 inverseViewProjection =
		Inverse(camera.GetViewProjectionMatrix());

	auto unproject = [&](float ndcZ) {
		Vector3 result{};
		const float worldX =
			ndcX * inverseViewProjection.m[0][0] +
			ndcY * inverseViewProjection.m[1][0] +
			ndcZ * inverseViewProjection.m[2][0] +
			inverseViewProjection.m[3][0];
		const float worldY =
			ndcX * inverseViewProjection.m[0][1] +
			ndcY * inverseViewProjection.m[1][1] +
			ndcZ * inverseViewProjection.m[2][1] +
			inverseViewProjection.m[3][1];
		const float worldZ =
			ndcX * inverseViewProjection.m[0][2] +
			ndcY * inverseViewProjection.m[1][2] +
			ndcZ * inverseViewProjection.m[2][2] +
			inverseViewProjection.m[3][2];
		const float worldW =
			ndcX * inverseViewProjection.m[0][3] +
			ndcY * inverseViewProjection.m[1][3] +
			ndcZ * inverseViewProjection.m[2][3] +
			inverseViewProjection.m[3][3];

		if (std::abs(worldW) > 0.0001f) {
			result = {
				worldX / worldW,
				worldY / worldW,
				worldZ / worldW
			};
		}
		return result;
		};

	const Vector3 nearPosition = unproject(0.0f);
	const Vector3 farPosition = unproject(1.0f);
	const Vector3 rayVector = {
		farPosition.x - nearPosition.x,
		farPosition.y - nearPosition.y,
		farPosition.z - nearPosition.z
	};
	const Vector3 rayDirection = Normalize(rayVector);
	const Vector3 cameraPosition = camera.GetTranslate();
	worldPosition_ = {
		cameraPosition.x + rayDirection.x * distance_,
		cameraPosition.y + rayDirection.y * distance_,
		cameraPosition.z + rayDirection.z * distance_
	};

	if (isVisible_) {
		sprite_->SetPosition(screenPosition_);
		sprite_->Update();
	}
}

void Reticle3D::Draw()
{
	if (isVisible_ && sprite_) {
		sprite_->Draw();
	}
	if (isLockMarkerVisible_) {
		for (auto& outline : lockCornerOutlines_) { outline->Draw(); }
		for (auto& corner : lockCorners_) { corner->Draw(); }
		lockLabelBackground_->Draw();
		for (auto& letter : lockLabel_) { letter->Draw(); }
	}
}

void Reticle3D::UpdateLockMarker(const Camera& camera, const Vector3* targetPosition)
{
	Vector2 center{};
	isLockMarkerVisible_ = isVisible_ && targetPosition &&
		WorldToScreen(camera, *targetPosition, center) &&
		center.x >= 0.0f && center.x <= kScreenWidth &&
		center.y >= 0.0f && center.y <= kScreenHeight;
	SetLocked(isLockMarkerVisible_);
	if (!isLockMarkerVisible_) {
		lockPulse_ = 0.0f;
		return;
	}
	lockPulse_ = std::fmod(lockPulse_ + 0.08f, 6.2831853f);
	const float pulse = (std::sin(lockPulse_) + 1.0f) * 0.5f;
	const float halfWidth = 38.0f + pulse * 4.0f;
	const float halfHeight = 46.0f + pulse * 4.0f;
	for (size_t i = 0; i < 4; ++i) {
		const float sx = (i % 2 == 0) ? -1.0f : 1.0f;
		const float sy = (i < 2) ? -1.0f : 1.0f;
		for (size_t axis = 0; axis < 2; ++axis) {
			const size_t index = i * 2 + axis;
			const Vector2 size = axis == 0 ? Vector2{ 18.0f, 4.0f } : Vector2{ 4.0f, 18.0f };
			const Vector2 position = {
				center.x + sx * (halfWidth - (axis == 0 ? 7.0f : 0.0f)),
				center.y + sy * (halfHeight - (axis == 1 ? 7.0f : 0.0f))
			};
			auto& outline = lockCornerOutlines_[index];
			outline->SetPosition(position);
			outline->SetSize({ size.x + 4.0f, size.y + 4.0f });
			outline->SetColor({ 0.02f, 0.02f, 0.03f, 0.9f });
			outline->Update();
			auto& corner = lockCorners_[index];
			corner->SetPosition(position);
			corner->SetSize(size);
			corner->SetColor({ 1.0f, 0.4f + pulse * 0.25f, 0.08f, 1.0f });
			corner->Update();
		}
	}
	// 画面端でもラベル全体が読める位置に収める。
	const float labelX = std::clamp(center.x, 77.0f, kScreenWidth - 77.0f);
	const float labelY = std::clamp(center.y - halfHeight - 26.0f, 23.0f, kScreenHeight - 23.0f);
	lockLabelBackground_->SetPosition({ labelX, labelY });
	lockLabelBackground_->Update();
	for (size_t i = 0; i < lockLabel_.size(); ++i) {
		lockLabel_[i]->SetPosition({ labelX - 63.0f + float(i) * 18.0f, labelY - 16.0f });
		lockLabel_[i]->Update();
	}
}

void Reticle3D::SetLocked(bool isLocked)
{
	if (isLocked_ == isLocked || !sprite_) {
		return;
	}
	isLocked_ = isLocked;
	if (isLocked_) {
		sprite_->SetColor({ 1.0f, 0.25f, 0.05f, 1.0f });
		sprite_->SetSize({ 42.0f, 42.0f });
	}
	else {
		sprite_->SetColor({ 0.2f, 1.0f, 0.3f, 0.9f });
		sprite_->SetSize({ 32.0f, 32.0f });
	}
	sprite_->Update();
}

void Reticle3D::DrawDebug(
	const Matrix4x4& viewProjectionMatrix,
	const Vector3& playerPosition,
	float screenWidth,
	float screenHeight) const
{
#ifdef _DEBUG
	auto project = [
		&viewProjectionMatrix,
		screenWidth,
		screenHeight
	](const Vector3& position, ImVec2& screenPosition) {
		const float clipX =
			position.x * viewProjectionMatrix.m[0][0] +
			position.y * viewProjectionMatrix.m[1][0] +
			position.z * viewProjectionMatrix.m[2][0] +
			viewProjectionMatrix.m[3][0];
		const float clipY =
			position.x * viewProjectionMatrix.m[0][1] +
			position.y * viewProjectionMatrix.m[1][1] +
			position.z * viewProjectionMatrix.m[2][1] +
			viewProjectionMatrix.m[3][1];
		const float clipW =
			position.x * viewProjectionMatrix.m[0][3] +
			position.y * viewProjectionMatrix.m[1][3] +
			position.z * viewProjectionMatrix.m[2][3] +
			viewProjectionMatrix.m[3][3];
		if (clipW <= 0.0f) {
			return false;
		}
		screenPosition.x =
			(clipX / clipW + 1.0f) * 0.5f * screenWidth;
		screenPosition.y =
			(1.0f - clipY / clipW) * 0.5f * screenHeight;
		return true;
		};

	ImVec2 playerScreen{};
	ImVec2 reticleScreen{};
	if (project(playerPosition, playerScreen) &&
		project(worldPosition_, reticleScreen)) {
		ImDrawList* drawList = ImGui::GetForegroundDrawList();
		drawList->AddLine(
			playerScreen,
			reticleScreen,
			IM_COL32(255, 0, 255, 220),
			2.0f);
		drawList->AddCircle(
			reticleScreen,
			8.0f,
			isLocked_
			? IM_COL32(255, 60, 0, 255)
			: IM_COL32(255, 255, 0, 255),
			0,
			2.0f);
	}
#else
	(void)viewProjectionMatrix;
	(void)playerPosition;
	(void)screenWidth;
	(void)screenHeight;
#endif
}

bool Reticle3D::WorldToScreen(
	const Camera& camera,
	const Vector3& worldPosition,
	Vector2& screenPosition) const
{
	const Matrix4x4& viewProjection = camera.GetViewProjectionMatrix();
	const float clipX =
		worldPosition.x * viewProjection.m[0][0] +
		worldPosition.y * viewProjection.m[1][0] +
		worldPosition.z * viewProjection.m[2][0] +
		viewProjection.m[3][0];
	const float clipY =
		worldPosition.x * viewProjection.m[0][1] +
		worldPosition.y * viewProjection.m[1][1] +
		worldPosition.z * viewProjection.m[2][1] +
		viewProjection.m[3][1];
	const float clipZ =
		worldPosition.x * viewProjection.m[0][2] +
		worldPosition.y * viewProjection.m[1][2] +
		worldPosition.z * viewProjection.m[2][2] +
		viewProjection.m[3][2];
	const float clipW =
		worldPosition.x * viewProjection.m[0][3] +
		worldPosition.y * viewProjection.m[1][3] +
		worldPosition.z * viewProjection.m[2][3] +
		viewProjection.m[3][3];

	if (clipW <= 0.0f) {
		return false;
	}

	const float ndcZ = clipZ / clipW;
	if (ndcZ < 0.0f || ndcZ > 1.0f) {
		return false;
	}

	const float ndcX = clipX / clipW;
	const float ndcY = clipY / clipW;
	screenPosition.x = (ndcX + 1.0f) * 0.5f * kScreenWidth;
	screenPosition.y = (1.0f - ndcY) * 0.5f * kScreenHeight;
	return true;
}
