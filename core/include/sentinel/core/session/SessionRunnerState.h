#pragma once

namespace sentinel::core {

enum class SessionRunnerState { Idle, Running, Shell, ShellThenRun };

class SessionRunnerStateMachine final {
public:
    SessionRunnerState state() const {
        return state_;
    }
    bool beginRun() {
        if (state_ != SessionRunnerState::Idle)
            return false;
        state_ = SessionRunnerState::Running;
        return true;
    }
    bool beginShell(bool runAfter) {
        if (state_ != SessionRunnerState::Idle)
            return false;
        state_ = runAfter ? SessionRunnerState::ShellThenRun : SessionRunnerState::Shell;
        return true;
    }
    bool finish() {
        if (state_ == SessionRunnerState::Idle)
            return false;
        state_ = SessionRunnerState::Idle;
        return true;
    }

private:
    SessionRunnerState state_ = SessionRunnerState::Idle;
};

} // namespace sentinel::core
