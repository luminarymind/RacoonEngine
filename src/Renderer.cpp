#include "Renderer.h"

namespace Racoon {

void Renderer::OnCreate(Device* pDevice, SwapChain* pSwapChain)
{
    m_pDevice = pDevice;
    m_pSwapChain = pSwapChain;

    m_CommandListRing.OnCreate(pDevice, BACKBUFFER_COUNT, 4, pDevice->GetGraphicsQueue()->GetDesc());

    m_RtvDescriptorSize = m_pDevice->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_CbvDescriptorSize = m_pDevice->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_SamplerDescriptorSize = m_pDevice->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    m_DsvDescriptorSize = m_pDevice->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_ResourceViewHeaps.OnCreate(pDevice, m_CbvDescriptorSize, m_CbvDescriptorSize, m_CbvDescriptorSize,
        m_DsvDescriptorSize, m_RtvDescriptorSize, m_SamplerDescriptorSize);
    const uint32_t uploadHeapMemSize = 1000 * 1024 * 1024;
    m_UploadHeap.OnCreate(pDevice, uploadHeapMemSize);
    const uint32_t constantsBufferMemSize = 200 * 1024 * 1024;
    m_ConstantsBufferRing.OnCreate(pDevice, BACKBUFFER_COUNT, constantsBufferMemSize, &m_ResourceViewHeaps);

    m_ImGUIHelper.OnCreate(pDevice, &m_UploadHeap, &m_ResourceViewHeaps, &m_ConstantsBufferRing, pSwapChain->GetFormat());
}

}
void Racoon::Renderer::OnRender(SwapChain* pSwapChain)
{
    m_CommandListRing.OnBeginFrame();
    m_ConstantsBufferRing.OnBeginFrame();
    auto CmdList = m_CommandListRing.GetNewCommandList();
    Clear(pSwapChain, CmdList);

    CmdList->OMSetRenderTargets(1, pSwapChain->GetCurrentBackBufferRTV(), true, nullptr);
    m_ImGUIHelper.Draw(CmdList);

    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetCurrentBackBufferResource(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    CmdList->ResourceBarrier(1, &barrier);

    CmdList->Close();

    ID3D12CommandList* CmdListLists[] = { CmdList };
    m_pDevice->GetGraphicsQueue()->ExecuteCommandLists(1, CmdListLists);
    pSwapChain->WaitForSwapChain();
}

void Racoon::Renderer::OnDestroy()
{
    m_ImGUIHelper.OnDestroy();
    m_ConstantsBufferRing.OnDestroy();
    m_UploadHeap.OnDestroy();
    m_ResourceViewHeaps.OnDestroy();
    m_CommandListRing.OnDestroy();
}

void Racoon::Renderer::Clear(SwapChain* pSwapChain, ID3D12GraphicsCommandList2* CmdList)
{
    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetCurrentBackBufferResource(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    CmdList->ResourceBarrier(1, &barrier);
    float clearColor[]{ 0.1f, 0.4f, 9.0f, 1.0f };
    CmdList->ClearRenderTargetView(*pSwapChain->GetCurrentBackBufferRTV(), clearColor, 0, nullptr);
    SetViewportAndScissor(CmdList, 0, 0, 1920, 1080);
}
