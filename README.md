# ROS2-Race-Simulator
ROS2 Race Simulator

## Minimal Demo

最小 race progress demo は `race_progress_publisher`、`race_progress_monitor`、`race_command_cli` で構成されています。
current demo は 2 台の固定 participating vehicles を前提にしています。
race-wide completion は fixed position 列の終端ではなく、全 participating vehicles が `race_progress_publisher` の `target_lap_count` に到達したときに成立します。
build / launch / command / expected behavior は [Milestone 1](docs/specs/milestone1.md) にあります。

## Docs

- [Project Status](docs/status/PROJECT_STATUS.md)
- [Handoff For ChatGPT](docs/status/HANDOFF_FOR_CHATGPT.md)
- [Development Validation](docs/process/dev_validation.md)
- [Review Checklist](docs/process/review_checklist.md)
- [Milestone 1](docs/specs/milestone1.md)
- [First Multi-Vehicle Implementation Slice](docs/design/first_multi_vehicle_slice.md)
- [Message Responsibility Boundary](docs/design/message_boundary.md)
- [Multi-Vehicle Design Memo](docs/design/multi_vehicle_design.md)
- [Multi-Vehicle Runtime Model](docs/design/multi_vehicle_runtime_model.md)
- [Race Completion Semantics](docs/design/race_completion_semantics.md)
