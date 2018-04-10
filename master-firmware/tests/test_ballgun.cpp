#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

#include <ballgun/ballgun.h>

namespace {
void set_servo_pos(void *s, float value) { *(float *)s = value; }
void set_turbine_speed(void *s, float value) { *(float *)s = value; }
}

TEST_GROUP(ABallGun)
{
    ballgun_t ballgun;
    float ballgun_servo_pos;
    float ballgun_tubine_speed;

    void setup()
    {
        ballgun_init(&ballgun);
        ballgun_set_callbacks(&ballgun, &set_servo_pos, &ballgun_servo_pos);
        ballgun_set_turbine_callbacks(&ballgun, &set_turbine_speed, &ballgun_tubine_speed);

        ballgun_set_servo_range(&ballgun, 0.001, 0.002);
        ballgun_set_turbine_range(&ballgun, 0.0, -0.001, 0.001);
    }

    void teardown()
    {
        lock_mocks_enable(false);
    }
};

TEST(ABallGun, initializesInDisabledState)
{
    CHECK_EQUAL(BALLGUN_DISABLED, ballgun.state);
    CHECK_EQUAL(BALLGUN_ARMED, ballgun.turbine_state);
}

TEST(ABallGun, deploys)
{
    ballgun_deploy(&ballgun);

    CHECK_EQUAL(BALLGUN_DEPLOYED, ballgun.state);
    CHECK(ballgun_servo_pos != 0.0f);
}

TEST(ABallGun, retracts)
{
    ballgun_retract(&ballgun);

    CHECK_EQUAL(BALLGUN_RETRACTED, ballgun.state);
    CHECK(ballgun_servo_pos != 0.0f);
}

TEST(ABallGun, charges)
{
    ballgun_charge(&ballgun);

    CHECK_EQUAL(BALLGUN_CHARGING, ballgun.turbine_state);
    CHECK(ballgun_tubine_speed < 0.0f);
}

TEST(ABallGun, fires)
{
    ballgun_fire(&ballgun);

    CHECK_EQUAL(BALLGUN_FIRING, ballgun.turbine_state);
    CHECK(ballgun_tubine_speed > 0.0f);
}

TEST(ABallGun, armsAfterFiring)
{
    ballgun_fire(&ballgun);
    ballgun_arm(&ballgun);

    CHECK_EQUAL(BALLGUN_ARMED, ballgun.turbine_state);
    CHECK(ballgun_tubine_speed == 0.0f);
}
