#include "Renderer.h"

#include "base/ShaderCompilerHelper.h"

#include <DirectXColors.h>

namespace Racoon {

void Renderer::OnCreate(Device* pDevice, SwapChain* pSwapChain)
{
    m_pDevice = pDevice;
    m_pSwapChain = pSwapChain;
    m_BackbufferFormat = pSwapChain->GetFormat();

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

    m_ResourceViewHeaps.AllocDSVDescriptor(1, &m_DepthDSV);
    D3D12_RESOURCE_DESC depthStencilDepsc;
    depthStencilDepsc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    m_Depth.InitDepthStencil(m_pDevice, "DepthStencil", &CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT,
        1920, 1080, 1, 1, m_4xMsaaQuality ? 4 : 1,
        m_4xMsaaQuality ? (m_4xMsaaQuality - 1) : 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        D3D12_TEXTURE_LAYOUT_UNKNOWN, 0), 1.f);
    m_Depth.CreateDSV(0, &m_DepthDSV);

    const uint32_t staticMemorySize = 512 * 1024 * 1024;
    m_StaticBufferPool.OnCreate(pDevice, staticMemorySize, true, "StaticGeometry");

    //m_4xMsaaQuality = CheckForMSAAQualitySupport();

    InitGeometryBuffers();
    CreateRootSignature();
    CreateGraphicsPipelineState();

    m_UploadHeap.FlushAndFinish();
}

}
void Racoon::Renderer::OnRender(SwapChain* pSwapChain, const Camera& camera)
{
    m_CommandListRing.OnBeginFrame();
    m_ConstantsBufferRing.OnBeginFrame();
    auto CmdList = m_CommandListRing.GetNewCommandList();
    Clear(pSwapChain, CmdList);

    CmdList->OMSetRenderTargets(1, pSwapChain->GetCurrentBackBufferRTV(), true, &m_DepthDSV.GetCPU());

    PerFrame perFrameData;
    perFrameData.mvp = GetPerFrameMatrix(camera);
    m_ConstantBuffer = m_ConstantsBufferRing.AllocConstantBuffer(sizeof(PerFrame), &perFrameData);
    ID3D12DescriptorHeap* descHeap = m_ResourceViewHeaps.GetCBV_SRV_UAVHeap();
    CmdList->SetDescriptorHeaps(1, &descHeap);
    CmdList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
    CmdList->IASetIndexBuffer(&m_IndexBufferView);
    CmdList->SetGraphicsRootSignature(m_RootSignature);
    CmdList->SetGraphicsRootConstantBufferView(0, m_ConstantBuffer);
    CmdList->SetPipelineState(m_PipelineState);
    CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    CmdList->DrawIndexedInstanced(36, 1, 0, 0, 0);

    m_ImGUIHelper.Draw(CmdList);

    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetCurrentBackBufferResource(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    CmdList->ResourceBarrier(1, &barrier);

    ThrowIfFailed(CmdList->Close());

    ID3D12CommandList* CmdListLists[] = { CmdList };
    m_pDevice->GetGraphicsQueue()->ExecuteCommandLists(1, CmdListLists);
    pSwapChain->WaitForSwapChain();
}

void Racoon::Renderer::Clear(SwapChain* pSwapChain, ID3D12GraphicsCommandList2* CmdList)
{
    const auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(pSwapChain->GetCurrentBackBufferResource(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    CmdList->ResourceBarrier(1, &barrier);

    CmdList->ClearRenderTargetView(*pSwapChain->GetCurrentBackBufferRTV(), Colors::DarkGreen, 0, nullptr);
    CmdList->ClearDepthStencilView(m_DepthDSV.GetCPU(), D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);
    SetViewportAndScissor(CmdList, 0, 0, 1920, 1080);
}

void Racoon::Renderer::OnDestroy()
{
    m_RootSignature->Release();
    m_PipelineState->Release();
    m_ImGUIHelper.OnDestroy();
    m_StaticBufferPool.OnDestroy();
    m_ConstantsBufferRing.OnDestroy();
    m_UploadHeap.OnDestroy();
    m_ResourceViewHeaps.OnDestroy();
    m_CommandListRing.OnDestroy();
    m_Depth.OnDestroy();
}

void Racoon::Renderer::InitGeometryBuffers()
{
    Vertex vertices[] = {
        { XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) },
        { XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) },
        { XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) },
        { XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) },
        { XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) },
        { XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) },
        { XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) },
        { XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }
    };

    m_StaticBufferPool.AllocVertexBuffer(sizeof(vertices) / sizeof(vertices[0]), sizeof(Vertex), vertices, &m_VertexBufferView);

    uint16_t indices[] = {
        // front face
        0, 1, 2,
        0, 2, 3,
        // back face
        4, 6, 5,
        4, 7, 6,
        // left face
        4, 5, 1,
        4, 1, 0,
        // right face
        3, 2, 6,
        3, 6, 7,
        // top face
        1, 5, 6,
        1, 6, 2,
        // bottom face
        4, 0, 3,
        4, 3, 7
    };

    m_StaticBufferPool.AllocIndexBuffer(sizeof(indices) / sizeof(indices[0]), sizeof(uint16_t), indices, &m_IndexBufferView);

    m_StaticBufferPool.UploadData(m_UploadHeap.GetCommandList());
}

void Racoon::Renderer::CreateRootSignature()
{
    CD3DX12_ROOT_PARAMETER rootParam[1];
    rootParam[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignDesc(1, rootParam, 0, nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ID3DBlob* pSerializedRootSignBlob, * pErrorBlob = nullptr;
    D3D12SerializeRootSignature(&rootSignDesc, D3D_ROOT_SIGNATURE_VERSION_1,
        &pSerializedRootSignBlob, &pErrorBlob);

    m_pDevice->GetDevice()->CreateRootSignature(0, pSerializedRootSignBlob->GetBufferPointer(),
        pSerializedRootSignBlob->GetBufferSize(), IID_PPV_ARGS(&m_RootSignature));
    pSerializedRootSignBlob->Release();
    if (pErrorBlob)
        pErrorBlob->Release();
    SetName(m_RootSignature, "Renderer::m_RootSignature");
}

void Racoon::Renderer::CreateGraphicsPipelineState()
{
    std::vector<D3D12_INPUT_ELEMENT_DESC> layout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    D3D12_SHADER_BYTECODE shaderVert, shaderPixel;
    CompileShaderFromFile("simple_vertex.hlsl", nullptr, "VS", "-T vs_6_0", &shaderVert);
    CompileShaderFromFile("simple_pixel.hlsl", nullptr, "PS", "-T ps_6_0", &shaderPixel);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC descPso = {};
    descPso.InputLayout = { layout.data(), (UINT)layout.size() };
    descPso.pRootSignature = m_RootSignature;
    descPso.VS = shaderVert;
    descPso.PS = shaderPixel;
    descPso.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    descPso.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
    descPso.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    descPso.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    descPso.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    descPso.SampleMask = UINT_MAX;
    descPso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    descPso.NumRenderTargets = 1;
    descPso.RTVFormats[0] = m_BackbufferFormat;
    descPso.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    descPso.SampleDesc.Count = m_4xMsaaQuality ? 4 : 1;
    descPso.SampleDesc.Quality = m_4xMsaaQuality ? (m_4xMsaaQuality - 1) : 0;
    descPso.NodeMask = 0;

    m_pDevice->GetDevice()->CreateGraphicsPipelineState(&descPso, IID_PPV_ARGS(&m_PipelineState));
}

math::Matrix4 Racoon::Renderer::GetPerFrameMatrix(const Camera& Cam)
{
    math::Matrix4 model(math::Matrix4::identity());
    auto camViewProj = Cam.GetProjection() * Cam.GetView();
    auto mvp = camViewProj * model;
    return math::transpose(mvp);
}

uint32_t Racoon::Renderer::CheckForMSAAQualitySupport()
{
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.Format = m_pSwapChain->GetFormat();
    msQualityLevels.SampleCount = 4;
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(m_pDevice->GetDevice()->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &msQualityLevels,
        sizeof(msQualityLevels)));

    assert(msQualityLevels.NumQualityLevels > 0 &&
        "Unexpected MSAA quality level support, should be more than 0.");
    return msQualityLevels.NumQualityLevels;
}
