# ROS2 Race Simulator
## Architecture Design

---

## 1. 文書の目的

本書は、ROS2 Race Simulator のアーキテクチャ設計を定義する文書である。  
要件定義および仕様書に基づき、以下を明確化することを目的とする。

- システム全体の責務分割
- ノード間の関係
- topic / message を介したデータフロー
- backend と上流ロジックの分離方針
- map / track / scenario の境界
- 将来の CARLA 拡張を見据えた adapter / bridge 方針
- 実装時に崩してはいけない設計原則

本書は、実装前にシステム構造を固定し、実装時の責務逸脱や構造破綻を防ぐための設計文書である。

---

## 2. 設計の基本方針

本システムは、**simulator-independent な ROS2 システム**として設計する。

初版では軽量な internal simulator backend を使うが、将来的に CARLA backend に置換できることを前提とする。

### 2.1 設計方針

- controller は simulator backend に依存しない
- race logic は simulator backend に依存しない
- visualization は race logic に依存しない
- simulator 固有コードは bridge / adapter 層に隔離する
- track は file + shared library で扱う
- external map asset は track と分離して扱う
- vehicle ごとに namespace を分ける
- レース全体情報は `/race/...` に集約する

### 2.2 重要な原則

本システムのアーキテクチャでは、以下を強く守る。

1. **車両制御 I/F の固定**
   - `/carX/cmd_drive`

2. **車両状態 I/F の固定**
   - `/carX/odom`
   - `odom -> carX/base_link`

3. **レース管理 I/F の固定**
   - `/race/command`
   - `/race/state`
   - `/race/lap_event`
   - `/race/vehicle_status`

4. **backend 差し替え可能性の確保**
   - internal backend でも CARLA backend でも、上流には同じ interface を見せる

---

## 2.3 Node Internal Design Principle
各ノードは ROS2 I/O と内部ロジックを分離することを原則とする。

### 2.3.1 ROS 依存部
ROS 依存部は以下に限定する。
- publisher / subscriber
- parameter 読み込み
- timer
- clock 取得
- TF broadcast
- launch / lifecycle に関わる処理

### 2.3.2 ROS 非依存部
計算ロジックは可能な限り ROS 非依存の class / function として分離する。
例:
- VehicleDynamics
- TrackGeometry
- LapDetector
- OffTrackChecker
- ControllerBase
- PurePursuitController

### 2.3.3 目的
この分離により以下を実現する。
- 単体テスト容易性
- simulator backend 差し替え時の再利用性
- ROS ノード肥大化の防止


---
## 3. システム全体構造

本システムは、概念的に以下のレイヤ構造で構成される。

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

各車両に対して制御入力を生成する層。

#### 構成要素
- `gamepad_input_node`
- `keyboard_input_node`
- `ai_driver_node`

#### 出力
- `/carX/cmd_drive`

#### 特徴
- 手動入力と AI 入力の source を分離する
- controller 種別が変わっても `DriveCommand` の message 契約は変えない
- 同一車両について manual input source は 1つだけ有効にする

---

### 3.2 Drive Command Interface

全ての車両制御入力を統一的に扱う I/F。

#### topic
- `/carX/cmd_drive`

#### message
- `race_interfaces/DriveCommand`

#### 目的
- 手動操作と AI 操作を統一
- vehicle backend を差し替えても制御インターフェースを保つ
- 将来、Stanley / MPC / external control node を追加しやすくする

---

### 3.3 Vehicle Simulation Backend

車両運動を計算し、ROS2 上の標準化された車両状態を生成する層。

#### 初版 backend
- `vehicle_sim_node`

#### 将来 backend
- `race_carla_bridge` + `CARLA`

#### 標準出力
- `/carX/odom`
- `odom -> carX/base_link`

#### 目的
- controller から受けた DriveCommand を車両状態へ変換する
- 上流ロジックに simulator 非依存の車両状態を提供する

---

### 3.4 Vehicle State Interface

車両状態を表す共通 I/F。

#### topic
- `/carX/odom`

#### message
- `nav_msgs/Odometry`

#### TF
- `odom -> carX/base_link`

#### 目的
- race logic
- AI control
- renderer

が共通の状態 source を扱えるようにする

---

### 3.5 Race Logic Layer

レース全体の進行を管理する層。

#### 主担当ノード
- `race_manager_node`

#### 扱う機能
- RaceState の管理
- Lap 判定
- elapsed time 計測
- off-track 判定
- `VehicleRaceStatus` の生成
- `LapEvent` の生成

#### 特徴
- 表示都合を持たない
- backend に依存しない
- track は shared library から読む
- simulator 固有 topic を直接読まない

---

### 3.6 Visualization Layer

状態を可視化する層。

#### 初版
- `renderer_node`

#### 将来
- simulator-native visualization
- CARLA の可視化系
- 他 frontend

#### 特徴
- 表示専用
- race logic を持たない
- 継続状態は `VehicleRaceStatus` を真とする
- 一時イベントは `LapEvent` を使う

---

## 4. ノード一覧と責務

### 4.1 `vehicle_sim_node`

#### 役割
1台の車両の簡易運動シミュレーションを行う internal backend。

#### 主責務
- `DriveCommand` の購読
- command timeout 処理
- Kinematic Bicycle Model による状態更新
- `/carX/odom` publish
- `odom -> carX/base_link` TF broadcast
- `RESET` による初期化

#### やらないこと
- lap 判定
- off-track 判定
- race state 管理
- 表示

#### 内部設計原則
`vehicle_sim_node` は ROS node であるが、車両運動計算や入力変換は ROS 非依存 class に分離する。
例:
- `VehicleDynamics`
- `DriveCommandAdapter`
- `VehicleState`
- `OdometryConverter`


#### アーキテクチャ上の位置づけ
初版の internal backend であり、将来的には external backend に置換可能なコンポーネント。

---

### 4.2 `ai_driver_node`

#### 役割
1台の AI 車両に対して、自動走行用 `DriveCommand` を生成する。

#### 主責務
- `/carX/odom` 購読
- `/race/state` 購読
- track の読み込み
- target point 選択
- steering 計算
- speed 制御
- `/carX/cmd_drive` publish

#### 対応 controller
- `pure_pursuit`
- `pid_heading`

#### やらないこと
- lap 判定
- off-track 判定
- race state 遷移
- simulator 固有 API 利用

#### アーキテクチャ上の位置づけ
simulator-independent な controller 層の一部。

---

### 4.3 `race_manager_node`

#### 役割
レース全体の進行を管理する central logic node。

#### 主責務
- `/race/command` 購読
- `/carX/odom` 購読
- RaceState 管理
- Lap 判定
- elapsed time 計測
- off-track 判定
- `/race/state` publish
- `/race/lap_event` publish
- `/race/vehicle_status` publish

#### やらないこと
- 車両運動計算
- controller 計算
- 表示レイアウトや UI 文字列加工
- UI 用文字列生成
- simulator 固有 API 呼び出し
- renderer のためだけの補助状態保持


#### アーキテクチャ上の位置づけ
本システムのルールエンジン。

---

### 4.4 `renderer_node`

#### 役割
レース状態を 2D で可視化する暫定 frontend。

#### 主責務
- `/carX/odom` 購読
- `/race/state` 購読
- `/race/vehicle_status` 購読
- `/race/lap_event` 購読
- track の読み込み
- コース描画
- 車両描画
- HUD 表示
- LapEvent の一時表示

#### やらないこと
- lap 判定
- off-track 計算
- RaceState 遷移
- backend 制御

#### アーキテクチャ上の位置づけ
初版におけるデバッグ・観測用 frontend。
本番品質の UI や高機能 GUI を目指さず、状態確認のための最小表示に徹する。
将来的に simulator-native visualization に置換可能である。


---

### 4.5 `gamepad_input_node`

#### 役割
gamepad から DriveCommand を生成する。

#### 主責務
- joystick 読み取り
- 入力正規化
- deadzone 適用
- `/carX/cmd_drive` publish

#### やらないこと
- race command 発行
- race state 管理
- 車両状態計算

---

### 4.6 `keyboard_input_node`

#### 役割
キーボードから DriveCommand を生成する。

#### 主責務
- キー入力取得
- `/carX/cmd_drive` publish

#### 用途
- fallback
- debug

---

### 4.7 `race_command_input_node`

#### 役割
レース全体制御コマンドを生成する。

#### 主責務
- START
- STOP
- RESET
の発行

#### 出力
- `/race/command`

#### 設計意図
車両操作入力とレース操作入力を分離する。

---

### 4.8 `race_carla_bridge`（将来）

#### 役割
CARLA と ROS2 システム標準 I/F の橋渡し。

#### 主責務
- `/carX/cmd_drive` → CARLA vehicle control
- CARLA vehicle state → `/carX/odom`
- CARLA state → TF
- 将来の sensor stream → ROS2 topic

#### アーキテクチャ上の位置づけ
simulator-specific code を隔離する adapter / bridge 層。

---

## 5. Vehicle Namespace Architecture

各車両は独立した namespace を持つ。

### 5.1 目的
- 多車両で topic が衝突しないようにする
- launch で同一ノードを複数起動しやすくする
- vehicle_id と topic / frame の対応を明確にする
- controller 比較をしやすくする

### 5.2 namespace 例

~~~text
/car1
/car2
/car3
~~~

### 5.3 車両固有 topic

~~~text
/carX/cmd_drive
/carX/odom
~~~

### 5.4 車両固有 frame

~~~text
carX/base_link
~~~

### 5.5 race-wide topic

~~~text
/race/command
/race/state
/race/lap_event
/race/vehicle_status
~~~

### 5.6 命名制約
以下は一致させる。

- `vehicle_id`
- namespace
- frame 接頭辞

例:
- `vehicle_id = car2`
- namespace = `/car2`
- frame = `car2/base_link`

---

## 6. Topic / Message アーキテクチャ

### 6.1 車両制御入力

#### topic
- `/carX/cmd_drive`

#### message
- `race_interfaces/DriveCommand`

#### publisher
- `gamepad_input_node`
- `keyboard_input_node`
- `ai_driver_node`

#### subscriber
- `vehicle_sim_node`
- 将来 `race_carla_bridge`

#### 設計意図
すべての controller source を共通 I/F で統一する。

---

### 6.2 車両状態

#### topic
- `/carX/odom`

#### message
- `nav_msgs/Odometry`

#### publisher
- `vehicle_sim_node`
- 将来 `race_carla_bridge`

#### subscriber
- `race_manager_node`
- `ai_driver_node`
- `renderer_node`

#### 設計意図
race logic と AI と renderer の共通状態 source とする。

---

### 6.3 レース制御

#### topic
- `/race/command`

#### message
- `race_interfaces/RaceCommand`

#### publisher
- `race_command_input_node`
- 将来 GUI
- 将来 external frontend

#### subscriber
- `race_manager_node`
- `vehicle_sim_node`

#### 設計意図
レース全体 command を 1本化する。

---

### 6.4 レース全体状態

#### topic
- `/race/state`

#### message
- `race_interfaces/RaceState`

#### publisher
- `race_manager_node`

#### subscriber
- `ai_driver_node`
- `renderer_node`

#### 設計意図
AI と renderer がレース進行状態を共通認識できるようにする。

---

### 6.5 ラップイベント

#### topic
- `/race/lap_event`

#### message
- `race_interfaces/LapEvent`

#### publisher
- `race_manager_node`

#### subscriber
- `renderer_node`
- 将来 logger / analytics

#### 設計意図
event-driven な通知専用 I/F とする。

---

### 6.6 車両ごとのレース状態

#### topic
- `/race/vehicle_status`

#### message
- `race_interfaces/VehicleRaceStatus`

#### publisher
- `race_manager_node`

#### subscriber
- `renderer_node`

#### 設計意図
継続表示の真実の source とする。  
1車両1メッセージを流し、`vehicle_id` で識別する。

---

## 7. Message 設計の考え方

### 7.1 custom message の範囲
custom message は以下に限定する。

- `DriveCommand`
- `RaceCommand`
- `RaceState`
- `LapEvent`
- `VehicleRaceStatus`

### 7.2 標準 message の利用
車両状態は ROS標準 message を使う。

- `nav_msgs/Odometry`

### 7.3 Track.msg を作らない理由
初版では track を topic で流さず、shared library と file で扱うため。

その理由:
- topic と node を増やしすぎない
- 初版の責務を軽く保つ
- 将来必要なら Track.msg を追加できる

---

## 8. Race Logic アーキテクチャ

### 8.1 RaceState Machine

状態:

- `READY`
- `COUNTDOWN`
- `RUNNING`
- `FINISHED`
- `STOPPED`

遷移:

- `READY --START--> COUNTDOWN`
- `COUNTDOWN --timeout--> RUNNING`
- `RUNNING --finish--> FINISHED`
- `COUNTDOWN/RUNNING --STOP--> STOPPED`
- `STOPPED/FINISHED --RESET--> READY`

### 8.2 elapsed_time
- `RUNNING` 開始時から加算
- `COUNTDOWN` は含めない
- `STOPPED` / `FINISHED` で停止

### 8.3 Lap Detection
ラップ成立条件:

1. start_line 交差
2. forward_hint と move_vec が整合
3. cooldown 超過
4. `RUNNING`
5. `has_finished == false`
6. odom 受信済み

### 8.4 Off-track Detection
- centerline 最近点距離で判定
- `distance > track_width / 2`
- `inside -> outside` で `off_track_count += 1`

### 8.5 RaceManager の責務境界
RaceManager が持つ:
- 判定
- 状態更新
- レース進行

RaceManager が持たない:
- vehicle dynamics
- controller logic
- renderer 表示整形
- simulator-specific API

---

## 9. AI Control Architecture

### 9.1 基本構造

`ai_driver_node` は simulator-independent な controller node とする。

入力:
- `/carX/odom`
- `/race/state`
- track file

出力:
- `/carX/cmd_drive`

### 9.2 Controller 切替
初版で対応する controller:
- `pure_pursuit`
- `pid_heading`

将来追加候補:
- `stanley`
- `mpc`

### 9.3 target point selection
- 毎周期最近点を再計算
- lookahead point を選択
- off-track しても復帰しやすい構造にする

### 9.4 internal structure 例

~~~text
TrackFollower
TargetPointSelector
ControllerBase
 ├ PurePursuitController
 └ PidHeadingController
SpeedController
~~~

### 9.5 AI driver がやらないこと
- lap 判定
- off-track 判定
- race state 遷移
- 他車回避
- overtaking
- simulator-specific state handling

---

## 10. Visualization Architecture

### 10.1 renderer の位置づけ
`renderer_node` は初版の暫定 visualization frontend である。

### 10.2 renderer の責務
- course 描画
- vehicle 描画
- HUD 表示
- LapEvent の一時表示

### 10.3 renderer の入力
- `/carX/odom`
- `/race/state`
- `/race/vehicle_status`
- `/race/lap_event`
- track file

### 10.4 renderer の制約
- race logic を持たない
- off-track 判定を行わない
- lap 計算を行わない
- 継続表示の真実は `VehicleRaceStatus`
- `LapEvent` は一時通知に限定

### 10.5 将来の置換性
将来、renderer は以下に置換可能である。

- simulator-native visualization
- CARLA visualization
- external frontend

---

## 11. Track / Map / Scenario アーキテクチャ

### 11.1 なぜ分離するか
外部 map asset と race logic 用 course と runtime 条件は役割が異なるため。

### 11.2 Physical / Environment Map
外部環境地図を表す。

例:
- OpenDRIVE
- simulator map
- competition-provided map

責務:
- `race_map`

### 11.3 Logical Race Track
レースや評価に使う logical course。

保持する情報:
- centerline
- start_line
- forward_hint
- track_width

責務:
- `race_track`

### 11.4 Scenario
runtime 条件を表す。

例:
- 車両配置
- manual / AI 割り当て
- controller_type
- race settings

### 11.5 重要な設計原則
external map を logical track と同一視しない。  
必要に応じて map adaptation によって logical track を生成する。

---

## 12. race_track アーキテクチャ

### 12.1 位置づけ
`race_track` は node ではなく shared library である。

### 12.2 提供機能
- YAML parse
- validation
- TrackModel 生成
- 最近点計算
- distance 計算
- lookahead 計算補助
- start_line 交差判定補助
- `forward_hint`, `track_width`, `start_line` 取得

### 12.3 使用ノード
- `ai_driver_node`
- `race_manager_node`
- `renderer_node`

### 12.4 TrackModel
最低限保持するもの:
- `track_name`
- `centerline`
- `track_width`
- `start_line`
- `forward_hint`

---

## 13. race_map アーキテクチャ

### 13.1 位置づけ
`race_map` は external map asset を扱うための package である。

### 13.2 主目的
- external map asset の読み込み
- OpenDRIVE parse
- normalization
- logical track 生成支援

### 13.3 主形式
- OpenDRIVE を第一候補とする

### 13.4 将来の利用方法
- OpenDRIVE → logical race track
- simulator map → logical race track
- competition-provided map → logical race track

### 13.5 race_track との境界
- `race_map`: physical map
- `race_track`: logical track

この分離を崩さない。

---

## 14. Backend 差し替えアーキテクチャ

### 14.1 初版 backend
- `vehicle_sim_node`

### 14.2 将来 backend
- `race_carla_bridge` + `CARLA`

### 14.3 bridge / adapter の役割
simulator 固有 interface を ROS2 system standard interface に変換する。

### 14.4 上流ロジックが依存する I/F
- `/carX/cmd_drive`
- `/carX/odom`
- `odom -> carX/base_link`

### 14.5 期待される効果
- internal backend でも future backend でも上流ロジックを再利用できる
- RaceManager / AI Driver / Renderer の作り直しを避ける

---

## 15. パッケージ構成アーキテクチャ

想定パッケージ:

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

### 15.1 `race_interfaces`
custom message 定義

### 15.2 `race_track`
logical track shared library

### 15.3 `race_map`
external map asset package

### 15.4 `race_vehicle`
`vehicle_sim_node`

### 15.5 `race_ai`
`ai_driver_node`

### 15.6 `race_manager`
`race_manager_node`

### 15.7 `race_renderer`
`renderer_node`

### 15.8 `race_input`
- `gamepad_input_node`
- `keyboard_input_node`
- `race_command_input_node`

### 15.9 `race_bringup`
launch / config / scenario 読み込み

### 15.10 `race_carla_bridge`
future simulator bridge

---

## 16. 設計制約

本アーキテクチャで絶対に崩してはいけない制約を以下に示す。

### 16.1 Interface 固定
- 車両制御 I/F は `/carX/cmd_drive`
- 車両状態 I/F は `/carX/odom`
- race-wide I/F は `/race/...`

### 16.2 Namespace 一貫性
- `vehicle_id`
- namespace
- frame prefix
は一致させる

### 16.3 renderer は表示専用
- 判定を持たない
- race state 更新を持たない

### 16.4 race_manager は判定専用
- vehicle dynamics を持たない
- renderer 表示都合を持たない

### 16.5 simulator-specific code の隔離
simulator 固有処理は bridge / adapter に隔離する

### 16.6 map / track / scenario の境界維持
- map = physical environment
- track = logical race course
- scenario = runtime condition

### 16.7 track の扱い
初版で track は topic 配信しない。  
file + library で扱う。

### 16.8 時間源の一貫性
全ノードの時間計算は ROS clock を唯一の正とする。
- countdown
- cooldown
- lap timing
- elapsed time
はすべて ROS time で計算する。
time source の混在を禁止する。

### 16.9 初版の抽象化抑制
初版では将来拡張のための過剰な抽象化を避ける。
- 未使用 backend のための plugin 機構を先行導入しない
- 未使用 controller のための汎用化を行いすぎない
- 実際の使用箇所が出るまで interface を増やしすぎない

---

## 17. 将来拡張の見取り図

### 17.1 simulator
- CARLA
- 将来 Isaac Sim なども候補になりうる

### 17.2 sensor
- LiDAR
- Radar
- Camera

### 17.3 controller
- Stanley
- MPC

### 17.4 map source
- OpenDRIVE
- simulator map
- competition map

### 17.5 autonomy stack
- perception
- planning
- control

---

## 18. 実装順とアーキテクチャの関係

推奨実装順:

1. `race_interfaces`
2. `race_track`
3. `vehicle_sim_node`
4. `race_manager_node`
5. `gamepad_input_node` / `keyboard_input_node` / `race_command_input_node`
6. `renderer_node`
7. `ai_driver_node`

### 理由
- 依存の少ないものから固める
- 各段階で inspect 可能な状態を作る
- architecture を崩さずに前進できる

---

## 19. 本書の位置づけ

本書は、以下の基準となる architecture document である。

- 実装タスク分解
- GitHub Issues
- README / docs
- backend 差し替え時の指針
- future CARLA integration design

本書に定義した責務分離と interface 契約を維持することを、本プロジェクトの構造上の前提とする。

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


