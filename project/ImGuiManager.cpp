#include "ImGuiManager.h"
#include "SrvManager.h"

ImGuiManager* ImGuiManager::GetInstance()
{
	static ImGuiManager instance;
	return &instance;
}

void ImGuiManager::Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice)
{
	graphicsDevice_ = graphicsDevice;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGui_ImplWin32_Init(winApp->GetHwnd());

	// SrvManagerを使ってImGui用のSRVを1つ確保する
	SrvManager* srvManager = SrvManager::GetInstance();

	// 1. SRVヒープ内の場所(インデックス)を確保
	uint32_t useIndex = srvManager->Allocate();

	// 2. その場所のCPU/GPUハンドルなどを取得してImGuiに渡す
	ImGui_ImplDX12_Init(
		graphicsDevice_->GetDevice().Get(),
		2, 
		DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,                 // RTVのフォーマット
		srvManager->GetDescriptorHeap(),                 // ヒープのポインタ
		srvManager->GetCPUDescriptorHandle(useIndex),    // フォント用SRVのCPUハンドル
		srvManager->GetGPUDescriptorHandle(useIndex)     // フォント用SRVのGPUハンドル
	);
}

void ImGuiManager::Finalize()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void ImGuiManager::Begin()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void ImGuiManager::End()
{
	ImGui::Render();
}

void ImGuiManager::Draw()
{
	ID3D12GraphicsCommandList* commandList = graphicsDevice_->GetCommandList().Get();

	// ★SRVヒープの設定（重要）
	// ImGuiを描画する前に、SrvManagerが管理しているヒープをセットする必要があります
	ID3D12DescriptorHeap* descriptorHeaps[] = { SrvManager::GetInstance()->GetDescriptorHeap() };
	commandList->SetDescriptorHeaps(1, descriptorHeaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);
}