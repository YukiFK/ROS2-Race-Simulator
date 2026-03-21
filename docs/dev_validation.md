# Development Validation

current single-vehicle demo を前提にした開発用の最小 validation 手順です。
対象は `race_progress_publisher`、`race_progress_monitor`、`race_command_cli` の組み合わせです。

## Scope

- `ros2 launch race_track race_progress_demo.launch.py` を使う current demo の確認に限定する
- 完了条件は固定 position 列の終端ではなく `target_lap_count=2` 到達とする
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

上の 2 ノードが出ていないこと。

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

### 3. Stop / Reset

停止確認:

```bash
ros2 run race_track race_command_cli stop
```

期待:

- monitor 側に `race_state status=stopped`
- 以後 `vehicle_race_status` の更新が止まる

リセット確認:

```bash
ros2 run race_track race_command_cli reset
```

期待:

- monitor 側に `race_state status=stopped`
- `lap_count` と経過時間が初期状態に戻る

## Target Lap Completion

`target_lap_count=2` 到達を確認する最小手順です。

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

- スタートライン通過時に `lap_event`
- `lap_count=1` の後も `race_state status=running`
- `lap_count=2` 到達時に `vehicle_race_status ... has_finished=true`
- 同じタイミングで `race_state status=completed`

補足:

- fixed position 列の終端に着いたこと自体は完了条件ではない
- `completed` は `lap_count >= target_lap_count` のときだけ成立する

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
- `/vehicle_race_status` が来ない場合は `start` 未送信か publisher 側ログを確認する

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
