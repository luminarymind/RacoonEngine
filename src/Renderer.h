#pragma once

#include "stdafx.h"

#include "base/SwapChain.h"
#include "base/Texture.h"
#include "base/Device.h"
#include "base/Imgui.h"

#include "base/Buffer.h"
#include "base/StaticConstantBufferPool.h"
#include "base/StaticBufferPool.h"

using namespace CAULDRON_DX12;

namespace Racoon {

static const uint8_t BACKBUFFER_COUNT = 2;

class Renderer
{
public:
    struct Vertex
    {
        XMFLOAT3 Position;
        XMFLOAT4 Color;
    };

    struct PerFrame
    {
        XMFLOAT4X4 mvp;
    };

    void OnCreate(Device* pDevice, SwapChain* pSwapChain);
    void OnRender(SwapChain* pSwapChain);
    void OnDestroy();

private:
    void Clear(SwapChain* pSwapChain, ID3D12GraphicsCommandList2* CmdList);
    void InitGeometryBuffers();
    void CreateRootSignature();
    void CreateGraphicsPipelineState();

    XMFLOAT4X4 GetPerFrameMatrix();

    uint32_t CheckForMSAAQualitySupport();

    D3D12_VIEWPORT m_Viewport;
    Device* m_pDevice;
    SwapChain* m_pSwapChain;
    CommandListRing m_CommandListRing;
    DXGI_FORMAT m_BackbufferFormat;

    ResourceViewHeaps m_ResourceViewHeaps;
    UploadHeap m_UploadHeap;
    DynamicBufferRing m_ConstantsBufferRing;
    StaticBufferPool m_StaticBufferPool;

    DSV m_DepthDSV;
    Texture m_Depth;

    D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
    D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
    D3D12_GPU_VIRTUAL_ADDRESS m_ConstantBuffer;

    ID3D12PipelineState* m_PipelineState{ nullptr };
    ID3D12RootSignature* m_RootSignature{ nullptr };

    ImGUI m_ImGUIHelper;

    uint32_t m_RtvDescriptorSize,
        m_CbvDescriptorSize,
        m_SamplerDescriptorSize,
        m_DsvDescriptorSize;

    uint32_t m_4xMsaaQuality{ 0 };
};

}