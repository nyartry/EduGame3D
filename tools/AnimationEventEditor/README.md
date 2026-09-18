# EduGame3D アニメーションイベントエディター

授業用ゲームプログラミングフレームワーク「EduGame3D」で使う、FBXアニメーションのイベント編集ツールです。

## ビルド

Visual Studioで `EduGame3D.sln` を開きます。

ソリューションには、次のアプリケーション・ライブラリのプロジェクトと `VisualStudioTests` が含まれています。

- `GameApp`
- `GameModule`
- `EngineFramework`
- `AnimationEventEditor`

構成を `Debug|x64` または `Release|x64` にして、`AnimationEventEditor` をビルドします。

## 起動

ビルド後、次のいずれかのファイルを実行します。

- `tools/AnimationEventEditor/bin/Debug/AnimationEventEditor.exe`
- `tools/AnimationEventEditor/bin/Release/AnimationEventEditor.exe`

Visual Studioによる元の出力ファイルは、次の場所にもあります。

- `x64/Debug/AnimationEventEditor.exe`
- `x64/Release/AnimationEventEditor.exe`

どちらの出力先にも、ビルド時に`LICENSE`と`THIRD_PARTY_NOTICES.txt`を配置します。配布が許可される構成でエディターを渡す際は、この2ファイルも同梱してください。[利用条件と許諾文の案内](../../docs/rights-inventory.md#exeを配布するとき)を確認してください。

## 基本的な操作の流れ

1. `AnimationEventEditor.exe` を起動します。
2. `File > Open FBX...` を選びます。
3. `Asset` パネルでアニメーションを選びます。
4. `Play` で再生するか、タイムライン上で再生位置を動かします。
5. タイムラインをクリックするか、`Add Event At Current Time` を押します。
6. `Event Properties` でイベントの種類、ボーン、キューを編集します。
7. `File > Save Events` を選びます。

イベントは、`edugame3d-animation-events-v1` スキーマの `.anim_events.json` ファイルとして保存されます。旧スキーマのファイルも読み込めますが、保存すると現在のスキーマ名に書き換わります。`sourceFbx` にはFBXのファイル名だけを記録し、端末固有のフォルダーパスは含めません。

FBXを開くと、同じフォルダーにある対応する `.anim_events.json` ファイルも探し、自動で読み込みます。

エディターでファイルを開く・保存する際、FBXとイベントJSONのファイル名には、表示可能なASCII文字を使ってください。使えるのは英字、数字、半角スペース、Windowsのファイル名として有効なASCII記号です。例えば、`walk.fbx` や `walk.anim_events.json` のような名前にします。フォルダー名には日本語などのUnicode文字を使えます。

ファイル名が受け付けられない場合は、`Status` パネルに制限の説明が表示されます。その場合も、現在編集中の内容、編集履歴、保存済みファイルは保持されます。

## 編集

- `Edit > Undo` / `Ctrl+Z`：イベントの編集を元に戻します。
- `Edit > Redo` / `Ctrl+Y`：元に戻したイベントの編集をやり直します。
- `File > Open Events...`：現在のモデルに対して、既存の `.anim_events.json` を読み込みます。
- `View > Reset Layout`：パネルの配置を初期状態に戻し、保存済みのImGuiレイアウトを書き換えます。

レイアウト設定は `%LOCALAPPDATA%\EduGame3D\AnimationEventEditor\imgui.ini` に保存されます。初回利用時にこのファイルがなく、旧プロジェクト名のレイアウト設定がある場合は、その設定をコピーします。この移行処理で既存の設定を上書きすることはありません。

## コンボ入力の調整

ゲームで次の攻撃を受け付ける時間帯を編集するには、`Content/Models/EduHuman/EduHuman_Kick.fbx` を開きます。同じフォルダーの `EduHuman_Kick.anim_events.json` は自動で読み込まれます。新しく生成したクリップは `Kick` で、長さは1.6秒です（1秒あたり30 tick、合計48 tick）。編集可能なBlenderソースと生成スクリプトについては、[モデル素材](../../Content/Models/README.md)を参照してください。

- `ComboWindowOpen` を選ぶと、次の攻撃の入力受付を開始する時刻を調整できます。初期値は0.5秒です。
- `ComboWindowClose` を選ぶと、入力受付を終了する時刻を調整できます。初期値は1.2秒です。
- `Event Properties` の `Time` を変更するか、タイムライン上のイベントをドラッグします。`0 < open < close <= 1.6` を満たすようにし、`Bone` と `Cue` は空欄のままにしてください。
- `File > Save Events` で保存し、ゲームを再起動すると変更した時刻が反映されます。コード内の時間定数の変更や、新しいJSON形式の追加は不要です。

ゲームでは、各攻撃をクリップの実際の終端まで1回再生します。受付時間内にXキーを新たに押すと、次の攻撃を1回分予約します。受付開始前や終了後の入力は無視され、Xキーを押し続けても攻撃は繰り返されません。受付時間が終了しても、予約済みの攻撃は保持されます。予約した攻撃は、現在の攻撃が完了した次のシミュレーション更新の先頭で始まるため、終端の音声・エフェクトイベントを先に処理できます。現在の素材構成では、コンボで同じキックを繰り返します。

調整するたびに、1回だけ押す、2回目を早めに押す、受付時間内に押す、受付終了後に押す、押し続ける、という各操作を試してください。付属の時刻設定は、このクリップを調整するための初期値です。
