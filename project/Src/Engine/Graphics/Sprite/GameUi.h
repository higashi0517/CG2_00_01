#pragma once
#include "Sprite.h"
#include "SpriteManager.h"
#include "TextureManager.h"
#include <memory>
#include <string_view>
#include <vector>

// 既存のASCIIフォントと白画像だけを使うゲーム用UI。各描画に専用のバッファを再利用する。
class GameUi {
public:
	void Initialize(SpriteManager* manager) {
		manager_ = manager;
		TextureManager::GetInstance()->LoadTexture("Resources/white1x1.png");
		TextureManager::GetInstance()->LoadTexture("Resources/debugfont.png");
	}
	void Begin() { solidIndex_ = 0; glyphIndex_ = 0; }
	void Rect(float x, float y, float width, float height, const Vector4& color) {
		Sprite& sprite = Next(solids_, solidIndex_, "Resources/white1x1.png");
		sprite.SetTextureSize({ 1.0f, 1.0f });
		sprite.SetPosition({ x, y });
		sprite.SetSize({ width, height });
		sprite.SetColor(color);
		sprite.Update();
		sprite.Draw();
	}
	void Text(std::string_view text, float x, float y, float scale, const Vector4& color) {
		for (const unsigned char ch : text) {
			if (ch >= 33 && ch <= 126) {
				Sprite& sprite = Next(glyphs_, glyphIndex_, "Resources/debugfont.png");
				const int glyph = ch - 32;
				sprite.SetTextureLeftTop({ float(glyph % 14 * 9), float(glyph / 14 * 18) });
				sprite.SetTextureSize({ 9.0f, 18.0f });
				sprite.SetPosition({ x, y });
				sprite.SetSize({ 9.0f * scale, 18.0f * scale });
				sprite.SetColor(color);
				sprite.Update();
				sprite.Draw();
			}
			x += 9.0f * scale;
		}
	}
	void CenteredText(std::string_view text, float y, float scale, const Vector4& color) {
		Text(text, (1280.0f - float(text.size()) * 9.0f * scale) * 0.5f, y, scale, color);
	}
private:
	Sprite& Next(std::vector<std::unique_ptr<Sprite>>& pool, size_t& index, const char* texture) {
		if (index == pool.size()) {
			auto sprite = std::make_unique<Sprite>();
			sprite->Initialize(manager_, texture);
			pool.push_back(std::move(sprite));
		}
		return *pool[index++];
	}
	SpriteManager* manager_ = nullptr;
	std::vector<std::unique_ptr<Sprite>> solids_, glyphs_;
	size_t solidIndex_ = 0, glyphIndex_ = 0;
};
