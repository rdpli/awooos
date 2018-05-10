#include <ali/event.h>
#include <ali/text.h>
#include <awoo/build_info.h>
#include <dmm.h>
#include <flail.h>
#include <hal.h>
#include <tinker.h>

#define ADD_TESTS(TEST_CATEGORY) { \
        print("Adding " #TEST_CATEGORY " tests... ");  \
        add_##TEST_CATEGORY##_tests();                  \
        print("Done!\n");                              \
    };

void test_shutdown(bool all_tests_passed)
{
    // If a test failed, do a test-fail shutdown.
    if (!all_tests_passed) {
        print("Encountered failing tests; not booting.\n");
        event_trigger("HAL shutdown test fail", NULL, 0);
    }

    // If we get this far, we assume all of the tests passed.
    event_trigger("HAL shutdown", NULL, 0);
}

void run_tests()
{
    bool all_tests_passed;

    ADD_TESTS(hal);
    ADD_TESTS(ali);
    ADD_TESTS(dmm);

    all_tests_passed = tinker_run_tests(&print);

    // Handle things we _only_ do in test builds.
    if (AWOO_BUILD_TYPE_NUMBER == AWOO_TEST_BUILD) {
        // This doesn't catch all failures, but it'll catch
        // unintentional infinite loops by causing the CI
        // build to time out.
        //
        // We currently only run this during test builds,
        // to avoid concerning people
        test_flail_intentional_panic();

        // Test builds always shut down.
        // The `all_tests_passed` argument dictates
        // if it tries to tell QEMU to return a nonzero exit code.
        test_shutdown(all_tests_passed);
    }
}

void tests_register_events()
{
    event_watch("tests run", run_tests);
}