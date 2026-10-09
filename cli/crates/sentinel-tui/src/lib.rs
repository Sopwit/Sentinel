mod editor;
mod picker;
use crossterm::{
    event::{
        self, DisableBracketedPaste, EnableBracketedPaste, Event, KeyCode, KeyEventKind,
        KeyModifiers,
    },
    execute,
};
use editor::Editor;
use picker::{Item, Picker};
use ratatui::{
    Frame,
    layout::{Alignment, Constraint, Layout, Rect},
    style::{Color, Modifier, Style},
    text::Line,
    widgets::{Block, Clear, Paragraph, Wrap},
};
use sentinel_ipc::{Client, Envelope, Error};
use serde_json::{Value, json};
use std::{
    cell::Cell,
    path::Path,
    sync::{
        Arc,
        atomic::{AtomicBool, Ordering},
        mpsc::{self, Receiver, SyncSender},
    },
    time::{Duration, Instant},
};
const COMMANDS: &[(&str, &str)] = &[
    ("/reconnect", "Reconnect without replaying work"),
    ("/remove", "Remove last selected file reference"),
    ("/model", "Select a discovered model"),
    ("/provider", "Inspect providers and select a model"),
    ("/workspace", "Select a daemon-owned workspace"),
    ("/sessions", "Attach a session"),
    ("/new", "Create a session"),
    ("/chat", "Switch explicitly to Chat"),
    ("/agent", "Switch explicitly to Agent"),
    ("/tools", "Tool registry state"),
    ("/mcp", "MCP state"),
    ("/permissions", "Permission service state"),
    ("/context", "Authoritative context state"),
    ("/memory", "Memory state"),
    ("/tasks", "Task and subagent state"),
    ("/status", "Daemon connectivity and status"),
    ("/doctor", "Service diagnostics"),
    ("/compact", "Explain context compaction availability"),
    ("/help", "Keybindings and commands"),
    ("/activity", "Inspect safe structured runtime activity"),
    ("/files", "File context availability"),
    ("/diff", "Change review availability"),
];
const HELP: &str = "Enter: send · Ctrl+O: newline · Alt/Shift+Enter: newline when supported\nCtrl+P / Tab: commands · Ctrl+L: models · Ctrl+W: workspaces\n/sessions: sessions · Ctrl+N: new · /reconnect: reconnect\nCtrl+R / Ctrl+F: transcript search · F1 or /help: help\nCtrl+A/E: line start/end · Ctrl+U/K: erase to start/end\nArrows: edit · Alt+Up/Down: draft history · Ctrl+Left/Right: word\nPgUp/PgDn: scroll · Ctrl+B: follow latest · /activity: tool details\nCtrl+C: cancel run; idle draft is preserved · Ctrl+D: exit only idle/empty\n/remove: remove last file reference · @: authorized file picker\nPicker: type to filter, Up/Down or Tab/Shift+Tab, Enter, Esc\nApproval: y Allow Once / n Deny; Esc preserves pending approval\nPaste never sends. Input recall is memory-only; sessions belong to daemon.";

// A guard complements Ratatui's panic hook on recoverable error/early-return paths.
struct TerminalCleanup;
impl Drop for TerminalCleanup {
    fn drop(&mut self) {
        let _ = execute!(std::io::stdout(), DisableBracketedPaste);
        ratatui::restore();
    }
}
fn newline_key(key: crossterm::event::KeyEvent) -> bool {
    (key.code == KeyCode::Char('o') && key.modifiers.contains(KeyModifiers::CONTROL))
        || (key.code == KeyCode::Enter
            && key
                .modifiers
                .intersects(KeyModifiers::ALT | KeyModifiers::SHIFT))
}
struct Request {
    name: &'static str,
    payload: Value,
    tag: String,
}
enum Update {
    Reply(String, Value),
    Event(Envelope),
    Failure(String, Error),
    Connection(bool),
}
struct Worker {
    tx: SyncSender<Request>,
    rx: Receiver<Update>,
    stop: Arc<AtomicBool>,
}
impl Drop for Worker {
    fn drop(&mut self) {
        self.stop.store(true, Ordering::Relaxed);
    }
}
fn worker(path: &Path) -> Worker {
    let path = path.to_owned();
    let (tx, commands) = mpsc::sync_channel::<Request>(32);
    let (updates, rx) = mpsc::sync_channel(256);
    let stop = Arc::new(AtomicBool::new(false));
    let stopping = stop.clone();
    std::thread::spawn(move || {
        let mut client: Option<Client> = None;
        let mut sid: Option<String> = None;
        let mut retry = Instant::now();
        while !stopping.load(Ordering::Relaxed) {
            if client.is_none() && retry.elapsed() >= Duration::from_secs(1) {
                match Client::connect(&path, "sentinel-tui") {
                    Ok(mut c) => {
                        let _ = c.set_read_timeout(Some(Duration::from_secs(10)));
                        let mut restored = true;
                        if let Some(id) = &sid {
                            match c.request("terminal.attach", json!({"session_id":id})) {
                                Ok(snapshot) => {
                                    if updates
                                        .send(Update::Reply("attach".into(), snapshot))
                                        .is_err()
                                    {
                                        return;
                                    }
                                }
                                Err(e) => {
                                    restored = false;
                                    if updates.send(Update::Failure("attach".into(), e)).is_err() {
                                        return;
                                    }
                                }
                            }
                        }
                        if restored {
                            client = Some(c);
                            let _ = updates.send(Update::Connection(true));
                        }
                    }
                    Err(e) => {
                        let _ = updates.send(Update::Failure("connection".into(), e));
                    }
                }
                retry = Instant::now();
            }
            match commands.recv_timeout(Duration::from_millis(25)) {
                Ok(request) => {
                    if request.name == "reconnect" {
                        client = None;
                        retry = Instant::now() - Duration::from_secs(2);
                        continue;
                    }
                    if client.is_none() {
                        match Client::connect(&path, "sentinel-tui") {
                            Ok(mut c) => {
                                if let Some(id) = &sid {
                                    match c.request("terminal.attach", json!({"session_id":id})) {
                                        Ok(snapshot) => {
                                            let _ = updates
                                                .send(Update::Reply("attach".into(), snapshot));
                                        }
                                        Err(error) => {
                                            let _ = updates
                                                .send(Update::Failure("attach".into(), error));
                                        }
                                    }
                                }
                                client = Some(c);
                            }
                            Err(e) => {
                                let _ = updates.send(Update::Failure(request.tag, e));
                                continue;
                            }
                        }
                    }
                    let c = client.as_mut().unwrap();
                    let _ = c.set_read_timeout(Some(Duration::from_secs(10)));
                    match c.request(request.name, request.payload) {
                        Ok(value) => {
                            if ["attach", "new", "start"].contains(&request.tag.as_str()) {
                                sid = value["session_id"].as_str().map(str::to_owned);
                            }
                            if updates.send(Update::Reply(request.tag, value)).is_err() {
                                return;
                            }
                        }
                        Err(e) => {
                            let disconnected = matches!(
                                e,
                                Error::Disconnected | Error::Unavailable(_) | Error::Timeout
                            );
                            let _ = updates.send(Update::Failure(request.tag, e));
                            if disconnected {
                                client = None;
                            }
                        }
                    }
                }
                Err(mpsc::RecvTimeoutError::Disconnected) => return,
                Err(mpsc::RecvTimeoutError::Timeout) => {}
            }
            if let Some(c) = &mut client {
                let _ = c.set_read_timeout(Some(Duration::from_millis(25)));
                match c.next_event() {
                    Ok(e) => {
                        if updates.send(Update::Event(e)).is_err() {
                            return;
                        }
                    }
                    Err(Error::Timeout) => {}
                    Err(e) => {
                        client = None;
                        retry = Instant::now();
                        let _ = updates.send(Update::Connection(false));
                        let _ = updates.send(Update::Failure("connection".into(), e));
                    }
                }
            }
        }
    });
    Worker { tx, rx, stop }
}
#[derive(Default)]
struct App {
    cancel_after_start: bool,
    pending_text: String,
    pending_references: Vec<String>,
    latest_user: String,
    changes: Vec<Value>,
    references: Vec<String>,
    editor: Editor,
    sessions: Vec<Value>,
    models: Vec<Value>,
    diagnostics: Value,
    sid: String,
    title: String,
    run_id: String,
    state: String,
    agent: bool,
    connected: bool,
    output: String,
    activity: Vec<String>,
    approval: Option<Value>,
    approval_pending: bool,
    approval_allow: Option<bool>,
    picker: Option<Picker>,
    notice: String,
    busy: bool,
    scroll: u16,
    viewport_bottom: Cell<u16>,
    follow: bool,
    search: Option<String>,
    last_sequence: u64,
    generation: String,
    attach: Option<String>,
}
impl App {
    fn active(&self) -> bool {
        ["starting", "running", "approval", "cancelling"].contains(&self.state.as_str())
    }
    fn request(&mut self, worker: &Worker, name: &'static str, payload: Value, tag: &str) {
        if worker
            .tx
            .try_send(Request {
                name,
                payload,
                tag: tag.into(),
            })
            .is_err()
        {
            self.notice =
                "Connection queue unavailable. Draft preserved; wait for connection recovery."
                    .into();
            if tag == "start" {
                self.state = "rejected".into();
                self.busy = false;
                self.editor.insert(&std::mem::take(&mut self.pending_text));
                self.references.append(&mut self.pending_references);
            }
        }
    }
    fn refresh(&mut self, worker: &Worker) {
        self.request(worker, "terminal.state", json!({}), "diagnostics");
        self.request(worker, "model.list", json!({}), "models");
        self.request(worker, "session.list", json!({}), "sessions");
    }
    fn snapshot(&mut self, value: Value) {
        if value["run_id"].as_str().unwrap_or("") != self.run_id {
            if self.state != "starting" {
                self.latest_user.clear();
            }
            self.activity.clear();
        }
        self.sid = value["session_id"].as_str().unwrap_or("").into();
        self.title = value["title"].as_str().unwrap_or("Terminal session").into();
        self.run_id = value["run_id"].as_str().unwrap_or("").into();
        self.state = value["state"].as_str().unwrap_or("idle").into();
        self.agent = value["run_type"].as_str() == Some("agent");
        self.output = value["output"].as_str().unwrap_or("").into();
        self.approval = if self.state == "approval" {
            value.get("approval").filter(|p| p.is_object()).cloned()
        } else {
            None
        };
        self.approval_pending = false;
        self.last_sequence = value["event_sequence"].as_u64().unwrap_or(0);
        self.generation = value["server_generation"].as_str().unwrap_or("").into();
        self.busy = false;
        if value["output_truncated"] == true {
            self.notice = "Snapshot output is truncated by the daemon.".into();
        }
    }
    fn update(&mut self, update: Update, worker: &Worker) {
        match update {
            Update::Connection(connected) => {
                let recovered = connected && !self.connected;
                self.connected = connected;
                if recovered {
                    self.refresh(worker);
                }
            }
            Update::Failure(tag, error) => {
                if matches!(
                    error,
                    Error::Disconnected | Error::Unavailable(_) | Error::Timeout
                ) {
                    self.connected = false;
                }
                self.notice = error.to_string();
                self.busy = false;
                if tag == "approval" {
                    self.approval_pending = false;
                }
                if tag == "start" {
                    self.state = "rejected".into();
                    if self.editor.text.is_empty() {
                        self.editor.insert(&std::mem::take(&mut self.pending_text));
                    }
                    self.references.append(&mut self.pending_references);
                    self.cancel_after_start = false;
                }
                if tag == "connection" {
                    self.connected = false;
                }
            }
            Update::Reply(tag, value) => {
                self.connected = true;
                match tag.as_str() {
                    "doctor" => {
                        self.output = serde_json::to_string_pretty(&value).unwrap_or_default();
                        self.diagnostics = value;
                    }
                    "diagnostics" => {
                        self.diagnostics = value;
                    }
                    "changes" => {
                        self.changes = value["files"].as_array().cloned().unwrap_or_default();
                        self.output = format!(
                            "{}\n{}",
                            value["reason"]
                                .as_str()
                                .unwrap_or("Change review unavailable."),
                            if value["truncated"] == true {
                                "Review is truncated; omissions are not evidence of no changes."
                            } else {
                                ""
                            }
                        );
                        let items = self
                            .changes
                            .iter()
                            .enumerate()
                            .map(|(i, file)| Item {
                                label: format!(
                                    "{} · Applied",
                                    file["path"].as_str().unwrap_or("?")
                                ),
                                id: i.to_string(),
                                extra: String::new(),
                                enabled: true,
                            })
                            .collect();
                        self.picker = Some(Picker {
                            kind: "review",
                            query: String::new(),
                            selected: 0,
                            items,
                        });
                    }
                    "files" => {
                        let root = value["root"].as_str().unwrap_or("");
                        let items = value["files"]
                            .as_array()
                            .into_iter()
                            .flatten()
                            .filter_map(Value::as_str)
                            .map(|path| Item {
                                label: path
                                    .strip_prefix(root)
                                    .unwrap_or(path)
                                    .trim_start_matches('/')
                                    .into(),
                                id: path.into(),
                                extra: String::new(),
                                enabled: true,
                            })
                            .collect();
                        self.picker = Some(Picker {
                            kind: "file",
                            query: String::new(),
                            selected: 0,
                            items,
                        });
                        if value["truncated"] == true {
                            self.notice =
                                "File list is truncated by the authoritative tool.".into();
                        }
                    }
                    "models" => {
                        self.models = value["models"].as_array().cloned().unwrap_or_default();
                    }
                    "sessions" => {
                        self.sessions = value["sessions"].as_array().cloned().unwrap_or_default();
                        if self.sid.is_empty() {
                            if let Some(id) = self.attach.take().or_else(|| {
                                self.sessions
                                    .first()
                                    .and_then(|s| s["session_id"].as_str())
                                    .map(str::to_owned)
                            }) {
                                self.request(
                                    worker,
                                    "terminal.attach",
                                    json!({"session_id":id}),
                                    "attach",
                                );
                            } else {
                                self.request(
                                    worker,
                                    "session.create",
                                    json!({"title":"Terminal session"}),
                                    "new",
                                );
                            }
                        }
                    }
                    "attach" | "new" => {
                        self.snapshot(value);
                        self.request(
                            worker,
                            "session.messages",
                            json!({"session_id":self.sid}),
                            "messages",
                        );
                        self.refresh(worker);
                    }
                    "messages" => {
                        if !self.active() {
                            self.output = value["messages"]
                                .as_array()
                                .into_iter()
                                .flatten()
                                .map(|m| {
                                    format!(
                                        "{}\n{}\n",
                                        m["role"].as_str().unwrap_or("message"),
                                        m["content"].as_str().unwrap_or("")
                                    )
                                })
                                .collect::<Vec<_>>()
                                .join("\n");
                        }
                    }
                    "start" => {
                        self.pending_text.clear();
                        self.pending_references.clear();
                        self.run_id = value["run_id"].as_str().unwrap_or("").into();
                        self.state = "running".into();
                        if self.cancel_after_start {
                            self.cancel_after_start = false;
                            self.state = "cancelling".into();
                            self.request(
                                worker,
                                "run.cancel",
                                json!({"run_id":self.run_id}),
                                "cancel",
                            );
                        }
                        self.busy = false;
                        self.output.clear();
                        self.activity.clear();
                        self.follow = true;
                    }
                    "cancel" => {
                        self.state = "cancelling".into();
                        self.notice =
                            "Cancellation requested; waiting for the daemon's terminal state."
                                .into();
                    }
                    "approval" => {
                        if value["accepted"] == true {
                            self.approval = None;
                            self.approval_pending = false;
                            if self.state == "approval" {
                                self.state = "running".into();
                            }
                            self.notice = if self.approval_allow == Some(false) {
                                "Deny accepted by daemon."
                            } else {
                                "Allow Once accepted by daemon."
                            }
                            .into();
                        }
                    }
                    "provider-selection" => {
                        self.busy = false;
                        self.notice="Provider explicitly selected. Discovery is refreshing; /model selects the model when available.".into();
                        self.request(worker,"desktop.action",json!({"action":"refreshModelDiscovery","arguments":[],"session_id":""}),"refresh-provider");
                    }
                    "refresh-provider" => self.refresh(worker),
                    "selection" => {
                        self.busy = false;
                        if value.get("accepted") == Some(&json!(false)) {
                            self.notice = "Daemon refused selection.".into();
                        } else {
                            self.notice = "Selection updated for future requests.".into();
                        }
                        self.refresh(worker);
                    }
                    "status" => {
                        self.output = serde_json::to_string_pretty(&value).unwrap_or_default();
                    }
                    _ => {}
                }
            }
            Update::Event(event) => {
                if event.payload["session_id"].as_str() != Some(self.sid.as_str()) {
                    return;
                }
                let generation = event.payload["server_generation"].as_str().unwrap_or("");
                let sequence = event.payload["event_sequence"].as_u64().unwrap_or(0);
                if !generation.is_empty()
                    && !self.generation.is_empty()
                    && generation != self.generation
                {
                    return;
                }
                if event.name == "output.delta" && !self.active() {
                    return;
                }
                if !generation.is_empty()
                    && generation == self.generation
                    && sequence > 0
                    && sequence <= self.last_sequence
                {
                    return;
                }
                if sequence > 0 {
                    self.last_sequence = sequence;
                    self.generation = generation.into();
                }
                if event.name == "run.started" {
                    self.snapshot(event.payload);
                    return;
                }
                if event.payload["run_id"].as_str() != Some(self.run_id.as_str())
                    && !self.run_id.is_empty()
                {
                    return;
                }
                match event.name.as_str() {
                    "output.delta" => {
                        let text = event.payload["text"].as_str().unwrap_or("");
                        if self.output.len() + text.len() <= 262144 {
                            self.output.push_str(text);
                        } else {
                            self.notice="Local display limit reached; authoritative output remains in daemon history.".into();
                        }
                    }
                    "approval.requested" => {
                        self.approval = Some(event.payload.clone());
                        self.approval_pending = false;
                        self.state = "approval".into();
                    }
                    "run.completed" | "run.failed" | "run.cancelled" => {
                        self.state = event.name.trim_start_matches("run.").into();
                        self.output = event.payload["text"].as_str().unwrap_or("").into();
                        self.approval = None;
                        self.approval_pending = false;
                        if event.name == "run.failed" {
                            self.notice = Error::from_run_detail(
                                event.payload["detail"].as_str().unwrap_or("task-failed"),
                            )
                            .to_string();
                        }
                        self.request(
                            worker,
                            "session.messages",
                            json!({"session_id":self.sid}),
                            "messages",
                        );
                        self.refresh(worker);
                    }
                    "agent.activity" => {
                        self.activity.push(format!(
                            "step {} · {} {}",
                            event.payload["step"],
                            event.payload["activity"].as_str().unwrap_or("activity"),
                            event.payload["decision"].as_str().unwrap_or("")
                        ));
                    }
                    "subagent.activity" => {
                        self.activity.push(format!(
                            "Subagent {} · {} · {} · parent {}",
                            event.payload["subagent_id"].as_str().unwrap_or("?"),
                            event.payload["role"].as_str().unwrap_or(""),
                            event.payload["state"].as_str().unwrap_or("?"),
                            event.payload["parent_run_id"].as_str().unwrap_or("?")
                        ));
                    }
                    "tool.requested" | "tool.running" | "tool.result" => {
                        let resources = event.payload["resources"]
                            .as_array()
                            .into_iter()
                            .flatten()
                            .filter_map(|resource| resource["resource"].as_str())
                            .take(4)
                            .map(|resource| resource.chars().take(200).collect::<String>())
                            .collect::<Vec<_>>()
                            .join(", ");
                        let duration = event.payload["duration_ms"]
                            .as_u64()
                            .map(|ms| format!("{ms} ms"))
                            .unwrap_or_else(|| "duration pending".into());
                        self.activity.push(format!(
                            "{} · {} · risk {} · {} · {}\n  {} · resources: {}",
                            event.name,
                            event.payload["tool"].as_str().unwrap_or("tool"),
                            event.payload["risk"],
                            event.payload["timestamp"].as_str().unwrap_or(""),
                            duration,
                            event.payload["detail"]
                                .as_str()
                                .unwrap_or("")
                                .chars()
                                .take(256)
                                .collect::<String>(),
                            resources
                        ));
                    }
                    _ => {}
                }
                if self.activity.len() > 200 {
                    self.activity.remove(0);
                }
            }
        }
    }
    fn open_picker(&mut self, kind: &'static str) {
        let items = match kind {
            "model" => self
                .models
                .iter()
                .map(|m| Item {
                    label: format!(
                        "{} / {} · {} · {}{}",
                        m["provider_id"].as_str().unwrap_or("?"),
                        m["model_id"].as_str().unwrap_or("?"),
                        m["health"].as_str().unwrap_or("unknown"),
                        m["capabilities"].as_str().unwrap_or("capabilities unknown"),
                        if m["provider_id"]
                            == self.diagnostics["properties"]["effective_selection"]["provider_id"]
                            && m["model_id"]
                                == self.diagnostics["properties"]["effective_selection"]["model_id"]
                        {
                            " · active"
                        } else {
                            ""
                        }
                    ),
                    id: m["model_id"].as_str().unwrap_or("").into(),
                    extra: m["provider_id"].as_str().unwrap_or("").into(),
                    enabled: m["health"].as_str() != Some("Unavailable"),
                })
                .collect(),
            "provider" => self.diagnostics["providers"]
                .as_array()
                .into_iter()
                .flatten()
                .map(|p| Item {
                    label: format!(
                        "{} · {} · {}",
                        p["provider_id"].as_str().unwrap_or("?"),
                        p["catalog"].as_str().unwrap_or("unknown"),
                        p["reason"].as_str().unwrap_or("")
                    ),
                    id: p["provider_id"].as_str().unwrap_or("").into(),
                    extra: String::new(),
                    enabled: true,
                })
                .collect(),
            "workspace" => self.diagnostics["workspaces"]
                .as_array()
                .into_iter()
                .flatten()
                .map(|w| Item {
                    label: format!(
                        "{} · {}{}",
                        w["name"].as_str().unwrap_or("?"),
                        w["root"].as_str().unwrap_or("no root"),
                        if w["id"] == self.diagnostics["workspace"]["id"] {
                            " · active"
                        } else {
                            ""
                        }
                    ),
                    id: w["id"].as_str().unwrap_or("").into(),
                    extra: String::new(),
                    enabled: w["archived"] != true,
                })
                .collect(),
            "session" => self
                .sessions
                .iter()
                .map(|s| Item {
                    label: format!(
                        "{} · {} · {}{}{}",
                        s["title"].as_str().unwrap_or("Untitled"),
                        s["run_type"].as_str().unwrap_or("chat"),
                        s["state"].as_str().unwrap_or("idle"),
                        if s["pinned"] == true {
                            " · pinned"
                        } else {
                            ""
                        },
                        if s["archived"] == true {
                            " · archived"
                        } else {
                            ""
                        }
                    ),
                    id: s["session_id"].as_str().unwrap_or("").into(),
                    extra: format!(
                        "{} · {}/{}",
                        s["created_at"].as_str().unwrap_or(""),
                        s["provider_id"].as_str().unwrap_or(""),
                        s["model_id"].as_str().unwrap_or("")
                    ),
                    enabled: true,
                })
                .collect(),
            _ => COMMANDS
                .iter()
                .map(|(name, description)| Item {
                    label: format!("{name}  {description}"),
                    id: (*name).into(),
                    extra: String::new(),
                    enabled: true,
                })
                .collect(),
        };
        self.picker = Some(Picker {
            kind,
            query: String::new(),
            selected: 0,
            items,
        });
    }
    fn command(&mut self, text: &str, worker: &Worker) {
        match text.trim(){
            "/reconnect"=>self.request(worker,"reconnect",json!({}),"reconnect"),
            "/remove"=>{self.references.pop();},
            "/model"=>self.open_picker("model"),"/provider"=>self.open_picker("provider"),"/workspace"=>self.open_picker("workspace"),"/sessions"=>self.open_picker("session"),
            "/new"=>{if self.active(){self.notice="Finish or cancel the active run before creating a session.".into();}else{self.request(worker,"session.create",json!({"title":"Terminal session"}),"new");}},
            "/agent"|"/chat"=>{if !self.active(){self.agent=text.trim()=="/agent";}else{self.notice="Mode cannot change during an active run.".into();}},
            "/help"=>self.output=format!("{HELP}\n\n{}",COMMANDS.iter().map(|(name,description)|format!("{name} — {description}")).collect::<Vec<_>>().join("\n")),
            "/status"=>self.request(worker,"daemon.status",json!({}),"status"),
            "/doctor"=>self.request(worker,"terminal.state",json!({}),"doctor"),
            "/compact"=>self.output="Context compaction is unavailable: the daemon exposes no authoritative context-reduction operation. Conversation history has not been changed.".into(),
            "/activity"=>self.output=self.activity.join("\n"),
            "/files"=>self.request(worker,"workspace.files",json!({"session_id":self.sid}),"files"),
            "/diff"=>self.request(worker,"workspace.changes",json!({"session_id":self.sid}),"changes"),
            name if ["/tools","/mcp","/permissions","/context","/memory","/tasks"].contains(&name)=>{
                let needle=match name{"/tools"=>"tool","/permissions"=>"permission","/tasks"=>"agent",_=>name.trim_start_matches('/')};
                let properties=self.diagnostics["properties"].as_object().map(|p|p.iter().filter(|(key,_)|key.to_lowercase().contains(needle)).map(|(key,value)|(key.clone(),value.clone())).collect::<serde_json::Map<_,_>>()).unwrap_or_default();
                self.output=if properties.is_empty(){format!("No {needle} state is exposed by the authoritative daemon projection.")}else{serde_json::to_string_pretty(&properties).unwrap_or_default()};self.refresh(worker);
            },
            _=>self.notice="Unknown slash command. Open Ctrl+P or /help; command text was not executed.".into()
        }
        self.follow = true;
    }
    fn send(&mut self, worker: &Worker) {
        if self.active() || self.busy || self.editor.text.trim().is_empty() {
            return;
        }
        if self.editor.text.starts_with('/') {
            let command = self.editor.take();
            self.command(&command, worker);
            return;
        }
        if !self.connected {
            self.notice = "Disconnected. /reconnect reconnects; your draft is preserved.".into();
            return;
        }
        if self.models.is_empty() {
            self.notice="No discovered model is available. /provider and /doctor explain readiness; configure the provider in Sentinel settings.".into();
            return;
        }
        if self.sid.is_empty() {
            self.notice = "Wait for session attachment before sending.".into();
            return;
        }
        if !self.references.is_empty() && !self.agent {
            self.notice="File references require Agent mode and its authorized observation tools. Ctrl+G switches mode explicitly.".into();
            return;
        }
        self.pending_text = self.editor.text.clone();
        self.latest_user = self.editor.text.clone();
        self.pending_references = self.references.clone();
        let mut text = self.editor.take();
        if !self.references.is_empty() {
            text.push_str("\nReferenced workspace files (observe through authorized tools):\n");
            for path in self.references.drain(..) {
                text.push('@');
                text.push_str(&serde_json::to_string(&path).unwrap());
                text.push('\n');
            }
        }
        self.state = "starting".into();
        self.busy = true;
        self.request(
            worker,
            if self.agent {
                "agent.start"
            } else {
                "chat.send"
            },
            json!({"session_id":self.sid,"text":text}),
            "start",
        );
    }
}
fn panel_block() -> Block<'static> {
    let block = Block::bordered();
    if std::env::var_os("SENTINEL_TUI_ASCII").is_some() {
        block.border_set(ratatui::symbols::border::Set {
            top_left: "+",
            top_right: "+",
            bottom_left: "+",
            bottom_right: "+",
            vertical_left: "|",
            vertical_right: "|",
            horizontal_top: "-",
            horizontal_bottom: "-",
        })
    } else {
        block
    }
}
fn header_style() -> Style {
    let style = Style::default().add_modifier(Modifier::BOLD);
    if std::env::var_os("NO_COLOR").is_some() {
        return style;
    }
    match std::env::var("SENTINEL_TUI_THEME").as_deref() {
        Ok("dark") => style.fg(Color::Rgb(169, 202, 211)),
        Ok("light") => style.fg(Color::Rgb(21, 23, 25)),
        _ => style,
    }
}
fn modal(area: Rect) -> Rect {
    let width = area.width.saturating_sub(4).min(90);
    let height = area.height.saturating_sub(2).min(18);
    Rect::new(
        area.x + (area.width - width) / 2,
        area.y + (area.height - height) / 2,
        width,
        height,
    )
}
fn draw(frame: &mut Frame, app: &App) {
    if frame.area().width < 24 || frame.area().height < 8 {
        frame.render_widget(
            Paragraph::new(
                "Sentinel: enlarge terminal (24×8 minimum). Ctrl+C exits idle; draft retained.",
            )
            .wrap(Wrap { trim: false }),
            frame.area(),
        );
        return;
    }
    let composer_height =
        (app.editor.text.lines().count().max(1) as u16 + 2).min((frame.area().height / 3).max(3));
    let areas = Layout::vertical([
        Constraint::Length(3),
        Constraint::Min(1),
        Constraint::Length(composer_height),
        Constraint::Length(2),
    ])
    .split(frame.area());
    let properties = &app.diagnostics["properties"];
    let header_rows = Layout::vertical([
        Constraint::Length(1),
        Constraint::Length(1),
        Constraint::Length(1),
    ])
    .split(areas[0]);
    let header_columns =
        Layout::horizontal([Constraint::Min(1), Constraint::Length(14)]).split(header_rows[0]);
    frame.render_widget(
        Paragraph::new(format!(
            "SENTINEL / {}",
            if app.agent { "Agent" } else { "Chat" }
        ))
        .style(header_style()),
        header_columns[0],
    );
    frame.render_widget(
        Paragraph::new(if app.connected {
            "Connected"
        } else {
            "Disconnected"
        })
        .alignment(Alignment::Right),
        header_columns[1],
    );
    frame.render_widget(
        Block::default()
            .borders(ratatui::widgets::Borders::BOTTOM)
            .border_style(Style::default().add_modifier(Modifier::DIM)),
        header_rows[1],
    );
    frame.render_widget(
        Paragraph::new(format!(
            "{} / {} · {}",
            properties["activeRuntimeProviderLabel"]
                .as_str()
                .unwrap_or("Provider unselected"),
            properties["activeRuntimeModelLabel"]
                .as_str()
                .unwrap_or("Model unselected"),
            properties["activeRuntimeReadinessSummary"]
                .as_str()
                .unwrap_or("readiness unknown"),
        ))
        .style(Style::default().add_modifier(Modifier::DIM)),
        header_rows[2],
    );
    let height = areas[1].height as usize;
    let width = areas[1].width.saturating_sub(2).max(1) as usize;
    let empty = app.output.is_empty() && !app.active();
    let transcript = if empty {
        if !app.connected {
            "Daemon disconnected\nStart sentinel-daemon, then /reconnect.\nYour draft stays here. /help lists commands.".into()
        } else if app.models.is_empty() {
            "Choose a model to begin\n/provider inspects readiness · /model selects a model\n/doctor shows diagnostics".into()
        } else {
            format!(
                "Start a new {} conversation\nType a message below and press Enter.\nCtrl+P commands · /model change model",
                if app.agent { "Agent" } else { "Chat" }
            )
        }
    } else if app.active() {
        format!(
            "{}{} · {}\n{}\n\n{}",
            if app.latest_user.is_empty() {
                String::new()
            } else {
                format!("You\n{}\n\n", app.latest_user)
            },
            if app.agent { "Agent" } else { "Assistant" },
            app.state,
            app.output,
            app.activity
                .iter()
                .rev()
                .take(3)
                .rev()
                .cloned()
                .collect::<Vec<_>>()
                .join("\n")
        )
    } else {
        app.output.clone()
    };
    let lines = transcript
        .lines()
        .map(|line| Line::raw(line).width().max(1).div_ceil(width))
        .sum::<usize>();
    let scroll = if app.follow {
        lines.saturating_sub(height).min(u16::MAX as usize) as u16
    } else {
        app.scroll
    };
    app.viewport_bottom
        .set(lines.saturating_sub(height).min(u16::MAX as usize) as u16);
    let transcript_area = if empty {
        let content_height = transcript.lines().count() as u16;
        Rect::new(
            areas[1].x,
            areas[1].y + areas[1].height.saturating_sub(content_height) / 2,
            areas[1].width,
            content_height.min(areas[1].height),
        )
    } else {
        areas[1]
    };
    let styled_lines = transcript
        .lines()
        .map(|line| {
            if [
                "You",
                "Assistant",
                "Agent",
                "user",
                "assistant",
                "agent",
                "system",
            ]
            .contains(&line)
                || line.starts_with("Assistant ·")
                || line.starts_with("Agent ·")
            {
                Line::styled(line.to_owned(), header_style())
            } else {
                Line::raw(line.to_owned())
            }
        })
        .collect::<Vec<_>>();
    frame.render_widget(
        Paragraph::new(styled_lines)
            .alignment(if empty {
                Alignment::Center
            } else {
                Alignment::Left
            })
            .wrap(Wrap { trim: false })
            .scroll((if empty { 0 } else { scroll }, 0)),
        transcript_area,
    );
    let before = &app.editor.text[..app.editor.cursor];
    let row = before.chars().filter(|&c| c == '\n').count();
    let col = Line::raw(before.rsplit('\n').next().unwrap_or("")).width();
    let horizontal = col.saturating_sub(areas[2].width.saturating_sub(3) as usize);
    let visible = areas[2].height.saturating_sub(2) as usize;
    let start = row.saturating_sub(visible.saturating_sub(1));
    frame.render_widget(
        Paragraph::new(app.editor.text.as_str())
            .scroll((
                start.min(u16::MAX as usize) as u16,
                horizontal.min(u16::MAX as usize) as u16,
            ))
            .block(
                panel_block()
                    .border_style(Style::default().add_modifier(Modifier::DIM))
                    .title(" Message "),
            ),
        areas[2],
    );
    if app.picker.is_none() && app.approval.is_none() && areas[2].width > 2 && areas[2].height > 2 {
        frame.set_cursor_position((
            areas[2].x
                + 1
                + col
                    .saturating_sub(horizontal)
                    .min(areas[2].width.saturating_sub(3) as usize) as u16,
            areas[2].y + 1 + row.saturating_sub(start).min(visible.saturating_sub(1)) as u16,
        ));
    }
    let activity = app
        .activity
        .last()
        .map(String::as_str)
        .unwrap_or(app.state.as_str());
    let reference_hint = if app.references.is_empty() {
        String::new()
    } else {
        format!(
            "Refs: {} · /remove · ",
            app.references
                .iter()
                .map(|p| format!("@{}", serde_json::to_string(p).unwrap_or_default()))
                .collect::<Vec<_>>()
                .join(", ")
        )
    };
    let notice = if !app.follow && lines > height + app.scroll as usize {
        "Viewing earlier output · new content below · Ctrl+B follows latest".to_owned()
    } else if let Some(search) = &app.search {
        format!("Search: {search} · Enter find / Esc close")
    } else if app.notice.is_empty() {
        activity.to_owned()
    } else {
        app.notice.clone()
    };
    let footer_rows =
        Layout::vertical([Constraint::Length(1), Constraint::Length(1)]).split(areas[3]);
    frame.render_widget(
        Paragraph::new(format!("{reference_hint}{notice}")),
        footer_rows[0],
    );
    let footer_columns =
        Layout::horizontal([Constraint::Min(1), Constraint::Length(14)]).split(footer_rows[1]);
    frame.render_widget(
        Paragraph::new("Enter send · Ctrl+O newline · Ctrl+P commands · F1 help")
            .style(Style::default().add_modifier(Modifier::DIM)),
        footer_columns[0],
    );
    frame.render_widget(
        Paragraph::new(app.state.as_str())
            .alignment(Alignment::Right)
            .style(header_style()),
        footer_columns[1],
    );
    if let Some(picker) = &app.picker {
        let area = modal(frame.area());
        frame.render_widget(Clear, area);
        let visible = picker.visible();
        let capacity = area.height.saturating_sub(4) as usize;
        let start = picker.selected.saturating_sub(capacity.saturating_sub(1));
        let rows = visible
            .iter()
            .enumerate()
            .skip(start)
            .take(capacity)
            .map(|(i, item)| {
                format!(
                    "{} {}{}",
                    if i == picker.selected { ">" } else { " " },
                    item.label,
                    if item.enabled { "" } else { " [unavailable]" }
                )
            })
            .collect::<Vec<_>>()
            .join("\n");
        let selected = visible
            .get(picker.selected)
            .map(|item| format!("{} · {}", item.id, item.extra))
            .unwrap_or_else(|| {
                if !app.connected {
                    "Disconnected: cached choices may be stale; /reconnect".into()
                } else {
                    "No matches. Change filter or Esc to return.".into()
                }
            });
        frame.render_widget(
            Paragraph::new(format!("Filter: {}\n{rows}\n{selected}", picker.query))
                .block(panel_block().title(format!("{} · Enter select · Esc cancel", picker.kind))),
            area,
        );
    }
    if let Some(p) = &app.approval {
        let area = modal(frame.area());
        frame.render_widget(Clear, area);
        frame.render_widget(Paragraph::new(format!("Tool: {} · risk {}\nResources: {}\n{}\nSession: {}\nRun: {}\nScope: this pending operation only\n{}",p["tool"].as_str().unwrap_or("unknown"),p["risk"],p["resources"],p["detail"].as_str().unwrap_or(""),app.sid,app.run_id,if app.approval_pending{"Waiting for authoritative response…"}else{"y Allow Once · n Deny · Esc keeps approval open"})).wrap(Wrap{trim:false}).block(panel_block().title("Approval required").style(Style::default().add_modifier(Modifier::BOLD))),area);
    }
}
pub fn run(path: &Path, attach: Option<&str>) -> Result<(), Error> {
    let worker = worker(path);
    let mut app = App {
        state: "idle".into(),
        follow: true,
        attach: attach.map(str::to_owned),
        ..App::default()
    };
    app.refresh(&worker);
    let mut terminal = ratatui::try_init().map_err(|e| Error::Unavailable(e.to_string()))?;
    let _cleanup = TerminalCleanup;
    let _ = execute!(std::io::stdout(), EnableBracketedPaste);
    (|| -> Result<(), Error> {
        let mut dirty = true;
        loop {
            while let Ok(update) = worker.rx.try_recv() {
                app.update(update, &worker);
                dirty = true;
            }
            if dirty {
                terminal
                    .draw(|frame| draw(frame, &app))
                    .map_err(|e| Error::Protocol(e.to_string()))?;
                dirty = false;
            }
            if !event::poll(Duration::from_millis(50))
                .map_err(|e| Error::Protocol(e.to_string()))?
            {
                continue;
            }
            dirty = true;
            match event::read().map_err(|e| Error::Protocol(e.to_string()))? {
                Event::Paste(text) => {
                    if app.picker.is_none() && app.approval.is_none() {
                        app.editor.insert(&text);
                    }
                }
                Event::Key(key) if key.kind == KeyEventKind::Press => {
                    let ctrl = key.modifiers.contains(KeyModifiers::CONTROL);
                    let alt = key.modifiers.contains(KeyModifiers::ALT);
                    if ctrl && key.code == KeyCode::Char('c') {
                        if app.active() {
                            if app.state != "cancelling" && app.state != "starting" {
                                app.request(
                                    &worker,
                                    "run.cancel",
                                    json!({"run_id":app.run_id}),
                                    "cancel",
                                );
                                app.state = "cancelling".into();
                            }
                        } else if app.editor.text.is_empty()
                            && app.picker.is_none()
                            && app.search.is_none()
                        {
                            break;
                        } else {
                            app.picker = None;
                            app.search = None;
                            app.notice =
                                "Draft preserved. Ctrl+D exits only with an empty composer.".into();
                        }
                        if app.state == "starting" {
                            app.cancel_after_start = true;
                        }
                        continue;
                    }
                    if let Some(p) = &app.approval {
                        if !app.approval_pending
                            && [KeyCode::Char('y'), KeyCode::Char('n')].contains(&key.code)
                        {
                            let payload = json!({"run_id":app.run_id,"approval_id":p["approval_id"],"allow":key.code==KeyCode::Char('y')});
                            app.approval_pending = true;
                            app.approval_allow = Some(key.code == KeyCode::Char('y'));
                            app.request(&worker, "approval.respond", payload, "approval");
                        }
                        continue;
                    }
                    if let Some(picker) = &mut app.picker {
                        match key.code {
                            KeyCode::Esc => app.picker = None,
                            KeyCode::Up | KeyCode::BackTab => picker.move_by(false),
                            KeyCode::Down | KeyCode::Tab => picker.move_by(true),
                            KeyCode::Backspace => {
                                picker.query.pop();
                                picker.selected = 0;
                            }
                            KeyCode::Char(c) if !ctrl => {
                                picker.query.push(c);
                                picker.selected = 0;
                            }
                            KeyCode::Enter => {
                                let item = picker
                                    .visible()
                                    .get(picker.selected)
                                    .map(|item| (*item).clone());
                                let kind = picker.kind;
                                if let Some(item) = item {
                                    if !item.enabled {
                                        app.notice = "Selection is unavailable.".into();
                                        continue;
                                    }
                                    if !app.connected && kind != "command" {
                                        app.notice = "Disconnected. Selection was not changed; /reconnect retries.".into();
                                        continue;
                                    }
                                    if app.active() && kind != "command" {
                                        app.notice="Active immutable run prevents switching model, workspace or session.".into();
                                        continue;
                                    }
                                    app.picker = None;
                                    match kind {
                                        "model" => {
                                            app.busy = true;
                                            app.request(&worker,"model.select",json!({"provider_id":item.extra,"model_id":item.id}),"selection");
                                        }
                                        "provider" => {
                                            if app
                                                .models
                                                .iter()
                                                .any(|model| model["provider_id"] == item.id)
                                            {
                                                app.open_picker("model");
                                                if let Some(p) = &mut app.picker {
                                                    p.items.retain(|model| model.extra == item.id);
                                                }
                                            } else {
                                                app.busy = true;
                                                app.request(
                                                    &worker,
                                                    "provider.select",
                                                    json!({"provider_id":item.id}),
                                                    "provider-selection",
                                                );
                                            }
                                        }
                                        "workspace" => {
                                            app.busy = true;
                                            app.request(
                                                &worker,
                                                "workspace.select",
                                                json!({"workspace_id":item.id}),
                                                "selection",
                                            );
                                        }
                                        "review" => {
                                            if let Ok(index) = item.id.parse::<usize>()
                                                && let Some(file) = app.changes.get(index)
                                            {
                                                app.output = format!(
                                                    "{} · Applied\n{}",
                                                    file["path"].as_str().unwrap_or("?"),
                                                    file["diff"].as_str().unwrap_or("")
                                                );
                                                app.follow = false;
                                                app.scroll = 0;
                                            }
                                        }
                                        "file" => {
                                            if !app.references.contains(&item.id)
                                                && app.references.len() < 16
                                            {
                                                app.references.push(item.id);
                                            }
                                        }
                                        "session" => app.request(
                                            &worker,
                                            "terminal.attach",
                                            json!({"session_id":item.id}),
                                            "attach",
                                        ),
                                        _ => app.command(&item.id, &worker),
                                    }
                                }
                            }
                            _ => {}
                        }
                        continue;
                    }
                    if let Some(search) = &mut app.search {
                        match key.code {
                            KeyCode::Esc => app.search = None,
                            KeyCode::Backspace => {
                                search.pop();
                            }
                            KeyCode::Char(c) if !ctrl => search.push(c),
                            KeyCode::Enter => {
                                let query = search.to_lowercase();
                                if let Some(line) = app
                                    .output
                                    .lines()
                                    .position(|line| line.to_lowercase().contains(&query))
                                {
                                    app.scroll = line.min(u16::MAX as usize) as u16;
                                    app.follow = false;
                                } else {
                                    app.notice = "No transcript match.".into();
                                }
                                app.search = None;
                            }
                            _ => {}
                        }
                        continue;
                    }
                    if newline_key(key) {
                        app.editor.insert("\n");
                        continue;
                    }
                    match key.code {
                        KeyCode::Char('a') if ctrl => app.editor.home(),
                        KeyCode::Char('e') if ctrl => app.editor.end(),
                        KeyCode::Char('u') if ctrl => app.editor.kill_to_start(),
                        KeyCode::Char('k') if ctrl => app.editor.kill_to_end(),
                        KeyCode::Char('g') if ctrl => {
                            if !app.active() {
                                app.agent = !app.agent;
                            }
                        }
                        KeyCode::Char('p') if ctrl => app.open_picker("command"),
                        KeyCode::Tab => app.open_picker("command"),
                        KeyCode::Char('l') if ctrl => app.open_picker("model"),
                        KeyCode::Char('w') if ctrl => app.open_picker("workspace"),

                        KeyCode::Char('n') if ctrl => app.command("/new", &worker),
                        KeyCode::Char('r') if ctrl => app.search = Some(String::new()),
                        KeyCode::Char('b') if ctrl => app.follow = true,
                        KeyCode::Char('f') if ctrl => app.search = Some(String::new()),
                        KeyCode::F(1) => app.command("/help", &worker),
                        KeyCode::PageUp => {
                            if app.follow {
                                app.scroll = app.viewport_bottom.get();
                            }
                            app.follow = false;
                            app.scroll = app.scroll.saturating_sub(10);
                        }
                        KeyCode::PageDown => {
                            app.follow = false;
                            app.scroll = app.scroll.saturating_add(10);
                        }
                        KeyCode::Enter => app.send(&worker),
                        KeyCode::Left if ctrl => app.editor.word(false),
                        KeyCode::Right if ctrl => app.editor.word(true),
                        KeyCode::Left => app.editor.left(),
                        KeyCode::Right => app.editor.right(),
                        KeyCode::Home => app.editor.home(),
                        KeyCode::End => app.editor.end(),
                        KeyCode::Up if alt || !app.editor.text.contains('\n') => {
                            app.editor.history(false)
                        }
                        KeyCode::Down if alt || !app.editor.text.contains('\n') => {
                            app.editor.history(true)
                        }
                        KeyCode::Up => app.editor.vertical(false),
                        KeyCode::Down => app.editor.vertical(true),
                        KeyCode::Backspace => app.editor.backspace(),
                        KeyCode::Delete => app.editor.delete(),
                        KeyCode::Char('?') if app.editor.text.is_empty() => {
                            app.command("/help", &worker)
                        }
                        KeyCode::Char('@') if !ctrl => {
                            app.request(
                                &worker,
                                "workspace.files",
                                json!({"session_id":app.sid}),
                                "files",
                            );
                        }
                        KeyCode::Char('d') if ctrl => {
                            if !app.active()
                                && !app.busy
                                && app.editor.text.is_empty()
                                && app.references.is_empty()
                            {
                                break;
                            }
                            app.editor.delete();
                        }
                        KeyCode::Char(c) if !ctrl => app.editor.insert(&c.to_string()),
                        _ => {}
                    }
                }
                _ => {}
            }
        }
        Ok(())
    })()
}
#[cfg(test)]
mod tests {
    use super::*;
    use ratatui::{Terminal, backend::TestBackend};
    #[test]
    fn compact_header_centered_empty_state_and_composer() {
        for (width, height) in [(80, 24), (120, 40), (160, 48)] {
            let mut terminal = Terminal::new(TestBackend::new(width, height)).unwrap();
            let mut app = App {
                state: "idle".into(),
                ..App::default()
            };
            terminal.draw(|frame| draw(frame, &app)).unwrap();
            let buffer = terminal.backend().buffer();
            let row = |y| {
                (0..width)
                    .map(|x| buffer[(x, y)].symbol())
                    .collect::<String>()
            };
            assert!(row(0).starts_with("SENTINEL / Chat"));
            assert!(row(0).ends_with("Disconnected"));
            assert!(row(2).contains("Provider unselected / Model unselected"));
            let center = (3..height - 5)
                .find(|&y| row(y).contains("Daemon disconnected"))
                .unwrap();
            assert!(center > height / 3 && center < height * 2 / 3);
            assert!(row(height - 5).contains(" Message "));
            app.agent = true;
            app.state = "running".into();
            app.run_id = "fixture-run".into();
            app.latest_user = "Check the workspace".into();
            app.output = "Inspecting authorized context".into();
            app.activity.push("step 1 · tool requested".into());
            terminal.draw(|frame| draw(frame, &app)).unwrap();
            let text = terminal
                .backend()
                .buffer()
                .content
                .iter()
                .map(|c| c.symbol())
                .collect::<String>();
            assert!(text.contains("SENTINEL / Agent"));
            assert!(text.contains("You"));
            assert!(text.contains("Agent · running"));
            assert!(text.contains("step 1 · tool requested"));
        }
    }
    #[test]
    fn terminal_size_matrix() {
        for (width, height) in [(80, 24), (100, 30), (120, 40), (160, 48), (40, 12), (10, 4)] {
            let mut terminal = Terminal::new(TestBackend::new(width, height)).unwrap();
            let mut app = App::default();
            app.open_picker("command");
            terminal.draw(|frame| draw(frame, &app)).unwrap();
            app.approval = Some(json!({"tool":"file.write","resources":[]}));
            terminal.draw(|frame| draw(frame, &app)).unwrap();
        }
    }
    fn test_worker() -> Worker {
        let (tx, _) = mpsc::sync_channel(32);
        let (_, rx) = mpsc::sync_channel(8);
        Worker {
            tx,
            rx,
            stop: Arc::new(AtomicBool::new(false)),
        }
    }
    #[test]
    fn reconnect_restores_approval_and_draft() {
        let mut app = App::default();
        app.editor.insert("unfinished draft");
        app.snapshot(json!({"session_id":"s", "run_id":"r", "state":"approval", "run_type":"agent", "approval":{"approval_id":"a", "tool":"write-file"}, "event_sequence":8, "server_generation":"g"}));
        assert_eq!(app.editor.text, "unfinished draft");
        assert_eq!(app.approval.as_ref().unwrap()["approval_id"], "a");
        app.approval_pending = true;
        app.update(
            Update::Failure("approval".into(), Error::Remote("invalid-approval".into())),
            &test_worker(),
        );
        assert!(app.approval.is_some());
        assert!(!app.approval_pending);
    }
    #[test]
    fn duplicate_events_and_other_sessions_are_ignored() {
        let mut app = App::default();
        app.snapshot(json!({"session_id":"s", "run_id":"r", "state":"running", "output":"existing", "event_sequence":8, "server_generation":"g"}));
        let worker = test_worker();
        for (sid, sequence) in [("s", 8), ("other", 9), ("s", 9), ("s", 9)] {
            app.update(Update::Event(Envelope { version: sentinel_ipc::Version { major:1, minor:1 }, kind: sentinel_ipc::MessageType::Event, id: String::new(), name:"output.delta".into(), payload:json!({"session_id":sid,"run_id":"r","event_sequence":sequence,"server_generation":"g","text":" once"}) }), &worker);
        }
        assert_eq!(app.output, "existing once");
    }
    #[test]
    fn tool_activity_projects_safe_metadata_only() {
        let mut app = App::default();
        app.snapshot(json!({"session_id":"s", "run_id":"r", "state":"running"}));
        app.update(Update::Event(Envelope { version: sentinel_ipc::Version { major:1, minor:1 }, kind: sentinel_ipc::MessageType::Event, id:String::new(), name:"tool.result".into(), payload:json!({"session_id":"s","run_id":"r","tool":"read-file","risk":0,"detail":"Tool completed","duration_ms":12,"resources":[{"resource":"/workspace/file.txt"}],"raw_output":"secret-must-not-be-projected"}) }), &test_worker());
        assert!(app.activity[0].contains("/workspace/file.txt"));
        assert!(app.activity[0].contains("12 ms"));
        assert!(app.activity[0].contains("Tool completed"));
        assert!(!app.activity[0].contains("secret-must-not-be-projected"));
    }
    #[test]
    fn bounded_stream_render_measurement() {
        let mut terminal = Terminal::new(TestBackend::new(160, 48)).unwrap();
        let mut app = App {
            state: "running".into(),
            connected: true,
            follow: true,
            ..App::default()
        };
        let begin = Instant::now();
        for _ in 0..100 {
            app.output.push_str(&"stream 界 output\n".repeat(100));
            terminal.draw(|frame| draw(frame, &app)).unwrap();
        }
        eprintln!(
            "100 incremental TestBackend renders at 160x48: {:?}, retained {} bytes",
            begin.elapsed(),
            app.output.len()
        );
        assert!(app.output.len() <= 262144);
    }
    #[test]
    fn stale_generation_and_closed_run_cannot_append_output() {
        let mut app = App::default();
        app.snapshot(json!({"session_id":"s", "run_id":"r", "state":"running", "output":"stable", "server_generation":"current"}));
        let worker = test_worker();
        let event = |generation: &str| {
            Update::Event(Envelope {
                version: sentinel_ipc::Version { major: 1, minor: 1 },
                kind: sentinel_ipc::MessageType::Event,
                id: String::new(),
                name: "output.delta".into(),
                payload: json!({"session_id":"s","run_id":"r","text":" stale", "server_generation":generation,"event_sequence":99}),
            })
        };
        app.update(event("old"), &worker);
        assert_eq!(app.output, "stable");
        app.state = "cancelled".into();
        app.update(event("current"), &worker);
        assert_eq!(app.output, "stable");
        assert_eq!(app.state, "cancelled");
    }
    #[test]
    fn keyboard_contract_has_reliable_newline_without_flow_control() {
        use crossterm::event::KeyEvent;
        assert!(newline_key(KeyEvent::new(
            KeyCode::Char('o'),
            KeyModifiers::CONTROL
        )));
        assert!(newline_key(KeyEvent::new(
            KeyCode::Enter,
            KeyModifiers::ALT
        )));
        assert!(newline_key(KeyEvent::new(
            KeyCode::Enter,
            KeyModifiers::SHIFT
        )));
        assert!(!newline_key(KeyEvent::new(
            KeyCode::Enter,
            KeyModifiers::NONE
        )));
        assert!(!newline_key(KeyEvent::new(
            KeyCode::Char('s'),
            KeyModifiers::CONTROL
        )));
    }
    #[test]
    fn rejected_submission_restores_draft_and_references() {
        let mut app = App {
            pending_text: "ü draft".into(),
            pending_references: vec!["file name".into()],
            state: "starting".into(),
            ..App::default()
        };
        app.update(
            Update::Failure("start".into(), Error::Timeout),
            &test_worker(),
        );
        assert_eq!(app.editor.text, "ü draft");
        assert_eq!(app.references, ["file name"]);
        assert_eq!(app.state, "rejected");
    }
    #[test]
    fn empty_composer_is_three_rows_and_unicode_cursor_is_safe() {
        let mut terminal = Terminal::new(TestBackend::new(80, 24)).unwrap();
        let mut app = App::default();
        terminal.draw(|frame| draw(frame, &app)).unwrap();
        let text = terminal
            .backend()
            .buffer()
            .content
            .iter()
            .map(|c| c.symbol())
            .collect::<String>();
        assert!(text.contains("Daemon disconnected"));
        assert!(!text.contains("Ctrl+S"));
        app.editor.insert(&"界🙂ü".repeat(100));
        terminal.draw(|frame| draw(frame, &app)).unwrap();
        app.editor.insert("\nsecond\nthird");
        terminal.draw(|frame| draw(frame, &app)).unwrap();
    }
    #[test]
    fn mode_is_explicit() {
        let app = App::default();
        assert!(!app.agent);
        assert!(!app.active());
    }
}
