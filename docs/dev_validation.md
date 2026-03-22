# Development Validation

current 2-vehicle demo を前提にした開発用の最小 validation 手順です。
対象は `race_progress_publisher`、`race_progress_monitor`、`race_command_cli` の組み合わせです。

## Scope

- `ros2 launch race_track race_progress_demo.launch.py` を使う current demo の確認に限定する
- participating vehicles は `demo_vehicle_1` と `demo_vehicle_2` の 2 台固定とする
- `RaceState` は race-wide state の観測手段として扱う
- `VehicleRaceStatus` と `LapEvent` は vehicle-local 観測手段として扱う
- race-wide completion は `all participating vehicles finished` とする
- 実装に存在しない運用や将来構成はここに持ち込まない

## Clean Start

まず既存の demo プロセスが残っていない状態から始めます。

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 node list
```

期待:

- `race_progress_publisher`
- `race_progress_monitor`

上の 2 ノードだけが出ていないこと。

残っている場合は、起動しているターミナルで `Ctrl-C` してから再確認します。
どのターミナルが残しているか分からない場合だけ、次で親プロセスを確認します。

```bash
ps -ef | grep -E 'race_progress_demo.launch.py|race_progress_publisher|race_progress_monitor' | grep -v grep
```

## Build

ワークスペース直下で実行します。

```bash
source /opt/ros/jazzy/setup.bash
colcon build --packages-select race_track race_interfaces
source install/setup.bash
```

確認ポイント:

- `race_interfaces` と `race_track` が build 対象に入る
- build 完了後に `source install/setup.bash` をやり直す

## Minimal Demo

### 1. Launch

ターミナル A:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch race_track race_progress_demo.launch.py
```

起動直後の期待:

- publisher 側に `Target lap count: 2`
- publisher 側に `Race coordinator initialized for 2 participating vehicles`
- publisher 側に `Waiting for race commands on /race_command`
- monitor 側に `Monitoring /race_state, /vehicle_race_status, and /lap_event`
- monitor 側に `race_state status=stopped`

### 2. Start

ターミナル B:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run race_track race_command_cli start
```

期待:

- CLI 側に `sent START`
- monitor 側に `race_state status=running`
- monitor 側に `vehicle_race_status vehicle_id=demo_vehicle_1 ...`
- monitor 側に `vehicle_race_status vehicle_id=demo_vehicle_2 ...`

### 3. Stop / Reset

停止確認:

```bash
ros2 run race_track race_command_cli stop
```

期待:

- monitor 側に `race_state status=stopped`
- `demo_vehicle_1` と `demo_vehicle_2` の `vehicle_race_status` 更新が止まる

リセット確認:

```bash
ros2 run race_track race_command_cli reset
```

期待:

- monitor 側に `race_state status=stopped`
- 2 台とも `lap_count=0` の初期状態から再開できる
- 直前に `has_finished=true` だった車両があっても finish 状態を引きずらない

## Vehicle-Local Observations

`VehicleRaceStatus` と `LapEvent` が 2 台分流れることを確認する最小手順です。

ターミナル A:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch race_track race_progress_demo.launch.py
```

ターミナル B:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run race_track race_command_cli start
```

monitor 側の期待:

- `vehicle_race_status vehicle_id=demo_vehicle_1 ...`
- `vehicle_race_status vehicle_id=demo_vehicle_2 ...`
- `lap_event vehicle_id=demo_vehicle_1 lap_count=1 ...`
- `lap_event vehicle_id=demo_vehicle_2 lap_count=1 ...`

補足:

- `VehicleRaceStatus` は車両ごとの継続状態を見るための主な観測手段
- `LapEvent` はラップ確定のイベント観測に使う

## Race-Wide Completion

`target_lap_count=2` 到達と race-wide completion の関係を確認する最小手順です。

ターミナル A:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 launch race_track race_progress_demo.launch.py
```

ターミナル B:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 run race_track race_command_cli start
```

monitor 側の期待:

- 片方の車両が先に `lap_count=2` に到達すると、その車両の `VehicleRaceStatus` で `has_finished=true` が見える
- 上の時点では、もう片方が未完了なら `race_state status=running` のまま
- もう片方も `lap_count=2` に到達した後にだけ `race_state status=completed` になる

補足:

- fixed position 列の終端に着いたこと自体は完了条件ではない
- `has_finished=true` は vehicle-local completion を表す
- `race_state status=completed` は all participating vehicles finished を表す
- current implementation の `RaceState.completed_laps` は primary vehicle snapshot 由来であり、2 台全体の集約値ではない

## Quick Checks

launch 後に別ターミナルで必要最小限の接続状態だけ見たい場合:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 node list
ros2 topic info /race_command -v
```

期待:

- `ros2 node list` に `race_progress_publisher` と `race_progress_monitor`
- `/race_command` の subscriber に `race_progress_publisher`

注記:

- `ros2 topic echo --no-daemon /race_state --once` は「これから来る 1 件」を待つため、観測タイミングによっては何も出ず、接続確認用途としては不安定

## Troubleshooting

### `race_command_cli` で `no subscribers connected to /race_command before publish` が出る

確認:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 topic info /race_command -v
```

見る点:

- subscriber が 0 の場合、publisher が未起動か `source install/setup.bash` 漏れ
- launch 直後なら 1 秒待ってから再実行する

### monitor に何も流れない

確認:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 topic echo --no-daemon /race_state --once
ros2 topic echo --no-daemon /vehicle_race_status --once
```

見る点:

- `/race_state` が来るのに monitor が無反応なら monitor 側プロセス異常を疑う
- `/vehicle_race_status` が 2 台とも来ない場合は `start` 未送信か publisher 側ログを確認する
- 1 台だけしか見えない場合は `vehicle_id` を含む monitor 出力を見て 2 台分の更新有無を切り分ける

### `completed` のタイミングが期待と違う

確認:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 topic echo --no-daemon /vehicle_race_status
```

見る点:

- `has_finished=true` が 1 台だけなら race-wide completion はまだ成立しない
- `has_finished=true` が `demo_vehicle_1` と `demo_vehicle_2` の両方で観測された後に `race_state status=completed` になること

### node 重複で挙動が読めない

確認:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 node list
ps -ef | grep -E 'race_progress_demo.launch.py|race_progress_publisher|race_progress_monitor' | grep -v grep
```

見る点:

- 同じ demo を複数ターミナルで launch していないか
- `Ctrl-C` 後も親 launch か node が残っていないか

対処:

- 残っているターミナルを止める
- 状態が分からなくなったら clean start からやり直す

## Cleanup

通常は launch しているターミナル A で `Ctrl-C` します。

終了確認:

```bash
source /opt/ros/jazzy/setup.bash
source install/setup.bash
ros2 node list
```

期待:

- `race_progress_publisher`
- `race_progress_monitor`

上の 2 ノードが消えていること。
