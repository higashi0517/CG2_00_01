#include "ResourceObject.h"
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

// クライアント領域のサイズ
//using float32_t = float;

using namespace Logger;
using namespace StringUtility;

// 
//struct Vector4 {
//	float32_t x;
//	float32_t y;
//	float32_t z;
//	float32_t w;
//};
//
//struct Vector2 {
//	float32_t x;
//	float32_t y;
//};

//struct Matrix3x3 {
//	float32_t m[3][3];
//};

//struct Transform {
//	Vector3 scale;
//	Vector3 rotate;
//	Vector3 translate;
//};
//
//struct VertexData {
//	Vector4 position;
//	Vector2 texcoord;
//	Vector3 normal;
//};

//struct Material {
//	Vector4 color;
//	int32_t enableLighting;
//	float padding[3];
//	Matrix4x4 uvTransform;
//};

//struct TransformationMatrix {
//	Matrix4x4 WVP;
//	Matrix4x4 World;
//};

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

	SpriteManager* spriteManager = nullptr;
	// スプライト共通部の初期化
	spriteManager = new SpriteManager();
	spriteManager->Initialize(graphicsDevice);

	Sprite* sprite = new Sprite();
	sprite->Initialize(spriteManager);

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
	DirectX::ScratchImage mipImages = graphicsDevice->LoadTexture("Resources/uvChecker.png");
	const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = graphicsDevice->CreateTextureResource(metadata);

	textureUploadBuffers.push_back(
		graphicsDevice->UploadTextureData(mipImages, textureResource)
	);

	// metaDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	// SRVを作成するDescriptorHeapの場所
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = graphicsDevice->GetSRVCPUDescriptorHandle(1);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = graphicsDevice->GetSRVGPUDescriptorHandle(1);

	// SRVを生成
	graphicsDevice->GetDevice()->CreateShaderResourceView(
		textureResource.Get(),
		&srvDesc,
		textureSrvHandleCPU
	);

	// モデル読み込み
	ModelData modelData = LoadObjFile("resources", "axis.obj");

	DirectX::ScratchImage mipImages2 = GraphicsDevice::LoadTexture(modelData.material.textureFilePath);
	const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = graphicsDevice->CreateTextureResource(metadata2);
	textureUploadBuffers.push_back(
		graphicsDevice->UploadTextureData(mipImages2, textureResource2)
	);

	// metaDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 = {};
	srvDesc2.Format = metadata2.format;
	srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	// SRVを作成するDescriptorHeapの場所
	D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = graphicsDevice->GetSRVCPUDescriptorHandle(2);
	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = graphicsDevice->GetSRVGPUDescriptorHandle(2);

	// SRVを生成
	graphicsDevice->GetDevice()->CreateShaderResourceView(
		textureResource2.Get(),
		&srvDesc2,
		textureSrvHandleCPU2
	);

	//// RootSignatureの設定
	//D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	//descriptionRootSignature.Flags =
	//	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//// DescriptorRangeの設定
	//D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	//descriptorRange[0].BaseShaderRegister = 0;
	//descriptorRange[0].NumDescriptors = 1;
	//descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	//descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	//// RootParameterの設定
	//D3D12_ROOT_PARAMETER rootParameters[4] = {};
	//rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//rootParameters[0].Descriptor.ShaderRegister = 0;
	//rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	//rootParameters[1].Descriptor.ShaderRegister = 0;
	//rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	//rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	//rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
	//rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	//rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//rootParameters[3].Descriptor.ShaderRegister = 1;
	//descriptionRootSignature.pParameters = rootParameters;
	//descriptionRootSignature.NumParameters = _countof(rootParameters);

	//// Samplerの設定
	//D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	//staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	//staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	//staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	//staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	//staticSamplers[0].ShaderRegister = 0;
	//staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	//descriptionRootSignature.pStaticSamplers = staticSamplers;
	//descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

	//Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob = nullptr;
	//Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
	//HRESULT hr = D3D12SerializeRootSignature(
	//	&descriptionRootSignature,
	//	D3D_ROOT_SIGNATURE_VERSION_1,
	//	&signatureBlob,
	//	&errorBlob
	//);

	//if (FAILED(hr)) {
	//	Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
	//	assert(false);
	//}

	//// RootSignatureの生成
	//Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
	//hr = graphicsDevice->GetDevice()->CreateRootSignature(
	//	0, signatureBlob->GetBufferPointer(),
	//	signatureBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature));
	//assert(SUCCEEDED(hr));

	//// InputLayout
	//D3D12_INPUT_ELEMENT_DESC inputElementDescs[3] = {};
	//inputElementDescs[0].SemanticName = "POSITION";
	//inputElementDescs[0].SemanticIndex = 0;
	//inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	//inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//inputElementDescs[1].SemanticName = "TEXCOORD";
	//inputElementDescs[1].SemanticIndex = 0;
	//inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	//inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//inputElementDescs[2].SemanticName = "NORMAL";
	//inputElementDescs[2].SemanticIndex = 0;
	//inputElementDescs[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
	//inputElementDescs[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
	//D3D12_INPUT_LAYOUT_DESC inputLayoutDesc{};
	//inputLayoutDesc.pInputElementDescs = inputElementDescs;
	//inputLayoutDesc.NumElements = _countof(inputElementDescs);

	//// BlendStateの設定
	//D3D12_BLEND_DESC blendDesc{};
	//blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//// RasterizerStateの設定
	//D3D12_RASTERIZER_DESC rasterizerDesc{};
	//rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//// Shaderのコンパイル
	//Microsoft::WRL::ComPtr<IDxcBlob> vertexShaderBlob = graphicsDevice->CompileShader(L"Resources/shaders/Object3d.VS.hlsl", L"vs_6_0");
	//assert(vertexShaderBlob != nullptr);

	//Microsoft::WRL::ComPtr<IDxcBlob> pixelShaderBlob = graphicsDevice->CompileShader(L"Resources/shaders/Object3d.PS.hlsl", L"ps_6_0");
	//assert(pixelShaderBlob != nullptr);

	//// PSOの設定
	//D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc{};
	//graphicsPipelineStateDesc.pRootSignature = rootSignature.Get();
	//graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
	//graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),vertexShaderBlob->GetBufferSize() };
	//graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),pixelShaderBlob->GetBufferSize() };
	//graphicsPipelineStateDesc.BlendState = blendDesc;
	//graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;

	//// DepthStencilStateの設定
	//D3D12_DEPTH_STENCIL_DESC depthStencilDesc{};
	//depthStencilDesc.DepthEnable = true;
	//depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	//graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//// RTVの設定
	//graphicsPipelineStateDesc.NumRenderTargets = 1;
	//graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//graphicsPipelineStateDesc.SampleDesc.Count = 1;
	//graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//// 生成
	//Microsoft::WRL::ComPtr < ID3D12PipelineState> graphicsPipelineState = nullptr;
	//hr = graphicsDevice->GetDevice()->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	//assert(SUCCEEDED(hr));

	//// Materialの設定
	//Microsoft::WRL::ComPtr <ID3D12Resource> materialResource = graphicsDevice->CreateBufferResource(sizeof(Material));
	//Material* materialData = nullptr;
	//materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//materialData->enableLighting = true;
	//materialData->uvTransform = MakeIdentity4x4();

	//// WVP行列の設定
	//Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource = graphicsDevice->CreateBufferResource(sizeof(TransformationMatrix));
	//TransformationMatrix* transformationMatrixData = nullptr;
	//wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));

	// 平行光源の設定
	Microsoft::WRL::ComPtr <ID3D12Resource> directionalLightResource = graphicsDevice->CreateBufferResource(sizeof(DirectionalLight));
	DirectionalLight* directionalLightData = nullptr;
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 1.0f;

	//// 頂点リソ－スを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = graphicsDevice->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	//vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	//vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	//vertexBufferView.StrideInBytes = sizeof(VertexData);

	//VertexData* vertexData = nullptr;
	//vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	//std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

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

	//// Transformの設定
	//Transform transform{ {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };

	//// cameraTransformの設定
	//Transform cameraTransform{ {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, -10.0f} };

	// 切り替え用のフラグ
	bool useMonsterBall = false;

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

		//// ImGuiによるカメラ移動
		//ImGui::SliderFloat3("Camera Translate", &cameraTransform.translate.x, -20.0f, 0.0f);
		//ImGui::Checkbox("useMonsterBall", &useMonsterBall);
		//ImGui::SliderFloat("Transform Rotate Y", &transform.rotate.y, -3.14f, 3.14f);

		//// ライティング
		//bool enableLighting = (materialData->enableLighting != 0);
		//if (ImGui::Checkbox("enableLighting", &enableLighting)) {
		//	materialData->enableLighting = enableLighting ? 1 : 0;
		//}
		//ImGui::ColorEdit4("Directional Light Color", &directionalLightData->color.x);
		//ImGui::SliderFloat3("Directional Light Direction", &directionalLightData->direction.x, -1.0f, 1.0f);
		//ImGui::SliderFloat("Directional Light Intensity", &directionalLightData->intensity, 0.0f, 10.0f);

		ImGui::Render();

		// Transformの更新
		/*Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
		Matrix4x4 viewMatrix = Inverse(cameraMatrix);
		Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
		Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
		transformationMatrixData->WVP = worldViewProjectionMatrix;
		transformationMatrixData->World = worldMatrix;*/

		// PreDrawの処理
		graphicsDevice->PreDraw();

		//// sprite描画前共通設定
		spriteManager->SetCommonRenderState();

		sprite->Update();
		sprite->Draw();

		//// 描画コマンド
		//graphicsDevice->GetCommandList()->SetGraphicsRootSignature(rootSignature.Get());
		//graphicsDevice->GetCommandList()->SetPipelineState(graphicsPipelineState.Get());
		//graphicsDevice->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
		//graphicsDevice->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//graphicsDevice->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		//graphicsDevice->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
		//graphicsDevice->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
		//graphicsDevice->GetCommandList()->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);
		//graphicsDevice->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

		// ImGuiの描画
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), graphicsDevice->GetCommandList().Get());

		// PostDrawの処理
		graphicsDevice->PostDraw();
	}

	winApp->Finalize();
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	sound.Unload(&soundData1);
	delete input;
	delete winApp;
	delete graphicsDevice;
	delete sprite;
	delete spriteManager;

	return 0;
}