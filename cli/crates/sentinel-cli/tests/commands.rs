#![cfg(unix)]
use serde_json::{Value, json};
use std::{
    io::{BufRead, BufReader, Write},
    os::unix::net::UnixListener,
    process::{Command, Stdio},
    sync::atomic::{AtomicU64, Ordering},
};
static NEXT: AtomicU64 = AtomicU64::new(0);
fn cli(args: &[&str], stdin: Option<&str>, terminal: &str) -> std::process::Output {
    let root = std::env::temp_dir().join(format!(
        "sentinel-cli-test-{}-{}",
        std::process::id(),
        NEXT.fetch_add(1, Ordering::Relaxed)
    ));
    std::fs::create_dir_all(&root).unwrap();
    let path = root.join("daemon.sock");
    let listener = UnixListener::bind(&path).unwrap();
    let terminal = terminal.to_owned();
    let server = std::thread::spawn(move || {
        let (mut stream, _) = listener.accept().unwrap();
        let mut reader = BufReader::new(stream.try_clone().unwrap());
        for _ in 0..3 {
            let mut line = String::new();
            reader.read_line(&mut line).unwrap();
            let request: Value = serde_json::from_str(&line).unwrap();
            let payload = match request["name"].as_str().unwrap() {
                "hello" => json!({"major":1,"minor":0,"daemon_version":"test","capabilities":[]}),
                "session.create" => {
                    json!({"session_id":"session","type":"conversation","created_at":"test"})
                }
                "chat.send" | "agent.start" => {
                    assert!(
                        request["payload"]["text"]
                            .as_str()
                            .is_some_and(|s| !s.is_empty())
                    );
                    json!({"run_id":"run","session_id":"session"})
                }
                _ => panic!("unexpected command"),
            };
            writeln!(stream,"{}",json!({"version":{"major":1,"minor":0},"type":"response","id":request["id"],"name":request["name"],"payload":payload})).unwrap();
        }
        writeln!(stream,"{}",json!({"version":{"major":1,"minor":0},"type":"event","id":"","name":"output.delta","payload":{"text":"MAVI","run_id":"run","session_id":"session"}})).unwrap();
        writeln!(stream,"{}",json!({"version":{"major":1,"minor":0},"type":"event","id":"","name":terminal,"payload":{"text":"MAVI","run_id":"run","session_id":"session","state":terminal.trim_start_matches("run.")}})).unwrap();
    });
    let mut child = Command::new(env!("CARGO_BIN_EXE_sentinel"))
        .arg("--socket")
        .arg(&path)
        .args(args)
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .unwrap();
    if let Some(text) = stdin {
        child
            .stdin
            .take()
            .unwrap()
            .write_all(text.as_bytes())
            .unwrap();
    }
    let result = child.wait_with_output().unwrap();
    server.join().unwrap();
    std::fs::remove_dir_all(root).unwrap();
    result
}
#[test]
fn text_final_not_duplicated() {
    let r = cli(&["chat", "hello"], None, "run.completed");
    assert!(r.status.success());
    assert_eq!(r.stdout, b"MAVI\n");
}
#[test]
fn json_final() {
    let r = cli(
        &["--output", "json", "agent", "task"],
        None,
        "run.completed",
    );
    assert!(r.status.success());
    let v: Value = serde_json::from_slice(&r.stdout).unwrap();
    assert_eq!(v["text"], "MAVI");
    assert_eq!(v["state"], "completed");
}
#[test]
fn stream_json() {
    let r = cli(
        &["--output", "stream-json", "chat", "hello"],
        None,
        "run.completed",
    );
    assert!(r.status.success());
    let rows: Vec<Value> = String::from_utf8(r.stdout)
        .unwrap()
        .lines()
        .map(|s| serde_json::from_str(s).unwrap())
        .collect();
    assert_eq!(rows.len(), 2);
    assert_eq!(rows[0]["name"], "output.delta");
    assert_eq!(rows[1]["name"], "run.completed");
}
#[test]
fn stdin_task() {
    let r = cli(&["run"], Some("pipeline task"), "run.completed");
    assert!(r.status.success());
}
#[test]
fn cancelled_exit_is_distinct() {
    let r = cli(&["run", "task"], None, "run.cancelled");
    assert_eq!(
        r.status.code(),
        Some(130),
        "{}",
        String::from_utf8_lossy(&r.stderr)
    );
}
#[test]
fn unavailable_exit_is_distinct() {
    let r = Command::new(env!("CARGO_BIN_EXE_sentinel"))
        .args(["--socket", "/nonexistent/sentinel.sock", "status"])
        .output()
        .unwrap();
    assert_eq!(r.status.code(), Some(3));
    assert!(r.stdout.is_empty());
}

#[test]
fn failed_exit_is_nonzero() {
    let r = cli(&["run", "task"], None, "run.failed");
    assert_eq!(
        r.status.code(),
        Some(1),
        "{}",
        String::from_utf8_lossy(&r.stderr)
    );
}

#[test]
fn incompatible_exit_is_distinct() {
    let root = std::env::temp_dir().join(format!(
        "sentinel-major-test-{}-{}",
        std::process::id(),
        NEXT.fetch_add(1, Ordering::Relaxed)
    ));
    std::fs::create_dir_all(&root).unwrap();
    let path = root.join("daemon.sock");
    let listener = UnixListener::bind(&path).unwrap();
    let server = std::thread::spawn(move || {
        let (mut stream, _) = listener.accept().unwrap();
        let mut reader = BufReader::new(stream.try_clone().unwrap());
        let mut line = String::new();
        reader.read_line(&mut line).unwrap();
        let request: Value = serde_json::from_str(&line).unwrap();
        writeln!(stream,"{}",json!({"version":{"major":2,"minor":0},"type":"response","id":request["id"],"name":"hello","payload":{}})).unwrap();
    });
    let result = Command::new(env!("CARGO_BIN_EXE_sentinel"))
        .arg("--socket")
        .arg(&path)
        .arg("status")
        .output()
        .unwrap();
    server.join().unwrap();
    std::fs::remove_dir_all(root).unwrap();
    assert_eq!(result.status.code(), Some(4));
    assert!(result.stdout.is_empty());
}

#[test]
fn help_is_offline_and_successful() {
    for command in [
        "",
        "status",
        "models",
        "sessions",
        "chat",
        "agent",
        "run",
        "attach",
        "tui",
        "doctor",
        "model",
        "completions",
        "shutdown",
    ] {
        let mut invocation = Command::new(env!("CARGO_BIN_EXE_sentinel"));
        if !command.is_empty() {
            invocation.arg(command);
        }
        let result = invocation.arg("--help").output().unwrap();
        assert!(result.status.success(), "{command}: {:?}", result.stderr);
        let text = String::from_utf8(result.stdout).unwrap();
        assert!(text.contains("Usage:"));
        assert!(!text.contains("Protocol("));
    }
}
#[test]
fn usage_validation_precedes_daemon_connection() {
    for args in [
        vec!["invented"],
        vec!["--output", "wrong", "status"],
        vec!["attach"],
        vec!["model", "select", "ollama"],
    ] {
        let result = Command::new(env!("CARGO_BIN_EXE_sentinel"))
            .args(args)
            .output()
            .unwrap();
        assert_eq!(result.status.code(), Some(2));
    }
}
#[test]
fn missing_task_is_usage_error() {
    let result = Command::new(env!("CARGO_BIN_EXE_sentinel"))
        .args(["run", "--output", "json"])
        .stdin(Stdio::null())
        .output()
        .unwrap();
    assert_eq!(result.status.code(), Some(2));
    let json: Value = serde_json::from_slice(&result.stdout).unwrap();
    assert_eq!(json["error"]["code"], "cli-usage");
}
#[test]
fn machine_error_is_typed() {
    let result = Command::new(env!("CARGO_BIN_EXE_sentinel"))
        .args([
            "status",
            "--socket",
            "/nonexistent/sentinel.sock",
            "--output",
            "json",
        ])
        .output()
        .unwrap();
    assert_eq!(result.status.code(), Some(3));
    let json: Value = serde_json::from_slice(&result.stdout).unwrap();
    assert_eq!(json["error"]["code"], "daemon-unavailable");
    assert!(!result.stderr.is_empty());
}
#[test]
fn completions_are_offline() {
    for shell in ["bash", "zsh", "fish"] {
        let result = Command::new(env!("CARGO_BIN_EXE_sentinel"))
            .args(["completions", shell])
            .output()
            .unwrap();
        assert!(result.status.success());
        assert!(!result.stdout.is_empty());
    }
}

#[test]
fn global_output_after_task_is_parsed() {
    let r = cli(
        &["chat", "hello", "--output", "json"],
        None,
        "run.completed",
    );
    assert!(r.status.success());
    let value: Value = serde_json::from_slice(&r.stdout).unwrap();
    assert_eq!(value["text"], "MAVI");
}
