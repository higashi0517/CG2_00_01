#include "ImGuiManager.h"
#include "SrvManager.h"

ImGuiManager* ImGuiManager::GetInstance()
{
	static ImGuiManager instance;
	return &instance;
}

void ImGuiManager::Initialize([[maybe_unused]] WinApp* winApp, [[maybe_unused]] GraphicsDevice* graphicsDevice)
{
#ifdef USE_IMGUI
	graphicsDevice_ = graphicsDevice;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(winApp->GetHwnd());

	ImGui_ImplDX12_InitInfo initInfo = {};

	initInfo.Device = graphicsDevice_->GetDevice().Get();

	// ※注意1：GraphicsDeviceクラスにGetCommandQueue()がない場合は追加してください
	initInfo.CommandQueue = graphicsDevice_->GetCommandQueue().Get();

	// ※注意2：スワップチェーンの数。スライドに合わせてgetterを使うか、直接指定します
	initInfo.NumFramesInFlight = 2;

	initInfo.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;

	// ※注意3：DSVのフォーマット（深度バッファ）。プロジェクトの設定に合わせてください（例：DXGI_FORMAT_D24_UNORM_S8_UINTなど）
	initInfo.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

	initInfo.SrvDescriptorHeap = SrvManager::GetInstance()->GetDescriptorHeap();

	// SRV確保用関数の設定（ラムダ式）
	initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle) {
		SrvManager* srvManager = SrvManager::GetInstance();
		uint32_t index = srvManager->Allocate();
		*out_cpu_handle = srvManager->GetCPUDescriptorHandle(index);
		*out_gpu_handle = srvManager->GetGPUDescriptorHandle(index);
		};

	// SRV解放用関数の設定（ラムダ式）
	initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) {
		// 現状は何もしない
		};

	// DirectX12用の初期化を行う
	ImGui_ImplDX12_Init(&initInfo);

#endif // USE_IMGUI
}


void ImGuiManager::Finalize()
{
#ifdef USE_IMGUI

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

#endif // USE_IMGUI
}

void ImGuiManager::Begin()
{
#ifdef USE_IMGUI

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif // USE_IMGUI
}

void ImGuiManager::End()
{
#ifdef USE_IMGUI

	ImGui::Render();

#endif // USE_IMGUI
}

void ImGuiManager::Draw()
{
#ifdef USE_IMGUI

	ID3D12GraphicsCommandList* commandList = graphicsDevice_->GetCommandList().Get();

	// ImGuiを描画する前に、SrvManagerが管理しているヒープをセットする必要があります
	ID3D12DescriptorHeap* descriptorHeaps[] = { SrvManager::GetInstance()->GetDescriptorHeap() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

#endif // USE_IMGUI
}