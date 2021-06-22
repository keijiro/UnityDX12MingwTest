UnityDX12MingwTest
==================

**UnityDX12MingwTest** is a Unity sample project that shows how to build a
low-level native plugin for the DirectX 12 (D3D12) mode using the [Mingw-w64]
GCC toolchain. It means that now you can build DX12 plugins on [WSL] without
using Visual Studio.

[Mingw-w64]: http://mingw-w64.org/
[WSL]: https://docs.microsoft.com/en-us/windows/wsl/about

How to build the plugin
-----------------------

The D3D12 headers and libraries with Mingw-w64 are only available in v8.0.0 or
later. If you are using Ubuntu, you have to upgrade to 21.04 (Hirsute Hippo) as
it's the only version officially supporting Mingw-w64 v8.0.0.

I was using Ubuntu 20.04 on WSL2 and failed to upgrade using
`do-release-upgrade` for unknown reasons. I had to use a [workaround] to solve
the problem.

[workaround]:
  https://www.reddit.com/r/Ubuntu/comments/mwdlj0/bug_cannot_upgrade_from_2010_to_2104/gvtt6zz/

Once the upgrade is completed, install mingw-w64.

```
sudo apt install mingw-w64
```

Now you can simply run `make` in the `Plugin` directory.
