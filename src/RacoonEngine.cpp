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

    m_Camera.SetFov(AMD_PI_OVER_4, 1920, 1080, 0.1f, 1000.f);
    m_Camera.LookAt({ 0, 0, 5, 0 }, { 0, 0, 0, 0 });
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
    m_Renderer->OnRender(&m_swapChain, m_Camera);
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

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse)
    {
        io.MouseDelta.x = 0;
        io.MouseDelta.y = 0;
        io.MouseWheel = 0;
    }
    UpdateCamera(m_Camera, io);

}

void Racoon::RacoonEngine::UpdateCamera(Camera& cam, const ImGuiIO& io)
{
    float yaw = cam.GetYaw();
    float pitch = cam.GetPitch();
    float distance = cam.GetDistance();

    if (io.MouseDown[0])
    {
        // read mouse movement
        yaw -= io.MouseDelta.x / 100.f;
        pitch += io.MouseDelta.y / 100.f;
    }

    distance -= io.MouseWheel;
    distance = std::max<float>(distance, 0.1f);

    cam.UpdateCameraPolar(yaw, pitch, 0, 0, distance);
}

int WINAPI WinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow)
{
    LPCSTR name = "Racoon Engine 0.0.1";
    return RunFramework(hInstance, lpCmdLine, nCmdShow, new Racoon::RacoonEngine(name));
}
