#include "stdafx.h"

#include "RacoonEngine.h"

#include "base/ShaderCompilerHelper.h"
#include "base/ImGuiHelper.h"

Racoon::RacoonEngine::RacoonEngine(LPCSTR name) :
    CAULDRON_DX12::FrameworkWindows(name)
{
}

void Racoon::RacoonEngine::OnParseCommandLine(LPSTR lpCmdLine, uint32_t* pWidth, uint32_t* pHeight)
{
}

void Racoon::RacoonEngine::OnCreate()
{
    InitDirectXCompiler();
    CAULDRON_DX12::CreateShaderCache();
    ImGUI_Init((void*)m_windowHwnd);

    OnResize(true);
    OnUpdateDisplay();

    m_Renderer.reset(new Renderer());
    m_Renderer->OnCreate(&m_device, &m_swapChain);
}

void Racoon::RacoonEngine::OnDestroy()
{
    ImGUI_Shutdown();

    CAULDRON_DX12::DestroyShaderCache(&m_device);

    m_Renderer->OnDestroy();
}

void Racoon::RacoonEngine::OnRender()
{
    BeginFrame();
    RECT rect;
    GetClientRect(m_windowHwnd, &rect);
    ImGUI_UpdateIO((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));
    ImGui::NewFrame();
    BuildUI();
    OnUpdate();
    m_Renderer->OnRender(&m_swapChain);
    EndFrame();
}

bool Racoon::RacoonEngine::OnEvent(MSG msg)
{
    if (ImGUI_WndProcHandler(m_windowHwnd, msg.message, msg.wParam, msg.lParam))
        return true;
    const WPARAM& KeyPressed = msg.wParam;
    switch (msg.message)
    {
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (KeyPressed == VK_F1)
            m_UIState.m_bShowUI = !m_UIState.m_bShowUI;
        break;
    }
    return true;
}

void Racoon::RacoonEngine::OnResize(bool resizeRender)
{
}

void Racoon::RacoonEngine::OnUpdateDisplay()
{
}

void Racoon::RacoonEngine::OnUpdate()
{
    m_Timer.Tick();
}

int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    LPCSTR name = "Racoon Engine 0.0.1";
    return RunFramework(hInstance, lpCmdLine, nCmdShow, new Racoon::RacoonEngine(name));
}
