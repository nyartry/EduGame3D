# モデル素材

ゲームの人型モデルは、このプロジェクト向けに新しく生成した **EduHuman** です。外部のキャラクター・テクスチャ・リグ・モーションキャプチャを入力に使っていません。制作方法と範囲は [制作記録](EduHuman/PROVENANCE.md) を参照してください。

| ファイル | 用途 |
|---|---|
| `EduHuman/EduHuman_Idle.fbx` | 操作キャラクターの待機と静止展示 |
| `EduHuman/EduHuman_Jog.fbx` | 操作キャラクターの移動と歩行サンプル |
| `EduHuman/EduHuman_Kick.fbx` | 右脚のキック。1.6秒で1回再生 |
| `EduHuman/EduHuman_Kick.anim_events.json` | 0.5秒でコンボ受付を開き、1.2秒で閉じる |
| `EduHuman/` 内の `.blend` | Blenderで体形・色・ボーン・動作を編集する制作元 |
| `Untitled/Untitled.fbx`、`Untitled/old_Untitled.fbx` | 所有者が自作と確認した既存サンプル。立方体表示とテストで使用 |

EduHumanは頭、胴、骨盤、左右の肩・腕・手・脚・足先を持つ22ボーンの人型です。Unity固有のHumanoid機能には依存しません。各FBXは同じ骨格とスキンウェイトを持ち、このプロジェクトのAssimp読込とGPUスキニングで再生します。色はマテリアルで表現し、画像テクスチャは不要です。ゲーム側で身長を1.8に正規化し、移動は入力と衝突処理で制御します。

## 再生成・編集

Blender 5.1で生成します。リポジトリのルートから実行してください。

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.1\blender.exe' --background --factory-startup --python .\tools\generate_eduhuman.py
```

再生成はEduHumanの制作元とFBXを上書きします。手作業で編集した `.blend` を保持したい場合は、別名で保存してから実行してください。ゲームの通常のビルド・起動にはBlenderは不要です。

スクリプトはBlender内のデータを初期化してから生成し、個人のスタートアップファイルの素材を持ち込みません。プレビューが不要なら末尾に `-- --no-preview` を付けられます。

コンボ受付時間はAnimation Event Editorで `EduHuman_Kick.fbx` を開いて変更できます。[編集手順](../../tools/AnimationEventEditor/README.md#tuning-combo-input) を参照してください。

## 履歴を含まないソースの出力

リポジトリのルートで次を実行すると、現在のソース・ライブラリ・素材をまとめたフォルダーとZIPを作成できます。先に `git lfs pull` でファイルの実体を取得してください。

```powershell
.\tools\export_clean_source.ps1
```

出力先は `x64/CleanSource/EduGame3D/` と `x64/CleanSource/EduGame3D-clean-source.zip` です。Git履歴とビルド結果は含めず、モデルはEduHuman・Untitledに限定します。既知の旧外部モデルや未取得のLFSファイルが混ざると出力を中止します。モデルの照合用ハッシュは `MODEL_MANIFEST.json`、元のcommitと出力日時は `CLEAN_SOURCE_EXPORT.json` に入ります。

既存の出力を更新するときは `-Force`、別の出力先を指定するときは `-OutputDirectory` を使用します。`-Force` は、このスクリプトの管理記録がある出力だけを置き換えます。取り出したソースをビルドするときは、出力フォルダー内の `EduGame3D.sln` を開きます。

音源・エフェクト・画像や外部ライブラリの配布条件は、出力に含まれる [利用条件](../../docs/rights-inventory.md) を確認してください。出力の成功は、すべての素材を再配布してよいという意味ではありません。

この文書は制作・構成の記録です。利用条件は [ルートのLICENSE](../../LICENSE) を参照してください。制作記録は、著作権の成立・SARTRASの分配対象であることを保証するものではありません。
