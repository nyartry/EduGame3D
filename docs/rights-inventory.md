# 素材・外部ライブラリの利用条件

EduGame3D本体の利用条件は [LICENSE](../LICENSE) を確認してください。このページは、同梱する素材・外部ライブラリを利用・配布するときの補足です。

## 素材

| 素材 | 利用条件・確認状況 |
|---|---|
| 人型モデル「EduHuman」 | 本プロジェクトのLICENSEの対象。[制作元と入力の記録](../Content/Models/EduHuman/PROVENANCE.md)があります。 |
| サンプルモデル「Untitled」2点 | 本人が自作と確認したモデル。本プロジェクトのLICENSEの対象です。 |
| [BGM 2曲](../Content/Audio/BGM/) | Codexで生成したとの本人確認あり。生成時の外部入力などの記録は未確認です。 |
| [クリック音](../Content/Audio/SE/button_click.wav) | 制作元・利用条件が未確認です。 |
| [Laser01エフェクトと画像3点](../Content/Effects/Effekseer/Samples/) | Effekseer公式素材と一致するCC0の素材です。[対象ファイル・出典・CC0本文](../Content/Effects/Effekseer/Samples/LICENSE.txt)を同梱しています。 |
| [紋章画像](../Content/Textures/UI/edugame3d_crest.png) | 制作者・生成経緯・利用条件が未確認です。 |

**BGM・効果音・エフェクト・画像は、現行のプロジェクトLICENSEによる許諾の対象外です。** CC0の公式エフェクト素材には、その素材自身の条件が適用されます。利用条件が未確認の素材は、権利者や入手元の条件を確認するか、利用条件の明らかな素材へ差し替えてください。

コードやモデルについても、LICENSEが適用されるのは許諾者が実際に権利を持つ部分に限られます。第三者の部分にはそれぞれの条件が適用されます。

## 外部ライブラリ

各ライブラリの著作権表示・許諾文を保持してください。同梱バイナリの版や追加の依存物については確認が完了していない部分もあります。配布する構成に適用される条件は、下記の本文と併せて確認してください。

| ライブラリ | 許諾文・注意点 |
|---|---|
| Assimp | [本体・Poly2Triの許諾文](../third_party/assimp/LICENSE)と[追加の依存物の許諾文](../third_party/assimp/LICENSE_DEPENDENCIES.txt)。同梱ヘッダーに記載された上流リビジョンの通知です。 |
| DirectXTK12 | [MIT](../third_party/directxtk12/LICENSE)。 |
| Effekseer | [MIT](../third_party/effekseer/LICENSE.txt)、[DirectXランタイムの通知](../third_party/effekseer/LICENSE_RUNTIME_DIRECTX.txt)、その通知が参照する[MS-PL本文](../third_party/effekseer/LICENSE_MSPL.txt)、[画像読み込み用stbの許諾文](../third_party/effekseer/LICENSE_STB.txt)。エフェクト素材の条件は別です。 |
| LLGI（Effekseerに同梱） | [zlib形式の許諾文](../third_party/effekseer/LICENSE_LLGI.txt)。 |
| RmlUi | [本体のMIT](../third_party/rmlui/LICENSE.txt)と[内部コンテナのMIT](../third_party/rmlui/include/RmlUi/Core/Containers/LICENSE.txt)の両方を保持してください。 |
| Dear ImGui | [本体のMIT](../third_party/imgui/LICENSE.txt)、[フォント・文字範囲データの通知](../third_party/imgui/LICENSE_FONTS.txt)、[stbの許諾文](../third_party/imgui/LICENSE_STB.txt)。 |

ImGuiの元の表示は、[imgui_draw.cpp](../third_party/imgui/imgui_draw.cpp)、[imstb_rectpack.h](../third_party/imgui/imstb_rectpack.h)、[imstb_textedit.h](../third_party/imgui/imstb_textedit.h)、[imstb_truetype.h](../third_party/imgui/imstb_truetype.h)にも保持しています。DirectXTK12本体のMITと、Effekseerの既存通知が参照するMS-PLは、それぞれの対象に適用されます。

## EXEを配布するとき

ゲームとエディターのビルド時に、次の2ファイルをEXEの出力先へ自動で配置します。**EXEを配布するときは、この2ファイルも一緒に渡してください。**

- `LICENSE`：EduGame3D本体の利用条件。
- `THIRD_PARTY_NOTICES.txt`：上記の外部ライブラリと公式エフェクト素材の著作権表示・許諾文。

配置先は`x64/Debug/`・`x64/Release/`と、エディターの`tools/AnimationEventEditor/bin/Debug/`・`Release/`です。許諾文を更新した場合や、出力先の通知を削除した場合も、再ビルドで反映されます。許諾文の入力ファイルが不足している場合はエラーになります。

通知の同梱だけで、利用条件が未確認の素材の配布や、EduGame3D本体の個別許可が必要な利用まで許可されるわけではありません。

ソース一式を取り出す方法は [モデル素材・ソース出力の手順](../Content/Models/README.md) を参照してください。
