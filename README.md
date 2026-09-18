# EduGame3D

日本国内の教育機関の通常授業で、3Dゲームのプログラミングを学ぶためのC++ / DirectX 12ライブラリです。

**[最新のライブラリ・使い方はこちら（develop）](https://github.com/nyartry/EduGame3D/tree/develop)**

授業やゲーム制作に役立ったら、**[GitHub Sponsorsで開発を応援する](https://github.com/sponsors/nyartry)**。単発・月額の支援を受け付けています。支援は完全に任意で、利用条件は変わりません。詳しくは[支援について](SUPPORT.md)をご覧ください。

このmainブランチには初期版を残しています。現在の機能・導入手順・利用条件は、上記developブランチをご確認ください。

## 初期版について

Windows desktop DirectX 12 starter project for Visual Studio 2022.

## What is included

- Win32 window creation
- Direct3D 12 device, swap chain, command queue, command list
- Root signature and graphics pipeline state
- A simple animated triangle
- Visual Studio 2022 solution and project files

## Requirements

- Windows 10 or later
- Visual Studio 2022 with the Desktop development with C++ workload
- Windows 10/11 SDK
## Build

Open `DirectX12OpenCampusGame.sln` in Visual Studio 2022, then build and run with `Debug|x64`.

## Good next template upgrades

- Add DirectXTK12 for sprites, text, models, input helpers, and audio.
- Split rendering, input, scene, and asset loading into separate modules.
- Add a `Content/` directory for textures, models, and compiled shaders.
- Move shader code from `src/main.cpp` to `.hlsl` files once the first build is stable.
