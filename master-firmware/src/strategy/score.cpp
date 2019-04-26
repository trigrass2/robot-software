#include "table.h"
#include "score.h"

int score_count_atoms_in_zone(const RobotState& state, PuckColor color)
{
    return state.pucks_in_deposit_zone[color] * 6;
}

int score_count_accelerator(const RobotState& state)
{
    return state.accelerator_is_done ? 20 : 0;
}

int score_count_experiment(const RobotState& state)
{
    return state.electron_launched ? 20 : 5;
}
