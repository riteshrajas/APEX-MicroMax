# APEX MicroMax HAL Research

## Objective
Support multi-board deployment (ATmega328P, RP2040, ESP32) without polluting the core logic with `#ifdef` macros.

## Investigated Patterns

### 1. Interface-based Delegation (Runtime Polymorphism)
This uses standard C++ virtual functions.
```cpp
class IHardware {
public:
    virtual void writeDigital(uint8_t pin, uint8_t val) = 0;
    virtual unsigned long getMillis() = 0;
};
```
- **Pros:** Extremely clean separation of concerns. Easy to mock for unit tests.
- **Cons:** Virtual function overhead (vtable lookup) can be noticeable on very constrained devices, though often negligible for standard GPIO.

### 2. Template-based Polymorphism / CRTP (Compile-time Polymorphism)
```cpp
template <typename T>
class CoreLogic {
    T hardware;
public:
    void loop() {
        hardware.writeDigital(1, 1);
    }
};
```
- **Pros:** Zero runtime overhead. Method calls are resolved at compile time.
- **Cons:** Code can become hard to read. Harder to mock cleanly in standard OOP frameworks without template mocks.

### 3. Build-System Selection
We define a single `IHardware` interface. The implementation (e.g., `HardwareAVR.cpp`, `HardwareESP32.cpp`) is selected by the build system (PlatformIO) based on the environment. There is no `#ifdef` in the main code.

## Conclusion
Given the memory constraints of the ATmega328P and the need for high-reliability, a combination of **Interface-based Delegation** and **Build-System Selection** is the best approach.

We define a pure abstract base class `IHardware`. PlatformIO compiles the correct `src/hal/Hardware*.cpp` based on the active environment. This keeps the core business logic (`Communication.cpp`, `IO_Manager.cpp`) 100% agnostic to the hardware platform while allowing for easy dependency injection during unit testing via a Native test environment.
