#pragma once

#include "IHardware.h"

enum class NodeState {
    INIT,
    NORMAL,
    FAIL_SAFE
};

class FailSafeStateMachine {
private:
    NodeState currentState = NodeState::INIT;
    unsigned long lastHeartbeatTime = 0;
    const unsigned long HEARTBEAT_TIMEOUT = 5000; // 5 seconds
    IHardware* hw;

public:
    FailSafeStateMachine(IHardware* hardware) : hw(hardware) {}

    void begin() {
        currentState = NodeState::NORMAL;
        heartbeatReceived();
    }

    void update() {
        if (currentState == NodeState::NORMAL) {
            unsigned long currentMillis = hw->getMillis();
            if (currentMillis - lastHeartbeatTime > HEARTBEAT_TIMEOUT) {
                currentState = NodeState::FAIL_SAFE;
            }
        }
    }

    void heartbeatReceived() {
        lastHeartbeatTime = hw->getMillis();
        if (currentState == NodeState::FAIL_SAFE) {
            currentState = NodeState::NORMAL;
        }
    }

    NodeState getState() const {
        return currentState;
    }
};
