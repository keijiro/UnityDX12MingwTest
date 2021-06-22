#include <d3d12.h>
#include <cstdio>
#include <cstddef>
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D12.h"
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

namespace {

// Unity native plugin interface
IUnityInterfaces* _unity;

// Command list
ComPtr<ID3D12CommandAllocator> _cmd_alloc;
ComPtr<ID3D12GraphicsCommandList> _cmd_list;

// Texture buffers
ComPtr<ID3D12Resource> _upload;
ComPtr<ID3D12Resource> _texture;

//
// Interop structure
//

struct TextureDescription { int width, height; };

//
// Callback functions
//

void UNITY_INTERFACE_API
  OnGraphicsDeviceEvent(UnityGfxDeviceEventType event_type)
{
    auto renderer_type = _unity->Get<IUnityGraphics>()->GetRenderer();
    if (renderer_type != kUnityGfxRendererD3D12) return;

    if (event_type == kUnityGfxDeviceEventInitialize)
    {
        // Command list allocation
        auto dev = _unity->Get<IUnityGraphicsD3D12v6>()->GetDevice();

        dev->CreateCommandAllocator
          (D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_cmd_alloc));

        dev->CreateCommandList
          (0, D3D12_COMMAND_LIST_TYPE_DIRECT, _cmd_alloc.Get(), nullptr,
           IID_PPV_ARGS(&_cmd_list));
    }
    else if (event_type == kUnityGfxDeviceEventShutdown)
    {
        // Command list deallocation
        _cmd_list = nullptr;
        _cmd_alloc = nullptr;
    }
}

void UNITY_INTERFACE_API OnRenderEvent(int event_id, void* data)
{
    if (event_id == 0)
    {
        auto dev = _unity->Get<IUnityGraphicsD3D12v6>()->GetDevice();
        auto params = reinterpret_cast<TextureDescription*>(data);

        // Upload buffer allocation
        D3D12_HEAP_PROPERTIES heap_upload = {};
        heap_upload.Type = D3D12_HEAP_TYPE_UPLOAD;
        heap_upload.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_upload.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC desc_upload = {};
        desc_upload.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc_upload.Alignment = 0;
        desc_upload.Width = params->width * params->height * 4;
        desc_upload.Height = 1;
        desc_upload.DepthOrArraySize = 1;
        desc_upload.MipLevels = 1;
        desc_upload.Format = DXGI_FORMAT_UNKNOWN;
        desc_upload.SampleDesc.Count = 1;
        desc_upload.SampleDesc.Quality = 0;
        desc_upload.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc_upload.Flags = D3D12_RESOURCE_FLAG_NONE;

        dev->CreateCommittedResource
          (&heap_upload, D3D12_HEAP_FLAG_NONE, &desc_upload,
           D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_upload));

        // Texture resource allocation
        D3D12_HEAP_PROPERTIES heap_texture = {};
        heap_texture.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap_texture.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_texture.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC desc_tex = {};
        desc_tex.MipLevels = 1;
        desc_tex.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc_tex.Width = params->width;
        desc_tex.Height = params->height;
        desc_tex.Flags = D3D12_RESOURCE_FLAG_NONE;
        desc_tex.DepthOrArraySize = 1;
        desc_tex.SampleDesc.Count = 1;
        desc_tex.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

        dev->CreateCommittedResource
          (&heap_texture, D3D12_HEAP_FLAG_NONE, &desc_tex,
           D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&_texture));

        // Create a test image in the upload buffer.
        void* mapped = nullptr;
        _upload->Map(0, nullptr, &mapped);

        auto ptr = reinterpret_cast<std::byte*>(mapped);
        auto offs = 0;

        for (auto y = 0; y < params->height; y++)
        {
            for (auto x = 0; x < params->width; x++)
            {
                ptr[offs++] = static_cast<std::byte>(x);
                ptr[offs++] = static_cast<std::byte>(y);
                ptr[offs++] = static_cast<std::byte>(1);
                ptr[offs++] = static_cast<std::byte>(0);
            }
        }

        _upload->Unmap(0, nullptr);

        // Image data transfer
        D3D12_RESOURCE_DESC desc_resrc = _texture->GetDesc();

        D3D12_TEXTURE_COPY_LOCATION src_loc = {};
        src_loc.pResource = _upload.Get();
        src_loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        dev->GetCopyableFootprints
          (&desc_resrc, 0, 1, 0, &src_loc.PlacedFootprint,
           nullptr, nullptr, nullptr);

        D3D12_TEXTURE_COPY_LOCATION dst_loc = {};
        dst_loc.pResource = _texture.Get();
        dst_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        dst_loc.SubresourceIndex = 0;

        UnityGraphicsD3D12ResourceState resrc_state = {};
        resrc_state.resource = _texture.Get();
        resrc_state.expected = D3D12_RESOURCE_STATE_COPY_DEST;
        resrc_state.current = D3D12_RESOURCE_STATE_COPY_DEST;

        _cmd_alloc->Reset();
        _cmd_list->Reset(_cmd_alloc.Get(), nullptr);

        _cmd_list->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, nullptr);
        _cmd_list->Close();

        _unity->Get<IUnityGraphicsD3D12v6>()->
          ExecuteCommandList(_cmd_list.Get(), 1, &resrc_state);
    }
    else if (event_id == 1)
    {
        _upload = nullptr;
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

    _unity->Get<IUnityGraphics>()
      ->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);

    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API UnityPluginUnload()
{
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
