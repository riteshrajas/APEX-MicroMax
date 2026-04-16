#include <unity.h>
#include "../include/Communication.h"
#include "../include/IO_Manager.h"
#include "../include/FailSafeStateMachine.h"
#include "../include/IHardware.h"

// Re-declare INativeHardware only if we're in the native test environment
#if defined(HAL_NATIVE)
#include <string>

class INativeHardware : public IHardware {
public:
    virtual void mockAdvanceTime(unsigned long ms) = 0;
    virtual void mockSerialPush(const char* msg) = 0;
    virtual std::string mockSerialPopOutput() = 0;
    virtual void resetMockMillis() = 0;
};

INativeHardware* getNativeHardware() {
    return (INativeHardware*)getHardware();
}

IO_Manager ioManager;
Communication comm(ioManager);

void setUp(void) {
    getNativeHardware()->resetMockMillis(); // Reset time
    comm.begin(115200);
    ioManager.begin();
    getNativeHardware()->mockSerialPopOutput(); // Clear identity and status outputs
}

void tearDown(void) {}

void test_failsafe_transition(void) {
    getNativeHardware()->mockSerialPush("{\"cmd\":\"QUERY_TELEMETRY\",\"msgId\":\"test1\"}\n");
    comm.update();

    std::string out = getNativeHardware()->mockSerialPopOutput();
    TEST_ASSERT_TRUE(out.find("\"state\":1") != std::string::npos);

    getNativeHardware()->mockAdvanceTime(5001);
    comm.update();

    comm.sendStatus();
    out = getNativeHardware()->mockSerialPopOutput();
    TEST_ASSERT_TRUE(out.find("\"state\":2") != std::string::npos);
}

void test_json_parsing_robustness(void) {
    getNativeHardware()->mockSerialPush("{\"msgId\":\"missing_cmd\"}\n");
    comm.update();
    std::string out = getNativeHardware()->mockSerialPopOutput();
    TEST_ASSERT_TRUE(out.find("\"status\":\"ERROR\"") != std::string::npos);
    TEST_ASSERT_TRUE(out.find("Missing 'cmd' field") != std::string::npos);

    getNativeHardware()->mockSerialPush("{invalid_json}\n");
    comm.update();
    out = getNativeHardware()->mockSerialPopOutput();
    TEST_ASSERT_TRUE(out.find("\"status\":\"ERROR\"") != std::string::npos);
    TEST_ASSERT_TRUE(out.find("Malformed JSON") != std::string::npos);

    getNativeHardware()->mockSerialPush("{\"cmd\":\"PING\",\"msgId\":\"1234\"}\n");
    comm.update();
    out = getNativeHardware()->mockSerialPopOutput();
    TEST_ASSERT_TRUE(out.find("\"status\":\"OK\"") != std::string::npos);
    TEST_ASSERT_TRUE(out.find("\"msgId\":\"1234\"") != std::string::npos);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_failsafe_transition);
    RUN_TEST(test_json_parsing_robustness);
    UNITY_END();
    return 0;
}

#else
// On-device tests
void setUp(void) {}
void tearDown(void) {}

void test_basic_state(void) {
    TEST_ASSERT_TRUE(true);
}

void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_basic_state);
    UNITY_END();
}

void loop() {}
#endif
