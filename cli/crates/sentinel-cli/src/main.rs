use sentinel_ipc::{Client, Error, default_socket_path};
use serde_json::json;
use std::sync::atomic::{AtomicBool, Ordering};
use std::{
    io::{self, IsTerminal, Read, Write},
    path::PathBuf,
    sync::{Arc, Mutex},
};
fn main() {
    if let Err(error) = run() {
        eprintln!("sentinel: {error}");
        std::process::exit(error.exit_code());
    }
}
fn run() -> Result<(), Error> {
    let mut args: Vec<String> = std::env::args().skip(1).collect();
    if args.iter().any(|arg| arg == "--version") {
        println!(
            "Sentinel {} (IPC {}.{})",
            sentinel_ipc::APP_VERSION,
            sentinel_ipc::contract::MAJOR,
            sentinel_ipc::contract::MINOR
        );
        return Ok(());
    }
    let mut output = "text".to_owned();
    let mut socket = default_socket_path();
    for flag in ["--output", "--socket"] {
        if let Some(index) = args.iter().position(|x| x == flag) {
            if index + 1 >= args.len() {
                return Err(Error::Protocol(format!("{flag} requires a value")));
            }
            let value = args.remove(index + 1);
            args.remove(index);
            if flag == "--output" {
                output = value;
            } else {
                socket = PathBuf::from(value);
            }
        }
    }
    if !["text", "json", "stream-json"].contains(&output.as_str()) {
        return Err(Error::Protocol("invalid output format".into()));
    }
    let command = args.first().map(String::as_str).unwrap_or("status");
    if command == "attach" || command == "tui" {
        return sentinel_tui::run(&socket, args.get(1).map(String::as_str));
    }
    let mut client = Client::connect(&socket, "sentinel-cli")?;
    let request = match command {
        "status" => Some("daemon.status"),
        "models" => Some("model.list"),
        "sessions" => Some("session.list"),
        "shutdown" => Some("daemon.shutdown"),
        _ => None,
    };
    if let Some(name) = request {
        println!("{}", client.request(name, json!({}))?);
        return Ok(());
    }
    if !["chat", "agent", "run"].contains(&command) {
        return Err(Error::Protocol(
            "usage: sentinel status|models|sessions|chat|agent|run|attach|tui".into(),
        ));
    }
    let mut text = args.iter().skip(1).cloned().collect::<Vec<_>>().join(" ");
    // Explicit arguments take precedence over stdin.
    if text.is_empty() {
        io::stdin()
            .take(65537)
            .read_to_string(&mut text)
            .map_err(|e| Error::Protocol(e.to_string()))?;
    }
    if text.trim().is_empty() || text.len() > 65536 {
        return Err(Error::Protocol("task must contain 1..65536 bytes".into()));
    }
    let session = client.request(
        "session.create",
        json!({"title":text.chars().take(80).collect::<String>()}),
    )?;
    let sid = session["session_id"]
        .as_str()
        .ok_or_else(|| Error::Protocol("missing session id".into()))?;
    let active: Arc<Mutex<Option<String>>> = Arc::new(Mutex::new(None));
    let interrupted = Arc::new(AtomicBool::new(false));
    let cancel_signal = interrupted.clone();
    let cancel = active.clone();
    let control_socket = socket.clone();
    ctrlc::set_handler(move || {
        cancel_signal.store(true, Ordering::SeqCst);
        if let Some(id) = cancel.lock().unwrap().clone()
            && let Ok(mut control) = Client::connect(&control_socket, "sentinel-cli-cancel")
        {
            let _ = control.request("run.cancel", json!({"run_id":id}));
        }
    })
    .map_err(|e| Error::Protocol(e.to_string()))?;
    let start = client.request(
        if command == "chat" {
            "chat.send"
        } else {
            "agent.start"
        },
        json!({"session_id":sid,"text":text}),
    )?;
    *active.lock().unwrap() = start["run_id"].as_str().map(str::to_owned);
    if interrupted.load(Ordering::SeqCst) {
        client.request("run.cancel", json!({"run_id":start["run_id"]}))?;
    }
    loop {
        let event = client.next_event()?;
        if output == "stream-json" {
            println!("{}", serde_json::to_string(&event).unwrap());
        }
        if event.name == "approval.requested" {
            eprintln!(
                "Approval required: {} — {}",
                event.payload["tool"], event.payload["resources"]
            );
            let mut response = String::new();
            if io::stdin().is_terminal() {
                eprint!("Allow? [y/N] ");
                let _ = io::stderr().flush();
                let (sender, receiver) = std::sync::mpsc::sync_channel(1);
                std::thread::spawn(move || {
                    let mut line = String::new();
                    let result = io::stdin().read_line(&mut line).map(|_| line);
                    let _ = sender.send(result);
                });
                loop {
                    if interrupted.load(Ordering::SeqCst) {
                        break;
                    }
                    match receiver.recv_timeout(std::time::Duration::from_millis(100)) {
                        Ok(line) => {
                            response = line.map_err(|e| Error::Protocol(e.to_string()))?;
                            break;
                        }
                        Err(std::sync::mpsc::RecvTimeoutError::Timeout) => {}
                        Err(_) => return Err(Error::Protocol("approval input unavailable".into())),
                    }
                }
            }
            if interrupted.load(Ordering::SeqCst) {
                continue;
            }
            let allow = response.trim().eq_ignore_ascii_case("y");
            client.request("approval.respond",json!({"run_id":event.payload["run_id"],"approval_id":event.payload["approval_id"],"allow":allow}))?;
        }
        if ["run.completed", "run.failed", "run.cancelled"].contains(&event.name.as_str()) {
            *active.lock().unwrap() = None;
            if output == "json" {
                println!("{}", event.payload);
            } else if output == "text" {
                println!("{}", event.payload["text"].as_str().unwrap_or(""));
            }
            if event.name == "run.cancelled" {
                std::process::exit(2);
            }
            if event.name == "run.failed" {
                return Err(Error::Remote(event.payload["detail"].to_string()));
            }
            return Ok(());
        }
    }
}
