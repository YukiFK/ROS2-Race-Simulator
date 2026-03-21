from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription([
        Node(
            package="race_track",
            executable="race_progress_publisher",
            output="screen",
        ),
        Node(
            package="race_track",
            executable="race_progress_monitor",
            output="screen",
        ),
    ])
