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
| `completed_laps` | `int32` |

current first multi-vehicle slice における補足:

- `RaceState.completed_laps` は race-wide aggregate ではない
- current implementation では primary vehicle snapshot 由来の値を保持する
- race-wide completion の判断には使わず、`race_state status` を見る
- vehicle-local finish の判断には `VehicleRaceStatus.has_finished` を見る
- 詳細な current semantics は `docs/design/race_state_completed_laps_current_semantics.md` を参照

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

current demo は first multi-vehicle implementation slice として、default launch では
`demo_vehicle_1` と `demo_vehicle_2` の 2 台 participating vehicles を前提にしています。
一方で entry/config では、matching する `participating_vehicle_ids` と `runtime_position_specs`
を与えることで 3 台以上を流し込めます。

観測責務は次のように分かれています。

- `RaceState`: race-wide state の観測手段
- `VehicleRaceStatus`: vehicle-local progress の主な観測手段
- `LapEvent`: vehicle-local な lap crossing event の観測手段

race-wide `completed` は first slice では `all participating vehicles finished` として扱います。したがって、片方の車両だけが `has_finished=true` になっても race-wide `completed` にはなりません。

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

この launch は default では `config/race_progress_demo.publisher.yaml` を通して
`race_progress_publisher` に current 2-vehicle demo config を渡します。

起動直後の期待:

- publisher 側に `Target lap count: 2`
- publisher 側に `Race coordinator initialized for 2 participating vehicles`
- monitor 側に `race_state status=stopped`

3 台 sample config を流し込みたい場合:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch race_track race_progress_demo.launch.py \
  publisher_params_file:=$(ros2 pkg prefix race_track)/share/race_track/race_progress_demo_three_vehicle.publisher.yaml
```

この override は default behavior を変えず、entry/config 側だけで 3 台以上を追加できます。

3 台 sample config の最小観測:

- `Race coordinator initialized for 3 participating vehicles`
- `vehicle_race_status vehicle_id=demo_vehicle_1 ...`
- `vehicle_race_status vehicle_id=demo_vehicle_2 ...`
- `vehicle_race_status vehicle_id=demo_vehicle_3 ...`
- `lap_event vehicle_id=demo_vehicle_1 ...`
- `lap_event vehicle_id=demo_vehicle_2 ...`
- `lap_event vehicle_id=demo_vehicle_3 ...`
- 2 台だけ `has_finished=true` の時点では `race_state status=running`
- 3 台全員が `has_finished=true` になった後にだけ `race_state status=completed`

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

`start` 後の最小観測:

- `race_state status=running`
- `vehicle_race_status vehicle_id=demo_vehicle_1 ...`
- `vehicle_race_status vehicle_id=demo_vehicle_2 ...`
- `lap_event vehicle_id=demo_vehicle_1 ...`
- `lap_event vehicle_id=demo_vehicle_2 ...`

completion の最小観測:

- 片方だけ `lap_count=2` に達した時点では、その車両の `has_finished=true` を観測できても `race_state status=running` のまま
- 両方の participating vehicles が `lap_count=2` に達した後にだけ `race_state status=completed` になる

`stop` / `reset` の最小観測:

- `stop` は 2 台の progression を race-wide に止める
- `reset` は 2 台の lap progress と finish 状態を初期化し、`race_state status=stopped` に戻す

実装上の補足:

- current implementation の `RaceState.completed_laps` は race-wide 集約値ではなく primary vehicle snapshot 由来
- race-wide completion の確認は `race_state status` を主に見て、vehicle-local completion の確認は `VehicleRaceStatus.has_finished` を見る

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
