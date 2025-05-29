#pragma once

#include "stdafx.h"

#include "base/SwapChain.h"
#include "base/Texture.h"
#include "base/Device.h"
#include "base/Imgui.h"

using namespace CAULDRON_DX12;

namespace Racoon {

static const uint8_t BACKBUFFER_COUNT = 2;

class Renderer
{
public:
    void OnCreate(Device* pDevice, SwapChain* pSwapChain);
    void OnRender(SwapChain* pSwapChain);
    void OnDestroy();

private:
    void Clear(SwapChain* pSwapChain, ID3D12GraphicsCommandList2* CmdList);

    D3D12_VIEWPORT m_Viewport;
    Device* m_pDevice;
    SwapChain* m_pSwapChain;
    CommandListRing m_CommandListRing;

    ResourceViewHeaps m_ResourceViewHeaps;
    UploadHeap m_UploadHeap;
    DynamicBufferRing m_ConstantsBufferRing;

    ImGUI m_ImGUIHelper;

    uint32_t m_RtvDescriptorSize,
        m_CbvDescriptorSize,
        m_SamplerDescriptorSize,
        m_DsvDescriptorSize;

};

}