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

現在の最小デモは次の 2 ノードで構成されています。

- `race_progress_publisher`: `/race_command` を受けて `/race_state`、`/vehicle_race_status`、`/lap_event` を publish する
- `race_progress_monitor`: `/race_state`、`/vehicle_race_status`、`/lap_event` を subscribe して画面に表示する

ワークスペース直下で build します。

```bash
source /opt/ros/jazzy/setup.bash
colcon build
source install/setup.bash
```

別ターミナルで demo を起動します。

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch race_track race_progress_demo.launch.py
```

もう 1 つ別ターミナルを開き、`RaceCommand` を送ります。

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
```

START (`command: 0`)

```bash
ros2 topic pub --once /race_command race_interfaces/msg/RaceCommand "{header: {frame_id: 'map'}, command: 0}"
```

STOP (`command: 1`)

```bash
ros2 topic pub --once /race_command race_interfaces/msg/RaceCommand "{header: {frame_id: 'map'}, command: 1}"
```

RESET (`command: 2`)

```bash
ros2 topic pub --once /race_command race_interfaces/msg/RaceCommand "{header: {frame_id: 'map'}, command: 2}"
```

期待される挙動:

- launch 直後、monitor に `race_state status=stopped` が表示される
- `START` 送信後、monitor に `race_state` と `vehicle_race_status` が継続して表示される
- スタートライン通過時に `lap_event` が表示される
- `STOP` 送信後、`race_state status=stopped` が表示されて進行が止まる
- `RESET` 送信後、経過とラップ数が初期状態に戻り、`race_state status=stopped` が表示される

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
