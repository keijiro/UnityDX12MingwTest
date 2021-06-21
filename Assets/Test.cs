using UnityEngine;
using UnityEngine.Rendering;
using System.Collections;
using System.Runtime.InteropServices;
using IntPtr = System.IntPtr;

sealed class Test : MonoBehaviour
{
    #region Editable attributes

    [SerializeField] int _width = 512;
    [SerializeField] int _height = 512;

    #endregion

    #region Native plugin interface

    [StructLayout(LayoutKind.Sequential)]
    struct TextureDescription
    {
        int width, height;
        public TextureDescription(int width, int height)
          => (this.width, this.height) = (width, height);
    }

    [DllImport("Plugin")]
    static extern IntPtr GetRenderEventCallback();

    [DllImport("Plugin")]
    static extern IntPtr GetPointer();

    #endregion

    #region Private objects

    CommandBuffer _commands;
    Texture2D _texture;
    IntPtr _texDescMem;

    #endregion

    #region MonoBehaviour implementation

    IEnumerator Start()
    {
        // Allocate a texture description structure on the unmanaged heap.
        _texDescMem = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(TextureDescription)));
        Marshal.StructureToPtr(new TextureDescription(_width, _height), _texDescMem, false);

        // Invoke the texture initialization event on the render thread.
        _commands = new CommandBuffer();
        _commands.IssuePluginEventAndData(GetRenderEventCallback(), 0, _texDescMem);
        Graphics.ExecuteCommandBuffer(_commands);

        // Wait for the next frame.
        yield return null;

        // Create an external texture.
        _texture = Texture2D.CreateExternalTexture
          (_width, _height, TextureFormat.RGBA32, false, false, GetPointer());

        // Set the texture to the material.
        GetComponent<Renderer>().material.mainTexture = _texture;
    }

    void OnDestroy()
    {
        // Invoke the texture finalization event on the render thread.
        _commands.Clear();
        _commands.IssuePluginEventAndData(GetRenderEventCallback(), 1, IntPtr.Zero);
        Graphics.ExecuteCommandBuffer(_commands);

        // Destroy the texture object.
        Destroy(_texture);

        // Deallocate the texture description structure.
        Marshal.FreeHGlobal(_texDescMem);
    }

    #endregion
}
