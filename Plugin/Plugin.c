#define COBJMACROS
#include <d3d11.h>
#include <stdio.h>
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D11.h"

IUnityInterfaces* _unity;
IUnityGraphics* _gfx;
ID3D11Device* _dev;
ID3D11Texture2D* _tex;

//
// Interop structure
//

typedef struct { int width, height; } TextureDescription;

//
// Test image construction
//

static void* CreateTestImage(const TextureDescription* desc)
{
    int w = desc->width;
    int h = desc->height;

    UCHAR* ptr = malloc(w * h * 4);

    int offs = 0;
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            ptr[offs++] = x;
            ptr[offs++] = y;
            ptr[offs++] = 1;
            ptr[offs++] = 0;
        }
    }

    return ptr;
}

//
// Callback functions
//

static void UNITY_INTERFACE_API
  OnGraphicsDeviceEvent(UnityGfxDeviceEventType event_type)
{
    if (_gfx->GetRenderer() != kUnityGfxRendererD3D11) return;

    printf("OnGraphicsDeviceEvent (%d)\n", event_type);

    if (event_type == kUnityGfxDeviceEventInitialize)
    {
        UnityInterfaceGUID guid = UNITY_GET_INTERFACE_GUID(IUnityGraphicsD3D11);
        IUnityGraphicsD3D11* ifc = _unity->GetInterface(guid);
        _dev = ifc->GetDevice();
    }
    else if (event_type == kUnityGfxDeviceEventShutdown)
    {
        _dev = NULL;
    }
}

static void UNITY_INTERFACE_API OnRenderEvent(int event_id, void* data)
{
    if (event_id == 0)
    {
        // Test image construction
        TextureDescription* tex_desc = data;
        void* image = CreateTestImage(tex_desc);

        // Subresource description
        D3D11_SUBRESOURCE_DATA d3d_data;
        ZeroMemory(&d3d_data, sizeof(D3D11_SUBRESOURCE_DATA));
        d3d_data.pSysMem = image;
        d3d_data.SysMemPitch = tex_desc->width * 4;
        d3d_data.SysMemSlicePitch = tex_desc->height * tex_desc->width * 4;

        // Texture2D description
        D3D11_TEXTURE2D_DESC d3d_tex;
        ZeroMemory(&d3d_tex, sizeof(D3D11_TEXTURE2D_DESC));
        d3d_tex.Width = tex_desc->width;
        d3d_tex.Height = tex_desc->height;
        d3d_tex.MipLevels = 1;
        d3d_tex.ArraySize = 1;
        d3d_tex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        d3d_tex.SampleDesc.Count = 1;
        d3d_tex.BindFlags = D3D11_BIND_SHADER_RESOURCE;

        // Texture instantiation
        ID3D11Device_CreateTexture2D(_dev, &d3d_tex, &d3d_data, &_tex);

        // Cleaning up
        free(image);
    }
    else if (event_id == 1)
    {
        ID3D11Texture2D_Release(_tex);
        _tex = NULL;
    }
}

//
// Unity low-level native plugin interface
//

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
  UnityPluginLoad(IUnityInterfaces* interfaces)
{
    //FILE * pConsole;
    //AllocConsole();
    //freopen_s(&pConsole, "CONOUT$", "wb", stdout);

    _unity = interfaces;
    _gfx = _unity->GetInterface(UNITY_GET_INTERFACE_GUID(IUnityGraphics));

    _gfx->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    _gfx->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);

    _unity = NULL;
    _gfx = NULL;
}

//
// Plugin functions
//

UnityRenderingEventAndData UNITY_INTERFACE_EXPORT GetRenderEventCallback()
{
    return OnRenderEvent;
}

void UNITY_INTERFACE_EXPORT * GetPointer()
{
    return _tex;
}
