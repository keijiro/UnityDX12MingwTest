#include <d3d11.h>
#include <cstdio>
#include <cstddef>
#include <memory>
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D11.h"

namespace {

IUnityInterfaces* _unity;
IUnityGraphics* _graphics;
ID3D11Device* _device;
ID3D11Texture2D* _texture;

//
// Interop structure
//

struct TextureDescription { int width, height; };

//
// Test image construction
//

std::byte* CreateTestImage(const TextureDescription* desc)
{
    auto w = desc->width;
    auto h = desc->height;

    auto ptr = new std::byte[w * h * 4];
    auto offs = 0;

    for (auto y = 0; y < h; y++)
    {
        for (auto x = 0; x < w; x++)
        {
            ptr[offs++] = static_cast<std::byte>(x);
            ptr[offs++] = static_cast<std::byte>(y);
            ptr[offs++] = static_cast<std::byte>(1);
            ptr[offs++] = static_cast<std::byte>(0);
        }
    }

    return ptr;
}

//
// Callback functions
//

void UNITY_INTERFACE_API
  OnGraphicsDeviceEvent(UnityGfxDeviceEventType event_type)
{
    if (_graphics->GetRenderer() != kUnityGfxRendererD3D11) return;

    if (event_type == kUnityGfxDeviceEventInitialize)
    {
        _device = _unity->Get<IUnityGraphicsD3D11>()->GetDevice();
    }
    else if (event_type == kUnityGfxDeviceEventShutdown)
    {
        _device = nullptr;
    }
}

void UNITY_INTERFACE_API OnRenderEvent(int event_id, void* data)
{
    if (event_id == 0)
    {
        // Test image construction
        auto tex_desc = reinterpret_cast<TextureDescription*>(data);
        auto image = std::unique_ptr<std::byte>(CreateTestImage(tex_desc));

        // Subresource description
        D3D11_SUBRESOURCE_DATA d3d_data = {};
        d3d_data.pSysMem = image.get();
        d3d_data.SysMemPitch = tex_desc->width * 4;
        d3d_data.SysMemSlicePitch = tex_desc->height * tex_desc->width * 4;

        // Texture2D description
        D3D11_TEXTURE2D_DESC d3d_tex = {};
        d3d_tex.Width = tex_desc->width;
        d3d_tex.Height = tex_desc->height;
        d3d_tex.MipLevels = 1;
        d3d_tex.ArraySize = 1;
        d3d_tex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        d3d_tex.SampleDesc.Count = 1;
        d3d_tex.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        // Texture instantiation
        _device->CreateTexture2D(&d3d_tex, &d3d_data, &_texture);
    }
    else if (event_id == 1)
    {
        _texture->Release();
        _texture = nullptr;
    }
}

} // anonymous namespace

//
// Unity low-level native plugin interface
//

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
  UnityPluginLoad(IUnityInterfaces* interfaces)
{
    //FILE * pConsole;
    //AllocConsole();
    //freopen_s(&pConsole, "CONOUT$", "wb", stdout);

    _unity = interfaces;
    _graphics = _unity->Get<IUnityGraphics>();

    _graphics->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    _graphics->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);

    _unity = nullptr;
    _graphics = nullptr;
}

//
// Plugin functions
//

extern "C" UnityRenderingEventAndData UNITY_INTERFACE_EXPORT GetRenderEventCallback()
{
    return OnRenderEvent;
}

extern "C" void UNITY_INTERFACE_EXPORT * GetPointer()
{
    return _texture;
}
