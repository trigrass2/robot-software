#ifndef MANIPULATOR_PATH_H
#define MANIPULATOR_PATH_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MANIPULATOR_INIT = 0,
    MANIPULATOR_RETRACT,
    MANIPULATOR_DEPLOY,
    MANIPULATOR_DEPLOY_FULLY,
    MANIPULATOR_LIFT_HORZ,
    MANIPULATOR_PICK_HORZ,
    MANIPULATOR_PICK_VERT,
    MANIPULATOR_LIFT_VERT,
    MANIPULATOR_PICK_GOLDONIUM,
    MANIPULATOR_LIFT_GOLDONIUM,
    MANIPULATOR_COUNT // Dummy, used for last element
} manipulator_state_t;

#ifdef __cplusplus
}
#endif

#endif /* MANIPULATOR_PATH_H */
