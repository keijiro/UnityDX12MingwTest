#include <d3d12.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstddef>
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D12.h"

using Microsoft::WRL::ComPtr;

namespace {

// Unity native plugin interface
IUnityInterfaces* _unity;

// Command list
ComPtr<ID3D12CommandAllocator> _cmd_alloc;
ComPtr<ID3D12GraphicsCommandList> _cmd_list;

// Texture resource and upload buffer
ComPtr<ID3D12Resource> _texture;
ComPtr<ID3D12Resource> _upload;

// C++/C# Interop structure
struct TextureDescription { int width, height; };

//
// Texture resource management
//

void InitializeTexture(const TextureDescription& params)
{
    auto dev = _unity->Get<IUnityGraphicsD3D12v6>()->GetDevice();

    // Texture resource allocation
    D3D12_HEAP_PROPERTIES heap_texture = { .Type = D3D12_HEAP_TYPE_DEFAULT };

    D3D12_RESOURCE_DESC desc_tex = {};
    desc_tex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc_tex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc_tex.Width = params.width;
    desc_tex.Height = params.height;
    desc_tex.DepthOrArraySize = 1;
    desc_tex.MipLevels = 1;
    desc_tex.SampleDesc.Count = 1;

    dev->CreateCommittedResource
      (&heap_texture, D3D12_HEAP_FLAG_NONE, &desc_tex,
       D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&_texture));

    // Intermediate upload buffer allocation
    D3D12_HEAP_PROPERTIES heap_upload = { .Type = D3D12_HEAP_TYPE_UPLOAD };

    D3D12_RESOURCE_DESC desc_upload = {};
    desc_upload.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc_upload.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc_upload.Width = params.width * params.height * 4;
    desc_upload.Height = 1;
    desc_upload.DepthOrArraySize = 1;
    desc_upload.MipLevels = 1;
    desc_upload.SampleDesc.Count = 1;

    dev->CreateCommittedResource
      (&heap_upload, D3D12_HEAP_FLAG_NONE, &desc_upload,
       D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_upload));

    // Construct the test image on the upload buffer.
    void* mapped = nullptr;
    _upload->Map(0, nullptr, &mapped);

    auto ptr = reinterpret_cast<std::byte*>(mapped);
    auto offs = 0;

    for (auto y = 0; y < params.height; y++)
    {
        for (auto x = 0; x < params.width; x++)
        {
            ptr[offs++] = static_cast<std::byte>(x);
            ptr[offs++] = static_cast<std::byte>(y);
            ptr[offs++] = static_cast<std::byte>(1);
            ptr[offs++] = static_cast<std::byte>(0);
        }
    }

    _upload->Unmap(0, nullptr);

    // Texture data transfer settings
    D3D12_RESOURCE_DESC desc_resrc = _texture->GetDesc();

    D3D12_TEXTURE_COPY_LOCATION loc_src = {};
    loc_src.pResource = _upload.Get();
    loc_src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    dev->GetCopyableFootprints
      (&desc_resrc, 0, 1, 0,
       &loc_src.PlacedFootprint, nullptr, nullptr, nullptr);

    D3D12_TEXTURE_COPY_LOCATION loc_dst = {};
    loc_dst.pResource = _texture.Get();
    loc_dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    loc_dst.SubresourceIndex = 0;

    UnityGraphicsD3D12ResourceState resrc_state = {};
    resrc_state.resource = _texture.Get();
    resrc_state.expected = D3D12_RESOURCE_STATE_COPY_DEST;
    resrc_state.current = D3D12_RESOURCE_STATE_COPY_DEST;

    // Command list setup and execution
    _cmd_alloc->Reset();
    _cmd_list->Reset(_cmd_alloc.Get(), nullptr);

    _cmd_list->CopyTextureRegion(&loc_dst, 0, 0, 0, &loc_src, nullptr);
    _cmd_list->Close();

    _unity->Get<IUnityGraphicsD3D12v6>()
      ->ExecuteCommandList(_cmd_list.Get(), 1, &resrc_state);
}

void ReleaseTexture()
{
    _upload = nullptr;
    _texture = nullptr;
}

//
// Callback functions
//

// Graphics device events from the Unity graphics system
void UNITY_INTERFACE_API
  OnGraphicsDeviceEvent(UnityGfxDeviceEventType event_type)
{
    auto renderer_type = _unity->Get<IUnityGraphics>()->GetRenderer();
    if (renderer_type != kUnityGfxRendererD3D12) return;

    if (event_type == kUnityGfxDeviceEventInitialize)
    {
        // Device initialization event: Command list allocation
        auto dev = _unity->Get<IUnityGraphicsD3D12v6>()->GetDevice();

        dev->CreateCommandAllocator
          (D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmd_alloc));

        dev->CreateCommandList
          (0, D3D12_COMMAND_LIST_TYPE_DIRECT,
           _cmd_alloc.Get(), nullptr, IID_PPV_ARGS(&_cmd_list));
    }
    else if (event_type == kUnityGfxDeviceEventShutdown)
    {
        // Device shutdown event: Command list deallocation
        _cmd_list = nullptr;
        _cmd_alloc = nullptr;
    }
}

// Render events from Unity user scripts
void UNITY_INTERFACE_API OnRenderEvent(int event_id, void* data)
{
    if (event_id == 0)
    {
        // Event 0: Texture initialization
        InitializeTexture(*reinterpret_cast<TextureDescription*>(data));
    }
    else if (event_id == 1)
    {
        // Event 1: Texture finalization
        ReleaseTexture();
    }
}

} // anonymous namespace

//
// Unity low-level native plugin interface
//

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
  UnityPluginLoad(IUnityInterfaces* interfaces)
{
    _unity = interfaces;

    // Graphics device event callback setup
    _unity->Get<IUnityGraphics>()
      ->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

    // We must manually trigger the initialization event.
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
    // Graphics device event callback cleanup
    _unity->Get<IUnityGraphics>()
      ->UnregisterDeviceEventCallback(OnGraphicsDeviceEvent);

    _unity = nullptr;
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
    return _texture.Get();
}
