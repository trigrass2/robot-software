#include <CppUTest/TestHarness.h>
#include <CppUTestExt/MockSupport.h>

#include <error/error.h>

TEST_GROUP (ErrorLogging) {
    void teardown() override
    {
        error_register_error(nullptr);
        error_register_warning(nullptr);
        error_register_notice(nullptr);
        error_register_debug(nullptr);
    }
};

void error_mock(struct error* e, ...)
{
    mock().actualCall("error").withIntParameter("severity", e->severity).withStringParameter("file", e->file).withIntParameter("line", e->line).withStringParameter("text", e->text);
}

TEST(ErrorLogging, PopulatesFieldError)
{
    auto error = error_generate(ERROR_SEVERITY_WARNING,
                                "foo", __FILE__, 120);

    CHECK_EQUAL(ERROR_SEVERITY_WARNING, error.severity);
    STRCMP_EQUAL("foo", error.text);
    CHECK_EQUAL(120, error.line);
    STRCMP_EQUAL(__FILE__, error.file);
}

TEST(ErrorLogging, MockWorks)
{
    auto error = error_generate(ERROR_SEVERITY_WARNING,
                                "foo", __FILE__, 120);
    mock().expectOneCall("error").withIntParameter("severity", ERROR_SEVERITY_WARNING).withIntParameter("line", 120).withStringParameter("text", "foo").withStringParameter("file", __FILE__);
    error_mock(&error);
}

TEST(ErrorLogging, GeneratesError)
{
    error_register_error(error_mock);
    mock().expectOneCall("error").withIntParameter("severity", ERROR_SEVERITY_ERROR).withIntParameter("line", __LINE__ + 2).withStringParameter("text", "foo").withStringParameter("file", __FILE__);

    ERROR("foo");
}

TEST(ErrorLogging, GeneratesWarning)
{
    error_register_warning(error_mock);
    mock().expectOneCall("error").withIntParameter("severity", ERROR_SEVERITY_WARNING).withIntParameter("line", __LINE__ + 2).withStringParameter("text", "foo").withStringParameter("file", __FILE__);

    WARNING("foo");
}

TEST(ErrorLogging, GeneratesNotice)
{
    error_register_notice(error_mock);
    mock().expectOneCall("error").withIntParameter("severity", ERROR_SEVERITY_NOTICE).withIntParameter("line", __LINE__ + 2).withStringParameter("text", "foo").withStringParameter("file", __FILE__);

    NOTICE("foo");
}

TEST(ErrorLogging, GeneratesDebug)
{
    error_register_debug(error_mock);
    mock().expectOneCall("error").withIntParameter("severity", ERROR_SEVERITY_DEBUG).withIntParameter("line", __LINE__ + 2).withStringParameter("text", "foo").withStringParameter("file", __FILE__);

    DEBUG("foo");
}

TEST(ErrorLogging, GeneratesWarningEvery10)
{
    error_register_warning(error_mock);

    // We only expect two calls to occur
    mock().expectOneCall("error").withStringParameter("text", "foo").ignoreOtherParameters();
    mock().expectOneCall("error").withStringParameter("text", "bar").ignoreOtherParameters();
    mock().expectOneCall("error").withStringParameter("text", "foo").ignoreOtherParameters();
    mock().expectOneCall("error").withStringParameter("text", "bar").ignoreOtherParameters();

    for (int i = 0; i < 20; i++) {
        // Log every 10 loop iteration
        WARNING_EVERY_N(10, "foo");
        WARNING_EVERY_N(10, "bar");
    }
}

TEST(ErrorLogging, GeneratesNoticeEvery10)
{
    error_register_notice(error_mock);

    // We only expect two calls to occur
    mock().expectOneCall("error").withStringParameter("text", "foo").ignoreOtherParameters();
    mock().expectOneCall("error").withStringParameter("text", "bar").ignoreOtherParameters();
    mock().expectOneCall("error").withStringParameter("text", "foo").ignoreOtherParameters();
    mock().expectOneCall("error").withStringParameter("text", "bar").ignoreOtherParameters();

    for (int i = 0; i < 20; i++) {
        // Log every 10 loop iteration
        NOTICE_EVERY_N(10, "foo");
        NOTICE_EVERY_N(10, "bar");
    }
}

TEST(ErrorLogging, GeneratesDebugEvery10)
{
    error_register_debug(error_mock);

    // We only expect two calls to occur
    mock().expectOneCall("error").withStringParameter("text", "foo").ignoreOtherParameters();
    mock().expectOneCall("error").withStringParameter("text", "bar").ignoreOtherParameters();
    mock().expectOneCall("error").withStringParameter("text", "foo").ignoreOtherParameters();
    mock().expectOneCall("error").withStringParameter("text", "bar").ignoreOtherParameters();

    for (int i = 0; i < 20; i++) {
        // Log every 10 loop iteration
        DEBUG_EVERY_N(10, "foo");
        DEBUG_EVERY_N(10, "bar");
    }
}

TEST(ErrorLogging, ErrorName)
{
    const auto* name = error_severity_get_name(ERROR_SEVERITY_ERROR);
    STRCMP_EQUAL("ERROR", name);
}

TEST(ErrorLogging, WarningName)
{
    const auto* name = error_severity_get_name(ERROR_SEVERITY_WARNING);
    STRCMP_EQUAL("WARNING", name);
}

TEST(ErrorLogging, NoticeName)
{
    const auto* name = error_severity_get_name(ERROR_SEVERITY_NOTICE);
    STRCMP_EQUAL("NOTICE", name);
}

TEST(ErrorLogging, DebugName)
{
    const auto* name = error_severity_get_name(ERROR_SEVERITY_DEBUG);
    STRCMP_EQUAL("DEBUG", name);
}

TEST(ErrorLogging, UnknownName)
{
    auto invalid_severity = 42;
    const auto* name = error_severity_get_name(invalid_severity);
    STRCMP_EQUAL("UNKNOWN", name);
}
