//#include "ResourceObject.h"
#include <Windows.h>
#include <wrl.h>
#include <filesystem>
#include <fstream>
#include <chrono>
//#include <d3d12.h>
//#include <dxgi1_6.h>
#include <cassert>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#include <string>
#include <dbghelp.h>
#pragma comment(lib,"Dbghelp.lib")
#include <strsafe.h>
#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")
#include <dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")
#include "Matrix4x4.h"
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>
#include <sstream>
#include <xaudio2.h>
#pragma comment (lib,"xaudio2.lib")
#include "Sound.h"
#include "DebugCamera.h"
#include "Input.h"
#include "WinApp.h"
#include "GraphicsDevice.h"
#include "Logger.h"
#include "StringUtility.h"
#include "D3DResourceLeakChecker.h"
#include "SpriteManager.h"
#include "Sprite.h"
#include "TextureManager.h"
#include "Object3DManager.h"
#include "Object3D.h"
#include "ModelCommon.h"
#include "Model.h"
#include "ModelManager.h"

// クライアント領域のサイズ
//using float32_t = float;

using namespace Logger;
using namespace StringUtility;

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {

	// 時間を取得して、時刻を名前の入れたファイルを作成
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dump", NULL);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dump/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	// processIdとクラッシュの発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;

	// Dumpを出力
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

	return EXCEPTION_EXECUTE_HANDLER;
}

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	CoInitializeEx(0, COINIT_MULTITHREADED);

	// リソースリークチェック
	D3DResourceLeakChecker* leakChecker = nullptr;

	SetUnhandledExceptionFilter(ExportDump);

	// ポインタ
	WinApp* winApp = nullptr;

	// WindowsAPIの初期化
	winApp = new WinApp();
	winApp->Initialize();

	// ポインタ
	GraphicsDevice* graphicsDevice = nullptr;
	// グラフィックスデバイスの初期化
	graphicsDevice = new GraphicsDevice();
	graphicsDevice->Initialize(winApp);

	// テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(graphicsDevice);
	TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("Resources/monsterBall.png");

	// 3Dモデルマネジャの初期化
	ModelManager::GetInstance()->Initialize(graphicsDevice);
	// .objモデルの読み込み
	ModelManager::GetInstance()->LoadModel("plane.obj");

	//// モデルマネージャの初期化
	//ModelCommon* modelCommon = new ModelCommon();
	//modelCommon->Initialize(graphicsDevice);

	//// モデルの生成
	//Model* model = new Model();
	//model->Initialize(modelCommon);

	Object3DManager* object3DManager = nullptr;
	// 3Dオブジェクトマネージャの初期化
	object3DManager = new Object3DManager();
	object3DManager->Initialize(graphicsDevice);

	Object3D* object3D = nullptr;
	object3D = new Object3D();
	object3D->Initialize(object3DManager);
	//object3D->SetModel(model);           // モデルをセット
	// 初期化済みの3Dオブジェクトモデルを紐づける
	object3D->SetModel("plane.obj");

	Object3D* object3D_2 = new Object3D();      // 2つ目を生成
	object3D_2->Initialize(object3DManager);
	object3D_2->SetModel("plane.obj");                // ★同じモデルをセット（使い回し）

	SpriteManager* spriteManager = nullptr;
	// スプライト共通部の初期化
	spriteManager = new SpriteManager();
	spriteManager->Initialize(graphicsDevice);

	std::vector<Sprite*> sprites;
	for (uint32_t i = 0; i < 5; ++i) {
		Sprite* sprite = new Sprite();
		sprite->Initialize(spriteManager, "Resources/uvChecker.png");
		if (i % 2 == 1)
		{
			sprite->ChangeTexture("Resources/monsterBall.png");
		}
		sprite->SetPosition({ 100.0f + i * 120.0f, 50.0f });
		sprite->SetSize({ 100.0f,100.0f });
		sprites.push_back(sprite);
	}

	std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> textureUploadBuffers;

	// ログのディレクトリを表示
	std::filesystem::create_directory("logs");

	// 現在時間を取得
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	// ログファイルを秒に
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);

	// 日本時間に変更
	std::chrono::zoned_time localTime{ std::chrono::current_zone(),nowSeconds };

	// formatを使って年月日_時分秒の文字列に変換
	std::string dataString = std::format("{:%y%m%d_%H%M%S}", localTime);

	// 時刻を使ってファイル名の決定
	std::string logFilePath = std::string("logs/") + dataString + ".log";

	// ファイルを作って書き込み準備
	std::ofstream logStream(logFilePath);

	// 書き込み
	Log("ログの書き込み");

	// 初期化完了のログ
	Log("Complete create D3D12Device!!!\n");

	// テクスチャ読み込み
	//DirectX::ScratchImage mipImages = TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
	//const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	//Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = graphicsDevice->CreateTextureResource(metadata);

	/*textureUploadBuffers.push_back(
		graphicsDevice->UploadTextureData(mipImages, textureResource)
	);*/

	//// metaDataを基にSRVの設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//srvDesc.Format = metadata.format;
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	//// SRVを作成するDescriptorHeapの場所
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = graphicsDevice->GetSRVCPUDescriptorHandle(1);
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = graphicsDevice->GetSRVGPUDescriptorHandle(1);

	//// SRVを生成
	//graphicsDevice->GetDevice()->CreateShaderResourceView(
	//	textureResource.Get(),
	//	&srvDesc,
	//	textureSrvHandleCPU
	//);

	// モデル読み込み
	//ModelData modelData = LoadObjFile("resources", "axis.obj");

	/*DirectX::ScratchImage mipImages2 = GraphicsDevice::LoadTexture(modelData.material.textureFilePath);
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = graphicsDevice->CreateTextureResource(metadata2);
	textureUploadBuffers.push_back(
		graphicsDevice->UploadTextureData(mipImages2, textureResource2)
	);*/

	//// metaDataを基にSRVの設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 = {};
	//srvDesc2.Format = metadata2.format;
	//srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	//// SRVを作成するDescriptorHeapの場所
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = graphicsDevice->GetSRVCPUDescriptorHandle(2);
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = graphicsDevice->GetSRVGPUDescriptorHandle(2);

	//// SRVを生成
	//graphicsDevice->GetDevice()->CreateShaderResourceView(
	//	textureResource2.Get(),
	//	&srvDesc2,
	//	textureSrvHandleCPU2
	//);

	//// 平行光源の設定
	//Microsoft::WRL::ComPtr <ID3D12Resource> directionalLightResource = graphicsDevice->CreateBufferResource(sizeof(DirectionalLight));
	//DirectionalLight* directionalLightData = nullptr;
	//directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	//directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	//directionalLightData->intensity = 1.0f;

	// 音声の初期化
	Sound sound;
	Sound::SoundData soundData1 = sound.LoadWave("Resources/Alarm01.wav");
	//sound.PlayWave(soundData1);

	// 入力の初期化
	Input* input = nullptr;
	input = new Input();
	input->Initialize(winApp);

	DebugCamera debugCamera;

	MSG msg = {};

	// 切り替え用のフラグ
	bool useMonsterBall = false;

	static int selected = 0;

	// ウィンドウのボタンが押されるまでループ
	while (true) {

		// メッセージがあれば処理
		if (winApp->ProcessMessage()) {
			break;
		}

		input->Update();
		if (input->TriggerKey(DIK_0)) {
			OutputDebugStringA("Hit 0\n");
		}

		// ゲーム処理

		// Imguiのフレーム開始
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Sprites");

		ImGui::SliderInt("Selected", &selected, 0, (int)sprites.size() - 1);

		Sprite* s = sprites[selected];

		// sprite
		Vector2 pos = s->GetPosition();
		float   rot = s->GetRotation();
		Vector2 size = s->GetSize();
		Vector4 col = s->GetColor();

		if (ImGui::DragFloat2("Position", &pos.x, 1.0f)) s->SetPosition(pos);
		if (ImGui::DragFloat("Rotation", &rot, 0.01f))   s->SetRotation(rot);
		if (ImGui::DragFloat2("Size", &size.x, 1.0f))    s->SetSize(size);
		if (ImGui::ColorEdit4("Color", &col.x))          s->SetColor(col);

		// 3d object
		static Vector3 pos3D = object3D->GetTranslate();

		if (ImGui::DragFloat3("3D Position", &pos3D.x, 0.01f)) {
			object3D->SetTranslate(pos3D);
		}

		ImGui::End();

		ImGui::Render();

		// PreDrawの処理
		graphicsDevice->PreDraw();

		// 3Dオブジェクト描画前共通設定
		object3DManager->SetCommonRenderState();

		object3D->Update();
		object3D->Draw();
		object3D_2->Update();
		object3D_2->Draw();

		// sprite描画前共通設定
		spriteManager->SetCommonRenderState();

		for (auto& sprite : sprites) {

			sprite->Update();
			sprite->Draw();
		}

		// ImGuiの描画
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), graphicsDevice->GetCommandList().Get());

		// PostDrawの処理
		graphicsDevice->PostDraw();
	}

	// テクスチャマネージャの終了
	TextureManager::GetInstance()->Finalize();
	// 3Dモデルマネージャの終了
	ModelManager::GetInstance()->Finalize();

	winApp->Finalize();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	sound.Unload(&soundData1);
	delete input;
	delete winApp;
	delete graphicsDevice;
	delete object3D;
	delete object3D_2;
	delete object3DManager;
	//delete model;
	//delete modelCommon;
	for (Sprite* sprite : sprites) {
		delete sprite;
	}
	sprites.clear();
	delete spriteManager;

	return 0;
}