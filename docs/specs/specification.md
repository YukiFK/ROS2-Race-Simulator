# ROS2 Race Simulator
## System Specification v1.3

---

## 1. 文書の目的

本書は、ROS2 Race Simulator のシステム仕様を定義する文書である。  
本システムは、ROS2 の学習、自動運転アルゴリズムの比較、将来的な CARLA 連携を見据えた simulator-independent な実験基盤として設計される。

本書は、以下を明確にすることを目的とする。

- システム全体構造
- ノード構成
- トピックとメッセージ
- レースロジック
- トラック定義
- マップ／トラック／シナリオの責務分離
- 初版 backend と将来 backend の関係
- 実装時に守るべき設計制約

---

## 2. システム概要

本システムは、複数車両が同一トラック上で走行する ROS2 ベースのレースシミュレータである。  
各車両は手動操作または AI 操作を受け、レース全体は専用の race manager によって制御される。

初版では以下を提供する。

- 軽量な internal vehicle backend
- 手動操作車両
- AI 操作車両
- レース状態管理
- ラップ計測
- コースアウト判定
- 軽量 2D renderer

将来的には以下を想定する。

- CARLA backend
- OpenDRIVE を主とする外部地図資産利用
- LiDAR / Radar / Camera
- perception / planning / control への拡張

---
## 2.5 Initial Success Criteria
初版の成功は、以下の最小条件を満たすこととする。

1. 1台の車両で `DriveCommand -> vehicle_sim -> Odometry / TF` が成立する
2. `RaceCommand` により `READY / COUNTDOWN / RUNNING / STOPPED / FINISHED` が遷移する
3. lap 判定と off-track 判定が動作する
4. renderer で track / 車両位置 / race state / lap 情報を確認できる
5. Pure Pursuit により AI 車両が 1 周以上継続走行できる

上記を満たすまでは、将来拡張機能の追加を行わない。

---

## 3. システム全体構造

本システムは以下のレイヤ構造で構成される。

~~~text
Controller Layer
    │
    ▼
Drive Command Interface
    │
    ▼
Vehicle Simulation Backend
    │
    ▼
Vehicle State Interface
    │
    ▼
Race Logic Layer
    │
    ▼
Visualization Layer
~~~

### 3.1 Controller Layer

各車両の入力源を担当する。

例:
- `gamepad_input_node`
- `keyboard_input_node`
- `ai_driver_node`

これらはすべて `DriveCommand` を生成する。

### 3.2 Vehicle Simulation Backend

車両運動を計算し、車両状態を ROS topic に変換して publish する。

初版:
- `vehicle_sim_node`

将来:
- `race_carla_bridge` + `CARLA`

### 3.3 Race Logic Layer

レース進行を管理する。

担当:
- RaceState 管理
- ラップ判定
- タイム計測
- コースアウト判定
- 車両別レース状態の生成

### 3.4 Visualization Layer

人間がレース状態を観測するための表示層。

初版:
- `renderer_node`

将来:
- simulator-native visualization
- CARLA 側可視化
- そのほか外部 frontend

---

## 4. ノード構成

本システムで想定する初版ノードは以下の通り。

- `vehicle_sim_node`
- `ai_driver_node`
- `race_manager_node`
- `renderer_node`
- `gamepad_input_node`
- `keyboard_input_node`
- `race_command_input_node`

将来追加想定:
- `race_carla_bridge`

---

## 5. Vehicle Namespace Policy

各車両は固有の namespace を持つ。

例:
- `/car1`
- `/car2`
- `/car3`

### 5.1 車両固有 topic

各車両 namespace 配下に、以下の車両固有 topic を配置する。

- `/carX/cmd_drive`
- `/carX/odom`

### 5.2 車両固有 frame

各車両の frame 名は `vehicle_id` を接頭辞に持つ。

例:
- `car1/base_link`
- `car2/base_link`

### 5.3 命名制約

`vehicle_id` は以下と一致させる。

- namespace 名
- status message 内の vehicle_id
- frame 名の接頭辞

例:
- `vehicle_id = car2`
- namespace = `/car2`
- frame = `car2/base_link`

### 5.4 race-wide topic

レース全体に関わる topic は `/race/...` 配下に配置する。

- `/race/command`
- `/race/state`
- `/race/lap_event`
- `/race/vehicle_status`

---

## 6. Topic Interface

### 6.1 topic 一覧

| Topic | Type | Publisher | Subscriber | Purpose |
|------|------|-----------|-----------|---------|
| `/carX/cmd_drive` | `race_interfaces/DriveCommand` | `gamepad_input_node` / `keyboard_input_node` / `ai_driver_node` | `vehicle_sim_node` / 将来 `race_carla_bridge` | 車両制御入力 |
| `/carX/odom` | `nav_msgs/Odometry` | `vehicle_sim_node` / 将来 `race_carla_bridge` | `race_manager_node` / `ai_driver_node` / `renderer_node` | 車両状態 |
| `/race/command` | `race_interfaces/RaceCommand` | `race_command_input_node` / 将来 GUI | `race_manager_node` / `vehicle_sim_node` | レース制御 |
| `/race/state` | `race_interfaces/RaceState` | `race_manager_node` | `ai_driver_node` / `renderer_node` | レース全体状態 |
| `/race/lap_event` | `race_interfaces/LapEvent` | `race_manager_node` | `renderer_node` / 将来 logger | ラップイベント |
| `/race/vehicle_status` | `race_interfaces/VehicleRaceStatus` | `race_manager_node` | `renderer_node` | 車両ごとのレース状態 |

### 6.2 TF

初版では以下の動的 TF を扱う。

- `odom -> carX/base_link`

publisher:
- 初版: `vehicle_sim_node`
- 将来: `race_carla_bridge`

静的 TF は初版では不要だが、将来センサ導入時に以下を想定する。

- `carX/base_link -> carX/lidar`
- `carX/base_link -> carX/camera`

---

## 7. Message Definitions

### 7.1 DriveCommand.msg

車両制御入力を表す。

~~~text
std_msgs/Header header

float32 throttle_cmd
float32 steering_cmd
~~~

#### 意味
- `throttle_cmd ∈ [-1.0, 1.0]`
- `steering_cmd ∈ [-1.0, 1.0]`

#### 設計意図
- 手動入力でも AI 入力でも共通の message を使う
- controller の種類に依存しない
- 物理量変換は backend 側で行う

---

### 7.2 RaceCommand.msg

レース操作コマンドを表す。

~~~text
std_msgs/Header header

uint8 command

uint8 START=0
uint8 STOP=1
uint8 RESET=2
~~~

#### 意味
- `START`: レース開始
- `STOP`: レース停止
- `RESET`: race state / vehicle state 初期化

#### 設計意図
- typo を防ぐため文字列ではなく定数付き `uint8` を使う
- GUI / keyboard / external frontend から統一的に使える

---

### 7.3 RaceState.msg

レース全体状態を表す。

~~~text
std_msgs/Header header

string race_status
builtin_interfaces/Duration elapsed_time

int32 total_laps
~~~

#### race_status
以下の固定値を取る。

- `READY`
- `COUNTDOWN`
- `RUNNING`
- `FINISHED`
- `STOPPED`

#### elapsed_time
- `RUNNING` 開始時から加算
- `COUNTDOWN` は含めない
- `FINISHED` / `STOPPED` で停止

#### total_laps
- 規定周回数

---

### 7.4 LapEvent.msg

ラップ成立時に発行されるイベント。

~~~text
std_msgs/Header header

string vehicle_id

int32 lap_count

builtin_interfaces/Duration lap_time
builtin_interfaces/Duration best_lap_time

bool has_finished
~~~

#### 意味
- `vehicle_id`: ラップ成立車両
- `lap_count`: 確定したラップ数
- `lap_time`: 今回ラップ時間
- `best_lap_time`: その時点のベスト
- `has_finished`: このラップで完走したか

#### 用途
- renderer の一時演出
- logger / analytics 用のイベント通知

---

### 7.5 VehicleRaceStatus.msg

車両ごとの継続レース状態を表す。

~~~text
std_msgs/Header header

string vehicle_id

int32 lap_count

builtin_interfaces/Duration current_lap_time
builtin_interfaces/Duration last_lap_time
builtin_interfaces/Duration best_lap_time
builtin_interfaces/Duration total_elapsed_time

bool has_finished
bool is_off_track

int32 off_track_count
~~~

#### 意味
- `lap_count`: 完了ラップ数
- `current_lap_time`: 現在ラップ経過
- `last_lap_time`: 直前ラップ
- `best_lap_time`: 最速ラップ
- `total_elapsed_time`: `RUNNING` 開始からの累積時間
- `has_finished`: 完走済みか
- `is_off_track`: 現在コース外か
- `off_track_count`: 内→外 遷移回数

#### 制約
この message は **レース進行状態のみ**を表す。  
以下は含めない。

- 車速
- 操作入力
- controller 内部状態
- 車両運動状態

それらは `Odometry` や config を参照する。

---

### 7.6 Track.msg を作らない方針

初版では `Track.msg` は定義しない。

#### 理由
- track は file + shared library (`race_track`) で扱う
- track topic 配信は初版では不要
- track provider node は作らない

---

## 8. 座標系と時間源

### 8.1 座標系

ROS 側の world / vehicle 状態表現は ROS REP-103 / REP-105 に従う。  
初版では、RaceManager や AI Driver は ROS座標系を正として扱う。

将来、外部 simulator（CARLA など）との座標差異は adapter / bridge 層で吸収する。

#### 初版 frame
- world-like frame: `odom`
- vehicle frame: `carX/base_link`

### 8.2 時間源

全 ROS ノードは **ROS clock** を正とする。

- `node.get_clock().now()` を使用する
- wall time / sim time は `use_sim_time` により切替可能
- レースロジックは ROS 時間のみを参照する

将来、外部 simulator が独自時間を持つ場合は bridge / adapter 層で整合させる。

---

## 9. Track Definition

レース論理コースは track file (YAML) により定義する。

### 9.1 必須項目

- `track_name`
- `centerline`
- `track_width`
- `start_line`
- `forward_hint`

### 9.2 例

~~~yaml
track_name: example_track

centerline:
  - [0.0, 0.0]
  - [10.0, 0.0]
  - [20.0, 5.0]

track_width: 6.0

start_line:
  p1: [0.0, -3.0]
  p2: [0.0, 3.0]

forward_hint: [1.0, 0.0]
~~~

### 9.3 centerline
- 2D waypoint 列
- 等間隔不要
- closed loop 可
- 極端に疎な点列は禁止

### 9.4 track_width
- 固定幅
- centerline の両側に corridor を持つ

### 9.5 start_line
- lap 判定用の線分
- `p1`, `p2` の 2 点で表す

### 9.6 forward_hint
- intended travel direction を示すベクトル
- lap の正方向判定に利用する

---

## 10. Map / Track / Scenario Separation

本システムでは以下を明確に分離する。

### 10.1 Physical / Environment Map
外部環境地図を指す。

例:
- OpenDRIVE
- simulator map
- 将来の competition-provided map

担当:
- `race_map`

### 10.2 Logical Race Track
レースや評価に使う論理コースを指す。

内容:
- centerline
- start_line
- forward_hint
- track_width

担当:
- `race_track`

### 10.3 Scenario Configuration
実行条件を指す。
初版では scenario は複雑な editor や GUI を持たず、YAML file による最小設定とする。

最低限含む内容:
- total_laps
- 車両初期配置
- 手動 / AI の割り当て
- controller_type
- input source
- race settings

### 10.4 重要な設計原則
外部 map は、そのまま race logic 用 track と同一視しない。  
必要に応じて map adaptation によって logical track を生成する。

---

## 11. race_track Library Responsibilities

`race_track` は shared library として以下を提供する。

- track YAML parse
- validation
- TrackModel の生成
- centerline 最近点計算
- point-to-centerline 距離計算
- lookahead point 計算補助
- start_line 交差判定補助
- `forward_hint` / `track_width` / `start_line` の取得

### 11.1 TrackModel
最低限保持する情報:

- `track_name`
- `centerline`
- `track_width`
- `start_line`
- `forward_hint`

### 11.2 使用ノード
以下のノードが `race_track` を利用する。

- `ai_driver_node`
- `race_manager_node`
- `renderer_node`

---

## 12. race_map Responsibilities

`race_map` は外部地図資産の読み込み・正規化を担当する。  
初版では未実装でもよいが、将来 OpenDRIVE を主形式として扱うための責務をここに集約する。

想定機能:
- external map asset 読み込み
- OpenDRIVE parse
- map normalization
- logical track 生成支援
- lane / centerline 抽出支援

### 12.1 主形式
外部地図資産の主形式は **OpenDRIVE** を第一候補とする。

### 12.2 方針
将来、以下を可能にする。

- OpenDRIVE → logical race track
- simulator map → logical race track
- competition-provided map → logical race track

---

## 13. Simulation Backend Policy

### 13.1 初版 backend
初版の backend は `vehicle_sim_node` とする。

### 13.2 `vehicle_sim_node` の位置づけ
`vehicle_sim_node` は、初版における **軽量 internal vehicle simulation backend** である。  
開発・学習・初期検証のために用いる。

### 13.3 将来 backend
将来的には `race_carla_bridge` を導入し、外部 simulator backend（CARLA）へ置換可能な構造とする。

### 13.4 backend 非依存の契約
上位ロジックは以下の I/F のみに依存する。

- `/carX/cmd_drive`
- `/carX/odom`
- `odom -> carX/base_link`

これにより、internal backend と future backend を差し替え可能にする。

### 13.5 初版の抽象化方針
初版では backend 差し替え可能性を設計上は維持するが、未使用 backend のための過剰な抽象層は導入しない。

原則:
- interface 契約は固定する
- internal backend は単純な実装で成立させる
- 実際に 2つ以上の実装が必要になるまで汎用 plugin 機構は導入しない
- class 分離は責務分離のために行い、将来用途だけを理由に増やしすぎない

---

### 13.5 初版の抽象化方針
初版では backend 差し替え可能性を設計上は維持するが、未使用 backend のための過剰な抽象層は導入しない。
具体的には以下を原則とする。
- interface 契約は固定する
- internal backend は単純な実装で成立させる
- 実際に 2つ以上の実装が必要になるまで汎用 plugin 機構は導入しない
- class 分離は責務分離のために行い、将来用途だけを理由に増やしすぎない

---

## 14. vehicle_sim_node Specification

### 14.1 役割
1台の車両の運動状態をシミュレーションし、ROS標準形式で状態を publish する。

### 14.2 入力
- `/carX/cmd_drive`
- `/race/command`（主に `RESET` を処理）

### 14.3 出力
- `/carX/odom`
- `odom -> carX/base_link`

### 14.4 車両モデル
- Kinematic Bicycle Model

### 14.5 パラメータ
- `vehicle_id`
- `wheelbase_m`
- `max_steering_angle_rad`
- `max_acceleration_mps2`
- `max_deceleration_mps2`
- `max_speed_mps`
- `update_rate_hz`
- `cmd_timeout_sec`
- `initial_pose_x`
- `initial_pose_y`
- `initial_pose_yaw`

### 14.6 確定値
- `max_speed_mps = 22.22`（80 km/h）

### 14.7 command timeout
一定時間 `DriveCommand` を受信しなければ中立入力に戻す。

### 14.8 RESET
`RESET` を受信したら以下を初期化する。

- pose
- yaw
- velocity
- steering_angle
- current command

### 14.9 設計制約
内部責務を分離する。

例:
- `VehicleDynamics`
- `DriveCommandAdapter`
- `OdometryPublisher`
- `TfPublisher`

将来的に以下へ拡張しやすい構造とする。

- 加速度一次遅れ
- 操舵速度制限
- より高次の vehicle model

---

## 15. race_manager_node Specification

### 15.1 役割
レース全体の進行と、各車両のラップ・タイム・off-track 状態を管理する。

### 15.2 入力
- `/race/command`
- `/carX/odom`
- track file（`race_track` 経由）

### 15.3 出力
- `/race/state`
- `/race/lap_event`
- `/race/vehicle_status`

### 15.4 RaceState
- `READY`
- `COUNTDOWN`
- `RUNNING`
- `FINISHED`
- `STOPPED`

### 15.5 RaceCommand
- `START`
- `STOP`
- `RESET`

### 15.6 状態遷移
- `READY --START--> COUNTDOWN`
- `COUNTDOWN --timeout--> RUNNING`
- `RUNNING --finish--> FINISHED`
- `COUNTDOWN/RUNNING --STOP--> STOPPED`
- `STOPPED/FINISHED --RESET--> READY`

### 15.7 elapsed_time
- `RUNNING` 開始から加算
- `COUNTDOWN` は含めない

### 15.8 ラップ判定
以下を全て満たすときのみラップ成立。

1. 移動線分が `start_line` と交差
2. `dot(move_vec, forward_hint) > 0`
3. cooldown 超過
4. `race_status == RUNNING`
5. `has_finished == false`
6. odom 受信済み

### 15.9 off-track 判定
- centerline 最近点距離で判定
- `distance > track_width / 2` で off-track
- `inside -> outside` の遷移時のみ count を増やす

### 15.10 publish policy
- `RaceState`: 毎周期
- `VehicleRaceStatus`: 毎周期
- `LapEvent`: ラップ成立時のみ

### 15.11 設計制約
- race_manager は判定専用
- 表示用文字列整形を持たない
- renderer が使う継続状態の真実は `VehicleRaceStatus`

---

## 16. ai_driver_node Specification

### 16.1 役割
1台の AI 車両に対して、自動走行用 `DriveCommand` を生成する。

### 16.2 入力
- `/carX/odom`
- `/race/state`
- track file（`race_track` 経由）

### 16.3 出力
- `/carX/cmd_drive`

### 16.4 対応 controller
- `pure_pursuit`
- `pid_heading`

### 16.5 方針
- 初版の主 controller は `pure_pursuit`
- `pid_heading` は比較・学習用途
- 将来 `stanley`, `mpc` を追加可能な構造とする

### 16.6 target point
- 毎周期 centerline 最近点を再計算
- そこから lookahead point を選ぶ

### 16.7 race state との関係
- `RUNNING` のときのみ制御
- `READY`, `COUNTDOWN`, `FINISHED`, `STOPPED` では中立入力

### 16.8 速度制御
共通 speed controller を使う。

- `target_speed`
- `speed_kp`
- steering 量に応じた減速

### 16.9 設計制約
- simulator-independent
- lap 判定を持たない
- off-track 判定を持たない
- 他車回避を持たない

### 16.10 内部構造
例:
- `TrackFollower`
- `TargetPointSelector`
- `ControllerBase`
  - `PurePursuitController`
  - `PidHeadingController`
- `SpeedController`

### 16.11 初版実装優先順位
初版 AI の成立条件は `pure_pursuit` による安定周回である。
`pid_heading` は比較・学習用途の副 controller とし、`pure_pursuit` が成立するまで追加改善を優先しない。


---

## 17. input nodes Specification

### 17.1 gamepad_input_node
役割:
- gamepad から `DriveCommand` を生成する

対象:
- 手動操作車両

### 17.2 keyboard_input_node
役割:
- キーボード入力から `DriveCommand` を生成する

用途:
- fallback / debug

### 17.3 race_command_input_node
役割:
- `RaceCommand` を publish する

対応:
- START
- STOP
- RESET

### 17.4 設計制約
- 車両操作入力と race 操作入力は分離する
- manual input source は同一車両に対して 1つだけ有効

---

## 18. renderer_node Specification

### 18.1 役割
レースシステムの状態を 2D GUI として可視化する。

### 18.2 入力
- `/carX/odom`
- `/race/state`
- `/race/vehicle_status`
- `/race/lap_event`
- track file（`race_track` 経由）

### 18.3 出力
初版では必須出力なし。

### 18.4 表示内容
- centerline
- track width を持つ道路
- start_line
- 車両位置
- 車両向き
- vehicle_id
- race_state
- elapsed_time
- lap 情報
- off-track 情報
- LapEvent の一時表示

### 18.5 設計制約
- renderer は表示専用
- race logic を持たない
- 判定ロジックを持たない
- 継続表示の真実は `VehicleRaceStatus`
- `LapEvent` は一時通知用

### 18.6 位置づけ
`renderer_node` は初版の **暫定可視化 frontend** であり、将来的には simulator-native visualization 等へ置換可能である。

---

## 19. race_carla_bridge Specification (Future)

### 19.1 役割
CARLA 固有の状態・センサ・制御を ROS2 システム標準 I/F に変換する adapter / bridge とする。

### 19.2 入力
- `/carX/cmd_drive`
- CARLA vehicle state
- 将来の CARLA sensor streams

### 19.3 出力
- `/carX/odom`
- TF
- 将来の sensor topics

### 19.4 設計意図
上位ロジック (`race_manager`, `ai_driver`) を simulator 非依存に保つ。

---

## 20. Future Sensor Topics (Reserved)

初版では未実装だが、将来追加可能な topic として以下を想定する。

- `/carX/points_raw`
- `/carX/radar`
- `/carX/camera/image_raw`
- `/carX/camera/camera_info`

これらは CARLA 連携フェーズで扱う。

---

## 21. 実装順

推奨実装順は以下の通り。

1. `race_interfaces`
2. `race_track`
3. `vehicle_sim_node`
4. `race_manager_node`
5. `gamepad_input_node` / `keyboard_input_node` / `race_command_input_node`
6. `renderer_node`
7. `ai_driver_node`

各段階で、システムが build / run / inspect 可能であることを重視する。

---

## 22. 受け入れ条件（Acceptance Criteria）

### 22.1 起動
- 全パッケージが build できる
- launch により初版システムを起動できる

### 22.2 vehicle state
- `/carX/odom` が publish される
- `odom -> carX/base_link` が broadcast される

### 22.3 race logic
- START / STOP / RESET が動作する
- RaceState が正しく遷移する
- lap が数えられる
- best lap が更新される

### 22.4 off-track
- `is_off_track` と `off_track_count` が動く

### 22.5 renderer
- 車両位置と race 状態が表示される

### 22.6 AI
- `pure_pursuit` で周回可能
- `pid_heading` を controller として切り替え可能

---

## 23. パッケージ構成

想定パッケージ構成は以下。

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


## 24. 本仕様書の位置づけ

本書は、以下の実装・資料の基準となる。

- Architecture Design
- Implementation Task Breakdown
- GitHub Issues
- README / docs
- future CARLA integration design

本書に定義された interface と責務分離を維持することを、開発上の前提とする。

