# ROS2 Race Simulator
## Implementation Tasks

---

## 1. 文書の目的

本書は、ROS2 Race Simulator の仕様とアーキテクチャをもとに、実装作業を実行可能な粒度まで分解したタスク一覧である。

目的は以下の通り。

- どの順番で何を作るかを明確にする
- GitHub Issues にそのまま転記できる粒度にする
- 各タスクの完了条件を定義する
- 実装時に迷わないようにする
- 「今どこまでできているか」を追跡できるようにする

本書では、以下の観点でタスクを整理する。

- フェーズごとの実装順
- 各タスクの目的
- 実装内容
- 完了条件
- 依存関係
- optional / future 扱い

---

## 2. 実装方針

### 2.1 基本方針

実装は以下の順で進める。

1. interface を固定する
2. shared library を作る
3. internal backend を作る
4. race logic を作る
5. input をつなぐ
6. visualization をつなぐ
7. AI をつなぐ

### 2.2 なぜこの順番か

この順番を採用する理由は以下の通り。

- 依存関係の少ないものから作れる
- 各段階で build / run / inspect が可能
- 「全部できるまで何も確認できない」状態を避けられる
- backend と上流ロジックを分離した設計を崩さない

### 2.3 実装の考え方

- 最初から全部作らない
- 各フェーズで「動く最小構成」を作る
- まず internal backend で成立させる
- controller 比較や CARLA 連携は後段で扱う
- 将来拡張は設計に入れるが、初版実装を重くしない

### 2.4 Phase Completion Rule
各 Phase は、その Phase の最小動作確認が完了するまで次へ進まない。
新機能の先行実装や将来拡張の着手は禁止する。

### 2.5 初版実装の禁止事項
初版 acceptance criteria を満たすまで、以下への着手を禁止する。
- OpenDRIVE 対応
- CARLA 連携
- sensor topic 生成
- collision 判定
- overtaking
- renderer 高機能化
- controller の追加拡張


---

## 3. 実装フェーズ一覧

本プロジェクトは以下のフェーズに分けて実装する。

- Phase 0: Repository / Workspace Setup
- Phase 1: Interfaces
- Phase 2: Track Library
- Phase 3: Vehicle Backend
- Phase 4: Race Logic
- Phase 5: Input Nodes
- Phase 6: Renderer
- Phase 7: AI Driver
- Phase 8: Bringup / Scenario
- Phase 9: Test / Quality
- Phase 10: Future Extensions (Optional)

---

## 4. Phase 0: Repository / Workspace Setup

### Task 0-1: ROS2 workspace 作成

#### 目的
開発全体の作業基盤を作る。

#### 作業内容
- ROS2 workspace を作成する
- `src/` 配下にパッケージを配置できる状態を作る

#### 想定構成

~~~text
race_simulator_ws/
 ├─ src/
 ├─ build/
 ├─ install/
 └─ log/
~~~

#### 完了条件
- workspace が作成されている
- `colcon build` を実行できる

#### 依存
- なし

---

### Task 0-2: パッケージ skeleton 作成

#### 目的
プロジェクトの実装単位となる ROS2 パッケージを作成する。

#### 作業内容
以下のパッケージを作成する。

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

#### 完了条件
- 全パッケージが `src/` に作成されている
- `ros2 pkg list` で見える
- 空の状態で `colcon build` が通る

#### 依存
- Task 0-1

---

### Task 0-3: ドキュメント配置

#### 目的
要件・仕様・アーキテクチャ・タスク一覧をリポジトリ上に格納する。

#### 作業内容
以下の文書を配置する。

- `docs/requirements.md`
- `docs/specification.md`
- `docs/architecture.md`
- `docs/implementation_tasks.md`

#### 完了条件
- docs ディレクトリに文書が存在する
- リポジトリのトップから参照できる

#### 依存
- Task 0-2

---

### Task 0-4: ルート README の初版作成

#### 目的
リポジトリの入口を用意する。

#### 作業内容
README に以下を記載する。

- プロジェクト概要
- 目的
- パッケージ一覧
- docs へのリンク
- 開発の進め方

#### 完了条件
- README を読めばプロジェクトの全体像が分かる

#### 依存
- Task 0-3

---

## 5. Phase 1: Interfaces

### Task 1-1: `DriveCommand.msg` 作成

#### 目的
車両制御入力の共通 interface を定義する。

#### 内容

~~~text
std_msgs/Header header

float32 throttle_cmd
float32 steering_cmd
~~~

#### 完了条件
- `ros2 interface show race_interfaces/msg/DriveCommand` で確認できる
- build が通る

#### 依存
- Task 0-2

---

### Task 1-2: `RaceCommand.msg` 作成

#### 目的
レース操作 command の共通 interface を定義する。

#### 内容

~~~text
std_msgs/Header header

uint8 command

uint8 START=0
uint8 STOP=1
uint8 RESET=2
~~~

#### 完了条件
- interface が定義されている
- build が通る

#### 依存
- Task 0-2

---

### Task 1-3: `RaceState.msg` 作成

#### 目的
レース全体状態の interface を定義する。

#### 内容

~~~text
std_msgs/Header header

string race_status
builtin_interfaces/Duration elapsed_time

int32 total_laps
~~~

#### 完了条件
- interface が定義されている
- build が通る

#### 依存
- Task 0-2

---

### Task 1-4: `LapEvent.msg` 作成

#### 目的
ラップ成立時イベントの interface を定義する。

#### 内容

~~~text
std_msgs/Header header

string vehicle_id

int32 lap_count

builtin_interfaces/Duration lap_time
builtin_interfaces/Duration best_lap_time

bool has_finished
~~~

#### 完了条件
- interface が定義されている
- build が通る

#### 依存
- Task 0-2

---

### Task 1-5: `VehicleRaceStatus.msg` 作成

#### 目的
車両ごとの継続レース状態の interface を定義する。

#### 内容

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

#### 完了条件
- interface が定義されている
- build が通る

#### 依存
- Task 0-2

---

### Task 1-6: custom message 定義の整合確認

#### 目的
仕様書と message 定義が一致していることを確認する。

#### 作業内容
- 型が仕様通りか確認
- enum 値が仕様通りか確認
- Header / Duration が仕様通りか確認

#### 完了条件
- 仕様書との差分がない

#### 依存
- Task 1-1 ～ 1-5

---

## 6. Phase 2: Track Library

### Task 2-1: `TrackModel` クラス作成

#### 目的
track file の論理表現をコード上で統一する。

#### 保持項目
- `track_name`
- `centerline`
- `track_width`
- `start_line`
- `forward_hint`

#### 完了条件
- `TrackModel` をインスタンス化できる
- 最低限のデータ構造が定義されている

#### 依存
- Task 0-2

---

### Task 2-2: track YAML loader 実装

#### 目的
track file を読み込み、`TrackModel` に変換する。

#### 作業内容
- YAML parser 実装
- `TrackModel` 生成
- 読み込み失敗時の例外処理

#### 完了条件
- `sample_track.yaml` を読み込める
- 不正 YAML で明示的に失敗する

#### 依存
- Task 2-1

---

### Task 2-3: track validation 実装

#### 目的
不正な track を early fail させる。

#### 検査項目
- `centerline` 点数が十分か
- `track_width > 0` か
- `start_line` が 2 点定義されているか
- `forward_hint` が妥当か
- 必須キーが全てあるか

#### 完了条件
- validation 成功 / 失敗が判定できる

#### 依存
- Task 2-2

---

### Task 2-4: 最近点計算 utility 実装

#### 目的
AI・race logic・off-track 判定で使う最近点計算を共通化する。

#### 作業内容
- point に対する centerline 最近点 index 計算
- 最近点座標計算
- point-to-centerline 距離計算

#### 完了条件
- 単純な centerline に対して正しい最近点が返る
- 単体テストが通る

#### 依存
- Task 2-1

---

### Task 2-5: lookahead point 計算 utility 実装

#### 目的
AI controller が使う lookahead point を共通化する。

#### 作業内容
- 最近点から一定距離先を辿る
- centerline 上の lookahead point を返す

#### 完了条件
- 与えた距離に対して妥当な lookahead point が返る
- closed loop で動作する

#### 依存
- Task 2-4

---

### Task 2-6: start_line 交差判定 utility 実装

#### 目的
race_manager の lap 判定に使う線分交差処理を共通化する。

#### 作業内容
- 移動線分と start_line の交差判定
- move_vec と forward_hint の整合判定補助

#### 完了条件
- 正方向交差と逆方向交差を区別できる
- 単体テストが通る

#### 依存
- Task 2-1

---

### Task 2-7: sample track 作成

#### 目的
最低限の動作確認に使うトラックを用意する。

#### 作業内容
- 周回コース track 作成
- 任意で八の字コース track 作成

#### 完了条件
- race_manager / renderer / ai_driver が共通利用できる track file がある

#### 依存
- Task 2-2

---

## 7. Phase 3: Vehicle Backend

### Task 3-1: `vehicle_sim_node` skeleton 作成

#### 目的
internal vehicle backend の基本ノードを作る。

#### 作業内容
- node 起動
- parameter 読み込み
- `/carX/cmd_drive` subscribe
- `/carX/odom` publisher 作成
- timer loop 作成

#### 完了条件
- ノード起動可能
- odom が publish される

#### 依存
- Task 1-1
- Task 0-2

---

### Task 3-2: state 初期化実装

#### 目的
初期姿勢と初期状態を保持できるようにする。

#### 作業内容
- `initial_pose_x`
- `initial_pose_y`
- `initial_pose_yaw`
- velocity 初期化
- steering 初期化

#### 完了条件
- 起動時に初期姿勢で odom が出る

#### 依存
- Task 3-1

---

### Task 3-3: Kinematic Bicycle Model 実装

#### 目的
軽量な車両運動モデルを実装する。

#### 状態変数
- `x`
- `y`
- `yaw`
- `velocity`

#### 入力
- `throttle_cmd`
- `steering_cmd`

#### 完了条件
- throttle で加速できる
- steering で旋回できる

#### 依存
- Task 3-2

---

### Task 3-4: 入力変換 / 飽和 / 速度制限実装

#### 目的
DriveCommand を物理量へ変換し、安全に扱う。

#### 作業内容
- `throttle_cmd -> acceleration`
- `steering_cmd -> steering_angle`
- `[-1, 1]` 飽和
- `max_speed_mps` 制限
- 後退許可

#### 完了条件
- 異常値でも破綻しない
- 速度が上限を超えない

#### 依存
- Task 3-3

---

### Task 3-5: command timeout 実装

#### 目的
入力停止時に安全に中立へ戻す。

#### 作業内容
- `cmd_timeout_sec`
- 最終受信時刻管理
- timeout 時の中立入力化

#### 完了条件
- command 未受信で中立化される

#### 依存
- Task 3-1

---

### Task 3-6: odom publish 実装

#### 目的
車両状態を ROS 標準 message で publish する。

#### 作業内容
- `nav_msgs/Odometry` 作成
- pose を `odom` frame で設定
- twist を `carX/base_link` frame で設定

#### 完了条件
- `/carX/odom` が仕様通りに出る

#### 依存
- Task 3-3

---

### Task 3-7: TF broadcast 実装

#### 目的
車両 frame を TF で配信する。

#### 作業内容
- `odom -> carX/base_link` を broadcast
- odom と同一状態・同一時刻で生成

#### 完了条件
- TF が RViz / tf_tools 等で確認できる
- odom と矛盾しない

#### 依存
- Task 3-6

---

### Task 3-8: `/race/command` subscribe と RESET 対応

#### 目的
レースリセット時に車両状態を初期化できるようにする。

#### 作業内容
- `/race/command` subscribe
- `RESET` を受信したら初期姿勢へ戻す
- `START` / `STOP` は無視してよい

#### 完了条件
- RESET で位置・姿勢・速度・入力状態が初期化される

#### 依存
- Task 1-2
- Task 3-2

---

### Task 3-9: internal class 分離

#### 目的
責務肥大化を防ぎ、将来拡張しやすくする。

#### 想定分割
- `VehicleDynamics`
- `DriveCommandAdapter`
- `OdometryPublisher`
- `TfPublisher`

#### 完了条件
- 主要責務がクラス分離されている
- 単一ファイルに全ロジックが密結合していない

#### 依存
- Task 3-3 ～ 3-8

---

### Task 3-10: vehicle_sim の基本テスト

#### 目的
backend の最低限の信頼性を確保する。

#### テスト項目
- 直進
- 左右旋回
- 後退
- timeout
- reset
- speed clip

#### 完了条件
- 基本動作が一通り確認できる
- テスト手順を docs に記載できる

#### 依存
- Task 3-8

---

## 8. Phase 4: Race Logic

### Task 4-1: `race_manager_node` skeleton 作成

#### 目的
race logic の central node を作る。

#### 作業内容
- `/race/command` subscribe
- `/carX/odom` subscribe
- `/race/state` publisher
- `/race/lap_event` publisher
- `/race/vehicle_status` publisher

#### 完了条件
- `READY` 状態で起動できる

#### 依存
- Task 1-2 ～ 1-5

---

### Task 4-2: RaceState machine 実装

#### 目的
レース全体の状態遷移を作る。

#### 状態
- `READY`
- `COUNTDOWN`
- `RUNNING`
- `FINISHED`
- `STOPPED`

#### 遷移
- `START`
- countdown timeout
- `STOP`
- `RESET`
- finish 条件

#### 完了条件
- 状態遷移が仕様通りに動く

#### 依存
- Task 4-1

---

### Task 4-3: odom cache 実装

#### 目的
各車両の最新状態を race_manager 内で保持する。

#### 作業内容
- `vehicle_id` ごとの最新 odom をキャッシュ
- `has_received_odom` を管理

#### 完了条件
- 複数車両を同時に追跡できる

#### 依存
- Task 4-1

---

### Task 4-4: elapsed_time 実装

#### 目的
レース時間を正しく管理する。

#### 作業内容
- `RUNNING` 開始時刻を保持
- `elapsed_time` 計算
- `STOPPED` / `FINISHED` で停止

#### 完了条件
- countdown を含めずに時間が増える

#### 依存
- Task 4-2

---

### Task 4-5: lap 判定実装

#### 目的
レースの中心機能である lap 判定を作る。

#### 条件
- start_line 交差
- forward_hint 整合
- cooldown 超過
- `RUNNING`
- `has_finished == false`
- odom 受信済み

#### 完了条件
- 正方向でのみ lap_count が増える
- 多重カウントしない

#### 依存
- Task 2-6
- Task 4-3

---

### Task 4-6: lap timing 実装

#### 目的
各車両のラップ時間を計算する。

#### 作業内容
- `current_lap_time`
- `last_lap_time`
- `best_lap_time`
- `total_elapsed_time`
- `has_finished`

#### 完了条件
- ラップ成立時に正しく更新される

#### 依存
- Task 4-5

---

### Task 4-7: finish 判定実装

#### 目的
レース終了条件を管理する。

#### 条件
- 最初に `total_laps` を完了した車両が出た時点で `FINISHED`

#### 完了条件
- finish 条件で `RUNNING -> FINISHED` へ遷移する

#### 依存
- Task 4-6

---

### Task 4-8: off-track 判定実装

#### 目的
コース外判定を実装する。

#### 作業内容
- 最近点距離計算
- `distance > track_width / 2` で off-track
- `inside -> outside` で `off_track_count += 1`

#### 完了条件
- `is_off_track` と `off_track_count` が動作する

#### 依存
- Task 2-4
- Task 4-3

---

### Task 4-9: `VehicleRaceStatus` publish 実装

#### 目的
renderer が継続表示に使う status を出力する。

#### 作業内容
- 1車両1メッセージ publish
- 同一 topic に複数車両分を流す
- `vehicle_id` を付与

#### 完了条件
- `/race/vehicle_status` に status が流れる

#### 依存
- Task 4-6
- Task 4-8

---

### Task 4-10: `LapEvent` publish 実装

#### 目的
ラップ成立時イベントを通知する。

#### 作業内容
- lap 成立時のみ `LapEvent` publish
- `lap_time`, `best_lap_time`, `has_finished` を含める

#### 完了条件
- ラップ成立時のみイベントが発行される

#### 依存
- Task 4-6

---

### Task 4-11: RaceState publish 実装

#### 目的
AI や renderer が全体状態を参照できるようにする。

#### 作業内容
- 毎周期 `/race/state` publish
- `race_status`, `elapsed_time`, `total_laps` を設定

#### 完了条件
- 全状態で `/race/state` が確認できる

#### 依存
- Task 4-2
- Task 4-4

---

### Task 4-12: race_manager の基本テスト

#### 目的
state machine と race logic の妥当性を確認する。

#### テスト項目
- START
- COUNTDOWN
- RUNNING
- STOP
- RESET
- lap 成立
- finish
- off-track

#### 完了条件
- 主要状態遷移とイベントが確認できる

#### 依存
- Task 4-11

---

### Task 4-13: minimal scenario file 導入
#### 目的
初期姿勢や controller 割り当てのベタ書きを避ける。
#### 作業内容
YAML による最小 scenario を定義する。
- total_laps
- vehicle list
- initial poses
- controller_type
- input source
#### 完了条件
1台以上の vehicle 設定を file から与えられる
#### 依存
- Task 4-11


---

## 9. Phase 5: Input Nodes

### Task 5-1: `gamepad_input_node` skeleton 作成

#### 目的
manual input source を作る。

#### 作業内容
- node 作成
- joystick 入力購読
- `/carX/cmd_drive` publisher 作成

#### 完了条件
- node が起動できる

#### 依存
- Task 1-1

---

### Task 5-2: gamepad 軸マッピング実装

#### 目的
PS4 controller を想定した操作入力を実装する。

#### モード
- `trigger_steering`
- `right_stick_throttle`

#### 完了条件
- steering / throttle が正しく `DriveCommand` に変換される

#### 依存
- Task 5-1

---

### Task 5-3: deadzone / invert / normalize 実装

#### 目的
実用的な入力処理を行う。

#### 作業内容
- deadzone 適用
- 軸反転設定
- `[-1, 1]` 正規化

#### 完了条件
- ニュートラルで勝手に動かない

#### 依存
- Task 5-2

---

### Task 5-4: `keyboard_input_node` 実装

#### 目的
fallback / debug 用 manual input source を作る。

#### 作業内容
- キー入力読み取り
- `/carX/cmd_drive` publish

#### 完了条件
- キーボードで車両が動く

#### 依存
- Task 1-1

---

### Task 5-5: `race_command_input_node` 実装

#### 目的
レース操作を独立した入力ノードとして提供する。

#### 作業内容
- START
- STOP
- RESET
を `RaceCommand` として publish

#### 完了条件
- `/race/command` を手動送信できる

#### 依存
- Task 1-2

---

### Task 5-6: input source 切替確認

#### 目的
manual source の競合を防ぐ。

#### 作業内容
- gamepad / keyboard が同時に同一車両へ送られないよう確認
- launch / config で片方だけ有効にする

#### 完了条件
- 1車両につき 1 manual source で動作する

#### 依存
- Task 5-3
- Task 5-4

---

## 10. Phase 6: Renderer

### Task 6-1: `renderer_node` skeleton 作成

#### 目的
初版可視化ノードの枠組みを作る。

#### 作業内容
- node 作成
- 描画ループ作成
- topic subscriber 作成

#### 完了条件
- 空画面でも起動する

#### 依存
- Task 1-3 ～ 1-5

---

### Task 6-2: track 描画実装

#### 目的
logical track を表示する。

#### 作業内容
- centerline 描画
- track_width による corridor 表示
- start_line 表示

#### 完了条件
- sample track が画面に表示される

#### 依存
- Task 2-7

---

### Task 6-3: 車両描画実装

#### 目的
複数車両を可視化する。

#### 作業内容
- `/carX/odom` を使った位置表示
- 向き表示
- `vehicle_id` 表示

#### 完了条件
- 複数車両が区別可能に描画される

#### 依存
- Task 3-6
- Task 3-7

---

### Task 6-4: HUD 表示実装

#### 目的
レース状態を画面上で確認できるようにする。

#### 表示項目
- race_state
- elapsed_time
- lap_count
- current_lap_time
- best_lap_time
- has_finished
- is_off_track
- off_track_count

#### 完了条件
- `/race/state` と `/race/vehicle_status` を画面で確認できる

#### 依存
- Task 4-9
- Task 4-11

---

### Task 6-5: LapEvent 一時表示実装

#### 目的
イベント通知を見やすくする。

#### 表示内容
- LAP 更新
- BEST LAP
- FINISHED

#### 完了条件
- `LapEvent` 発生時に一時表示される

#### 依存
- Task 4-10

---

### Task 6-6: renderer の表示責務確認

#### 目的
renderer に判定ロジックが混入していないことを確認する。

#### 確認項目
- lap 判定をしていない
- off-track 判定をしていない
- `VehicleRaceStatus` を真としている

#### 完了条件
- renderer が表示専用に保たれている

#### 依存
- Task 6-5

---

## 11. Phase 7: AI Driver

### Task 7-1: `ai_driver_node` skeleton 作成

#### 目的
AI 車両制御ノードの枠組みを作る。

#### 作業内容
- `/carX/odom` subscribe
- `/race/state` subscribe
- `/carX/cmd_drive` publisher 作成

#### 完了条件
- node が起動する
- `RUNNING` 以外では中立入力を出せる

#### 依存
- Task 1-1
- Task 1-3

---

### Task 7-2: target point selection 実装

#### 目的
track から制御目標点を選べるようにする。

#### 作業内容
- centerline 最近点計算
- lookahead point 計算
- 毎周期の再計算

#### 完了条件
- target point が安定して選ばれる

#### 依存
- Task 2-5
- Task 7-1

---

### Task 7-3: Pure Pursuit controller 実装

#### 目的
初版の主 AI controller を実装する。

#### 作業内容
- lookahead point に基づく steering 計算
- `DriveCommand` の steering 側生成

#### 完了条件
- `controller_type = pure_pursuit` で動作する

#### 依存
- Task 7-2

---

### Task 7-4: speed controller 実装

#### 目的
縦方向制御を実装する。

#### 作業内容
- `base_target_speed_mps`
- `speed_kp`
- steering 量に応じた減速

#### 完了条件
- straight で加速し、曲がるとき少し減速する

#### 依存
- Task 7-3

---

### Task 7-5: `pid_heading` controller 実装

#### 目的
比較・学習用途の第二 controller を実装する。

#### 作業内容
- heading error 計算
- PID による steering 計算
- `heading_kp`, `heading_ki`, `heading_kd`

#### 完了条件
- `controller_type = pid_heading` で動作する

#### 依存
- Task 7-2

---

### Task 7-6: controller 切替機構実装

#### 目的
controller を strategy / plugin 的に切り替えられるようにする。

#### 作業内容
- `ControllerBase`
- `PurePursuitController`
- `PidHeadingController`
- `controller_type` parameter

#### 完了条件
- config だけで controller を切り替えられる

#### 依存
- Task 7-3
- Task 7-5

---

### Task 7-7: race state 連動確認

#### 目的
AI がレース状態に応じて動作することを確認する。

#### 期待動作
- `READY`: 中立入力
- `COUNTDOWN`: 中立入力
- `RUNNING`: 制御開始
- `FINISHED`: 中立入力
- `STOPPED`: 中立入力

#### 完了条件
- 全状態で期待通りに動く

#### 依存
- Task 7-6

---

### Task 7-8: AI integration test

#### 目的
AI 車両の初版成立性を確認する。

#### テスト項目
- `pure_pursuit` で周回可能か
- `pid_heading` で比較可能か
- off-track から復帰しやすいか
- `RUNNING` 以外で止まるか

#### 完了条件
- 初版 acceptance の AI 要件を満たす

#### 依存
- Task 7-7

---

## 12. Phase 8: Bringup / Scenario

### Task 8-1: `race.launch.py` 作成

#### 目的
初版システムを 1コマンドで起動できるようにする。

#### 起動対象
- `vehicle_sim_node`
- `race_manager_node`
- `renderer_node`
- input nodes
- `ai_driver_node`

#### 完了条件
- launch でシステムが立ち上がる

#### 依存
- Phase 3 ～ 7

---

### Task 8-2: config / scenario file 作成

#### 目的
runtime 条件を file ベースで切り替えられるようにする。

#### 内容
- race settings
- vehicle settings
- initial poses
- controller assignment
- input mode

#### 完了条件
- config を変えることで台数・controller・初期位置を変更できる

#### 依存
- Task 8-1

---

### Task 8-3: multi-vehicle 起動確認

#### 目的
vehicle namespace 設計が多車両で成立することを確認する。

#### 例
- `car1`: manual
- `car2`: pure_pursuit
- `car3`: pid_heading（任意）

#### 完了条件
- 複数車両が namespace 衝突なく動く

#### 依存
- Task 8-2

---

## 13. Phase 9: Test / Quality

### Task 9-1: acceptance checklist 作成

#### 目的
手動確認手順を明文化する。

#### 内容
- 起動
- command
- lap
- finish
- off-track
- renderer
- AI

#### 完了条件
- pass/fail を判断できる checklist がある

#### 依存
- Phase 8

---

### Task 9-2: logging / debug 出力整理

#### 目的
状態遷移や lap 判定の追跡をしやすくする。

#### 内容
- race state transition log
- lap detection log
- reset log
- off-track log

#### 完了条件
- 問題発生時にログで追える

#### 依存
- Task 4-12

---

### Task 9-3: docs 更新

#### 目的
使い方と開発状況を文書化する。

#### 内容
- README 更新
- 起動手順
- config 説明
- track file 説明
- controller 説明

#### 完了条件
- docs を読めば起動・開発ができる

#### 依存
- Phase 8

---

## 14. Phase 10: Future Extensions (Optional)

### Task 10-1: `race_map` の OpenDRIVE 対応

#### 目的
外部地図資産を取り込む準備をする。

#### 作業内容
- OpenDRIVE parse
- normalization
- logical track 生成支援

#### 完了条件
- OpenDRIVE を読み込める基礎ができる

---

### Task 10-2: `race_carla_bridge` skeleton

#### 目的
CARLA backend 差し替えのための入り口を作る。

#### 作業内容
- `/carX/cmd_drive` 受け取り
- CARLA control への変換
- CARLA state → `/carX/odom`

#### 完了条件
- bridge package が build 可能
- skeleton が存在する

---

### Task 10-3: sensor topic 予約対応

#### 目的
将来の sensor integration に備える。

#### 想定 topic
- `/carX/points_raw`
- `/carX/radar`
- `/carX/camera/image_raw`
- `/carX/camera/camera_info`

#### 完了条件
- architecture / docs 上で sensor extension を説明できる

---

### Task 10-4: Stanley / MPC controller 追加

#### 目的
controller 比較を拡張する。

#### 完了条件
- `controller_type` を増やせる構造になっている
- 実装に着手できる設計になっている

---

## 15. 最初に切るべき GitHub Issues
最初の 11 個に絞るなら、以下の順で切る。
1. workspace / package skeleton
2. custom messages
3. TrackModel
4. track loader
5. geometry utilities
6. sample track
7. vehicle_sim skeleton
8. bicycle model
9. odom + TF
10. race_manager skeleton
11. minimal scenario file


---

## 16. マイルストーン案
### Milestone 1: 車が動く
- `race_interfaces`
- `race_track`
- `vehicle_sim_node`
- `DriveCommand -> Odometry / TF` が成立する

### Milestone 2: レース状態が成立する
- `race_manager_node`
- `race_command_input_node`
- `READY / COUNTDOWN / RUNNING / STOPPED / FINISHED` が遷移する
- lap / off-track が成立する

### Milestone 3: 状態が観測できる
- `renderer_node`
- track / 車両位置 / race state / lap 情報が見える

### Milestone 4: 手動操作できる
- `gamepad_input_node`
- `keyboard_input_node`

### Milestone 5: AI で周回できる
- `ai_driver_node`
- `pure_pursuit`
- AI 車両が 1 周以上継続走行できる

### Milestone 6: 比較できる
- `pid_heading`
- multi-vehicle comparison

### Milestone 7: 将来拡張の入口
- `race_map`
- `race_carla_bridge`


---

## 17. 実装時の注意

### 17.1 今やらないことを守る
- OpenDRIVE を今すぐ読まない
- Track.msg を作らない
- track provider node を作らない
- renderer にロジックを入れない
- race_manager に表示都合を入れない
- AI node に lap 判定を入れない

### 17.2 今やるべきことに集中する
- interface を固定する
- track library を作る
- internal backend で成立させる
- race logic を固める

### 17.3 設計を崩さない
- vehicle namespace を守る
- `/carX/cmd_drive` と `/carX/odom` を守る
- `/race/...` を全体情報に限定する
- simulator-specific code は backend / bridge に隔離する

---

## 18. 本書の位置づけ

本書は、以下の開発作業の基準となる。

- GitHub Issue 作成
- Project board 作成
- 実装順の管理
- milestone 管理
- review 時の作業単位確認

本書により、仕様と実装の間を埋めることを目的とする。

