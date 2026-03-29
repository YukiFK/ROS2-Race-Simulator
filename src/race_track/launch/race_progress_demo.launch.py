from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    return LaunchDescription([
        DeclareLaunchArgument(
            "publisher_params_file",
            default_value=PathJoinSubstitution(
                [FindPackageShare("race_track"), "race_progress_demo.publisher.yaml"]
            ),
        ),
        Node(
            package="race_track",
            executable="race_progress_publisher",
            output="screen",
            parameters=[LaunchConfiguration("publisher_params_file")],
        ),
        Node(
            package="race_track",
            executable="race_progress_monitor",
            output="screen",
        ),
    ])
