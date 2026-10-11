use serde::{Deserialize, Serialize};
use serde_json::Value;

#[derive(Clone, Copy, Debug, Default, Eq, PartialEq, Hash, Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum State {
    #[default]
    Idle,
    Listening,
    Thinking,
    Processing,
    ToolRunning,
    Approval,
    Speaking,
    Success,
    Warning,
    Error,
}
impl State {
    pub const ALL: [Self; 10] = [
        Self::Idle,
        Self::Listening,
        Self::Thinking,
        Self::Processing,
        Self::ToolRunning,
        Self::Approval,
        Self::Speaking,
        Self::Success,
        Self::Warning,
        Self::Error,
    ];
    pub fn id(self) -> &'static str {
        match self {
            Self::Idle => "idle",
            Self::Listening => "listening",
            Self::Thinking => "thinking",
            Self::Processing => "processing",
            Self::ToolRunning => "tool_running",
            Self::Approval => "approval",
            Self::Speaking => "speaking",
            Self::Success => "success",
            Self::Warning => "warning",
            Self::Error => "error",
        }
    }
    pub fn label(self) -> &'static str {
        match self {
            Self::Idle => "Ready",
            Self::Listening => "Listening",
            Self::Thinking => "Thinking",
            Self::Processing => "Processing",
            Self::ToolRunning => "Tool running",
            Self::Approval => "Approval required",
            Self::Speaking => "Speaking",
            Self::Success => "Completed",
            Self::Warning => "Attention required",
            Self::Error => "Unavailable",
        }
    }
}

/// Feed only already authenticated, session/run/sequence-filtered daemon events.
/// This projection cannot start a task or decide that a run is accepted.
#[derive(Default, Debug)]
pub struct Projection {
    pub connected: bool,
    pub state: State,
    active: bool,
}
impl Projection {
    pub fn disconnected(&mut self) {
        self.connected = false;
        self.active = false;
        self.state = State::Error;
    }
    pub fn ready(&mut self) {
        self.connected = true;
        self.active = false;
        self.state = State::Idle;
    }
    pub fn model_pending(&mut self) {
        if self.connected {
            self.active = true;
            self.state = State::Thinking;
        }
    }
    pub fn snapshot(&mut self, payload: &Value) {
        if !self.connected {
            return;
        }
        self.active = matches!(
            payload["state"].as_str(),
            Some("starting" | "running" | "approval" | "cancelling")
        );
        self.state = match payload["state"].as_str().unwrap_or("idle") {
            "starting" | "running" => State::Thinking,
            "approval" => State::Approval,
            "cancelling" | "cancelled" => State::Warning,
            "failed" => State::Error,
            // A generic completed snapshot does not certify accepted Agent output.
            _ => State::Idle,
        };
    }
    pub fn event(&mut self, name: &str) {
        if !self.connected {
            return;
        }
        match name {
            "run.started" => {
                self.active = true;
                self.state = State::Thinking;
            }
            "tool.result" if self.active => self.state = State::Thinking,
            "tool.running" if self.active => self.state = State::ToolRunning,
            "approval.requested" if self.active => self.state = State::Approval,
            "run.failed" => {
                self.active = false;
                self.state = State::Error;
            }
            "run.cancelled" => {
                self.active = false;
                self.state = State::Warning;
            }
            // Completion display remains the TUI's accepted-result authority; no success inference.
            "run.completed" => {
                self.active = false;
                self.state = State::Idle;
            }
            _ => {}
        }
    }
}
