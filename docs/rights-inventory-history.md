> **2026年9月14日時点の調査記録です。** 現在の状況は [素材・ライブラリの出所と利用条件](rights-inventory.md) を参照してください。この記録には削除済み素材、当時のファイル数、すでに整備した事項の古い説明も含まれます。旧素材は現在の配布内容から外れていますが、Git履歴からの削除を示すものではありません。

# 権利台帳：公開・学生配布に向けた調査初版

調査日: 2026-09-14。対象: `e0714173e9e07d7ba5d625bb5727d54f414cd6af`。
名称更新: プロジェクト名は **EduGame3D**。リンク先とA10のファイル名は調査後の名称変更に合わせています。A10の画像内容は変更していません。
追加調査（2026-09-14）: モデルの取得元と配布条件は [3Dモデルの調査](model-provenance-audit.md) で具体化しました。A01～A07と以下の関連説明には、その結果とUntitledの本人確認を反映しています。
モデル置換（同日）: A01～A06・A11の旧素材は現在のソースツリーから除外し、A12のEduHumanへ切り替えました。旧素材の記述と当初のファイル数は履歴上の記録です。現在の構成・検証・履歴を含まない出力は [置換記録](model-replacement.md) を参照してください。A08～A10とコード・依存物の未確認事項は引き続き残っています。
音楽の制作元確認（同日）: 所有者から「音楽はAIで作らせたもの」「作成元はCodex」と回答がありました。A08のBGMをCodexによる生成との本人確認済みに更新しています。生成指示・コード・外部入力の有無はGit履歴では確認できず、効果音の制作元は別項目として未確認です。
作業の優先順位とSARTRAS制度上の整理は [調査報告](../docs/sartras-readiness-audit.md) を参照。

名義・利用条件の整備（2026-09-14）: 所有者の指定により、プロジェクトの著作者・権利者表示を **Haruyuki Ishinaka** とし、[LICENSE](../LICENSE) と [作品情報](work-identification.md) を追加しました。制作したゲームの公開・販売は個別相談です。これにより表示名義は決まりましたが、以下のファイルごとの権利帰属・第三者条件の未確認事項が解消したものではありません。音源・効果音・エフェクト素材・画像素材への新たな許諾は、このLICENSEの対象外です。

この台帳は確認済みの事実と不足資料を記録するもので、利用許諾を新たに与える文書ではありません。「未確認」は違反の認定を意味しません。バイナリの制作者文字列やファイル名は出所の手掛かりであり、本人が取得したライセンスの証拠とは区別します。

## 1. 自作候補のコード・資料

| ID | 対象 | 確認できたこと | 未確認／必要な証拠 | 対応 |
|---|---|---|---|---|
| O01 | `src/Framework/` 157ファイル | 共通API・エンジン実装。区分は [architecture.md 17行](../docs/architecture.md#L17)。 | 実際の著作者、職務著作・共同制作・譲渡、初期コードの参考元・転用、人の創作的寄与。 | R01、R02、R04 |
| O02 | `src/Game/` 35ファイル、`src/Launcher/` 3ファイル | サンプル規則・コンテンツ指定・起動処理。 | 著作者・権利者。参照する素材は別権利として下表に分離。 | R01、R03、R04 |
| O03 | 既存 `docs/` 12ファイル、README | 数学・アニメーション等の解説とコード例。権利者表示・配布版は未整備。 | 解説文・図・コード例それぞれの制作過程と出典、公開名義。 | R01、R02、R07、R08 |
| O04 | `tools/` 24ファイル、`tests/` 30ファイル | エディター、検証スクリプト、回帰テスト。 | 制作者と権利範囲。ImGui等の取り込み部分を本人のコードと混同しない。 | R01、R02、R05 |
| O05 | [SpriteBatch.cpp 17行](../src/Framework/Rendering/Sprites/SpriteBatch.cpp#L17) の5×7文字表 | コード内に文字パターンを保持。 | パターンの制作者・参考元。この存在だけで独創性や権利の帰属を判断しない。 | R01 |

作者ラベルはGit履歴に4種類ありますが、同じ人の別名義の可能性があります。全130コミットのラベルを実際の制作者と対応づけます。Gitの名義だけで権利者を決めません。

[引き継ぎ資料337行](../docs/student-template-core-handoff.md#L337) は、その節の実装・テストへのCodexの関与を記載しています。AIを使ったことだけで全体の権利の有無は決まりません。創作的な表現に関する指示・人の修正等を実際の記録から確認します。[文化庁の説明](https://www.bunka.go.jp/seisaku/bunkashingikai/chosakuken/pdf/94057901_01.pdf)

## 2. 外部コード・依存バイナリ

`third_party` は5ディレクトリ、418追跡ファイル。うち20個の `.lib` / `.dll` は [.gitattributes 8行](../.gitattributes#L8) 以降でGit LFS管理。以下のライセンス名はローカル本文を根拠にしています。

| ID | 同梱物 | 保存されている根拠 | 不足している作業 |
|---|---|---|---|
| D01 | Assimp、Debug/Release DLL・LIB | [version.h 6行](../third_party/assimp/include/assimp/version.h#L6) に著作権・BSD 3-Clause相当の本文。[revision.h 4行](../third_party/assimp/build_include/assimp/revision.h#L4) に `116c96c4`、同7行以降に6.0.5。 | 独立したLICENSEの同梱、採用版とバイナリの対応、組み込まれた二次依存・改変の有無、実行物用の通知を確認。ヘッダー中には許諾本文があり「ライセンス全部欠落」ではない。 |
| D02 | DirectXTK12 | [LICENSE](../third_party/directxtk12/LICENSE#L1) はMicrosoftのMIT。[README 3行](../third_party/directxtk12/README.md#L3) 以降に上流、`e656d54`、採用ビルドと構成。 | 既存の出所記録を維持し、実行物への通知同梱に接続。この記録方法を他の依存にも展開。 |
| D03 | Effekseerと各Renderer | [LICENSE.txt](../third_party/effekseer/LICENSE.txt#L1) はMIT。 | 採用revision、ビルド手順、組み込まれた依存と告知範囲を確定。素材のライセンスは別表。 |
| D04 | LLGI（Effekseer側に同梱） | [GameApp.vcxproj 19行](../GameApp.vcxproj#L19)・24行でリンク。ヘッダー・LIBが存在。 | LLGI自体の採用版とライセンスの対応を確認。[LICENSE_RUNTIME_DIRECTX.txt 22行](../third_party/effekseer/LICENSE_RUNTIME_DIRECTX.txt#L22) 付近には旧DirectX Tool KitとMs-PLの記述がある。別同梱のDirectXTK12はMITであり、対象コードを照合せず表記を削除しない。 |
| D05 | RmlUi | [LICENSE.txt](../third_party/rmlui/LICENSE.txt#L1) はMIT。コンテナ用の [LICENSE.txt](../third_party/rmlui/include/RmlUi/Core/Containers/LICENSE.txt#L1) にrobin_hood／itlibのMIT本文。 | 両方の告知の同梱、revision・ビルド条件・オプションを記録。 |
| D06 | Dear ImGui | [LICENSE.txt](../third_party/imgui/LICENSE.txt#L1) はMIT、[imgui.h 32行](../third_party/imgui/imgui.h#L32) の版は1.92.9 WIP。 | 本体に加え以下のフォント・stbの通知を整理。エディター配布にも含める。 |
| D07 | ImGui内のフォント・stb | [imgui_draw.cpp 6373行](../third_party/imgui/imgui_draw.cpp#L6373) にProggyClean、[同6563行](../third_party/imgui/imgui_draw.cpp#L6563) にProggyForeverのMIT表示。stb本文は [imstb_rectpack.h 591行](../third_party/imgui/imstb_rectpack.h#L591)、[imstb_textedit.h 1491行](../third_party/imgui/imstb_textedit.h#L1491)、[imstb_truetype.h 5049行](../third_party/imgui/imstb_truetype.h#L5049)。 | 本体LICENSEだけにまとめず、採用部分の著作権・許諾を保持。stbは記載された選択許諾に従って扱う。 |

RmlUi上流READMEにFreeTypeやDebuggerフォントの説明はありますが、実装は [独自フォントエンジン](../src/Framework/UI/RmlUi/RmlUiService.cpp#L32) を使用しています。Release LIBの代表シンボル調査でもそれらの混入は確認できず、ライセンス欠落を確定問題として挙げません。バイナリのビルド構成確認に含めます。

GameAppとエディターの現在のPostBuildには、通知文書のコピーがありません。外部コードの全文がソースツリーにあることと、EXEフォルダだけで必要表示が揃うことは別に確認します。[GameApp.vcxproj 20行](../GameApp.vcxproj#L20)、[AnimationEventEditor.vcxproj 20行](../AnimationEventEditor.vcxproj#L20)

Assimp DLLはVCランタイムにも依存します。EXEを配布する場合はReleaseの動作環境・正規のランタイム入手手順を整え、再頒布する場合のみその条件を確認します。MicrosoftのランタイムDLL自体を現在同梱しているとの証拠はありません。

## 3. 素材と生成物

置換前の調査時点の `Content` は186ファイル。FBX10、画像167（JPG93・PNG66・TGA8）、WAV3、エフェクト1、イベントJSON2、README2、`.gitkeep`1。独立したフォントファイルは確認できませんでした。以下のA01～A06・A11は除外した旧素材の記録で、原ファイルの再採用を推奨するものではありません。

| ID | パス／まとまり | 出所について確認できたこと | 公開・学生配布までに必要なこと |
|---|---|---|---|
| A01 | `Content/Models/Player/Jogging/Jogging.fbx` | FBX内部のMixamo表記に加え、Windowsの取得元記録にMixamoの参照URL・エクスポート先。 | 原FBXの公開同梱は保留。利用者の公式取得、別素材、個別許可を検討。 |
| A02 | `Content/Models/Player/Mma Kick/Mma Kick.fbx` | FBX内部にMixamoの表記。調査時のエディターREADMEにも旧クリップ情報があった（現在の説明はEduHuman向けに更新済み）。 | 除外した旧素材として記録を保持。新しいキックのイベント教材へ更新済み。 |
| A03 | `Content/Models/Player/Orc Idle/Orc Idle.fbx` | FBX内部のMixamo表記に加え、Windowsの取得元記録にもMixamo。 | アニメーション／リグとキャラクター本体の権利を分け、原FBXの再配布条件を確認。 |
| A04 | `Content/Models/Y_Bot/Y_Bot.fbx` | FBX内部にMixamoの表記。 | テンプレートへの生データ同梱条件を確認。必要なら利用者取得方式・別素材にする。 |
| A05 | `Content/Models/55-rp_nathan_animated_003_walking_fbx/` 8ファイル | Renderpeopleの同名作品と対応。公式SketchfabにCC BY 4.0版を確認。ArtStation・本家等は別条件。 | 公式CC BY配布物と手元の各ファイルを対応づけるか、同版へ差し替えて表示を整える。現在の8ファイルへ一括でCC BYを付与しない。 |
| A06 | `Content/Models/Daven/` 159ファイル（FBX1、画像158） | CGTraderの `Daven Male Rigged`（作者 `d-e-c`）が有力。内部の制作元名・Blender版・T-poseが整合。TurboSquidにも同名作品。 | 実際の取得経路と適用規約は未確定。通常ライセンスで原FBX・画像を公開配布する判断はせず、利用者取得・置換・個別許可を検討。 |
| A07 | `Content/Models/Untitled/Untitled.fbx`、`old_Untitled.fbx` | 2026-09-14に本人が両方の自作を確認。`067fcd2` の説明・FBX内部のBlender情報とも整合。制作元 `.blend` は未追跡。 | 自作サンプルとして保持する候補。制作元・作者の記録を残す。テストも使用しているため一括削除しない。 |
| A08 | `Content/Audio/BGM/title_theme.wav`、`game_theme.wav`、`Content/Audio/SE/button_click.wav` | 2026-09-14、所有者がBGMをCodexで生成したと確認。3WAVは `59eda5583961a7f10bb4f8dd34c73f4faaa90fbf`（2026-07-03）で導入。生成スクリプト・プロンプトは同コミットと現在のリポジトリに見つからなかった。 | BGMの制作元は本人確認済みとして保持。生成コード・指示・入力素材の記録を補完する。クリックSEの制作元は今回の音楽の申告と分けて確認。AI生成という事実だけで権利確認完了やSARTRAS分配対象とはしない。 |
| A09 | `Content/Effects/Effekseer/Samples/Laser01.efkefc` とテクスチャ3点 | `db9a391` で導入。元パッケージ・版の記録はない。 | 公式サンプルの取得版とファイルを対応づけ、素材のCC0等の適用を確認。ランタイムMITだけを素材の根拠にしない。 |
| A10 | `edugame3d_crest.png` | [GameScene.cpp 31行](../src/Game/Scenes/GameScene.cpp#L31) の変数名は `GeneratedImageTexturePath`。 | 実際の制作者・生成経緯・利用条件を記録。変数名だけでAI生成物や自作と断定しない。 |
| A11 | `Mma Kick.anim_events.json`、`Y_Bot.anim_events.json` | アニメーションに対応するイベントデータ。 | データの制作者を確認し、元アニメーション変更時には内容・教材との整合を確認。 |
| A12 | `Content/Models/EduHuman/`、`tools/generate_eduhuman.py` | 所有者の依頼でCodexがプリミティブ・数式から新規生成した人型、22ボーン、待機・走行・キック。外部の形状・画像・リグ・モーションは入力に使っていない。[制作記録](../Content/Models/EduHuman/PROVENANCE.md) と編集用Blenderデータを保持。 | A01～A06・A11の代替として採用。制作経緯と人による編集記録を保持し、AI支援の生成物それ自体の独占権やSARTRAS分配対象を断定しない。 |
| H01 | 過去の `Content/Models/forest_goddess/` | `94bcd6a` で削除されたFBX1点と画像26点、計27点がGit履歴に存在。 | 現在ツリーだけでなく、公開する履歴とLFSの範囲も確認。公開用成果物の構成を決める。今回履歴操作はしていない。 |

音楽の生成元はCodexとの本人申告を根拠とし、音楽生成専用サービスを使ったとは扱いません。2026-09-14に確認した [OpenAI個人向け利用規約のContent節](https://openai.com/policies/row-terms-of-use/#content) は、OpenAIと利用者との間では、適用法が認める範囲でOutputを利用者に帰属させています。一方、[サービス規約4節](https://openai.com/policies/service-terms/#4-codex-and-code-generation) はCodex等のコード生成出力に第三者ライセンスが適用される場合を示しています。生成したコードや音源に外部素材が含まれていないかは、制作記録に基づいて整理します。これらは利用条件の確認であり、AI生成した音楽自体の著作権の成立やSARTRASの分配対象を認定するものではありません。人の創作的寄与に関する考え方は [文化庁資料](https://www.bunka.go.jp/seisaku/bunkashingikai/chosakuken/pdf/94057901_01.pdf) を参照してください。

Mixamoの表記はサービスの関与を示しますが、アップロード元キャラクターも含めた権利を証明するものではありません。Adobeの [Mixamo公式FAQ](https://helpx.adobe.com/creative-cloud/faq/mixamo-faq.html) はゲーム等への利用を案内し、[一般利用条件3.6節](https://www.adobe.com/legal/terms.html) はContent Filesの単体配布を制限しています。学生向けソースひな型として元ファイルを渡す場合の適用条件を確認する必要があります。

Renderpeople本家の通常条件は無料素材にも適用され、原ファイルの配布等を制限しています。[本家利用条件2.5、4.2、4.3節](https://renderpeople.com/general-terms-and-conditions/) 一方、Nathanには [公式SketchfabのCC BY 4.0版](https://sketchfab.com/3d-models/nathan-animated-003-walking-3d-man-143a2b1ea5eb4385ae90a73657aca3bc) があるため、配布経路を一律の条件で扱いません。ファイルの対応・表示義務を確認すれば公開同梱の候補になります。詳細は [追加調査](model-provenance-audit.md) を参照してください。

Effekseer公式概要はランタイムと素材のライセンスを区別しています。A09が当該条件の対象ファイルであることを元パッケージと照合します。[公式概要](https://effekseer.github.io/Help_Tool/ja/overview.html)

現在の [GameScene.cpp](../src/Game/Scenes/GameScene.cpp) はEduHumanを使う操作キャラクター・静止展示・歩行サンプルを生成します。旧素材を除外した構成の検証は [置換記録](model-replacement.md) を参照してください。[AssetPathResolver.cpp](../src/Framework/Assets/AssetPathResolver.cpp) は親ディレクトリも探索するため、配布フォルダをソースツリー外へ置いた検証も行います。

## 4. 台帳を確定するための記録項目

各行は、取得元や条件が異なる場合にはファイル単位に分割します。添付書類・購入記録・個人情報をこの公開予定の文書へ直接埋め込まず、証拠の管理先を参照します。

| 項目 | 初版での扱い |
|---|---|
| ファイル範囲・作品名・版 | 上表の範囲を起点に特定。複数の権利があるモデル等は分ける。 |
| 著作者・権利者 | 未確定。自作／外部の推定だけで個人名を記入しない。 |
| 取得元URL・取得日・原本 | 本人の記録で補完。可能なら原本のハッシュと規約版も保存。 |
| 加工・AI支援・人の創作的寄与 | 実際の制作記録と変更差分を対応づける。 |
| 適用ライセンス・条件 | 採用した原本・版に適用される本文へ参照を付ける。 |
| 生ファイルの再配布 | Public GitHub、学生向け限定配布それぞれについて可否と根拠を記録。 |
| 完成作品の配布 | EXE・動画・就職作品への掲載等について条件を記録。 |
| 必要なクレジット・通知 | 表記内容、本文、同梱先を対応づける。 |
| SARTRASで本人が申し出る範囲 | 本人が権利を持つ部分と第三者部分を区別。対象かどうかは利用実態等で判断。 |
| 採否・完了証拠 | 許可確認済みで採用／置換／除外／追加許諾確認中のいずれかと根拠。 |

ソースやバイナリのライセンス確認、素材の許諾確認、SARTRASの分配判断はそれぞれ別の確認です。全行が整理できたことだけで補償金受給が確定するものではありません。
