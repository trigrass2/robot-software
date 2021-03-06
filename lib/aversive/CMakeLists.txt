add_library(aversive
    blocking_detection_manager/blocking_detection_manager.cpp
    control_system_manager/control_system_manager.c
    math/geometry/circles.c
    math/geometry/discrete_circles.c
    math/geometry/lines.c
    math/geometry/polygon.c
    math/geometry/vect_base.c
    math/vect2/vect2.c
    obstacle_avoidance/obstacle_avoidance.c
    position_manager/position_manager.cpp
    robot_system/robot_system.c
    robot_system/angle_distance.c
    trajectory_manager/trajectory_manager.cpp
    trajectory_manager/trajectory_manager_core.cpp
    trajectory_manager/trajectory_manager_utils.cpp
)

target_include_directories(aversive PUBLIC include)
target_link_libraries(aversive error quadramp absl::synchronization)

cvra_add_test(TARGET aversive_test SOURCES
    tests/test_blocking_detection_manager.cpp
    tests/test_geometry_discrete_circles.cpp
    tests/test_geometry_polygon_intersection.cpp
    DEPENDENCIES
    aversive
)
