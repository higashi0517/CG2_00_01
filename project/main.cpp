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

// クライアント領域のサイズ
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;
using float32_t = float;

using namespace Logger;
using namespace StringUtility;

// 
struct Vector4 {
	float32_t x;
	float32_t y;
	float32_t z;
	float32_t w;
};

struct Vector2 {
	float32_t x;
	float32_t y;
};

struct Matrix3x3 {
	float32_t m[3][3];
};

struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material {
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct TransformationMatrix {
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLight {
	Vector4 color;
	Vector3 direction;
	float intensity;
};

struct MaterialData {
	std::string textureFilePath;
};

struct ModelData {
	std::vector<VertexData> vertices;
	MaterialData material;
};
//
//void Log(std::ostream& os, const std::string& message) {
//
//	os << message << std::endl;
//
//	OutputDebugStringA(message.c_str());
//}

//std::wstring ConvertString(const std::string& str) {
//	if (str.empty()) {
//		return std::wstring();
//	}
//
//	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
//	if (sizeNeeded == 0) {
//		return std::wstring();
//	}
//	std::wstring result(sizeNeeded, 0);
//	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), (int)(str.size()), &result[0], sizeNeeded);
//	return result;
//}
//
//std::string ConvertString(const std::wstring& str) {
//	if (str.empty()) {
//		return std::string();
//	}
//
//	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
//	if (sizeNeeded == 0) {
//		return std::string();
//	}
//	std::string result(sizeNeeded, 0);
//	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
//	return result;
//}

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

// リソースリークチェック
struct D3DResourceLeakCheker {
	~D3DResourceLeakCheker() {

		Microsoft::WRL::ComPtr<IDXGIDebug> debug;

		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {

			debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
			debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
		}
	}
};

IDxcBlob* CompileShader(
	const std::wstring& filePath,
	const wchar_t* profile,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler,
	std::ostream& logStream
) {
	// シェーダーのコンパイル
	Log(ConvertString(std::format(L"Begin CompileShader,path:{},profile:{}\n", filePath, profile)));

	// hlslファイルの読み込み
	IDxcBlobEncoding* shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));

	// 読み込んだファイルの設定
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;

	LPCWSTR arguments[] = {
		filePath.c_str(),         // シェーダーのファイル名
		L"-E", L"main",           // エントリーポイント
		L"-T", profile,           // シェーダーのプロファイル
		L"-Zi",L"-Qembed_debug",  // デバッグ情報を出力
		L"-Od",                   // 最適化を無効にする
		L"-Zpr",                  // デバッグ情報を埋め込む
	};

	// コンパイルの実行
	IDxcResult* shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);

	assert(SUCCEEDED(hr));

	// 警告、エラーが出たらログに出力
	IDxcBlobUtf8* shaderError = nullptr;

	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);

	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {

		Log(shaderError->GetStringPointer());
		assert(false);
	}

	// コンパイル結果を取得
	IDxcBlob* shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));

	// 成功したログ
	Log(ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}/n", filePath, profile)));

	// 解放
	shaderSource->Release();
	shaderResult->Release();

	return shaderBlob;
}

Microsoft::WRL::ComPtr <ID3D12Resource> CreateBufferResource(Microsoft::WRL::ComPtr < ID3D12Device> device, size_t sizeInBytes) {

	Microsoft::WRL::ComPtr <ID3D12Resource> vertexResource = nullptr;

	// 頂点リソース用のヒープ設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes;
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// リソースの生成
	HRESULT hr = device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&vertexResource)
	);
	assert(SUCCEEDED(hr));

	return vertexResource;
}

//// DescriptorHeap
//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(Microsoft::WRL::ComPtr < ID3D12Device> device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
//
//	Microsoft::WRL::ComPtr <ID3D12DescriptorHeap> descriptorHeap = nullptr;
//	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
//	descriptorHeapDesc.Type = heapType;
//	descriptorHeapDesc.NumDescriptors = numDescriptors;
//	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
//	HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
//	assert(SUCCEEDED(hr));
//	return descriptorHeap;
//}

DirectX::ScratchImage LoadTexture(const std::string& filePath) {

	DirectX::ScratchImage image{};
	std::wstring filePathW = ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミニマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, mipImages);
	assert(SUCCEEDED(hr));

	return mipImages;
}

Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(Microsoft::WRL::ComPtr < ID3D12Device> device, const DirectX::TexMetadata& metadata) {

	// Metadataを基にResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = UINT(metadata.width);
	resourceDesc.Height = UINT(metadata.height);
	resourceDesc.MipLevels = UINT16(metadata.mipLevels);
	resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
	resourceDesc.Format = metadata.format;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

	// 利用するHeapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	// Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&resource)
	);
	assert(SUCCEEDED(hr));

	return resource;
}

[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(Microsoft::WRL::ComPtr<ID3D12Resource> texture, const DirectX::ScratchImage& mipImage,
	Microsoft::WRL::ComPtr<ID3D12Device> device, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList) {

	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	DirectX::PrepareUpload(
		device.Get(),
		mipImage.GetImages(),
		mipImage.GetImageCount(),
		mipImage.GetMetadata(),
		subresources
	);
	uint64_t intermediateSize = GetRequiredIntermediateSize(
		texture.Get(),
		0,
		UINT(subresources.size())
	);
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = CreateBufferResource(device.Get(), intermediateSize);
	UpdateSubresources(
		commandList.Get(),
		texture.Get(),
		intermediateResource.Get(),
		0,
		0,
		UINT(subresources.size()),
		subresources.data()
	);

	// Resourceの状態を変更
	D3D12_RESOURCE_BARRIER barrier{};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = texture.Get();
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
	commandList->ResourceBarrier(1, &barrier);
	return intermediateResource;
}

//Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(Microsoft::WRL::ComPtr<ID3D12Device> device, int32_t width, int32_t height) {
//
//	// 生成するResourceの設定
//	D3D12_RESOURCE_DESC resourceDesc{};
//	resourceDesc.Width = width;
//	resourceDesc.Height = height;
//	resourceDesc.MipLevels = 1;
//	resourceDesc.DepthOrArraySize = 1;
//	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
//	resourceDesc.SampleDesc.Count = 1;
//	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
//	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
//
//	// 利用するHeapの設定
//	D3D12_HEAP_PROPERTIES heapProperties{};
//	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
//
//	// 深度値のクリア設定
//	D3D12_CLEAR_VALUE depthClearValue{};
//	depthClearValue.DepthStencil.Depth = 1.0f;
//	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
//
//	// Resourceの生成
//	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
//	HRESULT hr = device->CreateCommittedResource(
//		&heapProperties,
//		D3D12_HEAP_FLAG_NONE,
//		&resourceDesc,
//		D3D12_RESOURCE_STATE_DEPTH_WRITE,
//		&depthClearValue,
//		IID_PPV_ARGS(&resource)
//	);
//	assert(SUCCEEDED(hr));
//
//	return resource;
//}

//D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(
//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap,
//	uint32_t descriptorSize, uint32_t index
//) {
//	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
//	handleCPU.ptr += (descriptorSize * index);
//	return handleCPU;
//}
//
//D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(
//	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap,
//	uint32_t descriptorSize, uint32_t index
//) {
//	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
//	handleGPU.ptr += (descriptorSize * index);
//	return handleGPU;
//}

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

			VertexData triangle[3];

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
	D3DResourceLeakCheker leakChecker;

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

#ifdef _DEBUG

	//Microsoft::WRL::ComPtr<ID3D12Debug1> debugController = nullptr;
	//if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {

	//	// デバッグレイヤーの有効化
	//	debugController->EnableDebugLayer();

	//	// GPU側でのチェック
	//	debugController->SetEnableGPUBasedValidation(TRUE);
	//}

#endif

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

	//// DXGIファクトリーの生成
	//Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory = nullptr;
	//HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	//assert(SUCCEEDED(hr));

	//// アダプタの変数
	//Microsoft::WRL::ComPtr<IDXGIAdapter4> useAdapter = nullptr;

	//// アダプタの列挙
	//for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(
	//	i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {

	//	// アダプタの情報を取得
	//	DXGI_ADAPTER_DESC3 adapterDesc;
	//	hr = useAdapter->GetDesc3(&adapterDesc);
	//	assert(SUCCEEDED(hr));

	//	if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {

	//		// アダプタの情報をログに出力
	//		Log(logStream, ConvertString(std::format(L"Use Adapter {}: \n", adapterDesc.Description)));
	//		break;
	//	}
	//	useAdapter = nullptr;
	//}
	//assert(useAdapter != nullptr);

	//Microsoft::WRL::ComPtr<ID3D12Device> device = nullptr;
	//D3D_FEATURE_LEVEL featureLevels[] = {
	//	D3D_FEATURE_LEVEL_12_2,
	//	D3D_FEATURE_LEVEL_12_1,
	//	D3D_FEATURE_LEVEL_12_0
	//};

	//const char* featureLevelStrings[] = {
	//	"12.2",
	//	"12.1",
	//	"12.0"
	//};

	//for (size_t i = 0; i < _countof(featureLevels); i++) {
	//	// デバイスの生成
	//	hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device));

	//	if (SUCCEEDED(hr)) {

	//		Log(logStream, std::format("Feature Level: {}\n", featureLevelStrings[i]));
	//		break;
	//	}
	//}
	//assert(device != nullptr);

	// 初期化完了のログ
	Log("Complete create D3D12Device!!!\n");

#ifdef _DEBUG

	//Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
	//if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {

	//	// 致命的なエラーで停止
	//	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);

	//	// エラーで停止
	//	infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);

	//	// 警告で停止
	//	//infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

	//	// 抑制するメッセージのID
	//	D3D12_MESSAGE_ID denyIds[] = {

	//		D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
	//	};

	//	// 抑制するレベル
	//	D3D12_MESSAGE_SEVERITY severities[] = {

	//		D3D12_MESSAGE_SEVERITY_INFO
	//	};

	//	D3D12_INFO_QUEUE_FILTER filter = {};
	//	filter.DenyList.NumIDs = _countof(denyIds);
	//	filter.DenyList.pIDList = denyIds;
	//	filter.DenyList.NumSeverities = _countof(severities);
	//	filter.DenyList.pSeverityList = severities;

	//	// 指定したメッセージを抑制
	//	infoQueue->PushStorageFilter(&filter);
	//}

#endif

	//// コマンドキューの生成
	//Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	//D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
	//hr = device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue));
	//assert(SUCCEEDED(hr));

	//// コマンドアロケータを生成
	//Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator = nullptr;
	//hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));
	//assert(SUCCEEDED(hr));

	//// コマンドリストの生成
	//Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList = nullptr;
	//hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));
	//assert(SUCCEEDED(hr));

	//// スワップチェーンの生成
	//Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain = nullptr;
	//DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	//swapChainDesc.Width = kClientWidth;                        // 画面の幅
	//swapChainDesc.Height = kClientHeight;                       // 画面の高さ
	//swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;          // 色の形式
	//swapChainDesc.SampleDesc.Count = 1;                         // マルチサンプルしない
	//swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;// 描画のターゲットとして利用する
	//swapChainDesc.BufferCount = 2;                              // ダブルバッファ
	//swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;   // モニタに移した後は捨てる

	//// コマンドキュー、ウィンドウハンドル、設定を渡して生成
	//hr = dxgiFactory->CreateSwapChainForHwnd(
	//	commandQueue.Get(),
	//	winApp->GetHwnd(),
	//	&swapChainDesc,
	//	nullptr,
	//	nullptr,
	//	reinterpret_cast <IDXGISwapChain1**> (swapChain.GetAddressOf())
	//);
	//assert(SUCCEEDED(hr));

	//const uint32_t descriptorSizeSRV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//const uint32_t descriptorSizeRTV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//const uint32_t descriptorSizeDSV = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	//// rtv用ディスクリプタヒープの生成
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 2, false);
	//GetCPUDescriptorHandle(rtvDescriptorHeap, descriptorSizeRTV, 0);

	//// srv用ディスクリプタヒープの生成
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

	//// dsv用ディスクリプタヒープの生成
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap = CreateDescriptorHeap(device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
	//Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilReosurce = CreateDepthStencilTextureResource(device.Get(), kClientWidth, kClientHeight);

	//// SwapChainからResourceを取得
	//Microsoft::WRL::ComPtr<ID3D12Resource> swapChainResources[2] = { nullptr };
	//hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
	//assert(SUCCEEDED(hr));
	//hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
	//assert(SUCCEEDED(hr));

	//// RTVの設定
	//D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
	//rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	//rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	//// ディスクリプタの先頭を取得
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[2];

	/*rtvHandle[0] = rtvStartHandle;
	device->CreateRenderTargetView(swapChainResources[0].Get(), &rtvDesc, rtvHandle[0]);
	rtvHandle[1].ptr = rtvHandle[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	device->CreateRenderTargetView(swapChainResources[1].Get(), &rtvDesc, rtvHandle[1]);*/

	//// DSVの設定
	//D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	//dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	//dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;

	//device->CreateDepthStencilView(depthStencilReosurce.Get(), &dsvDesc, dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	//ResourceObject depthStencilResource = CreateDepthStencilTextureResource(device.Get(), kClientWidth, kClientHeight);

	// バックバッファのインデックスを取得
//	UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

	//// dxcCompilerの初期化
	//IDxcUtils* dxcUtils = nullptr;
	//IDxcCompiler3* dxcCompiler = nullptr;
	//hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
	//assert(SUCCEEDED(hr));
	//hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));

	//if (FAILED(hr)) {
	//	Log(logStream, "Failed to create DxcCompiler");
	//	assert(false);
	//}

	//// includeに対応するための設定
	//IDxcIncludeHandler* includehandler = nullptr;
	//hr = dxcUtils->CreateDefaultIncludeHandler(&includehandler);
	//assert(SUCCEEDED(hr));

	//DirectX::ScratchImage mipImages = LoadTexture("Resources/uvChecker.png");
	//const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
	//Microsoft::WRL::ComPtr<ID3D12Resource> textureResource = CreateTextureResource(graphicsDevice->GetDevice(), metadata);
	//Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource = UploadTextureData(textureResource, mipImages, graphicsDevice->GetDevice(), graphicsDevice->GetCommandList());

	//// metaDataを基にSRVの設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//srvDesc.Format = metadata.format;
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels);

	//// SRVを作成するDescriptorHeapの場所
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	//textureSrvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//textureSrvHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//// SRVを生成
	//device->CreateShaderResourceView(
	//	textureResource.Get(),
	//	&srvDesc,
	//	textureSrvHandleCPU
	//);

	//// 2枚目
	//ModelData modelData = LoadObjFile("resources", "axis.obj");

	//DirectX::ScratchImage mipImages2 = LoadTexture(modelData.material.textureFilePath);
	//const DirectX::TexMetadata& metadata2 = mipImages2.GetMetadata();
	//Microsoft::WRL::ComPtr<ID3D12Resource> textureResource2 = CreateTextureResource(device.Get(), metadata2);
	//Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource2 = UploadTextureData(textureResource2, mipImages2, device, commandList);

	//// metaDataを基にSRVの設定
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc2 = {};
	//srvDesc2.Format = metadata2.format;
	//srvDesc2.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc2.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc2.Texture2D.MipLevels = UINT(metadata2.mipLevels);

	//// SRVを作成するDescriptorHeapの場所
	//D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU2 = GetCPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);
	//D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU2 = GetGPUDescriptorHandle(srvDescriptorHeap, descriptorSizeSRV, 2);

	//// SRVを生成
	//device->CreateShaderResourceView(
	//	textureResource2.Get(),
	//	&srvDesc2,
	//	textureSrvHandleCPU2
	//);

	//// RootSignatureの設定
	//D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
	//descriptionRootSignature.Flags =
	//	D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	//// DescriptorRangeの設定
	//D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
	//descriptorRange[0].BaseShaderRegister = 0;            // シェーダーのレジスタ番号
	//descriptorRange[0].NumDescriptors = 1;               // SRVの数
	//descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV; // SRV
	//descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; // SRVのオフセット

	//// RootParameterの設定
	//D3D12_ROOT_PARAMETER rootParameters[4] = {};
	//rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // CBV
	//rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // ピクセルシェーダー
	//rootParameters[0].Descriptor.ShaderRegister = 0;                     // シェーダーのレジスタ番号
	//rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;	 // CBV
	//rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX; // ピクセルシェーダー
	//rootParameters[1].Descriptor.ShaderRegister = 0;                     // シェーダーのレジスタ番号
	//rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;     // SRV
	//rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダー
	//rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
	//rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);         // SRVの数
	//rootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;     // CBV
	//rootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;  // ピクセルシェーダー
	//rootParameters[3].Descriptor.ShaderRegister = 1;                     // シェーダーのレジスタ番号
	//descriptionRootSignature.pParameters = rootParameters;               // RootParameterのポインタ
	//descriptionRootSignature.NumParameters = _countof(rootParameters);   // 配列の長さ

	//// Samplerの設定
	//D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	//staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR; // 線形フィルタ
	//staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // U方向のアドレスモード
	//staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // V方向のアドレスモード
	//staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP; // W方向のアドレスモード
	//staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER; // 比較関数
	//staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX; // 最大LOD
	//staticSamplers[0].ShaderRegister = 0; // シェーダーのレジスタ番号
	//staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL; // ピクセルシェーダー
	//descriptionRootSignature.pStaticSamplers = staticSamplers;
	//descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers); // StaticSamplerの数

	//ID3DBlob* signatureBlob = nullptr;
	//ID3DBlob* errorBlob = nullptr;
	//hr = D3D12SerializeRootSignature(
	//	&descriptionRootSignature,
	//	D3D_ROOT_SIGNATURE_VERSION_1,
	//	&signatureBlob,
	//	&errorBlob
	//);

	//if (FAILED(hr)) {

	//	Log(logStream, reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
	//	assert(false);
	//}

	//// RootSignatureの生成
	//Microsoft::WRL::ComPtr <ID3D12RootSignature> rootSignature = nullptr;
	//hr = device->CreateRootSignature(
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
	//// すべての色要素
	//blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	//// RasterizerStateの設定
	//D3D12_RASTERIZER_DESC rasterizerDesc{};
	//// 裏面を表示しない
	//rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	//// 三角形を塗りつぶす
	//rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

	//// Shaderのコンパイル
	//IDxcBlob* vertexShaderBlob = CompileShader(L"Resources/shaders/Object3d.VS.hlsl", L"vs_6_0", dxcUtils, dxcCompiler, includehandler, logStream);
	//assert(vertexShaderBlob != nullptr);

	//IDxcBlob* pixelShaderBlob = CompileShader(L"Resources/shaders/Object3d.PS.hlsl", L"ps_6_0", dxcUtils, dxcCompiler, includehandler, logStream);
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
	//// Depthの機能を有効化
	//depthStencilDesc.DepthEnable = true;
	//// 書き込み
	//depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//// 比較関数
	//depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;

	//// DepthStencilの設定
	//graphicsPipelineStateDesc.DepthStencilState = depthStencilDesc;
	//graphicsPipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	//// RTVの設定
	//graphicsPipelineStateDesc.NumRenderTargets = 1;
	//graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	//// 利用する形状のタイプ(三角形)
	//graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//graphicsPipelineStateDesc.SampleDesc.Count = 1;
	//graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	//// 生成
	//Microsoft::WRL::ComPtr < ID3D12PipelineState> graphicsPipelineState = nullptr;
	//hr = device->CreateGraphicsPipelineState(&graphicsPipelineStateDesc, IID_PPV_ARGS(&graphicsPipelineState));
	//assert(SUCCEEDED(hr));

	//// Materialの設定
	//Microsoft::WRL::ComPtr <ID3D12Resource> materialResource = CreateBufferResource(device.Get(), sizeof(Material));
	//// データを書き込む
	//Material* materialData = nullptr;
	//// 書き込むためのアドレスを取得
	//materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	//// 色を設定
	//materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//// フラグ
	//materialData->enableLighting = true;
	//materialData->uvTransform = MakeIdentity4x4();

	//// Sprite用のMaterialの設定
	//Microsoft::WRL::ComPtr<ID3D12Resource>materialResourceSprite = CreateBufferResource(device, sizeof(Material));
	//// データを書き込む
	//Material* materialDataSprite = nullptr;
	//// 書き込むためのアドレスを取得
	//materialResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&materialDataSprite));
	//// 色を設定
	//materialDataSprite->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	//// フラグ
	//materialDataSprite->enableLighting = false;
	//materialDataSprite->uvTransform = MakeIdentity4x4();

	//// WVP行列の設定
	//Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource = CreateBufferResource(device.Get(), sizeof(TransformationMatrix));
	//// データを書き込む
	//TransformationMatrix* transformationMatrixData = nullptr;
	//// 書き込むためのアドレスを取得
	//wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));

	//// Sprite用のTransformationの設定
	//Microsoft::WRL::ComPtr <ID3D12Resource> transformationMatrixResourceSprite = CreateBufferResource(device.Get(), sizeof(TransformationMatrix));

	//// データを書き込む
	//TransformationMatrix* transformationMatrixDataSprite = nullptr;
	//transformationMatrixResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixDataSprite));

	//// 平行光源の設定
	//Microsoft::WRL::ComPtr <ID3D12Resource> directionalLightResource = CreateBufferResource(device.Get(), sizeof(DirectionalLight));
	//// データを書き込む
	//DirectionalLight* directionalLightData = nullptr;
	//// 書き込むためのアドレスを取得
	//directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	//// 光の色を設定
	//directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	//// 光の方向を設定
	//directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	//directionalLightData->intensity = 1.0f;

	//// モデル読み込み
	////ModelData modelData = LoadObjFile("resources", "plane.obj");
	//// 頂点リソ－スを作る
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource = CreateBufferResource(device.Get(), sizeof(VertexData) * modelData.vertices.size());
	//// 頂点バッファビューの作成
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	//vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();// リソースの先頭のアドレスから使う
	//vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // 頂点バッファのサイズ
	//vertexBufferView.StrideInBytes = sizeof(VertexData); // 頂点1つ分のサイズ

	//// 頂点リソースにデータを書き込む
	//VertexData* vertexData = nullptr;
	//// 書き込むためのアドレスを取得
	//vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	//// 頂点データのコピー
	//std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

	//// スプライト用の頂点バッファの生成
	//Microsoft::WRL::ComPtr <ID3D12Resource> vertexResourceSprite = CreateBufferResource(device.Get(), sizeof(VertexData) * 6);

	//// 頂点バッファビューの作成
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferViewSprite{};
	//vertexBufferViewSprite.BufferLocation = vertexResourceSprite->GetGPUVirtualAddress();
	//vertexBufferViewSprite.SizeInBytes = sizeof(VertexData) * 6;
	//vertexBufferViewSprite.StrideInBytes = sizeof(VertexData);

	//// 頂点データの設定
	//VertexData* vertexDataSprite = nullptr;
	//vertexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataSprite));

	//// 左下
	//vertexDataSprite[0].position = { 0.0f, 360.0f, 0.0f, 1.0f };
	//vertexDataSprite[0].texcoord = { 0.0f, 1.0f };
	//vertexDataSprite[0].normal = { 0.0f, 0.0f, 1.0f };

	//// 左上
	//vertexDataSprite[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	//vertexDataSprite[1].texcoord = { 0.0f, 0.0f };
	//vertexDataSprite[1].normal = { 0.0f, 0.0f, 1.0f };

	//// 右下
	//vertexDataSprite[2].position = { 640.0f, 360.0f, 0.0f, 1.0f };
	//vertexDataSprite[2].texcoord = { 1.0f, 1.0f };
	//vertexDataSprite[2].normal = { 0.0f, 0.0f, 1.0f };

	//// 右上
	//vertexDataSprite[3].position = { 640.0f, 0.0f, 0.0f, 1.0f };
	//vertexDataSprite[3].texcoord = { 1.0f, 0.0f };
	//vertexDataSprite[3].normal = { 0.0f, 0.0f, 1.0f };

	//// スプライトのインデックス
	//Microsoft::WRL::ComPtr<ID3D12Resource> indexResourceSprite = CreateBufferResource(device.Get(), sizeof(uint32_t) * 6);
	//// インデックスバッファビューの作成
	//D3D12_INDEX_BUFFER_VIEW indexBufferViewSprite{};
	//// リソースの先頭のアドレスから使う
	//indexBufferViewSprite.BufferLocation = indexResourceSprite->GetGPUVirtualAddress();
	//// 使用するリソースのサイズはインデックス６つ分のサイズ
	//indexBufferViewSprite.SizeInBytes = sizeof(uint32_t) * 6;
	//// インデックスはuint32_t型
	//indexBufferViewSprite.Format = DXGI_FORMAT_R32_UINT;
	//// インデックスリソースのデータを書き込む
	//uint32_t* indexDataSprite = nullptr;
	//indexResourceSprite->Map(0, nullptr, reinterpret_cast<void**>(&indexDataSprite));
	//indexDataSprite[0] = 0;
	//indexDataSprite[1] = 1;
	//indexDataSprite[2] = 2;
	//indexDataSprite[3] = 1;
	//indexDataSprite[4] = 3;
	//indexDataSprite[5] = 2;

	//// ビューポート
	//D3D12_VIEWPORT viewport{};
	//viewport.Width = kClientWidth;
	//viewport.Height = kClientHeight;
	//viewport.TopLeftX = 0;
	//viewport.TopLeftY = 0;
	//viewport.MinDepth = 0.0f;
	//viewport.MaxDepth = 1.0f;

	//// シザー矩形
	//D3D12_RECT scissorRect{};
	//scissorRect.left = 0;
	//scissorRect.right = kClientWidth;
	//scissorRect.top = 0;
	//scissorRect.bottom = kClientHeight;

	//// フェンスの設定
	//Microsoft::WRL::ComPtr<ID3D12Fence> fence = nullptr;
	//HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	//uint64_t fenceValue = 0;
	//hr = device->CreateFence(fenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	//assert(SUCCEEDED(hr));

	// 音声の初期化
	Sound sound;
	Sound::SoundData soundData1 = sound.LoadWave("Resources/Alarm01.wav");

	// 音声の再生
	sound.PlayWave(/*xAudio2.Get(),*/soundData1);

	// ポインタ
	Input* input = nullptr;
	// 入力の初期化
	input = new Input();
	input->Initialize(winApp);

	DebugCamera debugCamera;

	MSG msg = {};

	// Transformの設定
	Transform transform{ {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };

	// TransformSpriteの設定
	Transform transformSprite{ {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };

	// cameraTransformの設定
	Transform cameraTransform{ {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, -10.0f} };

	// 
	Transform uvTransformSprite{ {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };

	// 切り替え用のフラグ
	bool useMonsterBall = false;

	//// Imguiの初期化
	//IMGUI_CHECKVERSION();
	//ImGui::CreateContext();
	//ImGui::StyleColorsDark();
	//ImGui_ImplWin32_Init(winApp->GetHwnd());
	//ImGui_ImplDX12_Init(
	//	device.Get(),
	//	swapChainDesc.BufferCount,
	//	rtvDesc.Format,
	//	srvDescriptorHeap.Get(),
	//	srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
	//	srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	//);

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
		//debugCamera.Update(key);

		//ゲーム処理

		//// Imguiのフレーム開始
		//ImGui_ImplDX12_NewFrame();
		//ImGui_ImplWin32_NewFrame();
		//ImGui::NewFrame();
		////ImGui::ShowDemoWindow();
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

		//// 
		//ImGui::DragFloat2("UVTranslate", &uvTransformSprite.translate.x, 0.01f, -10.0f, 10.0f);
		//ImGui::DragFloat2("UVScale", &uvTransformSprite.scale.x, 0.01f, -10.0f, 10.0f);
		//ImGui::SliderAngle("UVRotate", &uvTransformSprite.rotate.z);
		//ImGui::Render();

		//// Transformの更新
		////transform.rotate.y += 0.03f;
		//Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
		//Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
		//Matrix4x4 viewMatrix = Inverse(cameraMatrix);
		//Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kClientWidth) / float(kClientHeight), 0.1f, 100.0f);
		//Matrix4x4 worldViewProjectionMatrix = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
		//transformationMatrixData->WVP = worldViewProjectionMatrix;
		//transformationMatrixData->World = worldMatrix;

		//// TransformSpriteの更新
		//Matrix4x4 worldMatrixSprite = MakeAffineMatrix(transformSprite.scale, transformSprite.rotate, transformSprite.translate);
		//Matrix4x4 viewMatrixSprite = MakeIdentity4x4();
		//Matrix4x4 projectionMatrixSprite = MakeOrthographicMatrix(0.0f, 0.0f, float(kClientWidth), float(kClientHeight), 0.0f, 100.0f);
		//Matrix4x4 worldViewProjectionMatrixSprite = Multiply(worldMatrixSprite, Multiply(viewMatrixSprite, projectionMatrixSprite));
		//transformationMatrixDataSprite->WVP = worldViewProjectionMatrixSprite;
		//transformationMatrixDataSprite->World = worldMatrixSprite;

		//// 
		//Matrix4x4 uvTransformMatrix = MakeScaleMatrix(uvTransformSprite.scale);
		//uvTransformMatrix = Multiply(uvTransformMatrix, MakeRotateZMatrix(uvTransformSprite.rotate.z));
		//uvTransformMatrix = Multiply(uvTransformMatrix, MakeTranslateMatrix(uvTransformSprite.translate));
		//materialDataSprite->uvTransform = uvTransformMatrix;

		//// バックバッファのインデックスを取得
		//UINT backBufferIndex = swapChain->GetCurrentBackBufferIndex();

		//// TransitionBarrierの設定
		//D3D12_RESOURCE_BARRIER barrier = {};
		//barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		//barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		////バリアを張るリソース
		//barrier.Transition.pResource = swapChainResources[backBufferIndex].Get();
		//// 遷移前                                        
		//barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		//// 遷移後
		//barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		//// バリア
		//commandList->ResourceBarrier(1, &barrier);

		//// 描画先のRTVとDSVの設定
		//D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

		//// 描画先のRTVを設定
		//commandList->OMSetRenderTargets(1, &rtvHandle[backBufferIndex], false, &dsvHandle);

		//// 指定した色で画面全体をクリア
		//float clearColor[] = { 0.1f,0.25f,0.5f,1.0f };
		//commandList->ClearRenderTargetView(rtvHandle[backBufferIndex], clearColor, 0, nullptr);

		//// 指定した深度で画面全体をクリア
		//commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

		//// 描画先のRTVを設定
		//commandList->RSSetViewports(1, &viewport);
		//commandList->RSSetScissorRects(1, &scissorRect);
		//commandList->SetGraphicsRootSignature(rootSignature.Get());
		//commandList->SetPipelineState(graphicsPipelineState.Get());
		//commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
		//commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		//commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
		//commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
		//commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

		//// 描画用のDescriptorHeapを設定
		//ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap.Get() };
		//commandList->SetDescriptorHeaps(1, descriptorHeaps);

		//// SRVのDescriptortableを設定
		//commandList->SetGraphicsRootDescriptorTable(2, useMonsterBall ? textureSrvHandleGPU2 : textureSrvHandleGPU);

		//// 描画
		////commandList->DrawInstanced(totalSphereVertices, 1, 0, 0);
		////// インデックスの描画
		////commandList->IASetIndexBuffer(&indexBufferViewSphere);
		////commandList->DrawIndexedInstanced(totalSphereVertices, 1, 0, 0, 0);
		//commandList->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);

		//commandList->SetGraphicsRootDescriptorTable(2, textureSrvHandleGPU);


		//// スプライトの描画
		//commandList->IASetVertexBuffers(0, 1, &vertexBufferViewSprite);
		//commandList->SetGraphicsRootConstantBufferView(0, materialResourceSprite->GetGPUVirtualAddress());
		//commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResourceSprite->GetGPUVirtualAddress());
		////commandList->DrawInstanced(6, 1, 0, 0);

		//// スプライトのインデックスの描画
		//commandList->IASetIndexBuffer(&indexBufferViewSprite);
		////commandList->DrawIndexedInstanced(6, 1, 0, 0, 0);

		//// ImGuiの描画
		//ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

		//// 状態を遷移
		//barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		//barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		//// バリア
		//commandList->ResourceBarrier(1, &barrier);
		//hr = commandList->Close();
		//assert(SUCCEEDED(hr));

		//// コマンドリストを実行
		//ID3D12CommandList* commandLists[] = { commandList.Get() };
		//commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		//// GPUとOSに画面の変換を行うように指示
		//swapChain->Present(1, 0);

		//// フェンスの値の更新
		//fenceValue++;
		//commandQueue->Signal(fence.Get(), fenceValue);

		//if (fence->GetCompletedValue() < fenceValue) {

		//	fence->SetEventOnCompletion(fenceValue, fenceEvent);

		//	// イベントが発生するまで待機
		//	WaitForSingleObject(fenceEvent, INFINITE);
		//}
		//// 次フレームのコマンドリストを準備
		//hr = commandAllocator->Reset();
		//assert(SUCCEEDED(hr));
		//hr = commandList->Reset(commandAllocator.Get(), nullptr);
		//assert(SUCCEEDED(hr));

	}

	winApp->Finalize();
	//CloseHandle(fenceEvent);
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	sound.Unload(&soundData1);
	delete input;
	delete winApp;
	delete graphicsDevice;

	return 0;
}