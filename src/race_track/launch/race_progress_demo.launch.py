from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package="race_track",
            executable="race_progress_publisher",
            output="screen",
            parameters=[{"target_lap_count": 2}],
        ),
        Node(
            package="race_track",
            executable="race_progress_monitor",
            output="screen",
        ),
    ])
