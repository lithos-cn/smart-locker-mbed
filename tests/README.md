# Native tests

From the repository root, use Python and a GCC/Clang-compatible native C++ compiler:

```sh
python tests/run_tests.py --compiler g++
```

Alternatively, use Zig's bundled C++ compiler:

```sh
python tests/run_tests.py --zig /path/to/zig
```

Tests were run successfully on Windows using Zig 0.13.0, C++14, and `-Wall -Wextra -Werror -pedantic`:

```text
control tests passed
driver tests passed
Main program mock-interface compile check passed. No ARM firmware or hardware test is implied.
```

`test_control.cpp` checks signed temperature and acceleration conversion, arbitrary resting orientation, independent stable-state button debounce, bounce/hold/release behavior, long uptime, repeated correct and incorrect passwords, feedback expiry, concurrent environmental alarms, and sensor faults.

`test_drivers.cpp` runs the production sensor drivers against in-memory register/bus doubles. It checks ADXL345 identity/configuration/multi-byte decoding, TMP102 configuration and repeated-start reads, signed temperature values, and failed transactions. `main.cpp` is also compiled against the mock interfaces.

The fake header is for these tests only. The firmware CMake target uses the real Mbed headers and explicitly lists only the production sources. Native tests do not demonstrate ARM linking, interrupt latency, real peripheral behavior, or successful operation on a board.
