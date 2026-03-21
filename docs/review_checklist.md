# Review Checklist

PR 前の review 観点を固定するための checklist です。
current single-vehicle demo を前提に、`実装PR` と `docs PR` を分けて使います。

## 実装PR

### Scope

- 変更内容が current single-vehicle demo の前提を壊していない
- 実装以上の仕様を docs や PR description で主張していない
- 変更スコープ外の挙動を暗黙に変えていない

### Build And Validation

- `colcon build --packages-select race_track race_interfaces` が通る
- `source install/setup.bash` 後に demo を起動している
- `ros2 launch race_track race_progress_demo.launch.py` で起動できる
- `ros2 run race_track race_command_cli start` で進行開始する
- `ros2 run race_track race_command_cli stop` で停止する
- `ros2 run race_track race_command_cli reset` で初期状態に戻る
- `target_lap_count=2` 到達時に `race_state status=completed` と `has_finished=true` が揃う

### Behavior

- `/race_command` の publish-subscribe 関係が崩れていない
- `/race_state` と `/vehicle_race_status` の最低限の観測ができる
- node 重複や stale process があるときの挙動悪化を招いていない
- current demo の完了条件を fixed position 列の終端に戻していない

### Review Focus

- node 名、topic 名、message 型の変更は必要性が説明されている
- single-vehicle 前提の固定値を変えた場合、影響範囲が明示されている
- ログや状態遷移の変更が validation 手順で追える
- 手元確認で使ったコマンドと観測結果が PR description にある
- 必要なテスト追加や既存テスト更新が漏れていない

## docs PR

### Scope

- docs が current single-vehicle demo の実装範囲に収まっている
- 将来仕様や未実装機能を事実のように書いていない
- 実装変更なしで成立する記述だけにしている

### Accuracy

- build / launch / command のコマンドがそのままコピペできる
- package 名、launch 名、node 名、topic 名が実装と一致する
- `target_lap_count=2` と完了条件の説明が実装と一致する
- troubleshooting の切り分け手順が実装済みの観測手段に限定されている

### Maintainability

- 長すぎず、普段の開発で参照しやすい粒度になっている
- 手順と checklist が重複しすぎていない
- README への追記がある場合は導線だけに留めている
- 大規模な仕様整理や README 全面更新を混ぜていない
