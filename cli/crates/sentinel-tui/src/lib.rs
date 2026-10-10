mod commands;
mod editor;
mod picker;
mod theme;
use commands::{COMMANDS, CommandId, Context};
use crossterm::{
    event::{
        self, DisableBracketedPaste, EnableBracketedPaste, Event, KeyCode, KeyEventKind,
        KeyModifiers, KeyboardEnhancementFlags, PopKeyboardEnhancementFlags,
        PushKeyboardEnhancementFlags,
    },
    execute,
};
use editor::Editor;
use picker::{Item, Picker};
use ratatui::{
    Frame,
    layout::{Alignment, Constraint, Layout, Rect},
    style::{Modifier, Style},
    text::Line,
    widgets::{Block, Clear, Padding, Paragraph, Wrap},
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
use theme::Theme;
const HELP: &str = "Enter: send · Shift+Enter: newline · Alt+Enter / Ctrl+O: compatibility alternatives\nCtrl+P / Tab: commands · Ctrl+L: models · Ctrl+W: workspaces\n/sessions: sessions · Ctrl+N: new · /reconnect: reconnect\nCtrl+R / Ctrl+F: transcript search · F1 or /help: help\nCtrl+A/E: line start/end · Ctrl+U/K: erase to start/end\nArrows: edit · Alt+Up/Down: draft history · Ctrl+Left/Right: word\nPgUp/PgDn: scroll · Ctrl+B: follow latest · /activity: tool details\nCtrl+C: cancel run; idle draft is preserved · Ctrl+D: exit only idle/empty\n/remove: remove last file reference · @: authorized file picker\nPicker: type to filter, Up/Down or Tab/Shift+Tab, Enter, Esc\nApproval: y Allow Once / n Deny; Esc preserves pending approval\nSlash: / discovery, Tab completes, Enter exact executes, Esc dismisses; // sends literal slash text.\nPasted slash text stays literal. /references removes selected paths. Input recall is memory-only; sessions belong to daemon.";

// A guard complements Ratatui's panic hook on recoverable error/early-return paths.
struct TerminalCleanup;
impl Drop for TerminalCleanup {
    fn drop(&mut self) {
        let _ = execute!(
            std::io::stdout(),
            PopKeyboardEnhancementFlags,
            DisableBracketedPaste
        );
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
#[derive(Clone)]
struct SessionDraft {
    text: String,
    cursor: usize,
    references: Vec<String>,
    agent: bool,
    literal: bool,
}
#[derive(Default)]
struct App {
    cancel_after_start: bool,
    pending_text: String,
    pending_literal: bool,
    pending_references: Vec<String>,
    latest_user: String,
    prior_transcript: String,
    turn_visible: bool,
    theme: Theme,
    changes: Vec<Value>,
    references: Vec<String>,
    editor: Editor,
    slash_selected: usize,
    slash_dismissed: Option<String>,
    show_details: bool,
    inspection: Option<CommandId>,
    provider_filter: Option<String>,
    exit_requested: bool,
    request_failed: bool,
    connection_error: bool,
    drafts: Vec<(String, SessionDraft)>,
    file_request_session: String,
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
    view: Option<String>,
    system_notices: String,
    activity: Vec<String>,
    approval: Option<Value>,
    approval_pending: bool,
    approval_allow: Option<bool>,
    picker: Option<Picker>,
    notice: String,
    busy: bool,
    scroll: u16,
    viewport_bottom: Cell<u16>,
    reading_width: Cell<usize>,
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
            self.request_failed = true;
            self.busy = false;
            self.notice =
                "Connection queue unavailable. Draft preserved; wait for connection recovery."
                    .into();
            if tag == "start" {
                self.output = std::mem::take(&mut self.prior_transcript);
                self.turn_visible = false;
                self.state = "rejected".into();
                self.busy = false;
                self.editor.insert(&std::mem::take(&mut self.pending_text));
                self.editor.literal = self.pending_literal;
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
                self.prior_transcript.clear();
                self.turn_visible = false;
            }
            self.activity.clear();
        }
        let had_session = !self.sid.is_empty();
        let next_sid = value["session_id"].as_str().unwrap_or("");
        let switching = next_sid != self.sid;
        if switching {
            self.prior_transcript.clear();
            self.turn_visible = false;
            self.system_notices.clear();
            if !self.sid.is_empty() {
                let draft = SessionDraft {
                    text: self.editor.text.clone(),
                    cursor: self.editor.cursor,
                    references: self.references.clone(),
                    agent: self.agent,
                    literal: self.editor.literal,
                };
                self.drafts.retain(|(id, _)| id != &self.sid);
                if !draft.text.is_empty() || !draft.references.is_empty() {
                    self.drafts.push((self.sid.clone(), draft));
                }
            }
        }
        self.sid = value["session_id"].as_str().unwrap_or("").into();
        self.title = value["title"].as_str().unwrap_or("Terminal session").into();
        self.run_id = value["run_id"].as_str().unwrap_or("").into();
        self.state = value["state"].as_str().unwrap_or("idle").into();
        self.agent = value["run_type"].as_str() == Some("agent");
        if switching {
            if let Some((_, draft)) = self.drafts.iter().find(|(id, _)| id == &self.sid) {
                self.editor.replace(draft.text.clone(), draft.literal);
                self.editor.cursor = draft.cursor.min(self.editor.text.len());
                self.references = draft.references.clone();
                if !self.active() {
                    self.agent = draft.agent;
                }
            } else if had_session {
                self.editor.replace(String::new(), false);
                self.references.clear();
            }
            self.slash_dismissed = None;
        }
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
                if connected && self.connection_error {
                    self.notice = "Connection restored; draft preserved.".into();
                    self.connection_error = false;
                }
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
                    self.output = std::mem::take(&mut self.prior_transcript);
                    self.turn_visible = false;
                    self.state = "rejected".into();
                    if self.editor.text.is_empty() {
                        self.editor.insert(&std::mem::take(&mut self.pending_text));
                        self.editor.literal = self.pending_literal;
                    }
                    self.references.append(&mut self.pending_references);
                    self.cancel_after_start = false;
                }
                if tag == "connection" {
                    self.connection_error = true;
                    self.connected = false;
                }
            }
            Update::Reply(tag, value) => {
                self.connected = true;
                match tag.as_str() {
                    "inspect" => {
                        self.diagnostics = value;
                        if let Some(command) = self.inspection.take() {
                            self.inspect_projection(command);
                        }
                    }
                    "doctor" => {
                        self.view = Some(serde_json::to_string_pretty(&value).unwrap_or_default());
                        self.diagnostics = value;
                    }
                    "diagnostics" => {
                        self.diagnostics = value;
                        for kind in ["provider", "workspace"] {
                            self.refresh_picker(kind);
                        }
                    }
                    "changes" => {
                        self.busy = false;
                        self.changes = value["files"].as_array().cloned().unwrap_or_default();
                        self.view = Some(format!(
                            "{}\n{}",
                            value["reason"]
                                .as_str()
                                .unwrap_or("Change review unavailable."),
                            if value["truncated"] == true {
                                "Review is truncated; omissions are not evidence of no changes."
                            } else {
                                ""
                            }
                        ));
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
                        self.busy = false;
                        if self.file_request_session != self.sid {
                            return;
                        }
                        let root = value["root"].as_str().unwrap_or("");
                        let items = value["files"]
                            .as_array()
                            .into_iter()
                            .flatten()
                            .filter_map(Value::as_str)
                            .map(|path| Item {
                                label: serde_json::to_string(
                                    path.strip_prefix(root)
                                        .unwrap_or(path)
                                        .trim_start_matches('/'),
                                )
                                .unwrap_or_default(),
                                id: path.into(),
                                extra: format!(
                                    "@{} · reference only; contents require authorized Agent tools",
                                    serde_json::to_string(path).unwrap_or_default()
                                ),
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
                        self.refresh_picker("model");
                    }
                    "sessions" => {
                        self.sessions = value["sessions"].as_array().cloned().unwrap_or_default();
                        self.refresh_picker("session");
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
                        self.view = None;
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
                        if value["session_id"]
                            .as_str()
                            .is_some_and(|sid| sid != self.sid)
                        {
                            return;
                        }
                        let messages = value["messages"].as_array().cloned().unwrap_or_default();
                        self.system_notices = messages
                            .iter()
                            .filter(|m| m["role"].as_str() == Some("system"))
                            .map(|m| message_block("system", m["content"].as_str().unwrap_or("")))
                            .collect::<Vec<_>>()
                            .join("\n");
                        if self.active() && !self.turn_visible {
                            // Reattaching an active run has no local send buffer. Restore
                            // the task and prior conversation without replacing newer
                            // snapshot/stream output with a possibly older history reply.
                            if let Some(index) = messages.iter().rposition(|m| m["role"] == "user")
                            {
                                self.latest_user =
                                    messages[index]["content"].as_str().unwrap_or("").into();
                                self.prior_transcript = messages[..index]
                                    .iter()
                                    .filter(|m| m["role"].as_str() != Some("system"))
                                    .map(|m| {
                                        message_block(
                                            m["role"].as_str().unwrap_or("message"),
                                            m["content"].as_str().unwrap_or(""),
                                        )
                                    })
                                    .collect::<Vec<_>>()
                                    .join("\n");
                                self.turn_visible = true;
                            }
                        } else if !self.active() {
                            self.prior_transcript.clear();
                            self.turn_visible = false;
                            self.output = messages
                                .iter()
                                .filter(|m| m["role"].as_str() != Some("system"))
                                .map(|m| {
                                    message_block(
                                        m["role"].as_str().unwrap_or("message"),
                                        m["content"].as_str().unwrap_or(""),
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
                        self.view = None;
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
                    "session-action" | "export" | "archive" => {
                        self.busy = false;
                        let succeeded =
                            value["accepted"] == true && value["result"]["value"] == true;
                        self.notice = if succeeded {
                            if tag == "export" {
                                "Daemon confirmed transcript saved in its controlled export directory.".into()
                            } else {
                                "Daemon accepted session change.".into()
                            }
                        } else {
                            "Daemon refused the operation; no success is assumed.".into()
                        };
                        if tag == "archive" && succeeded {
                            self.open_picker("session");
                        }
                        self.refresh(worker);
                    }
                    "status" => {
                        self.view = Some(serde_json::to_string_pretty(&value).unwrap_or_default());
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
                    if !self.turn_visible {
                        self.request(
                            worker,
                            "session.messages",
                            json!({"session_id":self.sid}),
                            "messages",
                        );
                    }
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
                        self.notice = if event.name == "run.cancelled" {
                            "Run cancelled.".into()
                        } else {
                            String::new()
                        };
                        if event.name == "run.failed" {
                            self.notice = run_failure_notice(
                                event.payload["detail"].as_str().unwrap_or("task-failed"),
                            );
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
                .filter(|m| {
                    self.provider_filter
                        .as_ref()
                        .is_none_or(|p| m["provider_id"] == *p)
                })
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
            "mode" => ["chat", "agent"]
                .iter()
                .map(|mode| Item {
                    label: format!(
                        "{} · {}",
                        mode,
                        if *mode == "chat" {
                            "Conversation; no Agent tool execution"
                        } else {
                            "Authorized tools; approval policy unchanged"
                        }
                    ),
                    id: (*mode).into(),
                    extra: String::new(),
                    enabled: !self.active() && !self.busy,
                })
                .collect(),
            "references" => self
                .references
                .iter()
                .enumerate()
                .map(|(index, path)| Item {
                    label: format!(
                        "{}  @{}",
                        index + 1,
                        serde_json::to_string(path).unwrap_or_default()
                    ),
                    id: index.to_string(),
                    extra: "Enter removes this draft reference; /files adds a replacement.".into(),
                    enabled: true,
                })
                .collect(),
            "theme" => ["terminal", "obsidian", "glacier", "porcelain"]
                .iter()
                .map(|name| Item {
                    label: format!(
                        "{}{}",
                        name,
                        if *name == self.theme.name() {
                            " · active"
                        } else {
                            ""
                        }
                    ),
                    id: name.to_string(),
                    extra: "Presentation only; no permission or daemon change.".into(),
                    enabled: true,
                })
                .collect(),
            "help" => COMMANDS
                .iter()
                .map(|command| Item {
                    label: format!(
                        "{} {} · {}",
                        command.name, command.usage, command.description
                    ),
                    id: command.name.into(),
                    extra: command.help(self.command_context()),
                    enabled: true,
                })
                .chain(HELP.lines().enumerate().map(|(i, line)| Item {
                    label: line.into(),
                    id: format!("key-{i}"),
                    extra: line.into(),
                    enabled: true,
                }))
                .collect(),
            _ => COMMANDS
                .iter()
                .map(|command| Item {
                    label: format!(
                        "{} {} · {}",
                        command.name, command.usage, command.description
                    ),
                    id: command.name.into(),
                    extra: command.help(self.command_context()),
                    enabled: command.disabled(self.command_context()).is_none(),
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
    fn can_switch_session(&mut self) -> bool {
        if self.drafts.len() >= 32
            && !self.drafts.iter().any(|(id, _)| id == &self.sid)
            && (!self.editor.text.is_empty() || !self.references.is_empty())
        {
            self.notice="32 local session drafts retained. Clear this draft/references explicitly before switching; none were discarded.".into();
            false
        } else {
            true
        }
    }
    fn command_context(&self) -> Context {
        Context {
            connected: self.connected,
            active: self.active(),
            busy: self.busy,
            session: !self.sid.is_empty(),
        }
    }
    fn refresh_picker(&mut self, kind: &'static str) {
        if self.picker.as_ref().is_some_and(|p| p.kind == kind) {
            let old = self.picker.take().unwrap();
            self.open_picker(kind);
            if let Some(p) = &mut self.picker {
                p.query = old.query;
                p.selected = old.selected.min(p.visible().len().saturating_sub(1));
            }
        }
    }
    fn request_cancel(&mut self, worker: &Worker) {
        if self.state == "starting" {
            self.cancel_after_start = true;
            self.notice =
                "Cancellation will be requested when the daemon acknowledges the run ID.".into();
        } else if self.active() && self.state != "cancelling" {
            self.notice = "Cancellation requested; awaiting daemon acknowledgement.".into();
            self.request(
                worker,
                "run.cancel",
                json!({"run_id":self.run_id}),
                "cancel",
            );
        }
    }
    fn command(&mut self, text: &str, worker: &Worker) -> bool {
        self.request_failed = false;
        let (command, argument) = match commands::parse(text) {
            Ok(value) => value,
            Err(error) => {
                self.notice = error;
                return false;
            }
        };
        if let Some(reason) = command.disabled(self.command_context()) {
            self.notice = reason.into();
            return false;
        }
        use CommandId::*;
        if matches!(command.id, New | Resume) && !self.can_switch_session() {
            return false;
        }
        match command.id {
            Palette => self.open_picker("command"),
            Search => self.search = Some(argument.into()),
            Help => {
                self.open_picker("help");
                if let Some(p) = &mut self.picker {
                    p.query = argument.into();
                }
            }
            New => {
                self.busy = true;
                self.request(
                    worker,
                    "session.create",
                    json!({"title":if argument.is_empty(){"Terminal session"}else{argument}}),
                    "new",
                );
            }
            Sessions => {
                self.open_picker("session");
                if let Some(p) = &mut self.picker {
                    p.query = argument.into();
                }
                self.request(worker, "session.list", json!({}), "sessions");
            }
            Resume => {
                if argument.is_empty() {
                    return self.command("/sessions", worker);
                }
                let sid = if argument == "last" {
                    self.sessions
                        .iter()
                        .find(|s| s["archived"] != true)
                        .and_then(|s| s["session_id"].as_str())
                        .unwrap_or("")
                } else {
                    argument
                };
                if sid.is_empty() || !self.sessions.iter().any(|s| s["session_id"] == sid) {
                    self.notice =
                        "Session not in the current list. /sessions refreshes and searches it."
                            .into();
                    return false;
                }
                let sid = sid.to_owned();
                self.busy = true;
                self.request(
                    worker,
                    "terminal.attach",
                    json!({"session_id":sid}),
                    "attach",
                );
            }
            Model => {
                self.provider_filter = None;
                self.open_picker("model");
                self.request(worker, "model.list", json!({}), "models");
            }
            Provider => {
                self.open_picker("provider");
                self.refresh(worker);
            }
            Workspace => {
                self.open_picker("workspace");
                self.refresh(worker);
            }
            Mode => {
                if argument.is_empty() {
                    self.open_picker("mode");
                } else {
                    self.agent = argument == "agent";
                }
            }
            Chat => self.agent = false,
            Agent => self.agent = true,
            Status => self.request(worker, "daemon.status", json!({}), "status"),
            Doctor => self.request(worker, "terminal.state", json!({}), "doctor"),
            Exit => {
                let is_exit_command = commands::parse(self.editor.text.trim())
                    .is_ok_and(|(c, _)| c.id == Exit)
                    && !self.editor.literal;
                if (!self.editor.text.is_empty() && !is_exit_command) || !self.references.is_empty()
                {
                    self.notice =
                        "Unsent draft/references preserved. Clear them explicitly before exiting."
                            .into();
                    return false;
                }
                self.exit_requested = true;
            }
            Cancel => self.request_cancel(worker),
            Details => {
                self.view = None;
                self.show_details = !self.show_details;
                self.notice = format!(
                    "Execution details {} · safe metadata only; raw payloads/reasoning unavailable.",
                    if self.show_details {
                        "expanded"
                    } else {
                        "collapsed"
                    }
                );
            }
            History => {
                self.view = None;
                if argument.is_empty() {
                    self.request(
                        worker,
                        "session.messages",
                        json!({"session_id":self.sid}),
                        "messages",
                    );
                } else {
                    self.search = Some(argument.into());
                }
            }
            Export | Rename | Archive => {
                if command.id == Archive
                    && (!self.editor.text.is_empty() && self.editor.text.trim() != "/archive"
                        || !self.references.is_empty())
                {
                    self.notice =
                        "Archive requires an empty draft and no references; draft preserved."
                            .into();
                    return false;
                }
                let (action, args) = match command.id {
                    Export => (
                        "exportTranscript",
                        json!([if argument.is_empty() {
                            "markdown"
                        } else {
                            argument
                        }]),
                    ),
                    Rename => ("renameConversation", json!([self.sid, argument])),
                    _ => ("archiveConversation", json!([self.sid])),
                };
                self.busy = true;
                self.request(
                    worker,
                    "desktop.action",
                    json!({"session_id":self.sid,"action":action,"arguments":args}),
                    if command.id == Export {
                        "export"
                    } else {
                        if command.id == Archive {
                            "archive"
                        } else {
                            "session-action"
                        }
                    },
                );
            }
            Theme => {
                if argument.is_empty() {
                    self.open_picker("theme");
                } else if let Some(selected) = theme::Theme::parse(argument) {
                    self.theme = selected;
                    self.notice = format!(
                        "Theme: {} · {}",
                        selected.name(),
                        if std::env::var_os("NO_COLOR").is_some() {
                            "NO_COLOR disables color; launch without NO_COLOR to see the palette."
                        } else {
                            "this process; SENTINEL_TUI_THEME sets the launch default."
                        }
                    );
                } else {
                    self.notice =
                        "Unknown theme. /theme: terminal, obsidian, glacier, porcelain.".into();
                    return false;
                }
            }
            Settings => {
                self.view = Some(format!(
                    "TUI preferences (inspection)\nTheme: {} (/theme changes this process; SENTINEL_TUI_THEME sets launch default)\nNO_COLOR: {}\nASCII borders: {} (SENTINEL_TUI_ASCII)\nExecution details: {} (this TUI process; /details toggles)\nBindings: fixed V2 contract; /help is generated from the command registry.\nNo duplicate configuration file; persistent customization is deferred.",
                    self.theme.name(),
                    std::env::var_os("NO_COLOR").is_some(),
                    std::env::var_os("SENTINEL_TUI_ASCII").is_some(),
                    self.show_details
                ));
            }
            Tools | Permissions | Mcp | Tasks | Context | Memory => {
                self.inspection = Some(command.id);
                self.request(worker, "terminal.state", json!({}), "inspect");
            }
            Activity => {
                self.view = Some(format!(
                    "Execution detail · safe metadata\n{}",
                    self.activity.join("\n")
                ));
            }
            Notices => {
                self.view = Some(format!(
                    "System notices\n{}\n{}",
                    self.system_notices, self.notice
                ));
            }
            Files => {
                self.busy = true;
                self.file_request_session = self.sid.clone();
                self.request(
                    worker,
                    "workspace.files",
                    json!({"session_id":self.sid}),
                    "files",
                );
            }
            References => self.open_picker("references"),
            Remove => {
                let index = if argument.is_empty() {
                    self.references.len().checked_sub(1)
                } else {
                    argument
                        .parse::<usize>()
                        .ok()
                        .and_then(|n| n.checked_sub(1))
                };
                if let Some(index) = index.filter(|&i| i < self.references.len()) {
                    self.references.remove(index);
                } else {
                    self.notice =
                        "No reference at that index. /references lists selected paths.".into();
                    return false;
                }
            }
            Diff => {
                self.busy = true;
                self.request(
                    worker,
                    "workspace.changes",
                    json!({"session_id":self.sid}),
                    "changes",
                );
            }
            Reconnect => self.request(worker, "reconnect", json!({}), "reconnect"),
            Plan | Compact | Undo | Redo | Skills | Init | Grill | ExternalEditor => {
                unreachable!("unavailable commands are rejected before dispatch")
            }
        }
        self.follow = true;
        !self.request_failed
    }
    fn inspect_projection(&mut self, command: CommandId) {
        let needle = match command {
            CommandId::Tools => "tool",
            CommandId::Permissions => "permission",
            CommandId::Tasks => "agent",
            CommandId::Mcp => "mcp",
            CommandId::Context => "context",
            _ => "memory",
        };
        let properties = self.diagnostics["properties"]
            .as_object()
            .map(|p| {
                p.iter()
                    .filter(|(key, _)| key.to_lowercase().contains(needle))
                    .map(|(k, v)| (k.clone(), v.clone()))
                    .collect::<serde_json::Map<_, _>>()
            })
            .unwrap_or_default();
        self.view = Some(if properties.is_empty() {
            format!(
                "No {needle} metadata is exposed by the daemon. /doctor shows full supported diagnostics."
            )
        } else {
            format!(
                "{needle} metadata (daemon projection; not a new capability)\n{}",
                serde_json::to_string_pretty(&properties).unwrap_or_default()
            )
        });
    }
    fn slash_open(&self) -> bool {
        self.picker.is_none()
            && self.search.is_none()
            && self.approval.is_none()
            && !self.editor.literal
            && self.editor.text.starts_with('/')
            && !self.editor.text.starts_with("//")
            && !self.editor.text.chars().any(char::is_whitespace)
            && self.slash_dismissed.as_deref() != Some(self.editor.text.as_str())
    }
    fn complete_slash(&mut self, execute_exact: bool, worker: &Worker) {
        let matches = commands::matching(self.editor.text.trim_start_matches('/'));
        if let Some(command) = matches.get(self.slash_selected.min(matches.len().saturating_sub(1)))
        {
            if execute_exact && commands::find(&self.editor.text).is_some() {
                self.send(worker);
                return;
            }
            self.editor.replace(
                format!(
                    "{}{}",
                    command.name,
                    if command.arguments == commands::Arguments::None {
                        ""
                    } else {
                        " "
                    }
                ),
                false,
            );
            self.slash_dismissed = Some(self.editor.text.clone());
            self.slash_selected = 0;
            self.notice = format!(
                "{} · {} · Enter executes explicitly",
                command.name, command.usage
            );
        } else if execute_exact {
            self.send(worker);
        }
    }
    fn select_picker(&mut self, worker: &Worker) {
        let Some(picker) = &self.picker else {
            return;
        };
        let kind = picker.kind;
        let Some(item) = picker
            .visible()
            .get(picker.selected)
            .map(|item| (*item).clone())
        else {
            return;
        };
        if !item.enabled {
            self.notice = format!("Unavailable: {}", item.extra);
            return;
        }
        let guarded = matches!(
            kind,
            "model" | "provider" | "workspace" | "session" | "file"
        );
        if guarded && (!self.connected || self.active() || self.busy) {
            self.notice="Disconnected or active/pending operation prevents switching selections. Draft preserved.".into();
            return;
        }
        if kind == "session" && !self.can_switch_session() {
            return;
        }
        self.picker = None;
        match kind {
            "model" => {
                self.busy = true;
                self.request(
                    worker,
                    "model.select",
                    json!({"provider_id":item.extra,"model_id":item.id}),
                    "selection",
                );
            }
            "workspace" => {
                self.busy = true;
                self.request(
                    worker,
                    "workspace.select",
                    json!({"workspace_id":item.id}),
                    "selection",
                );
            }
            "provider" => {
                if self.models.iter().any(|m| m["provider_id"] == item.id) {
                    self.provider_filter = Some(item.id.clone());
                    self.open_picker("model");
                    if let Some(p) = &mut self.picker {
                        p.items.retain(|m| m.extra == item.id);
                    }
                } else {
                    self.busy = true;
                    self.request(
                        worker,
                        "provider.select",
                        json!({"provider_id":item.id}),
                        "provider-selection",
                    );
                }
            }
            "session" => {
                self.busy = true;
                self.request(
                    worker,
                    "terminal.attach",
                    json!({"session_id":item.id}),
                    "attach",
                );
            }
            "theme" => {
                self.command(&format!("/theme {}", item.id), worker);
            }
            "mode" => {
                self.command(&format!("/mode {}", item.id), worker);
            }
            "references" => {
                if let Ok(i) = item.id.parse::<usize>()
                    && i < self.references.len()
                {
                    self.references.remove(i);
                }
            }
            "file" => {
                if !self.references.contains(&item.id) && self.references.len() < 16 {
                    self.references.push(item.id);
                    self.notice =
                        "Workspace reference selected; /references inspects or removes it.".into();
                } else {
                    self.notice =
                        "Reference already selected or 16-reference limit reached.".into();
                }
            }
            "review" => {
                if let Ok(i) = item.id.parse::<usize>()
                    && let Some(file) = self.changes.get(i)
                {
                    self.view = Some(format!(
                        "{} · Applied\n{}",
                        file["path"].as_str().unwrap_or("?"),
                        file["diff"].as_str().unwrap_or("")
                    ));
                    self.follow = false;
                    self.scroll = 0;
                }
            }
            "help" => {
                self.view = Some(item.extra);
            }
            _ => {
                self.command(&item.id, worker);
            }
        }
    }
    fn paste(&mut self, text: &str) {
        if self.picker.is_none() && self.approval.is_none() && self.search.is_none() {
            self.editor.paste(text);
        }
    }
    fn handle_key(&mut self, key: crossterm::event::KeyEvent, worker: &Worker) {
        let app = self;
        let ctrl = key.modifiers.contains(KeyModifiers::CONTROL);
        let alt = key.modifiers.contains(KeyModifiers::ALT);
        if ctrl && key.code == KeyCode::Char('c') {
            if app.active() {
                app.command("/cancel", worker);
            } else if app.editor.text.is_empty()
                && app.picker.is_none()
                && app.search.is_none()
                && app.references.is_empty()
                && !app.busy
            {
                app.exit_requested = true;
                return;
            } else {
                app.picker = None;
                app.search = None;
                app.notice = "Draft preserved. Ctrl+D exits only with an empty composer.".into();
            }
            return;
        }
        if let Some(p) = &app.approval {
            if !app.approval_pending && [KeyCode::Char('y'), KeyCode::Char('n')].contains(&key.code)
            {
                let payload = json!({"run_id":app.run_id,"approval_id":p["approval_id"],"allow":key.code==KeyCode::Char('y')});
                app.approval_pending = true;
                app.approval_allow = Some(key.code == KeyCode::Char('y'));
                app.request(worker, "approval.respond", payload, "approval");
            }
            return;
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
                KeyCode::Enter => app.select_picker(worker),
                _ => {}
            }
            return;
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
                    let rows = wrap_transcript(
                        transcript_lines(&conversation_text(app)),
                        app.reading_width.get().max(1),
                    );
                    if let Some(line) = rows
                        .iter()
                        .position(|line| line.to_string().to_lowercase().contains(&query))
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
            return;
        }
        if app.slash_open() {
            match key.code {
                KeyCode::Esc => {
                    app.slash_dismissed = Some(app.editor.text.clone());
                    return;
                }
                KeyCode::Up => {
                    app.slash_selected = app.slash_selected.saturating_sub(1);
                    return;
                }
                KeyCode::Down => {
                    app.slash_selected = (app.slash_selected + 1).min(
                        commands::matching(app.editor.text.trim_start_matches('/'))
                            .len()
                            .saturating_sub(1),
                    );
                    return;
                }
                KeyCode::Tab => {
                    app.complete_slash(false, worker);
                    return;
                }
                KeyCode::Enter => {
                    app.complete_slash(true, worker);
                    return;
                }
                _ => {}
            }
        }
        if let Some(command) = commands::for_key(key) {
            if command.id == CommandId::Mode {
                app.command(
                    if app.agent {
                        "/mode chat"
                    } else {
                        "/mode agent"
                    },
                    worker,
                );
            } else {
                app.command(command.name, worker);
            }
            return;
        }
        if newline_key(key) {
            app.editor.insert("\n");
            return;
        }
        match key.code {
            KeyCode::Esc => {
                app.view = None;
                app.slash_dismissed = Some(app.editor.text.clone());
            }
            KeyCode::Char('a') if ctrl => app.editor.home(),
            KeyCode::Char('e') if ctrl => app.editor.end(),
            KeyCode::Char('u') if ctrl => app.editor.kill_to_start(),
            KeyCode::Char('k') if ctrl => app.editor.kill_to_end(),

            KeyCode::Char('b') if ctrl => app.follow = true,

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
            KeyCode::Enter => app.send(worker),
            KeyCode::Left if ctrl => app.editor.word(false),
            KeyCode::Right if ctrl => app.editor.word(true),
            KeyCode::Left => app.editor.left(),
            KeyCode::Right => app.editor.right(),
            KeyCode::Home => app.editor.home(),
            KeyCode::End => app.editor.end(),
            KeyCode::Up if alt || !app.editor.text.contains('\n') => app.editor.history(false),
            KeyCode::Down if alt || !app.editor.text.contains('\n') => app.editor.history(true),
            KeyCode::Up => app.editor.vertical(false),
            KeyCode::Down => app.editor.vertical(true),
            KeyCode::Backspace => app.editor.backspace(),
            KeyCode::Delete => app.editor.delete(),
            KeyCode::Char('?') if app.editor.text.is_empty() => {
                app.command("/help", worker);
            }
            KeyCode::Char('@') if !ctrl => {
                // An escaped @ is ordinary text; plain @ uses authorized discovery.
                if app.editor.text[..app.editor.cursor].ends_with('\\') {
                    app.editor.insert("@");
                } else {
                    app.command("/files", worker);
                }
            }
            KeyCode::Char('d') if ctrl => {
                if !app.active()
                    && !app.busy
                    && app.editor.text.is_empty()
                    && app.references.is_empty()
                {
                    app.exit_requested = true;
                    return;
                }
                app.editor.delete();
            }
            KeyCode::Char(c) if !ctrl => app.editor.insert(&c.to_string()),
            _ => {}
        }
    }
    fn send(&mut self, worker: &Worker) {
        if self.editor.text.trim().is_empty() {
            return;
        }
        if self.editor.text.starts_with('/')
            && !self.editor.text.starts_with("//")
            && !self.editor.literal
        {
            let command = self.editor.text.clone();
            if self.command(&command, worker) {
                self.editor.take();
            }
            return;
        }
        if self.active() || self.busy {
            self.notice = "Active/pending operation; your draft is preserved.".into();
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
        self.pending_literal = self.editor.literal;
        self.latest_user = self.editor.text.clone();
        self.prior_transcript = std::mem::take(&mut self.output);
        self.turn_visible = true;
        self.view = None;
        self.pending_references = self.references.clone();
        let mut text = self.editor.take();
        if !self.pending_literal && text.starts_with("//") {
            text.remove(0);
        }
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
    let block = Block::bordered().style(theme::base());
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
    theme::accent().add_modifier(Modifier::BOLD)
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
// Presentation only: canonical roles and content remain owned by the daemon.
fn message_block(role: &str, content: &str) -> String {
    let (label, indent) = match role {
        "user" => ("> You", "    "),
        "assistant" => ("Assistant", "  "),
        "agent" => ("Agent", "  "),
        "tool" => ("Execution detail", "    "),
        "system" => ("System notice", "  "),
        _ => (role, "  "),
    };
    format!(
        "{label}\n{}\n",
        content
            .lines()
            .map(|line| format!("{indent}{line}"))
            .collect::<Vec<_>>()
            .join("\n")
    )
}

fn transcript_lines(text: &str) -> Vec<Line<'static>> {
    let mut code = false;
    let mut detail = false;
    text.lines()
        .map(|line| {
            let trimmed = line.trim_start();
            let heading = [
                "> You",
                "Assistant",
                "Agent",
                "Execution detail",
                "System notice",
                "Execution timeline · /activity for details",
            ]
            .contains(&line)
                || line.starts_with("Assistant ·")
                || line.starts_with("Agent ·");
            if heading {
                detail = line.starts_with("Execution") || line == "System notice";
                code = false;
            }
            let style = if heading {
                if line == "> You" {
                    Style::default().add_modifier(Modifier::BOLD)
                } else {
                    header_style()
                }
            } else if code || detail || trimmed.starts_with("```") {
                Style::default().add_modifier(Modifier::DIM)
            } else {
                Style::default()
            };
            if trimmed.starts_with("```") {
                code = !code;
            }
            // Only the daemon's known failure wrapper is normalized; source/code stays literal.
            let rendered = if !code && trimmed == "**Ajan Görevi Başarısız / Agent Task Failed**"
            {
                line.replacen(
                    "**Ajan Görevi Başarısız / Agent Task Failed**",
                    "Ajan Görevi Başarısız / Agent Task Failed",
                    1,
                )
            } else if !code
                && trimmed.starts_with('*')
                && trimmed.ends_with(" steps executed.*")
                && trimmed.contains(" adım çalıştırıldı / ")
            {
                format!(
                    "{}{}",
                    &line[..line.len() - trimmed.len()],
                    &trimmed[1..trimmed.len() - 1]
                )
            } else {
                line.to_owned()
            };
            Line::styled(rendered, style)
        })
        .collect()
}

// Wrap once so scrolling and follow-latest use the same rows that are rendered.
// Prefer word boundaries, but allow uninterrupted paths/code to wrap by cell width.
fn wrap_transcript(lines: Vec<Line<'static>>, width: usize) -> Vec<Line<'static>> {
    let mut rows = Vec::new();
    for line in lines {
        let style = line.style;
        let text = line.to_string();
        let mut remaining = text.as_str();
        while !remaining.is_empty() {
            let mut cells = 0;
            let mut end = 0;
            let mut word_end = 0;
            let mut has_word = false;
            for (index, ch) in remaining.char_indices() {
                let cell_width =
                    ratatui::text::Span::raw(&remaining[index..index + ch.len_utf8()]).width();
                if cells + cell_width > width {
                    break;
                }
                cells += cell_width;
                end = index + ch.len_utf8();
                if !ch.is_whitespace() {
                    has_word = true;
                }
                if ch.is_whitespace() && has_word {
                    word_end = end;
                }
            }
            // A glyph wider than the entire viewport must still make progress.
            if end == 0 {
                end = remaining.chars().next().unwrap().len_utf8();
            }
            if end < remaining.len() && word_end > 0 {
                end = word_end;
            }
            rows.push(Line::styled(remaining[..end].to_owned(), style));
            remaining = &remaining[end..];
        }
        if text.is_empty() {
            rows.push(Line::raw(""));
        }
    }
    rows
}

fn conversation_text(app: &App) -> String {
    if let Some(view) = &app.view {
        return view.clone();
    }
    let empty = app.view.is_none() && app.output.is_empty() && !app.active() && !app.turn_visible;
    if empty {
        let symbol = if std::env::var_os("SENTINEL_TUI_ASCII").is_some() {
            "[ S ]"
        } else {
            "◇ S ◇"
        };
        format!(
            "{symbol}\n\nStart a new conversation\nAsk a question, explore an idea, or describe a task.\nCtrl+P commands  /help shortcuts"
        )
    } else if app.active() || app.turn_visible {
        format!(
            "{}{}{}{}\n{}{}",
            if app.prior_transcript.is_empty() {
                String::new()
            } else {
                format!("{}\n", app.prior_transcript)
            },
            if app.latest_user.is_empty() {
                String::new()
            } else {
                format!("{}\n", message_block("user", &app.latest_user))
            },
            if app.agent { "Agent" } else { "Assistant" },
            if app.active() {
                format!(" · {}", app.state)
            } else {
                String::new()
            },
            message_block(if app.agent { "agent" } else { "assistant" }, &app.output)
                .lines()
                .skip(1)
                .collect::<Vec<_>>()
                .join("\n"),
            if app.activity.is_empty() {
                String::new()
            } else {
                format!(
                    "\n\nExecution timeline · /activity for details\n{}",
                    app.activity
                        .iter()
                        .rev()
                        .take(if app.show_details { 200 } else { 3 })
                        .rev()
                        .map(|line| format!(
                            "  {}",
                            if app.show_details {
                                line.as_str()
                            } else {
                                line.lines().next().unwrap_or("")
                            }
                        ))
                        .collect::<Vec<_>>()
                        .join("\n")
                )
            }
        )
    } else {
        app.output.clone()
    }
}

fn content_grid(area: Rect) -> Rect {
    let width = area.width.saturating_sub(4).clamp(1, 180);
    Rect::new(
        area.x + (area.width - width) / 2,
        area.y,
        width,
        area.height,
    )
}

fn fit_label(text: &str, width: u16) -> String {
    if Line::raw(text).width() <= width as usize {
        return text.to_owned();
    }
    if width < 4 {
        return ".".repeat(width as usize);
    }
    let mut result = String::new();
    let mut cells = 0;
    for ch in text.chars() {
        let size = ratatui::text::Span::raw(ch.to_string()).width();
        if cells + size > width as usize - 3 {
            break;
        }
        result.push(ch);
        cells += size;
    }
    result.push_str("...");
    result
}

fn run_failure_notice(detail: &str) -> String {
    match detail {
        "ConnectionFailed" => "Provider connection failed. /doctor checks its endpoint; the daemon is still connected.".into(),
        "Timeout" => "Inference timed out. /doctor checks provider readiness; retry explicitly from prompt history.".into(),
        "RateLimited" => "Provider rate limit reached. Retry later explicitly from prompt history.".into(),
        "CapabilityUnsupported" => "Selected model does not support this operation. /model inspects available capabilities.".into(),
        _ => Error::from_run_detail(detail).to_string(),
    }
}

fn readiness_label(app: &App) -> &'static str {
    if !app.connected {
        return "Inference unknown";
    }
    match app.diagnostics["properties"]["activeRuntimeReadinessState"].as_str() {
        Some("Available") => "Inference unverified",
        Some("Degraded") => "Provider degraded",
        Some("Unavailable") => "Provider unavailable",
        _ => "Inference unknown",
    }
}

fn secondary_style() -> Style {
    theme::secondary()
}

fn draw(frame: &mut Frame, app: &App) {
    app.theme.apply();
    frame.render_widget(Block::default().style(theme::base()), frame.area());
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
    let composer_height = app
        .editor
        .text
        .split('\n')
        .count()
        .saturating_add(2)
        .min((frame.area().height / 3).max(3) as usize) as u16;
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
    let workspace = app.diagnostics["workspace"]["name"]
        .as_str()
        .unwrap_or("unknown");
    let workspace_x = header_columns[0].x + 21;
    if header_columns[0].width > 40 {
        frame.render_widget(
            Paragraph::new(fit_label(
                &format!("Workspace: {workspace}"),
                header_columns[0].width.saturating_sub(22),
            ))
            .style(secondary_style()),
            Rect::new(
                workspace_x,
                header_rows[0].y,
                header_columns[0].width.saturating_sub(22),
                1,
            ),
        );
    }
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
        panel_block()
            .borders(ratatui::widgets::Borders::BOTTOM)
            .border_style(Style::default().add_modifier(Modifier::DIM)),
        header_rows[1],
    );
    let info_columns = Layout::horizontal([
        Constraint::Min(1),
        Constraint::Length(readiness_label(app).len() as u16 + 1),
    ])
    .split(header_rows[2]);
    let identity_width = info_columns[0].width;
    let provider = properties["activeRuntimeProviderLabel"]
        .as_str()
        .filter(|s| !s.is_empty())
        .unwrap_or("Provider unselected");
    let model = properties["activeRuntimeModelLabel"]
        .as_str()
        .filter(|s| !s.is_empty())
        .unwrap_or("Model unselected");
    // Give both identities a bounded share; readiness has its own reserved space.
    let provider_width = (identity_width / 3).max(1);
    let identity = format!(
        "{} / {}",
        fit_label(provider, provider_width),
        fit_label(model, identity_width.saturating_sub(provider_width + 3))
    );
    frame.render_widget(
        Paragraph::new(identity).style(secondary_style()),
        info_columns[0],
    );
    frame.render_widget(
        Paragraph::new(readiness_label(app))
            .alignment(Alignment::Right)
            .style(secondary_style()),
        info_columns[1],
    );
    let grid = content_grid(areas[1]);
    let reading_area = Rect::new(
        grid.x + 2,
        grid.y + 1,
        grid.width.saturating_sub(4).max(1),
        grid.height.saturating_sub(1),
    );
    let composer_area = content_grid(areas[2]);
    let composer_inner = Rect::new(
        composer_area.x + 4,
        composer_area.y + 1,
        composer_area.width.saturating_sub(6),
        composer_area.height.saturating_sub(2),
    );
    let height = reading_area.height as usize;
    let width = reading_area.width as usize;
    app.reading_width.set(width);
    let empty = app.view.is_none() && app.output.is_empty() && !app.active() && !app.turn_visible;
    let transcript = conversation_text(app);
    let styled_lines = wrap_transcript(
        if empty {
            transcript
                .lines()
                .map(|line| {
                    Line::styled(
                        line.to_owned(),
                        if line == "Start a new conversation" || line.contains(" S ") {
                            header_style()
                        } else {
                            secondary_style()
                        },
                    )
                })
                .collect()
        } else {
            transcript_lines(&transcript)
        },
        width,
    );
    let lines = styled_lines.len();
    let scroll = if app.follow {
        lines.saturating_sub(height).min(u16::MAX as usize) as u16
    } else {
        app.scroll
    };
    app.viewport_bottom
        .set(lines.saturating_sub(height).min(u16::MAX as usize) as u16);
    let transcript_area = if empty {
        let content_height = lines.min(u16::MAX as usize) as u16;
        Rect::new(
            reading_area.x,
            reading_area.y + reading_area.height.saturating_sub(content_height) / 2,
            reading_area.width,
            content_height.min(reading_area.height),
        )
    } else {
        reading_area
    };
    frame.render_widget(
        Paragraph::new(styled_lines)
            .alignment(if empty {
                Alignment::Center
            } else {
                Alignment::Left
            })
            .scroll((if empty { 0 } else { scroll }, 0)),
        transcript_area,
    );
    let before = &app.editor.text[..app.editor.cursor];
    let row = before.chars().filter(|&c| c == '\n').count();
    let col = Line::raw(before.rsplit('\n').next().unwrap_or("")).width();
    let horizontal = col.saturating_sub(composer_inner.width.saturating_sub(1) as usize);
    let visible = composer_inner.height as usize;
    let start = row.saturating_sub(visible.saturating_sub(1));
    frame.render_widget(
        Paragraph::new(if app.editor.text.is_empty() {
            if app.agent {
                "Describe a task for Agent…"
            } else {
                "Ask a question or share an idea…"
            }
        } else {
            app.editor.text.as_str()
        })
        .style(if app.editor.text.is_empty() {
            secondary_style()
        } else {
            Style::default()
        })
        .scroll((
            start.min(u16::MAX as usize) as u16,
            horizontal.min(u16::MAX as usize) as u16,
        ))
        .block(
            panel_block()
                .border_style(
                    if app.picker.is_none() && app.approval.is_none() && app.search.is_none() {
                        header_style()
                    } else {
                        Style::default().add_modifier(Modifier::DIM)
                    },
                )
                .padding(Padding::new(3, 1, 0, 0))
                .title(if app.agent {
                    " Message · Agent "
                } else {
                    " Message · Chat "
                }),
        ),
        composer_area,
    );
    if app.picker.is_none()
        && app.approval.is_none()
        && app.search.is_none()
        && composer_inner.width > 0
        && composer_inner.height > 0
    {
        frame.set_cursor_position((
            composer_inner.x
                + col
                    .saturating_sub(horizontal)
                    .min(composer_inner.width.saturating_sub(1) as usize) as u16,
            composer_inner.y + row.saturating_sub(start).min(visible.saturating_sub(1)) as u16,
        ));
    }
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
    } else if !app.notice.is_empty() {
        format!("/notices · {}", app.notice)
    } else if !app.system_notices.is_empty() {
        "System notices · /notices to read".into()
    } else if !app.connected {
        "Daemon disconnected · /reconnect retries · draft preserved".into()
    } else if app.models.is_empty() {
        "No discovered models · /model or /doctor".into()
    } else {
        String::new()
    };
    let footer_rows = Layout::vertical([Constraint::Length(1), Constraint::Length(1)])
        .split(content_grid(areas[3]));
    frame.render_widget(
        Paragraph::new(fit_label(
            &format!("{reference_hint}{notice}"),
            footer_rows[0].width,
        )),
        footer_rows[0],
    );
    let footer_columns =
        Layout::horizontal([Constraint::Min(1), Constraint::Length(14)]).split(footer_rows[1]);
    frame.render_widget(
        Paragraph::new(if app.approval.is_some() {
            "y Allow Once · n Deny · Ctrl+C cancel"
        } else if app.picker.is_some() {
            "Arrows select · Enter confirm · Esc close"
        } else if app.view.is_some() {
            "Esc conversation · Ctrl+P commands · F1 help"
        } else if app.active() || app.busy {
            "Ctrl+C cancel · /details expand · F1 help"
        } else {
            "Enter send · Shift+Enter newline · Ctrl+P commands"
        })
        .style(secondary_style()),
        footer_columns[0],
    );
    frame.render_widget(
        Paragraph::new(app.state.as_str())
            .alignment(Alignment::Right)
            .style(header_style()),
        footer_columns[1],
    );
    if app.slash_open() {
        let matches = commands::matching(app.editor.text.trim_start_matches('/'));
        let selected = app.slash_selected.min(matches.len().saturating_sub(1));
        let popup_height = 8.min(composer_area.y.saturating_sub(3));
        if popup_height >= 4 {
            let popup = Rect::new(
                composer_area.x,
                composer_area.y - popup_height,
                composer_area.width,
                popup_height,
            );
            frame.render_widget(Clear, popup);
            frame.render_widget(
                panel_block().title(" Commands · Tab complete · Enter select · Esc dismiss "),
                popup,
            );
            let inner = Rect::new(
                popup.x + 1,
                popup.y + 1,
                popup.width.saturating_sub(2),
                popup.height.saturating_sub(2),
            );
            let capacity = inner.height.saturating_sub(2) as usize;
            let start = selected.saturating_sub(capacity.saturating_sub(1));
            let mut rows = matches
                .iter()
                .enumerate()
                .skip(start)
                .take(capacity)
                .map(|(i, c)| {
                    let disabled = c.disabled(app.command_context());
                    Line::styled(
                        fit_label(
                            &format!(
                                "{} {} {} · {}{}",
                                if i == selected { ">" } else { " " },
                                c.name,
                                c.usage,
                                c.description,
                                if disabled.is_some() {
                                    " [unavailable]"
                                } else {
                                    ""
                                }
                            ),
                            inner.width,
                        ),
                        if i == selected {
                            header_style()
                        } else {
                            secondary_style()
                        },
                    )
                })
                .collect::<Vec<_>>();
            if let Some(command) = matches.get(selected) {
                rows.push(Line::raw(fit_label(
                    &format!("Example: {}", command.example),
                    inner.width,
                )));
                rows.push(Line::raw(fit_label(
                    command
                        .disabled(app.command_context())
                        .unwrap_or("Completion never executes; Enter an exact command to run it."),
                    inner.width,
                )));
            } else {
                rows.push(Line::raw(
                    "No matching command · /help · // literal slash text",
                ));
            }
            frame.render_widget(Paragraph::new(rows), inner);
        }
    }
    if let Some(picker) = &app.picker {
        let area = modal(frame.area());
        frame.render_widget(Clear, area);
        frame.render_widget(
            panel_block().title(format!(
                "{} · Enter {} · Esc close",
                picker.kind,
                if picker.kind == "help" {
                    "inspect"
                } else {
                    "select"
                }
            )),
            area,
        );
        let inner = Rect::new(
            area.x + 1,
            area.y + 1,
            area.width.saturating_sub(2),
            area.height.saturating_sub(2),
        );
        let sections = Layout::vertical([
            Constraint::Length(1),
            Constraint::Min(1),
            Constraint::Length(if picker.kind == "help" && inner.height >= 16 {
                8
            } else {
                4
            }),
        ])
        .split(inner);
        frame.render_widget(
            Paragraph::new(fit_label(
                &format!("Filter: {}", picker.query),
                sections[0].width,
            )),
            sections[0],
        );
        let visible = picker.visible();
        let capacity = sections[1].height as usize;
        let start = picker.selected.saturating_sub(capacity.saturating_sub(1));
        let rows = visible
            .iter()
            .enumerate()
            .skip(start)
            .take(capacity)
            .map(|(i, item)| {
                Line::styled(
                    fit_label(
                        &format!(
                            "{} {}{}",
                            if i == picker.selected { ">" } else { " " },
                            item.label,
                            if item.enabled { "" } else { " [unavailable]" }
                        ),
                        sections[1].width,
                    ),
                    if i == picker.selected {
                        header_style()
                    } else {
                        Style::default()
                    },
                )
            })
            .collect::<Vec<_>>();
        frame.render_widget(Paragraph::new(rows), sections[1]);
        let detail = visible
            .get(picker.selected)
            .map(|item| {
                format!(
                    "{}\n{}",
                    if picker.kind == "file" {
                        json!(item.id).to_string()
                    } else {
                        item.id.clone()
                    },
                    item.extra
                )
            })
            .unwrap_or_else(|| {
                "No matches. Change filter or Esc to return; draft preserved.".into()
            });
        frame.render_widget(
            Paragraph::new(detail)
                .wrap(Wrap { trim: false })
                .style(secondary_style()),
            sections[2],
        );
    }
    if let Some(p) = &app.approval {
        let area = modal(frame.area());
        frame.render_widget(Clear, area);
        frame.render_widget(Paragraph::new(format!("Tool: {} · risk {}\nResources: {}\n{}\nSession: {}\nRun: {}\nScope: this pending operation only\n{}",p["tool"].as_str().unwrap_or("unknown"),p["risk"],p["resources"],p["detail"].as_str().unwrap_or(""),app.sid,app.run_id,if app.approval_pending{"Waiting for authoritative response…"}else{"y Allow Once · n Deny · Esc keeps approval open"})).wrap(Wrap{trim:false}).block(panel_block().title("Approval required").style(theme::base().add_modifier(Modifier::BOLD))),area);
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
    let _ = execute!(
        std::io::stdout(),
        EnableBracketedPaste,
        PushKeyboardEnhancementFlags(KeyboardEnhancementFlags::DISAMBIGUATE_ESCAPE_CODES)
    );
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
                    app.paste(&text);
                }
                Event::Key(key) if key.kind == KeyEventKind::Press => {
                    app.handle_key(key, &worker);
                    if app.exit_requested {
                        break;
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
        for (width, height) in [(80, 24), (100, 30), (120, 40), (160, 48)] {
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
                .find(|&y| row(y).contains("Start a new conversation"))
                .unwrap();
            assert!(center > height / 3 && center < height * 2 / 3);
            assert!(row(height - 5).contains(" Message · Chat "));
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
    fn message_roles_and_technical_details_are_distinct() {
        let user = message_block("user", "Hello\nAssistant");
        assert_eq!(user, "> You\n    Hello\n    Assistant\n");
        assert!(message_block("assistant", "Hello").starts_with("Assistant\n  Hello"));
        assert!(message_block("tool", "output").starts_with("Execution detail\n    output"));
        let lines = transcript_lines("Assistant\n  text\n  ```rs\n  fn main() {}\n  ```\n  prose");
        assert!(!lines[1].style.add_modifier.contains(Modifier::DIM));
        assert!(lines[3].style.add_modifier.contains(Modifier::DIM));
        assert!(!lines[5].style.add_modifier.contains(Modifier::DIM));
    }
    #[test]
    fn wrapping_preserves_content_and_cell_width() {
        for width in [4, 20, 76, 88] {
            let text = "  words to wrap 界🙂 and /a/very/long/unbroken/path\n\n  last row";
            let rows = wrap_transcript(transcript_lines(text), width);
            assert!(rows.iter().all(|line| line.width() <= width));
            assert_eq!(
                rows.iter().map(Line::to_string).collect::<String>(),
                text.replace('\n', "")
            );
            assert!(rows.iter().any(|line| line.width() == 0));
        }
    }
    #[test]
    fn footer_does_not_duplicate_completed_and_composer_has_mode() {
        let mut terminal = Terminal::new(TestBackend::new(120, 40)).unwrap();
        let app = App {
            state: "completed".into(),
            output: message_block("assistant", "Finished response"),
            ..App::default()
        };
        terminal.draw(|frame| draw(frame, &app)).unwrap();
        let text = terminal
            .backend()
            .buffer()
            .content
            .iter()
            .map(|cell| cell.symbol())
            .collect::<String>();
        assert_eq!(text.matches("completed").count(), 1);
        assert!(text.contains("Message · Chat"));
        assert!(text.contains("Ask a question or share an idea"));
        let rows = wrap_transcript(transcript_lines(&app.output), 88);
        assert!(rows.iter().all(|line| line.width() <= 88));
    }
    #[test]
    fn long_response_follows_final_row_inside_reading_column() {
        for (width, height) in [(80, 24), (100, 30), (120, 40), (160, 48)] {
            let mut terminal = Terminal::new(TestBackend::new(width, height)).unwrap();
            let app = App {
                follow: true,
                output: message_block(
                    "assistant",
                    &format!(
                        "{}\nFINAL RESPONSE ROW",
                        "Long technical response with words and /unbroken/path ".repeat(200)
                    ),
                ),
                ..App::default()
            };
            terminal.draw(|frame| draw(frame, &app)).unwrap();
            let buffer = terminal.backend().buffer();
            let rows = (3..height - 5)
                .map(|y| {
                    (0..width)
                        .map(|x| buffer[(x, y)].symbol())
                        .collect::<String>()
                })
                .collect::<Vec<_>>();
            assert!(rows.iter().any(|line| line.contains("FINAL RESPONSE ROW")));
            let margin = content_grid(Rect::new(0, 0, width, height)).x + 2;
            for y in 3..height - 5 {
                for x in 0..margin {
                    assert_eq!(buffer[(x, y)].symbol(), " ");
                }
                for x in width - margin..width {
                    assert_eq!(buffer[(x, y)].symbol(), " ");
                }
            }
        }
    }
    fn rendered_rows(terminal: &Terminal<TestBackend>) -> Vec<String> {
        let buffer = terminal.backend().buffer();
        (0..buffer.area.height)
            .map(|y| {
                (0..buffer.area.width)
                    .map(|x| buffer[(x, y)].symbol())
                    .collect()
            })
            .collect()
    }
    #[test]
    fn v23_responsive_state_matrix() {
        for (width, height) in [(80, 24), (100, 30), (120, 40), (160, 48)] {
            for scenario in [
                "connected",
                "disconnected",
                "populated",
                "long-names",
                "running",
                "approval",
                "unfocused",
                "search",
            ] {
                let mut app = App {
                    connected: scenario != "disconnected",
                    state: "idle".into(),
                    follow: true,
                    diagnostics: json!({"workspace":{"name":"Sentinel"},"properties":{"activeRuntimeProviderLabel":"Local provider", "activeRuntimeModelLabel":"Selected model", "activeRuntimeReadinessState":"Available", "activeRuntimeReadinessSummary":"Catalog reachable; no inference initialization attestation"}}),
                    ..App::default()
                };
                match scenario {
                    "populated" => {
                        app.output = format!(
                            "{}\n{}",
                            message_block("user", "Explain the layout"),
                            message_block(
                                "assistant",
                                "A centered reading column.\n```rust\nlet ready = false;\n```"
                            )
                        )
                    }
                    "long-names" => {
                        app.diagnostics["workspace"]["name"] = json!("Workspace-name-".repeat(40));
                        app.diagnostics["properties"]["activeRuntimeProviderLabel"] =
                            json!("Provider-name-".repeat(40));
                        app.diagnostics["properties"]["activeRuntimeModelLabel"] =
                            json!("Model-name-".repeat(40));
                    }
                    "running" | "approval" => {
                        app.agent = true;
                        app.state = scenario.into();
                        if scenario == "approval" {
                            app.approval = Some(
                                json!({"tool":"file.write","risk":"medium","resources":["example.txt"],"detail":"Write requested file"}),
                            );
                        }
                        app.latest_user = "Inspect the workspace".into();
                        app.output = "Reading authorized context".into();
                        app.activity.push("step 1 · tool requested".into());
                    }
                    "unfocused" => app.open_picker("command"),
                    "search" => app.search = Some("query".into()),
                    _ => {}
                }
                let mut terminal = Terminal::new(TestBackend::new(width, height)).unwrap();
                terminal.draw(|frame| draw(frame, &app)).unwrap();
                let rows = rendered_rows(&terminal);
                let text = rows.join("\n");
                assert!(rows[0].contains(if app.agent {
                    "SENTINEL / Agent"
                } else {
                    "SENTINEL / Chat"
                }));
                assert!(rows[0].ends_with(if app.connected {
                    "Connected"
                } else {
                    "Disconnected"
                }));
                assert!(rows[2].contains(if app.connected {
                    "Inference unverified"
                } else {
                    "Inference unknown"
                }));
                assert!(!text.contains("Catalog reachable;"));
                if ["connected", "disconnected", "long-names"].contains(&scenario) {
                    let title_row = rows
                        .iter()
                        .position(|row| row.contains("Start a new conversation"))
                        .unwrap();
                    let start = rows[title_row].find("Start a new conversation").unwrap();
                    assert!(
                        (start as isize * 2 + "Start a new conversation".len() as isize
                            - width as isize)
                            .abs()
                            <= 1
                    );
                    assert!((title_row as isize * 2 - (height as isize - 2)).abs() <= 2);
                    assert!(text.contains("Ctrl+P commands"));
                } else if scenario == "populated" {
                    assert!(!text.contains("Start a new conversation"));
                    assert!(text.contains("> You"));
                    assert!(text.contains("Assistant"));
                    assert!(text.contains("let ready = false;"));
                    let response_row = rows
                        .iter()
                        .find(|line| line.contains("A centered reading column."))
                        .unwrap();
                    assert_eq!(
                        response_row.find("A centered reading column."),
                        Some((content_grid(Rect::new(0, 0, width, height)).x + 4) as usize)
                    );
                } else if scenario == "running" {
                    assert!(!text.contains("Start a new conversation"));
                    assert!(text.contains("Execution timeline"));
                    assert!(text.contains("Ctrl+C cancel"));
                } else if scenario == "approval" {
                    assert!(text.contains("Approval required"));
                    assert!(text.contains("y Allow Once"));
                }
                if scenario == "long-names" {
                    assert!(rows[0].contains("Workspace:"));
                    assert!(rows[0].contains("..."));
                    assert!(rows[2].contains("Prov"));
                    assert!(rows[2].contains("Model"));
                }
                let grid = content_grid(Rect::new(0, height - 5, width, 3));
                let bottom = &terminal.backend().buffer()[(grid.x, height - 3)];
                assert_eq!(
                    bottom.modifier.contains(Modifier::BOLD),
                    !["approval", "unfocused", "search"].contains(&scenario)
                );
                if !["approval", "unfocused", "search"].contains(&scenario) {
                    assert_eq!(
                        terminal.get_cursor_position().unwrap(),
                        (grid.x + 4, height - 4).into()
                    );
                    assert!(rows[(height - 5) as usize].contains("Message"));
                    let row = &rows[(height - 4) as usize];
                    let offset = row
                        .find(if app.agent {
                            "Describe a task"
                        } else {
                            "Ask a question"
                        })
                        .unwrap();
                    assert_eq!(Line::raw(&row[..offset]).width(), (grid.x + 4) as usize);
                }
                if scenario == "unfocused" {
                    app.picker = None;
                    app.editor.insert("draft");
                    terminal.draw(|frame| draw(frame, &app)).unwrap();
                    assert_eq!(
                        terminal.get_cursor_position().unwrap(),
                        (grid.x + 9, height - 4).into()
                    );
                    assert_eq!(app.editor.text, "draft");
                }
                // Optional evidence exports are explicit synthetic TestBackend fixtures.
                if let Ok(dir) = std::env::var("SENTINEL_RENDER_EVIDENCE") {
                    std::fs::write(
                        format!("{dir}/fixture-{scenario}-{width}x{height}.txt"),
                        text,
                    )
                    .unwrap();
                }
            }
        }
    }
    #[test]
    fn system_only_history_keeps_welcome_and_notices_accessible() {
        let mut app = App::default();
        app.update(
            Update::Reply(
                "messages".into(),
                json!({"messages":[{"role":"system","content":"Authoritative system notice"}]}),
            ),
            &test_worker(),
        );
        assert!(app.output.is_empty());
        assert!(conversation_text(&app).contains("Start a new conversation"));
        app.command("/notices", &test_worker());
        assert!(conversation_text(&app).contains("System notice\n  Authoritative system notice"));
        assert!(!conversation_text(&app).contains("Assistant"));
        app.view = None;
        app.update(Update::Reply("messages".into(), json!({"messages":[{"role":"user","content":"Hello"},{"role":"assistant","content":"Response"}]})), &test_worker());
        assert!(!conversation_text(&app).contains("Start a new conversation"));
        assert!(app.system_notices.is_empty());
    }
    #[test]
    fn trailing_newline_expands_composer() {
        let mut app = App::default();
        app.editor.insert("draft\n");
        let mut terminal = Terminal::new(TestBackend::new(80, 24)).unwrap();
        terminal.draw(|frame| draw(frame, &app)).unwrap();
        let rows = rendered_rows(&terminal);
        assert!(rows[18].contains("Message · Chat"));
        assert_eq!(terminal.get_cursor_position().unwrap(), (6, 20).into());
    }
    #[test]
    fn composer_growth_keeps_cursor_inside_padded_grid() {
        for (width, height) in [(80, 24), (100, 30), (120, 40), (160, 48)] {
            let mut app = App::default();
            app.editor
                .insert(&format!("{}\n{}", "row\n".repeat(40), "界".repeat(150)));
            let mut terminal = Terminal::new(TestBackend::new(width, height)).unwrap();
            terminal.draw(|frame| draw(frame, &app)).unwrap();
            let cursor = terminal.get_cursor_position().unwrap();
            let grid = content_grid(Rect::new(0, 0, width, height));
            assert!(cursor.x >= grid.x + 2 && cursor.x < grid.right() - 2);
            assert!(cursor.y >= height - height / 3 - 1 && cursor.y < height - 3);
            assert!(!app.editor.text.is_empty());
        }
    }
    fn captured_worker() -> (Worker, Receiver<Request>) {
        let (tx, requests) = mpsc::sync_channel(32);
        let (_, rx) = mpsc::sync_channel(8);
        (
            Worker {
                tx,
                rx,
                stop: Arc::new(AtomicBool::new(false)),
            },
            requests,
        )
    }
    fn ready_app() -> App {
        App {
            connected: true,
            sid: "fixture-session".into(),
            state: "idle".into(),
            models: vec![
                json!({"provider_id":"fixture-provider","model_id":"fixture-model","health":"Available"}),
            ],
            sessions: vec![json!({"session_id":"fixture-session","title":"Fixture"})],
            ..App::default()
        }
    }
    fn key(code: KeyCode, modifiers: KeyModifiers) -> crossterm::event::KeyEvent {
        crossterm::event::KeyEvent::new(code, modifiers)
    }
    #[test]
    fn slash_completion_never_executes_partial_or_tab() {
        let (worker, requests) = captured_worker();
        let mut app = ready_app();
        app.editor.insert("/mdl");
        assert!(app.slash_open());
        app.handle_key(key(KeyCode::Tab, KeyModifiers::NONE), &worker);
        assert_eq!(app.editor.text, "/model");
        assert!(app.picker.is_none());
        assert!(requests.try_recv().is_err());
        app.handle_key(key(KeyCode::Enter, KeyModifiers::NONE), &worker);
        assert_eq!(app.picker.as_ref().unwrap().kind, "model");
        assert_eq!(requests.try_recv().unwrap().name, "model.list");
        app.picker = None;
        app.editor.replace("/mod".into(), false);
        app.handle_key(key(KeyCode::Enter, KeyModifiers::NONE), &worker);
        assert_eq!(app.editor.text, "/mode ");
        assert!(requests.try_recv().is_err());
        app.editor.replace("/modle".into(), false);
        app.send(&worker);
        assert!(app.notice.contains("/model"));
        assert_eq!(app.editor.text, "/modle");
        assert!(requests.try_recv().is_err());
        app.editor.replace("/".into(), false);
        app.handle_key(key(KeyCode::Esc, KeyModifiers::NONE), &worker);
        assert!(!app.slash_open());
        assert_eq!(app.editor.text, "/");
    }
    #[test]
    fn pasted_slash_and_escaped_slash_are_literal_chat() {
        for (input, paste, expected) in [
            ("/exit", true, "/exit"),
            ("//model", false, "/model"),
            ("!echo should-not-run", false, "!echo should-not-run"),
        ] {
            let (worker, requests) = captured_worker();
            let mut app = ready_app();
            if paste {
                app.paste(input);
            } else {
                app.editor.insert(input);
            }
            assert!(!app.slash_open());
            assert!(!app.exit_requested);
            app.handle_key(key(KeyCode::Enter, KeyModifiers::NONE), &worker);
            let request = requests.try_recv().unwrap();
            assert_eq!(request.name, "chat.send");
            assert_eq!(request.payload["text"], expected);
            assert!(!app.exit_requested);
            app.update(Update::Failure("start".into(), Error::Timeout), &worker);
            assert_eq!(app.editor.text, input);
            assert_eq!(app.editor.literal, paste);
        }
    }
    #[test]
    fn prior_conversation_survives_send_stream_final_and_rejection() {
        let (worker, requests) = captured_worker();
        let mut app = ready_app();
        let history = format!(
            "{}\n{}",
            message_block("user", "Earlier question"),
            message_block("assistant", "Earlier answer")
        );
        app.output = history.clone();
        app.editor.insert("New question");
        app.send(&worker);
        assert_eq!(requests.try_recv().unwrap().name, "chat.send");
        for expected in [
            "Earlier question",
            "Earlier answer",
            "New question",
            "Assistant · starting",
        ] {
            assert!(conversation_text(&app).contains(expected));
        }
        app.update(
            Update::Reply("start".into(), json!({"run_id":"new-run"})),
            &worker,
        );
        app.output = "New streamed answer".into();
        assert!(conversation_text(&app).contains("Earlier answer"));
        app.state = "completed".into();
        assert!(conversation_text(&app).contains("New streamed answer"));
        assert!(conversation_text(&app).contains("Earlier answer"));
        app.update(Update::Reply("messages".into(), json!({"session_id":app.sid,"messages":[{"role":"user","content":"Earlier question"},{"role":"assistant","content":"Earlier answer"},{"role":"user","content":"New question"},{"role":"assistant","content":"New streamed answer"}]})), &worker);
        assert_eq!(conversation_text(&app).matches("Earlier answer").count(), 1);
        assert_eq!(
            conversation_text(&app)
                .matches("New streamed answer")
                .count(),
            1
        );
        app.editor.insert("Rejected prompt");
        app.send(&worker);
        app.update(
            Update::Failure("start".into(), Error::Remote("model-unavailable".into())),
            &worker,
        );
        assert_eq!(app.editor.text, "Rejected prompt");
        assert!(conversation_text(&app).contains("Earlier answer"));
        assert!(!conversation_text(&app).contains("Rejected prompt"));
        app.editor.replace("Queue failure".into(), false);
        app.send(&test_worker());
        assert_eq!(app.editor.text, "Queue failure");
        assert!(!app.active());
        assert!(conversation_text(&app).contains("New streamed answer"));
    }
    #[test]
    fn provider_failures_are_distinct_from_daemon_disconnect() {
        assert!(run_failure_notice("ConnectionFailed").contains("Provider connection failed"));
        assert!(run_failure_notice("Timeout").contains("Inference timed out"));
        assert!(run_failure_notice("ModelNotFound").contains("model"));
    }
    #[test]
    fn action_invocation_is_not_operation_success() {
        let (worker, _requests) = captured_worker();
        let mut app = ready_app();
        app.update(
            Update::Reply(
                "export".into(),
                json!({"accepted":true,"result":{"value":false}}),
            ),
            &worker,
        );
        assert!(app.notice.contains("refused"));
        app.update(
            Update::Reply(
                "archive".into(),
                json!({"accepted":true,"result":{"value":false}}),
            ),
            &worker,
        );
        assert!(app.picker.is_none());
        app.update(
            Update::Reply(
                "export".into(),
                json!({"accepted":true,"result":{"value":true}}),
            ),
            &worker,
        );
        assert!(app.notice.contains("confirmed transcript saved"));
    }
    #[test]
    fn wide_grid_themes_and_shift_enter_are_presentation_only() {
        let (worker, requests) = captured_worker();
        for (width, height) in [(80, 24), (100, 30), (120, 40), (160, 48), (200, 48)] {
            let grid = content_grid(Rect::new(0, 0, width, height));
            assert_eq!(grid.width, width.saturating_sub(4).min(180));
            for name in ["terminal", "obsidian", "glacier", "porcelain"] {
                let mut app = ready_app();
                app.editor.insert("Draft stays here");
                assert!(app.command(&format!("/theme {name}"), &worker));
                assert_eq!(app.editor.text, "Draft stays here");
                assert!(requests.try_recv().is_err());
                app.handle_key(key(KeyCode::Enter, KeyModifiers::SHIFT), &worker);
                assert_eq!(app.editor.text, "Draft stays here\n");
                let mut terminal = Terminal::new(TestBackend::new(width, height)).unwrap();
                terminal.draw(|f| draw(f, &app)).unwrap();
                assert!(rendered_rows(&terminal).join("\n").contains("Shift+Enter"));
                if let Ok(dir) = std::env::var("SENTINEL_V31_EVIDENCE") {
                    std::fs::write(
                        format!("{dir}/fixture-theme-{name}-{width}x{height}.txt"),
                        rendered_rows(&terminal).join("\n"),
                    )
                    .unwrap();
                }
            }
        }
    }
    #[test]
    fn registry_handlers_emit_canonical_requests_and_disable_unsafe_workflows() {
        let (worker, requests) = captured_worker();
        for c in COMMANDS {
            let mut app = ready_app();
            if c.id == CommandId::Cancel {
                app.run_id = "fixture-run".into();
                app.state = "running".into();
            }
            if c.id == CommandId::Remove {
                app.references.push("fixture.rs".into());
            }
            let text = if c.arguments == commands::Arguments::RequiredText {
                format!("{} Example title", c.name)
            } else {
                c.name.into()
            };
            let accepted = app.command(&text, &worker);
            if c.availability == commands::Availability::Unsupported {
                assert!(!accepted);
                assert!(requests.try_recv().is_err());
                assert!(!app.agent);
                continue;
            }
            assert!(accepted, "{}: {}", c.name, app.notice);
            for request in requests.try_iter() {
                if c.id == CommandId::Reconnect {
                    assert_eq!(request.name, "reconnect");
                    continue;
                }
                let _: sentinel_ipc::contract::RequestPayload =
                    serde_json::from_value(json!({"name":request.name,"payload":request.payload}))
                        .unwrap();
            }
        }
    }
    #[test]
    fn keyboard_commands_share_handlers_without_readline_conflicts() {
        let (worker, requests) = captured_worker();
        let mut app = ready_app();
        app.editor.insert("keep this draft");
        app.editor.home();
        app.handle_key(key(KeyCode::Char('k'), KeyModifiers::CONTROL), &worker);
        assert!(app.editor.text.is_empty());
        assert!(app.picker.is_none());
        app.handle_key(key(KeyCode::Char('p'), KeyModifiers::CONTROL), &worker);
        assert_eq!(app.picker.as_ref().unwrap().kind, "command");
        app.handle_key(key(KeyCode::Esc, KeyModifiers::NONE), &worker);
        app.handle_key(key(KeyCode::Char('l'), KeyModifiers::CONTROL), &worker);
        assert_eq!(app.picker.as_ref().unwrap().kind, "model");
        assert_eq!(requests.try_recv().unwrap().name, "model.list");
        app.handle_key(key(KeyCode::Esc, KeyModifiers::NONE), &worker);
        app.handle_key(key(KeyCode::Char('g'), KeyModifiers::CONTROL), &worker);
        assert!(app.agent);
        assert!(requests.try_recv().is_err());
        app.handle_key(key(KeyCode::Char('o'), KeyModifiers::CONTROL), &worker);
        assert_eq!(app.editor.text, "\n");
    }
    #[test]
    fn draft_switching_is_bounded_and_active_runs_block_switches() {
        let (worker, requests) = captured_worker();
        let mut app = ready_app();
        app.editor.paste("/literal draft");
        app.editor.left();
        let cursor = app.editor.cursor;
        app.references.push("/workspace/file".into());
        app.agent = true;
        app.snapshot(json!({"session_id":"second","state":"idle"}));
        assert!(app.editor.text.is_empty());
        assert!(app.references.is_empty());
        app.editor.insert("second draft");
        app.snapshot(json!({"session_id":"fixture-session","state":"idle"}));
        assert_eq!(app.editor.text, "/literal draft");
        assert_eq!(app.editor.cursor, cursor);
        assert!(app.editor.literal);
        assert!(app.agent);
        assert_eq!(app.references, ["/workspace/file"]);
        app.state = "running".into();
        app.run_id = "run".into();
        assert!(!app.command("/resume last", &worker));
        assert!(!app.command("/new", &worker));
        assert!(requests.try_recv().is_err());
        app.state = "idle".into();
        app.drafts = (0..32)
            .map(|i| {
                (
                    format!("old-{i}"),
                    SessionDraft {
                        text: "draft".into(),
                        cursor: 5,
                        references: vec![],
                        agent: false,
                        literal: false,
                    },
                )
            })
            .collect();
        assert!(!app.command("/new", &worker));
        assert_eq!(app.drafts.len(), 32);
        assert!(requests.try_recv().is_err());
    }
    #[test]
    fn metadata_views_never_replace_streamed_text_and_details_are_bounded() {
        let (worker, requests) = captured_worker();
        let mut app = ready_app();
        app.state = "running".into();
        app.run_id = "r".into();
        app.output = "Real response".into();
        app.activity = (0..5)
            .map(|i| format!("tool {i}\n  resource metadata {i}"))
            .collect();
        assert!(!conversation_text(&app).contains("resource metadata"));
        app.command("/details", &worker);
        assert!(conversation_text(&app).contains("resource metadata 0"));
        assert_eq!(app.output, "Real response");
        app.command("/activity", &worker);
        assert!(app.view.is_some());
        assert_eq!(app.output, "Real response");
        app.update(
            Update::Event(Envelope {
                version: sentinel_ipc::Version { major: 1, minor: 1 },
                kind: sentinel_ipc::MessageType::Event,
                id: String::new(),
                name: "output.delta".into(),
                payload: json!({"session_id":app.sid,"run_id":"r","text":" continued"}),
            }),
            &worker,
        );
        assert_eq!(app.output, "Real response continued");
        app.handle_key(key(KeyCode::Esc, KeyModifiers::NONE), &worker);
        assert!(app.view.is_none());
        assert!(conversation_text(&app).contains("Real response continued"));
        assert!(requests.try_recv().is_err());
    }
    #[test]
    fn references_use_authorized_discovery_and_quoted_paths_only() {
        let (worker, requests) = captured_worker();
        let mut app = ready_app();
        app.agent = true;
        app.editor.insert("Inspect this");
        app.handle_key(key(KeyCode::Char('@'), KeyModifiers::NONE), &worker);
        let request = requests.try_recv().unwrap();
        assert_eq!(request.name, "workspace.files");
        assert_eq!(request.payload["session_id"], app.sid);
        app.update(Update::Reply("files".into(),json!({"root":"/workspace","files":["/workspace/nested/evil\nname $(command).rs"],"truncated":true})),&worker);
        assert!(app.notice.contains("truncated"));
        app.select_picker(&worker);
        assert_eq!(app.references.len(), 1);
        assert_eq!(app.editor.text, "Inspect this");
        app.send(&worker);
        let request = requests.try_recv().unwrap();
        assert_eq!(request.name, "agent.start");
        let text = request.payload["text"].as_str().unwrap();
        assert!(text.contains("@\"/workspace/nested/evil\\nname $(command).rs\""));
        app.update(Update::Failure("start".into(), Error::Timeout), &worker);
        assert_eq!(app.references.len(), 1);
        app.connected = true;
        app.state = "idle".into();
        app.command("/remove 1", &worker);
        assert!(app.references.is_empty());
        app.command("/files", &worker);
        requests.try_recv().unwrap();
        app.update(
            Update::Failure("files".into(), Error::Remote("permission-denied".into())),
            &worker,
        );
        assert!(app.picker.is_none());
        assert!(app.references.is_empty());
        assert_eq!(app.editor.text, "Inspect this");
    }
    #[test]
    fn generated_error_wrapper_is_readable_and_code_remains_literal() {
        let text = "Assistant\n  **Ajan Görevi Başarısız / Agent Task Failed**\n  *0 adım çalıştırıldı / 0 steps executed.*";
        let rendered = transcript_lines(text)
            .iter()
            .map(ToString::to_string)
            .collect::<Vec<_>>()
            .join("\n");
        assert!(!rendered.contains("**"));
        assert!(!rendered.contains("executed.*"));
        let code = transcript_lines("```\n**Ajan Görevi Başarısız / Agent Task Failed**\n```");
        assert!(code[1].to_string().contains("**"));
        assert_eq!(
            fit_label("/notices · A long recoverable provider error", 24),
            "/notices · A long rec..."
        );
    }
    #[test]
    fn terminal_cancellation_replaces_waiting_notice() {
        let (worker, _requests) = captured_worker();
        let mut app = ready_app();
        app.state = "running".into();
        app.run_id = "r".into();
        app.notice = "Cancellation requested; waiting for terminal state.".into();
        app.update(
            Update::Event(Envelope {
                version: sentinel_ipc::Version { major: 1, minor: 1 },
                kind: sentinel_ipc::MessageType::Event,
                id: String::new(),
                name: "run.cancelled".into(),
                payload: json!({"session_id":app.sid,"run_id":"r","text":""}),
            }),
            &worker,
        );
        assert_eq!(app.state, "cancelled");
        assert_eq!(app.notice, "Run cancelled.");
        assert!(app.approval.is_none());
    }
    #[test]
    fn approval_cancellation_and_exit_never_fake_completion() {
        let (worker, requests) = captured_worker();
        let mut app = ready_app();
        app.state = "approval".into();
        app.run_id = "r".into();
        app.approval = Some(json!({"approval_id":"a","tool":"write-file"}));
        app.paste("/exit");
        assert!(app.editor.text.is_empty());
        app.handle_key(key(KeyCode::Esc, KeyModifiers::NONE), &worker);
        assert!(app.approval.is_some());
        app.handle_key(key(KeyCode::Char('y'), KeyModifiers::NONE), &worker);
        let request = requests.try_recv().unwrap();
        assert_eq!(request.name, "approval.respond");
        assert_eq!(request.payload["allow"], true);
        assert_eq!(app.state, "approval");
        assert!(app.approval_pending);
        app.handle_key(key(KeyCode::Char('c'), KeyModifiers::CONTROL), &worker);
        assert_eq!(requests.try_recv().unwrap().name, "run.cancel");
        assert_eq!(app.state, "approval");
        assert!(!app.exit_requested);
        app.state = "starting".into();
        app.approval = None;
        app.command("/cancel", &worker);
        assert!(app.cancel_after_start);
        assert!(requests.try_recv().is_err());
        app.state = "idle".into();
        app.busy = false;
        app.references.push("file".into());
        app.command("/exit", &worker);
        assert!(!app.exit_requested);
        app.references.clear();
        app.command("/q", &worker);
        assert!(app.exit_requested);
    }
    #[test]
    fn disconnected_queue_failure_and_stale_history_preserve_input() {
        let mut app = App::default();
        app.editor.insert("/new");
        app.send(&test_worker());
        assert_eq!(app.editor.text, "/new");
        app.connected = true;
        app.send(&test_worker());
        assert_eq!(app.editor.text, "/new");
        assert!(!app.busy);
        let (recovery_worker, _recovery_requests) = captured_worker();
        app.update(
            Update::Failure("connection".into(), Error::Disconnected),
            &recovery_worker,
        );
        app.update(Update::Connection(true), &recovery_worker);
        assert!(app.connected);
        assert_eq!(app.notice, "Connection restored; draft preserved.");
        app.sid = "current".into();
        app.output = "keep".into();
        app.update(
            Update::Reply("messages".into(), json!({"session_id":"old","messages":[]})),
            &test_worker(),
        );
        assert_eq!(app.output, "keep");
    }
    #[test]
    fn command_popup_and_help_render_at_supported_sizes() {
        for (w, h) in [(80, 24), (100, 30), (120, 40), (160, 48), (24, 8)] {
            let mut app = ready_app();
            let mut terminal = Terminal::new(TestBackend::new(w, h)).unwrap();
            app.editor.insert("/");
            terminal.draw(|f| draw(f, &app)).unwrap();
            if h >= 24 {
                assert!(rendered_rows(&terminal).join("\n").contains("Tab complete"));
            }
            app.command("/help model", &test_worker());
            terminal.draw(|f| draw(f, &app)).unwrap();
            if h >= 24 {
                assert!(
                    rendered_rows(&terminal)
                        .join("\n")
                        .contains("Filter: model")
                );
            }
            app.picker = None;
            app.editor.replace("/plan".into(), false);
            terminal.draw(|f| draw(f, &app)).unwrap();
            if let Ok(dir) = std::env::var("SENTINEL_V3_EVIDENCE") {
                std::fs::write(
                    format!("{dir}/fixture-plan-disabled-{w}x{h}.txt"),
                    rendered_rows(&terminal).join("\n"),
                )
                .unwrap();
            }
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
    fn active_reattach_restores_task_without_replacing_stream() {
        let mut app = App::default();
        app.snapshot(json!({"session_id":"s", "run_id":"r", "state":"running", "run_type":"agent", "output":"newer streamed output"}));
        let worker = test_worker();
        let history = json!({"session_id":"s", "messages":[
            {"role":"system","content":"notice"},
            {"role":"user","content":"old question"},
            {"role":"assistant","content":"old answer"},
            {"role":"user","content":"current task"},
            {"role":"assistant","content":"older partial output"}]});
        app.update(Update::Reply("messages".into(), history.clone()), &worker);
        let visible = conversation_text(&app);
        for text in [
            "old question",
            "old answer",
            "current task",
            "newer streamed output",
        ] {
            assert_eq!(visible.matches(text).count(), 1);
        }
        assert!(!visible.contains("older partial output"));
        assert!(!visible.contains("notice"));
        assert!(app.system_notices.contains("notice"));
        app.update(Update::Reply("messages".into(), history), &worker);
        assert_eq!(conversation_text(&app), visible);
        assert_eq!(app.output, "newer streamed output");
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
