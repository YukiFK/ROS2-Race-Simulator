# ROS2-Race-Simulator
ROS2 Race Simulator

## Minimal Demo

最小 race progress demo は `race_progress_publisher`、`race_progress_monitor`、`race_command_cli` で構成されています。
single-vehicle demo の完了条件は固定 position 列の終端ではなく、`race_progress_publisher` の `target_lap_count` 到達です。
build / launch / command / expected behavior は [Milestone 1](docs/milestone1.md) にあります。

## Docs

- [Milestone 1](docs/milestone1.md)
