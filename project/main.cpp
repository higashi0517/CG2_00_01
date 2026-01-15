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
#include<fstream>
#include<sstream>
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
#include "Matrix4x4.h"
#include "TextureManager.h"
#include "Object3DManager.h"
#include "Object3D.h"

// クライアント領域のサイズ
//using float32_t = float;

using namespace Logger;
using namespace StringUtility;

struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

struct MaterialData {
	std::string textureFilePath;
};

struct ModelData {
	std::vector<Sprite::VertexData> vertices;
	MaterialData material;
};

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

MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {

	// 変数の宣言
	MaterialData materialData;
	std::string line;

	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	// ファイル読み込み
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連続してファイルパスする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}


// objファイルを読み込む関数
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename) {

	// 変数の宣言
	ModelData modelData;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;
	std::string line;

	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	// ファイル読み込み
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;// 先頭の識別子を読む

		// 頂点位置の読み込み
		if (identifier == "v") {

			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f; // 同次座標のためwは1.0
			// 位置のxを反転
			position.x *= -1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {

			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			// テクスチャ座標のyを反転
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if (identifier == "vn") {

			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			// 法線のxを反転
			normal.x *= -1.0f;
			normals.push_back(normal);
		}
		else if (identifier == "f") {

			Sprite::VertexData triangle[3];

			// 面は三角形限定、その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;

				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {

					std::string index;
					std::getline(v, index, '/');// 区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}

				// 要素へのindexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				//VertexData vetex = { position, texcoord, normal };
				//modelData.vertices.push_back(vetex);
				triangle[faceVertex] = { position, texcoord, normal };
			}

			// 頂点を逆順で登録することで、周り準を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		}
		else if (identifier == "mtllib") {

			// materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	return modelData;
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

	Object3DManager* object3DManager = nullptr;
	// 3Dオブジェクトマネージャの初期化
	object3DManager = new Object3DManager();
	object3DManager->Initialize(graphicsDevice);

	Object3D* object3D = nullptr;
	// 3Dオブジェクトの初期化
	object3D = new Object3D();

	// テクスチャマネージャの初期化
	TextureManager::GetInstance()->Initialize(graphicsDevice);
	TextureManager::GetInstance()->LoadTexture("Resources/uvChecker.png");
	TextureManager::GetInstance()->LoadTexture("Resources/monsterBall.png");

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
	ModelData modelData = LoadObjFile("resources", "axis.obj");

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

	// 平行光源の設定
	Microsoft::WRL::ComPtr <ID3D12Resource> directionalLightResource = graphicsDevice->CreateBufferResource(sizeof(DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 1.0f;

	// 音声の初期化
	Sound sound;
	Sound::SoundData soundData1 = sound.LoadWave("Resources/Alarm01.wav");
	sound.PlayWave(soundData1);

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

		// 取得（編集用のローカル変数）
		Vector2 pos = s->GetPosition();
		float   rot = s->GetRotation();
		Vector2 size = s->GetSize();
		Vector4 col = s->GetColor();

		// 編集
		if (ImGui::DragFloat2("Position", &pos.x, 1.0f)) s->SetPosition(pos);
		if (ImGui::DragFloat("Rotation", &rot, 0.01f))   s->SetRotation(rot);
		if (ImGui::DragFloat2("Size", &size.x, 1.0f))    s->SetSize(size);
		if (ImGui::ColorEdit4("Color", &col.x))          s->SetColor(col);

		ImGui::End();


		//// 変数を受け取る
		//static Vector2 pos = sprite->GetPosition();
		//static float rot = sprite->GetRotation();
		//static Vector2 size = sprite->GetSize();
		//static Vector4 col = sprite->GetColor();

		//ImGui::Begin("Sprite ");
		//// 位置
		//ImGui::DragFloat2("Position", &pos.x, 1.0f);
		//// 回転
		//ImGui::DragFloat("Rotation", &rot, 0.01f);
		//// サイズ
		//ImGui::DragFloat2("Size", &size.x, 1.0f);
		//// 色
		//ImGui::ColorEdit4("Color", &col.x);

		//ImGui::End();

		//// 変更を反映
		//sprite->SetPosition(pos);
		//sprite->SetRotation(rot);
		//sprite->SetSize(size);
		//sprite->SetColor(col);

		ImGui::Render();

		// PreDrawの処理
		graphicsDevice->PreDraw();

		// 3Dオブジェクト描画前共通設定
		object3DManager->SetCommonRenderState();


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

	winApp->Finalize();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	sound.Unload(&soundData1);
	delete input;
	delete winApp;
	delete graphicsDevice;
	delete object3D;
	delete object3DManager;
	for (Sprite* sprite : sprites) {
		delete sprite;
	}
	sprites.clear();
	delete spriteManager;

	return 0;
}