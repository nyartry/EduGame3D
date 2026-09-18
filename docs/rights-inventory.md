# 素材・外部ライブラリの利用条件

EduGame3D本体の利用条件は [LICENSE](../LICENSE) を確認してください。このページは、同梱する素材・外部ライブラリを利用・配布するときの補足です。

## 素材

| 素材 | 利用条件・確認状況 |
|---|---|
| 人型モデル「EduHuman」 | 本プロジェクトのLICENSEの対象。[制作元と入力の記録](../Content/Models/EduHuman/PROVENANCE.md)があります。 |
| サンプルモデル「Untitled」2点 | 本人が自作と確認したモデル。本プロジェクトのLICENSEの対象です。 |
| [BGM 2曲](../Content/Audio/BGM/) | Codexで生成したとの本人確認あり。生成時の外部入力などの記録は未確認です。 |
| [クリック音](../Content/Audio/SE/button_click.wav) | 制作元・利用条件が未確認です。 |
| [Laser01エフェクトと画像3点](../Content/Effects/Effekseer/Samples/) | 取得したサンプルの版と、素材に適用される条件が未確認です。 |
| [紋章画像](../Content/Textures/UI/edugame3d_crest.png) | 制作者・生成経緯・利用条件が未確認です。 |

**BGM・効果音・エフェクト・画像は、現行のプロジェクトLICENSEによる許諾の対象外です。** このリポジトリに含まれることだけで、利用・再配布が許可されたものとは扱わないでください。権利者や入手元の条件を確認するか、利用条件の明らかな素材へ差し替えてください。

コードやモデルについても、LICENSEが適用されるのは許諾者が実際に権利を持つ部分に限られます。第三者の部分にはそれぞれの条件が適用されます。

## 外部ライブラリ

各ライブラリの著作権表示・許諾文を保持してください。同梱バイナリの版や追加の依存物については確認が完了していない部分もあります。配布する構成に適用される条件は、下記の本文と併せて確認してください。

| ライブラリ | 許諾文・注意点 |
|---|---|
| Assimp | [version.h内の許諾本文](../third_party/assimp/include/assimp/version.h)（BSD 3-Clause相当）。独立したLICENSEファイルは未同梱です。 |
| DirectXTK12 | [MIT](../third_party/directxtk12/LICENSE)。 |
| Effekseer | [MIT](../third_party/effekseer/LICENSE.txt)。エフェクト素材の条件は別です。 |
| LLGI（Effekseerに同梱） | [同梱通知](../third_party/effekseer/LICENSE_RUNTIME_DIRECTX.txt)。LLGI自体の採用版と適用条件は確認が未完了です。 |
| RmlUi | [本体のMIT](../third_party/rmlui/LICENSE.txt)と[内部コンテナのMIT](../third_party/rmlui/include/RmlUi/Core/Containers/LICENSE.txt)の両方を保持してください。 |
| Dear ImGui | [本体のMIT](../third_party/imgui/LICENSE.txt)に加え、下記のフォント・stbの許諾文も保持してください。 |

ImGui内の表示・許諾文の保存先：

- フォント：[imgui_draw.cpp](../third_party/imgui/imgui_draw.cpp)内のProggyClean・ProggyForeverの表示。
- stb：[imstb_rectpack.h](../third_party/imgui/imstb_rectpack.h)、[imstb_textedit.h](../third_party/imgui/imstb_textedit.h)、[imstb_truetype.h](../third_party/imgui/imstb_truetype.h)。

**現在のビルド処理は、許諾文をEXEの出力先へ自動コピーしません。** EXEだけを配布するときは、使用したライブラリに必要な著作権表示・許諾文も同梱してください。

ソース一式を取り出す方法は [モデル素材・ソース出力の手順](../Content/Models/README.md) を参照してください。
