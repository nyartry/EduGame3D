# ルートモーションの設定

`SkinnedMeshActor` と `Player` は共通の `RootMotionSettings` を使います。ルートモーションはアニメーションによる平行移動を反映する仕組みです。どのモードでもアニメーションの姿勢は再生されます。現在の実装では、ルートの回転は抽出しません。

## 水平移動

| モード | 最終的な X/Z 方向の変位 |
| --- | --- |
| `Ignore` | プログラムによる移動のみ |
| `Blend` | プログラムによる移動 ×（1 − 重み）＋ アニメーションによる移動 × 重み |
| `Apply` | アニメーションによる移動のみ |

`Blend` の重みの既定値は 0.5 です。計算時には 0～1 の範囲に制限し、非有限値は 0 として扱います。ルートモーションは、合成前にモデル空間からワールド空間へ回転変換します。合成する両方の値は、その更新での変位であり、速度ではありません。プログラムによる移動がない `SkinnedMeshActor` では、`Blend` は変位 0 とアニメーションの変位を合成するため、アニメーションの変位に重みを掛けた結果になります。

初期設定は `SkinnedMeshActorDefinition::rootMotion` に指定します。プレイヤーの場合は `PlayerDefinition::mesh.rootMotion` です。`HumanoidPlayer.cpp` に設定例があります。初期化時にこの定義が読み込まれるため、実行中に変更する場合は `Initialize()` の後に、継承した次の API を使います。

```cpp
player.SetRootMotionSettings({
    .mode = RootMotionMode::Blend,
    .blendWeight = 0.5f,
    .verticalMode = RootMotionVerticalMode::Ignore
});

// 必要に応じてモードを選びます。モードを変えても垂直方向の設定は変わりません。
player.SetRootMotionMode(RootMotionMode::Ignore);
player.SetRootMotionMode(RootMotionMode::Apply);
player.SetRootMotionMode(RootMotionMode::Blend);
player.SetRootMotionBlendWeight(0.25f);
```

`Apply` は、その場で動くクリップなど、アニメーションの移動量が 0 の場合もその値を使います。プログラムによる移動を反映したい場合は、`Ignore` または `Blend` を選んでください。

## ジャンプ・重力とアニメーションの Y 方向移動

設定型の既定値は、水平方向が `Apply`、垂直方向が `Ignore` です。同梱の `HumanoidPlayer` は、EduHumanのアニメーションがその場で動くため、水平方向・垂直方向ともに `Ignore` を明示し、入力で水平移動します。合成の重みにかかわらず、ジャンプと重力はプログラムで処理します。移動後の位置には衝突処理による制約も適用されます。

`RootMotionVerticalMode::Apply` を指定すると、重みを掛けたアニメーションの Y 方向変位を加算します。ジャンプ速度や重力を拡大・縮小したり、置き換えたりはしません。プログラムによるジャンプが受け付けられると、離陸する更新から上昇・下降・着地まで、アニメーションの Y 方向変位を抑制します。ルートモーションだけで浮き上がった場合、この抑制は始まりません。重力を無効にすると、プログラムによるジャンプ状態は解除されます。

重力やプログラムによるジャンプを使わず、3軸すべての移動をアニメーションで制御する場合は、プレイヤーを次のように設定します。

```cpp
player.SetRootMotionSettings({
    .mode = RootMotionMode::Apply,
    .verticalMode = RootMotionVerticalMode::Apply
});
player.SetGravityEnabled(false);
```

この設定でも床との衝突は有効です。垂直移動の解決処理は、アニメーションで移動する前の位置から最終位置までを判定するため、アニメーションによる下降でも足場に着地します。この処理の戻り値は、プログラムによるジャンプが受け付けられたかを示します。単に空中へ移ったことを示す値ではありません。

## 回帰テスト

`powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_root_motion.ps1` を実行してください。ゲームの起動やキャラクター素材の読み込みを行わずに、合成の重み、座標変換、ジャンプ動作の維持、床との接触を検証します。
