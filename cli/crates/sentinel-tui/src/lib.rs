use crossterm::event::{self, Event, KeyCode, KeyEventKind, KeyModifiers};
use ratatui::{
    layout::{Constraint, Layout},
    widgets::{Block, Clear, Paragraph},
};
use sentinel_ipc::{Client, Envelope, Error};
use serde_json::{Value, json};
use std::{
    path::Path,
    sync::mpsc::{self, Receiver},
    time::Duration,
};
fn stream(mut client: Client) -> Receiver<Result<Envelope, Error>> {
    let (tx, rx) = mpsc::sync_channel(64);
    std::thread::spawn(move || {
        loop {
            let event = client.next_event();
            let failed = event.as_ref().map_or(true, |e| {
                ["run.completed", "run.failed", "run.cancelled"].contains(&e.name.as_str())
            });
            if tx.send(event).is_err() || failed {
                break;
            }
        }
    });
    rx
}
pub fn run(path: &Path, attach: Option<&str>) -> Result<(), Error> {
    let mut control = Client::connect(path, "sentinel-tui-control")?;
    let mut sessions = control.request("session.list", json!({}))?["sessions"]
        .as_array()
        .cloned()
        .unwrap_or_default();
    if sessions.is_empty() {
        sessions.push(control.request("session.create", json!({"title":"Terminal session"}))?);
    }
    let mut selected = attach
        .and_then(|id| sessions.iter().position(|s| s["session_id"] == id))
        .unwrap_or(0);
    if attach.is_some() && attach != sessions[selected]["session_id"].as_str() {
        return Err(Error::Remote("unknown-session".into()));
    }
    let mut events = None;
    let mut output = String::new();
    let mut input = String::new();
    let mut state = "idle".to_owned();
    let mut run_id = String::new();
    let mut approval: Option<Value> = None;
    let mut observer = Client::connect(path, "sentinel-tui")?;
    let snapshot = observer.request(
        "session.attach",
        json!({"session_id":sessions[selected]["session_id"]}),
    )?;
    output.push_str(snapshot["output"].as_str().unwrap_or(""));
    if ["running", "approval"].contains(&snapshot["state"].as_str().unwrap_or("")) {
        run_id = snapshot["run_id"].as_str().unwrap_or("").into();
        state = snapshot["state"].as_str().unwrap_or("idle").into();
        if state == "approval" {
            approval = Some(snapshot["approval"].clone());
        }
        events = Some(stream(observer));
    }
    let mut terminal = ratatui::init();
    let result = (|| -> Result<(), Error> {
        loop {
            if let Some(rx) = &events {
                while let Ok(message) = rx.try_recv() {
                    match message {
                        Ok(e) => {
                            if e.name == "output.delta" {
                                output.push_str(e.payload["text"].as_str().unwrap_or(""));
                            }
                            if e.name == "approval.requested" {
                                approval = Some(e.payload.clone());
                                state = "approval".into();
                            }
                            if ["run.completed", "run.failed", "run.cancelled"]
                                .contains(&e.name.as_str())
                            {
                                state = e.name.trim_start_matches("run.").into();
                                output = e.payload["text"].as_str().unwrap_or("").into();
                                approval = None;
                            }
                        }
                        Err(e) => {
                            state = format!("disconnected: {e}");
                        }
                    }
                }
            }
            terminal
                .draw(|frame| {
                    let areas = Layout::vertical([
                        Constraint::Length(4),
                        Constraint::Min(5),
                        Constraint::Length(3),
                        Constraint::Length(2),
                    ])
                    .split(frame.area());
                    let titles = sessions
                        .iter()
                        .enumerate()
                        .skip(selected.saturating_sub(1))
                        .take(2)
                        .map(|(i, s)| {
                            format!(
                                "{} {}",
                                if i == selected { ">" } else { " " },
                                s["title"].as_str().unwrap_or("session")
                            )
                        })
                        .collect::<Vec<_>>()
                        .join("\n");
                    frame.render_widget(
                        Paragraph::new(titles).block(Block::bordered().title("Sessions — Up/Down")),
                        areas[0],
                    );
                    frame.render_widget(
                        Paragraph::new(output.as_str()).block(Block::bordered().title("Output")),
                        areas[1],
                    );
                    frame.render_widget(
                        Paragraph::new(input.as_str())
                            .block(Block::bordered().title("Enter chat; /agent task")),
                        areas[2],
                    );
                    frame.render_widget(
                        Paragraph::new(format!(
                            "Sentinel {} | {} | {}/{} | Ctrl+C cancel/exit",
                            sentinel_ipc::APP_VERSION,
                            state,
                            sessions[selected]["provider_id"].as_str().unwrap_or(""),
                            sessions[selected]["model_id"].as_str().unwrap_or("")
                        )),
                        areas[3],
                    );
                    if let Some(p) = &approval {
                        let modal = Layout::vertical([
                            Constraint::Percentage(30),
                            Constraint::Length(8),
                            Constraint::Min(0),
                        ])
                        .split(frame.area())[1];
                        frame.render_widget(Clear, modal);
                        frame.render_widget(
                            Paragraph::new(format!(
                                "{}\n{}\ny Allow / n Deny / Esc Deny",
                                p["tool"], p["resources"]
                            ))
                            .block(Block::bordered().title("Approval")),
                            modal,
                        );
                    }
                })
                .map_err(|e| Error::Protocol(e.to_string()))?;
            if !event::poll(Duration::from_millis(50))
                .map_err(|e| Error::Protocol(e.to_string()))?
            {
                continue;
            }
            if let Event::Key(key) = event::read().map_err(|e| Error::Protocol(e.to_string()))? {
                if key.kind != KeyEventKind::Press {
                    continue;
                }
                if key.code == KeyCode::Char('c') && key.modifiers.contains(KeyModifiers::CONTROL) {
                    if state == "running" || state == "approval" {
                        control.request("run.cancel", json!({"run_id":run_id}))?;
                    } else {
                        break;
                    }
                } else if let Some(p) = &approval {
                    let allow = key.code == KeyCode::Char('y');
                    if allow || key.code == KeyCode::Char('n') || key.code == KeyCode::Esc {
                        control.request(
                            "approval.respond",
                            json!({"run_id":run_id,"approval_id":p["approval_id"],"allow":allow}),
                        )?;
                        approval = None;
                        state = "running".into();
                    }
                } else {
                    match key.code {
                        KeyCode::Enter if !input.trim().is_empty() && state != "running" => {
                            let mut client = Client::connect(path, "sentinel-tui-stream")?;
                            let agent = input.starts_with("/agent ");
                            let text = if agent { &input[7..] } else { &input };
                            let start = client.request(
                                if agent { "agent.start" } else { "chat.send" },
                                json!({"session_id":sessions[selected]["session_id"],"text":text}),
                            )?;
                            run_id = start["run_id"].as_str().unwrap_or("").into();
                            output.clear();
                            input.clear();
                            state = "running".into();
                            events = Some(stream(client));
                        }
                        KeyCode::Up | KeyCode::Down if state != "running" => {
                            if key.code == KeyCode::Up {
                                selected = selected.saturating_sub(1);
                            } else {
                                selected = (selected + 1).min(sessions.len() - 1);
                            }
                            let snapshot = control.request(
                                "session.attach",
                                json!({"session_id": sessions[selected]["session_id"]}),
                            )?;
                            output = snapshot["output"].as_str().unwrap_or("").into();
                        }
                        KeyCode::Char(c) if input.len() + c.len_utf8() <= 65536 => input.push(c),
                        KeyCode::Backspace => {
                            input.pop();
                        }
                        _ => {}
                    }
                }
            }
        }
        Ok(())
    })();
    ratatui::restore();
    result
}
