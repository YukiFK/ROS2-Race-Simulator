# ROS2 Race Simulator
## Requirements Definition

---

## 1. 文書の目的

本書は、ROS2 上で動作するレースシミュレーション環境の開発に向けて、プロジェクトの目的、対象範囲、要求事項、設計上の前提、および完成条件を整理するための要件定義書である。

本プロジェクトは単なるゲーム開発ではなく、以下を主目的とする。

- ROS2 の理解を深めること
- node / topic / message / TF / Odometry の設計理解
- simulation time / wall time の扱いの理解
- race logic の状態設計の理解
- vehicle control の理解
- 将来的な CARLA 連携や外部地図資産利用を見据えた拡張可能な構造を作ること

---

## 2. プロジェクトの目的

### 2.1 主目的

本プロジェクトの主目的は、ROS2 を用いた **軽量なレースシミュレーション環境** を構築し、その実装を通して ROS2 システム設計を学ぶことである。

このプロジェクトは、以下を学習・検証するための土台とする。

- ROS2 のノード分割
- topic 設計
- custom message 設計
- `nav_msgs/Odometry` の使い方
- TF の使い方
- 座標系設計
- `use_sim_time` の扱い
- RaceState / Lap 判定ロジック
- controller（Pure Pursuit / PID heading）の比較
- 将来の simulator-independent な構造設計

### 2.2 将来的な拡張目標

本システムは、初版では内部の軽量シミュレータを用いるが、長期的には以下へ拡張することを想定する。

- CARLA を用いた高忠実度シミュレーション
- LiDAR / Radar / Camera の利用
- perception / planning / control の統合
- OpenDRIVE を主とした外部地図資産の利用
- 競技・公開地図データ（例: つくばチャレンジ等）の取り込み
- simulator backend の差し替え
- controller の比較研究（Pure Pursuit / PID / Stanley / MPC）

---

## 3. スコープ

### 3.1 初版で実施する範囲

初版では、以下を実装対象とする。

#### レース環境
- 2D の周回トラックを扱う
- track file によりレース論理コースを定義する
- start_line, centerline, track_width, forward_hint を扱う

#### 車両
- 複数車両に対応する
- 手動操作車両を少なくとも 1 台持つ
- AI 操作車両を少なくとも 1 台持つ
- 各車両は namespace により分離される

#### 操作
- gamepad による手動入力
- keyboard による fallback 入力
- race command 入力（START / STOP / RESET）
- AI controller による自動入力

#### AI
- Pure Pursuit による追従走行
- PID on heading による比較用 controller
- controller 切替可能構造

#### レース機能
- READY / COUNTDOWN / RUNNING / FINISHED / STOPPED の状態管理
- lap 計測
- best lap 計測
- off-track 判定
- stop / reset 対応

#### 可視化
- 軽量な独自 2D renderer
- 車両位置、向き、race state、lap 情報の表示
- LapEvent の簡易表示

#### シミュレーション
- Kinematic Bicycle Model による軽量な車両運動
- 最高速度制限
- command timeout
- reset 時の初期姿勢復帰

---

### 3.2 初版では実施しない範囲

以下は初版では対象外とする。

- 3D 可視化
- Unity 連携
- Unreal Engine 連携
- 実センサシミュレーション
- LiDAR / Radar / Camera topic の生成
- perception ノード
- planning ノード
- localization ノード
- dynamic obstacle
- vehicle-to-vehicle interaction
- overtaking
- collision 判定
- 高忠実度 vehicle dynamics
- MPC の実装
- OpenDRIVE 直接読み込み
- 外部地図資産の本格利用
- track の topic 配信
- track provider node
- scenario editor
- GUI からの複雑な操作

---

### 3.2.1 初版で禁止する拡張
以下は、将来構想として文書に存在していても、初版では実装に着手してはならない。
- collision 判定
- overtaking
- vehicle-to-vehicle interaction
- dynamic obstacle
- 高忠実度 vehicle dynamics
- OpenDRIVE 読み込み
- sensor topic 生成
- Track.msg の導入
- track provider node の導入
- 複雑な GUI 操作系
- renderer の高機能化
- controller plugin 機構の過剰実装

これらは初版 acceptance criteria を満たした後にのみ再検討する。

---

### 3.3 初版実装方針
初版実装では、ROS2 システム設計の理解と、軽量な internal backend による最小レース成立を最優先とする。

以下を初版実装の原則とする。
- 最初から将来拡張用の過剰な抽象化を行わない
- plugin 機構や汎用 factory は、初版で必要になるまで導入しない
- internal backend で成立する最小構成を優先する
- 各フェーズで build / run / inspect 可能な状態を維持する
- 「将来必要かもしれない」機能は、明確な使用箇所が出るまで実装しない

初版では、設計上の拡張可能性は保持するが、未使用の拡張ポイントを実装しすぎないこと。

---

## 4. 想定ユーザと利用シナリオ

### 4.1 想定ユーザ
- プロジェクト開発者本人
- ROS2 学習者
- 将来的には controller の比較研究をしたい開発者
- 将来 CARLA や OpenDRIVE に接続したい開発者

### 4.2 主な利用シナリオ

#### シナリオ 1: ROS2 学習
- 手動車両を gamepad で操作する
- `cmd_drive` / `odom` / `RaceState` / `VehicleRaceStatus` の流れを確認する
- TF と Odometry の関係を学ぶ

#### シナリオ 2: AI 比較
- `car1` を手動
- `car2` を Pure Pursuit
- `car3` を PID heading
として比較する

#### シナリオ 3: race logic 検証
- start / stop / reset を試す
- lap 判定を確認する
- off-track 判定を確認する

#### シナリオ 4: 将来拡張
- internal backend を CARLA backend に差し替える
- OpenDRIVE から map adaptation を行い track を生成する
- sensor topic を追加する

---

## 5. 機能要求

### 5.1 車両制御要求

#### 5.1.1 DriveCommand
システムは車両制御入力として `DriveCommand` を扱うこと。

- throttle_cmd
- steering_cmd

両者の値域は以下とする。

- `throttle_cmd ∈ [-1.0, 1.0]`
- `steering_cmd ∈ [-1.0, 1.0]`

#### 5.1.2 車両ごとの入力源
各車両の `DriveCommand` 生成元は以下のいずれかである。

- gamepad_input_node
- keyboard_input_node
- ai_driver_node

同一車両に対し、複数の manual input source を同時に有効化してはならない。

---

### 5.2 車両状態要求

各車両は以下の状態を持つこと。

- position
- yaw
- velocity

その状態は ROS 標準の `nav_msgs/Odometry` で publish されること。

#### 5.2.1 frame
- `header.frame_id = "odom"`
- `child_frame_id = "carX/base_link"`

#### 5.2.2 TF
各車両は `odom -> carX/base_link` を broadcast すること。

---

### 5.3 race command 要求

システムはレース操作として以下を扱うこと。

- START
- STOP
- RESET

これらは独自 message `RaceCommand` で扱う。

#### 5.3.1 START
- `READY -> COUNTDOWN` の遷移開始

#### 5.3.2 STOP
- `COUNTDOWN` または `RUNNING` 中のレースを停止
- `STOPPED` に遷移

#### 5.3.3 RESET
- race state の初期化
- lap 情報の初期化
- vehicle_sim の初期姿勢復帰

---

### 5.4 race state 要求

システムはレース全体の状態として以下を扱うこと。

- READY
- COUNTDOWN
- RUNNING
- FINISHED
- STOPPED

#### 5.4.1 state 遷移
- `READY --START--> COUNTDOWN`
- `COUNTDOWN --timeout--> RUNNING`
- `RUNNING --finish--> FINISHED`
- `COUNTDOWN/RUNNING --STOP--> STOPPED`
- `STOPPED/FINISHED --RESET--> READY`

#### 5.4.2 elapsed_time
- `RUNNING` 開始時から加算
- `COUNTDOWN` は含めない
- `FINISHED` / `STOPPED` で停止

---

### 5.5 ラップ要求

#### 5.5.1 ラップ成立条件
ラップは以下を全て満たしたときのみ成立する。

1. 車両移動線分が `start_line` を横切る
2. `forward_hint` と進行方向が整合する
3. `lap_cooldown_sec` 以上経過している
4. `race_status == RUNNING`
5. `has_finished == false`
6. odom を受信済みである

#### 5.5.2 ラップ情報
各車両は以下を管理する。

- lap_count
- current_lap_time
- last_lap_time
- best_lap_time
- total_elapsed_time
- has_finished

#### 5.5.3 イベント
ラップ成立時には `LapEvent` を publish する。

---

### 5.6 off-track 要求

#### 5.6.1 判定
車両位置と centerline の最近点距離が `track_width / 2` を超えた場合、off-track とみなす。

#### 5.6.2 状態
各車両は以下を管理する。

- `is_off_track`
- `off_track_count`

#### 5.6.3 カウント条件
`off_track_count` は `inside -> outside` の遷移時のみ加算する。

#### 5.6.4 初版の扱い
初版では off-track しても走行は継続する。失格・停止・ペナルティは与えない。

---

### 5.7 track 要求

track は YAML 形式の file で定義する。

最低限必要な項目は以下。

- track_name
- centerline
- track_width
- start_line
- forward_hint

#### 5.7.1 centerline
- 2D waypoint 列
- 等間隔不要
- sparse すぎる定義は禁止

#### 5.7.2 start_line
- 2 点で定義される線分

#### 5.7.3 forward_hint
- intended travel direction を表すベクトル

#### 5.7.4 track_width
- 固定幅

---

### 5.8 map / track / scenario 要求

本システムは以下を分離して扱うこと。

#### 5.8.1 Map
- 外部環境地図
- 例: OpenDRIVE
- `race_map` が扱う

#### 5.8.2 Track
- レース論理コース
- centerline / start_line / forward_hint / track_width
- `race_track` が扱う

#### 5.8.3 Scenario
- 実行条件
- 初期姿勢
- controller割り当て
- race settings

---

### 5.9 AI 要求

#### 5.9.1 入力
AI driver は以下を入力とする。

- `/carX/odom`
- `/race/state`
- track file（`race_track` ライブラリ経由）

#### 5.9.2 出力
- `/carX/cmd_drive`

#### 5.9.3 controller
初版で扱う controller は以下。

- `pure_pursuit`
- `pid_heading`

#### 5.9.4 挙動
- `RUNNING` のときのみ制御
- それ以外は中立入力を出す

#### 5.9.5 target point
- centerline 最近点を毎周期取り直す
- lookahead point を用いる

---

### 5.10 renderer 要求

#### 5.10.1 役割
renderer は初版における軽量なデバッグ・観測用 frontend であり、表示専用とする。
renderer は race logic を持たず、見た目の完成度や演出性は初版の目的に含めない。


#### 5.10.2 表示内容
- centerline
- track_width を持ったコース
- start_line
- 各車両の位置・向き
- race state
- elapsed_time
- lap 情報
- off-track 状態

#### 5.10.3 入力
- `/carX/odom`
- `/race/state`
- `/race/vehicle_status`
- `/race/lap_event`
- track file

#### 5.10.4 継続表示の真実のソース
- `VehicleRaceStatus`

#### 5.10.5 イベント表示
- `LapEvent` は一時表示・通知用

---

## 6. 非機能要求

### 6.1 拡張性
システムは以下に拡張可能であること。

- simulator backend 差し替え
- CARLA 連携
- external map asset 読み込み
- sensor topic 追加
- controller 追加

### 6.2 simulator-independent
以下は simulator backend 非依存であること。

- race_manager
- ai_driver
- custom messages
- track / scenario の概念
- race command / race state

### 6.3 可読性
仕様書と実装は、README や docs に転用できる程度に整理されていること。

### 6.4 再利用性
track は topic ではなく file + shared library で扱うこと。

### 6.5 テスト容易性
実装は段階的に build / run / inspect 可能であること。

### 6.6 時間源の一貫性
全ノードの時間計算は ROS clock を唯一の正とすること。
- lap timing
- countdown
- cooldown
- elapsed_time
はすべて ROS time に基づいて計算する。
wall time や独自 clock を race logic に混在させてはならない。

### 6.7 責務境界の維持
初版実装では以下の責務境界を崩してはならない。
- vehicle backend は車両運動と車両状態 publish に限定する
- race_manager はレース進行判定と状態更新に限定する
- ai_driver は制御入力生成に限定する
- renderer は表示専用とする

---

## 7. 外部地図資産要求

### 7.1 主形式
外部地図資産の主形式は OpenDRIVE を第一候補とする。

### 7.2 位置づけ
OpenDRIVE は physical/environment map であり、logical race track とは分離する。

### 7.3 map adaptation
将来、OpenDRIVE から以下を抽出または定義できるようにする。

- centerline
- start_line
- forward_hint
- track_width
- lane 関連情報

### 7.4 前提
外部地図資産は、そのまま race logic 用入力として使えるとは限らない。  
必要に応じて `race_map` による読み込み・正規化・変換を行う。

---

## 8. 完成条件（Acceptance Criteria）

### 8.1 システム起動
- workspace build 成功
- launch でシステム起動可能

### 8.2 車両状態
- `/carX/odom` が publish される
- `odom -> carX/base_link` が broadcast される

### 8.3 race logic
- START / STOP / RESET が動作する
- RaceState が正しく遷移する
- lap 判定が成立する
- best lap が更新される

### 8.4 off-track
- `is_off_track` と `off_track_count` が動く

### 8.5 track
- YAML track file が読める
- validation が効く

### 8.6 AI
- Pure Pursuit で周回可能
- PID heading が controller として切り替え可能

### 8.7 renderer
- 車両位置と race 状態が可視化できる

---

## 9. 実装順の要求

実装は以下の順で進めること。

1. `race_interfaces`
2. `race_track`
3. `vehicle_sim_node`
4. `race_manager_node`
5. `gamepad_input_node` / `keyboard_input_node` / `race_command_input_node`
6. `renderer_node`
7. `ai_driver_node`

各段階で、動作確認可能な最小システムを保つこと。

---

## 10. パッケージ構成要求

想定パッケージ構成

- `race_interfaces`
- `race_track`
- `race_map`
- `race_vehicle`
- `race_ai`
- `race_manager`
- `race_renderer`
- `race_input`
- `race_bringup`
- `race_carla_bridge`

---

## 11. 設計上の重要な前提

- `vehicle_sim_node` は初版の internal backend である
- 将来は `race_carla_bridge` に置換可能である
- renderer は暫定 frontend である
- simulator 依存コードは bridge に隔離する
- controller は `/carX/cmd_drive` に統一する
- vehicle state は `/carX/odom` に統一する
- `vehicle_id` と namespace と frame 名は一致させる

---

## 12. 本要件定義の位置づけ

本書は、以下の文書の上流に位置する。

- 仕様書
- アーキテクチャ設計
- 実装タスク分解
- GitHub Issues

本書で定めた要求を基準として、以降の設計と実装を行う。

---

## 初版開発ポリシー
本プロジェクトの初版では、「軽量な internal backend により、ROS2 システム設計を成立させること」を最優先とする。

そのため、以下を厳守する。
- 動く最小構成を優先する
- 各段階で build / run / inspect 可能な状態を維持する
- 将来拡張のための過剰実装を行わない
- 未使用の抽象層や plugin 機構を先行導入しない
- renderer の高機能化を優先しない
- race logic / vehicle dynamics / controller / visualization の責務境界を崩さない
- 初版 acceptance criteria を満たすまで、将来機能には着手しない


