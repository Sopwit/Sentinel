// SPDX-License-Identifier: GPL-3.0-or-later
// Test-only CLI contract fixture: never launches Docker, a shell or a container.
#include <cstdio>
#include <string_view>

int main(int argc, char** argv) {
    if (argc != 17) {
        return 90;
    }
    const char* expected[] = {"run", "--rm",       "--network",   "none", "--memory",
                              "2g",  "--cpus",     "2",           "-v",   nullptr,
                              "-w",  "/workspace", "alpine:3.20", "sh",   "-c"};
    for (int i = 1; i < 16; ++i) {
        if (expected[i - 1] && std::string_view(argv[i]) != expected[i - 1]) {
            return 91;
        }
    }
    if (!std::string_view(argv[10]).ends_with(":/workspace")) {
        return 92;
    }
    const std::string_view command(argv[16]);
    if (command == "fixture-success") {
        std::puts("DOCKER_FIXTURE_OK");
        return 0;
    }
    if (command == "fixture-daemon-unavailable") {
        std::fputs("failed to connect to the docker API\n", stderr);
        return 1;
    }
    if (command == "fixture-command-failure") {
        std::fputs("DOCKER_FIXTURE_FAILURE\n", stderr);
        return 17;
    }
    return 93;
}
