# Milestone 1

Milestone 1 では、ROS 2 Race Simulator の基礎となる 2 つの成果物を定義しています。

- `race_interfaces`: レース進行や車両状態をやり取りする ROS 2 message 定義
- `race_track`: トラック形状を YAML で読み込み、`TrackModel` として扱うためのユーティリティ

この文書は、現在の実装に含まれている message 定義、track model、YAML 入力形式、build/test 手順をまとめたものです。

## race_interfaces の役割

`race_interfaces` は、シミュレータ内のノード間で共有する ROS 2 message を提供するパッケージです。`src/race_interfaces/msg/` 配下の `.msg` ファイルを `rosidl_generate_interfaces()` で生成し、レース制御、レース全体の状態、各車両の周回状態、ラップイベント、運転コマンドを共通の型として扱います。

依存関係は次のとおりです。

- `std_msgs/Header`
- `builtin_interfaces/Duration`

## message 一覧

### `DriveCommand`

ファイル: `src/race_interfaces/msg/DriveCommand.msg`

| Field | Type |
| --- | --- |
| `header` | `std_msgs/Header` |
| `throttle_cmd` | `float32` |
| `steering_cmd` | `float32` |

### `RaceCommand`

ファイル: `src/race_interfaces/msg/RaceCommand.msg`

| Field / Constant | Type | Value |
| --- | --- | --- |
| `header` | `std_msgs/Header` | |
| `command` | `uint8` | |
| `START` | `uint8` | `0` |
| `STOP` | `uint8` | `1` |
| `RESET` | `uint8` | `2` |

### `RaceState`

ファイル: `src/race_interfaces/msg/RaceState.msg`

| Field | Type |
| --- | --- |
| `header` | `std_msgs/Header` |
| `race_status` | `string` |
| `elapsed_time` | `builtin_interfaces/Duration` |
| `total_laps` | `int32` |

### `LapEvent`

ファイル: `src/race_interfaces/msg/LapEvent.msg`

| Field | Type |
| --- | --- |
| `header` | `std_msgs/Header` |
| `vehicle_id` | `string` |
| `lap_count` | `int32` |
| `lap_time` | `builtin_interfaces/Duration` |
| `best_lap_time` | `builtin_interfaces/Duration` |
| `has_finished` | `bool` |

### `VehicleRaceStatus`

ファイル: `src/race_interfaces/msg/VehicleRaceStatus.msg`

| Field | Type |
| --- | --- |
| `header` | `std_msgs/Header` |
| `vehicle_id` | `string` |
| `lap_count` | `int32` |
| `current_lap_time` | `builtin_interfaces/Duration` |
| `last_lap_time` | `builtin_interfaces/Duration` |
| `best_lap_time` | `builtin_interfaces/Duration` |
| `total_elapsed_time` | `builtin_interfaces/Duration` |
| `has_finished` | `bool` |
| `is_off_track` | `bool` |
| `off_track_count` | `int32` |

## race_track の役割

`race_track` は、トラック定義を YAML から読み込んで `TrackModel` に変換し、最低限の妥当性確認と幾何計算を提供するパッケージです。

主な構成は次のとおりです。

- `track_loader`: YAML ファイルを `TrackModel` に変換する
- `track_validator`: 読み込んだ `TrackModel` が最低条件を満たすか検証する
- `geometry`: centerline や start line を使う基礎幾何計算を提供する

`TrackModel` は次の要素を持ちます。

| Field | Type | 説明 |
| --- | --- | --- |
| `track_name` | `std::string` | トラック名 |
| `centerline` | `std::vector<Point2d>` | センターラインの点列 |
| `track_width` | `double` | トラック幅 |
| `start_line` | `LineSegment2d` | スタート / フィニッシュ線 |
| `forward_hint` | `Point2d` | 正方向を示すベクトル |

補助型は次の 2 つです。

- `Point2d`: `x`, `y` を持つ 2 次元点
- `LineSegment2d`: `p1`, `p2` を持つ線分

## YAML track format

`race_track::loadTrackFromYaml()` が受け付ける YAML は、ルートに次の必須キーを持ちます。

| Key | Type | 内容 |
| --- | --- | --- |
| `track_name` | string | トラック名 |
| `centerline` | sequence | `{x, y}` 配列 |
| `track_width` | number | トラック幅 |
| `start_line` | map | `p1`, `p2` を持つ線分 |
| `forward_hint` | map | `x`, `y` を持つ正方向ベクトル |

点と線分の形は次のとおりです。

```yaml
point:
  x: 0.0
  y: 0.0

line_segment:
  p1:
    x: 0.0
    y: -1.0
  p2:
    x: 0.0
    y: 1.0
```

`loadTrackFromYaml()` が要求する構文条件は次のとおりです。

- ルートに `track_name`、`centerline`、`track_width`、`start_line`、`forward_hint` の必須キーが存在する必要がある
- `centerline` は YAML sequence である必要がある
- `centerline` の各要素は `x` と `y` を持つ必要がある
- `start_line` は `p1` と `p2` を持つ必要がある
- `start_line.p1` と `start_line.p2` はそれぞれ `x` と `y` を持つ必要がある
- `forward_hint` は `x` と `y` を持つ必要がある
- malformed な YAML は例外になる
- required key の欠落は例外になる

`validateTrackOrThrow()` が要求する意味条件は次のとおりです。

- `track_name` は空文字であってはならない
- `centerline` は 3 点以上必要
- `track_width` は `0` より大きい必要がある
- `start_line` はゼロ長線分であってはならない
- `forward_hint` はゼロベクトルであってはならない

サンプル:

```yaml
track_name: sample_track
centerline:
  - x: 0.0
    y: 0.0
  - x: 10.0
    y: 0.0
  - x: 20.0
    y: 5.0
track_width: 6.5
start_line:
  p1:
    x: 0.0
    y: -3.25
  p2:
    x: 0.0
    y: 3.25
forward_hint:
  x: 1.0
  y: 0.0
```

このサンプルは `src/race_track/config/sample_track.yaml` と一致しています。

## build/test 手順

前提:

- ROS 2 環境が利用可能で、`colcon` が実行できること
- `yaml-cpp` を含む依存関係が導入済みであること

ワークスペース直下で実行します。

```bash
source /opt/ros/jazzy/setup.bash
colcon build
colcon test
colcon test-result --verbose
```

このリポジトリで現在実行されるテストは `race_track` の unit test です。

- `test_track_loader`
- `test_track_validator`
- `test_geometry`

`race_interfaces` は message 生成パッケージのため、独自の test ターゲットは定義されていません。

## 最小 race progress demo

現在の最小デモは次の 3 つで構成されています。

- `race_progress_publisher`: `/race_command` を受けて `/race_state`、`/vehicle_race_status`、`/lap_event` を publish する
- `race_progress_monitor`: `/race_state`、`/vehicle_race_status`、`/lap_event` を subscribe して画面に表示する
- `race_command_cli`: `start` / `stop` / `reset` を `/race_command` に publish する

### Build

ワークスペース直下で実行します。

```bash
source /opt/ros/jazzy/setup.bash
colcon build
source install/setup.bash
```

### Launch

別ターミナルで最小デモを起動します。

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch race_track race_progress_demo.launch.py
```

この launch は `race_progress_publisher` に `target_lap_count:=2` を渡します。単一車両デモでは、この値に到達した時点で race completion を判定します。

### Command CLI

もう 1 つ別ターミナルを開き、`race_command_cli` で操作します。

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
```

start:

```bash
ros2 run race_track race_command_cli start
```

stop:

```bash
ros2 run race_track race_command_cli stop
```

reset:

```bash
ros2 run race_track race_command_cli reset
```

### Expected Behavior

- launch 直後、monitor に `race_state status=stopped` が表示される
- `start` 実行後、monitor に `race_state status=running` と `vehicle_race_status` が継続して表示される
- スタートライン通過時に `lap_event` が表示される
- `lap_count` が `target_lap_count` に到達した時点で、`vehicle_race_status.has_finished=true` と `race_state status=completed` が揃う
- `target_lap_count` 未達のまま固定 position 列の終端に到達した場合、publisher は warning を出して停止し、publish される `race_state status=stopped` のまま `completed` には遷移しない
- `stop` 実行後、monitor に `race_state status=stopped` が表示されて進行が止まる
- `reset` 実行後、経過時間とラップ数が初期状態に戻り、monitor に `race_state status=stopped` が表示される

### Current State Semantics in the Single-Vehicle Demo

この節は、現在の最小デモ実装に限った意味を整理したものです。将来の本格的な race manager や multi-vehicle 前提の仕様は、ここでは確定しません。

前提:

- 車両は 1 台のみで、`vehicle_id` は固定の `demo_vehicle_1`
- `race_progress_publisher` は内部の固定 position 列を 1 秒ごとに順番に消費して進行する
- `race_progress_publisher` は `target_lap_count` パラメータを持ち、デフォルト値は `2`
- `lap_count` は forward 方向の start line crossing を検出したときだけ増える
- `completed` / `has_finished` は `lap_count >= target_lap_count` になったときに立つ
- 固定 position 列の最終 step 到達だけでは `completed` にならない

状態とフィールドの意味:

| Field | 現在の単一車両デモでの意味 |
| --- | --- |
| `RaceState.race_status = stopped` | publisher が進行していない状態。初期状態、`stop` 後、`reset` 後がこれに当たる。 |
| `RaceState.race_status = running` | publisher が固定 position 列を順に消費中の状態。 |
| `RaceState.race_status = completed` | `lap_count >= target_lap_count` が成立し、publisher が進行を停止した状態。 |
| `VehicleRaceStatus.lap_count` | その車両について、forward 方向の start line crossing が確定した回数。現在の実装では `ProgressTracker` が crossing ごとに `1` ずつ加算する。 |
| `VehicleRaceStatus.has_finished` | その車両が現在の最小デモで target lap に到達したことを表すフラグ。`lap_count >= target_lap_count` で `true` になり、`reset` で `false` に戻る。 |
| `RaceState.total_laps` | 現在 publish 時点の `lap_count` を `RaceState` 側にも載せた値。単一車両デモでは実質的に車両 1 台の確定 lap 数と同じで、target lap 設定値そのものではない。 |

補足:

- `LapEvent.lap_count` も crossing 検出後の確定 lap 数を表す
- `LapEvent.has_finished` は publish 時点の snapshot をそのまま載せるため、target lap に到達した crossing では `true` になる
- target 未達のまま固定 position 列が尽きた場合、publisher は停止するが `has_finished` は `false` のまま

現時点で未確定なこと:

- `RaceState.total_laps` を将来も進捗値として使うか、target laps のような設定値に再定義するか
- `VehicleRaceStatus.has_finished` を将来の正式な race finish semantics にどう結び付けるか
- `race_status=completed` の遷移条件を lap 数、順位、全車完了などのどの条件で定義するか
- `lap_count` の扱いを multi-vehicle 集約とどう切り分けるか

将来の拡張で再整理が必要な点:

- 現在は `target_lap_count` を publisher parameter として持つだけなので、multi-vehicle 化や race manager 導入時は設定の置き場を再整理する必要がある
- finish 条件を高度化する場合、`has_finished` は「デモ終了フラグ」ではなく「レース完了判定」との関係を改めて定義する必要がある
- multi-vehicle 対応時は、全体 state と各車 state の責務分離を明確にし直す必要がある

## 関連ファイル / ディレクトリ案内

- `src/race_interfaces/msg/`: message 定義
- `src/race_interfaces/CMakeLists.txt`: message 生成設定
- `src/race_interfaces/package.xml`: package 依存関係
- `src/race_track/include/race_track/types.hpp`: `Point2d`, `LineSegment2d`
- `src/race_track/include/race_track/track_model.hpp`: `TrackModel`
- `src/race_track/src/track_loader.cpp`: YAML ローダ実装
- `src/race_track/src/track_validator.cpp`: 妥当性検証
- `src/race_track/src/geometry.cpp`: 幾何計算
- `src/race_track/config/sample_track.yaml`: YAML サンプル
- `src/race_track/test/`: `race_track` の unit test
