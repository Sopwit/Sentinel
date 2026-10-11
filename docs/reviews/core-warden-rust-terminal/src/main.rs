//! Isolated diagnostic. Does not edit or launch the production TUI.
use crossterm::{
    execute,
    terminal::{EnterAlternateScreen, LeaveAlternateScreen, disable_raw_mode, enable_raw_mode},
};
use ratatui::{
    Terminal,
    backend::{Backend, CrosstermBackend},
    layout::Rect,
    widgets::Paragraph,
};
use sentinel_mascot::{Assets, Capabilities, DisplayMode, Player, State};
use serde_json::{Value, json};
use std::{
    fs::{File, OpenOptions},
    io::{self, IsTerminal, Read, Write},
    path::PathBuf,
    sync::{Arc, Mutex},
    time::{Duration, Instant},
};

type Result<T> = std::result::Result<T, Box<dyn std::error::Error>>;
struct Log {
    file: File,
    records: usize,
    bytes: usize,
}
#[derive(Clone)]
struct Trace {
    file: Arc<Mutex<Log>>,
    start: Instant,
}
impl Trace {
    fn event(&self, name: &str, detail: Value) {
        let mut log = self.file.lock().unwrap();
        let line = format!(
            "{}\n",
            json!({"us":self.start.elapsed().as_micros(), "operation":name, "detail":detail})
        );
        if log.records >= 1024 || log.bytes + line.len() > 512 * 1024 {
            return;
        }
        if log.file.write_all(line.as_bytes()).is_ok() {
            log.records += 1;
            log.bytes += line.len();
        }
    }
}
struct Out {
    trace: Trace,
    bytes: u64,
    flushes: u64,
}
impl Write for Out {
    fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
        let n = io::stdout().write(bytes)?;
        self.bytes += n as u64;
        self.trace.event(
            "stdout.write.completed",
            json!({"bytes":n,"cumulative_bytes":self.bytes}),
        );
        Ok(n)
    }
    fn flush(&mut self) -> io::Result<()> {
        io::stdout().flush()?;
        self.flushes += 1;
        self.trace.event(
            "stdout.flush.completed",
            json!({"flushes":self.flushes,"bytes":self.bytes}),
        );
        Ok(())
    }
}
struct Restore {
    raw: bool,
    alt: bool,
    id: u32,
    trace: Trace,
}
impl Drop for Restore {
    fn drop(&mut self) {
        // Only the explicitly owned control ID. Player owns and cleans its own IDs.
        let mut out = Out {
            trace: self.trace.clone(),
            bytes: 0,
            flushes: 0,
        };
        if self.id != 0 {
            let _ = write!(out, "\x1b_Ga=d,d=I,i={},q=2\x1b\\", self.id);
            let _ = out.flush();
            self.trace
                .event("owned_image.deleted", json!({"id":self.id}));
        }
        if self.alt {
            let _ = execute!(out, LeaveAlternateScreen);
            self.trace.event("alternate.left", Value::Null);
        }
        if self.raw {
            let result = disable_raw_mode();
            self.trace
                .event("raw.disabled", json!({"success":result.is_ok()}));
        }
    }
}
fn base64(bytes: &[u8]) -> String {
    const CHARS: &[u8] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    let mut s = String::with_capacity(bytes.len().div_ceil(3) * 4);
    for b in bytes.chunks(3) {
        let v = (u32::from(b[0]) << 16)
            | (u32::from(*b.get(1).unwrap_or(&0)) << 8)
            | u32::from(*b.get(2).unwrap_or(&0));
        for shift in [18, 12, 6, 0] {
            s.push(CHARS[((v >> shift) & 63) as usize] as char);
        }
        if b.len() < 3 {
            s.pop();
            s.push('=');
        }
        if b.len() < 2 {
            let n = s.len();
            s.replace_range(n - 2..n - 1, "=");
        }
    }
    s
}
fn upload(png: &[u8], id: u32) -> String {
    let data = base64(png);
    let chunks = data.as_bytes().chunks(4096);
    let count = chunks.len();
    let mut out = String::new();
    for (i, chunk) in chunks.enumerate() {
        if i == 0 {
            out.push_str(&format!(
                "\x1b_Ga=t,f=100,t=d,i={id},q=2,m={};",
                u8::from(i + 1 < count)
            ));
        } else {
            out.push_str(&format!("\x1b_Gq=2,m={};", u8::from(i + 1 < count)));
        }
        out.push_str(std::str::from_utf8(chunk).unwrap());
        out.push_str("\x1b\\");
    }
    out
}
fn direct(out: &mut impl Write, wire: &str, id: u32, trace: &Trace) -> io::Result<()> {
    trace.event(
        "upload.begin",
        json!({"id":id,"wire_bytes":wire.len(),"format":"PNG/f=100"}),
    );
    out.write_all(wire.as_bytes())?;
    trace.event("upload.write_all.completed", json!({"id":id}));
    trace.event(
        "placement.begin",
        json!({"id":id,"rect":[5,5,16,8],"cursor":"save; CSI 6;6H; placement C=1; restore"}),
    );
    write!(
        out,
        "\x1b7\x1b[6;6H\x1b_Ga=p,i={id},p=1,c=16,r=8,C=1,z=1,q=2\x1b\\\x1b8"
    )?;
    out.flush()?;
    trace.event("placement.flush.completed", json!({"id":id}));
    Ok(())
}
// Candidate correction lives ONLY in this isolated reproducer.
fn probe_without_status(trace: &Trace) -> Result<Capabilities> {
    use ratatui_image::picker::cap_parser::{Parser, QueryStdioOptions, Response};
    use rustix::event::{PollFd, PollFlags, poll};
    let query = Parser::query(
        false,
        QueryStdioOptions {
            text_sizing_protocol: false,
        },
    );
    let query = query
        .strip_suffix("\x1b[5n")
        .ok_or("unexpected upstream query suffix")?;
    trace.event("candidate.query",json!({"removed_only":"final CSI 5n status request","kitty_query":true,"device_attributes_query":true,"cell_query":true,"budget_ms":300}));
    let mut output = io::stdout();
    output.write_all(query.as_bytes())?;
    output.flush()?;
    let start = Instant::now();
    let mut input = io::stdin();
    let mut parser = Parser::new();
    let mut kitty = false;
    let mut sixel = false;
    let mut cell = None;
    let mut count = 0;
    while start.elapsed() < Duration::from_millis(300) && count < 4096 {
        let timeout = Duration::from_millis(300)
            .saturating_sub(start.elapsed())
            .as_millis() as i32;
        if !matches!(poll(&mut [PollFd::new(&input,PollFlags::IN)],timeout),Ok(n) if n>0) {
            break;
        }
        let mut bytes = [0u8; 256];
        let n = input.read(&mut bytes)?;
        if n == 0 {
            break;
        }
        count += n;
        for byte in &bytes[..n] {
            for response in parser.push(char::from(*byte)) {
                match response {
                    Response::Kitty => kitty = true,
                    Response::Sixel => sixel = true,
                    Response::CellSize(c) => cell = c,
                    _ => {}
                }
            }
        }
        if (kitty || sixel) && cell.is_some() {
            break;
        }
    }
    trace.event(
        "candidate.responses",
        json!({"kitty":kitty,"cell":cell,"bytes":count,"elapsed_us":start.elapsed().as_micros()}),
    );
    Ok(Capabilities::from_responses(kitty, sixel, cell))
}
fn main() -> Result<()> {
    let args = std::env::args().collect::<Vec<_>>();
    let value = |key: &str| {
        args.iter()
            .position(|s| s == key)
            .and_then(|i| args.get(i + 1))
            .cloned()
    };
    let stage = value("--stage").ok_or("--stage required")?;
    if ![
        "stdout",
        "stdout-alt",
        "raw",
        "raw-alt",
        "ratatui-before",
        "ratatui-after",
        "player",
        "player-probe",
        "player-poll",
        "player-probe-poll",
        "player-probe-poll-no-status",
    ]
    .contains(&stage.as_str())
    {
        return Err("unknown stage".into());
    }
    if !io::stdin().is_terminal() || !io::stdout().is_terminal() {
        return Err("requires a real terminal or explicit PTY fixture".into());
    }
    for key in [
        "TMUX",
        "STY",
        "SSH_CONNECTION",
        "SSH_TTY",
        "NO_COLOR",
        "SENTINEL_TUI_ASCII",
    ] {
        if std::env::var_os(key).is_some() {
            return Err(format!("unsafe diagnostic environment: {key}").into());
        }
    }
    // Receipt comes from the runner's bounded, actual Kitty ACK + cell-size query.
    // No TERM/program-name inference and no per-stage query contaminating the comparison.
    let receipt: Value = serde_json::from_slice(&std::fs::read(
        value("--capability").ok_or("--capability receipt required")?,
    )?)?;
    if receipt["kitty_ack"] != true {
        return Err("Kitty not confirmed".into());
    }
    let cell = (
        receipt["cell"][0].as_u64().ok_or("cell width")? as u16,
        receipt["cell"][1].as_u64().ok_or("cell height")? as u16,
    );
    if cell.0 == 0 || cell.0 > 128 || cell.1 == 0 || cell.1 > 256 {
        return Err("invalid cell size".into());
    }
    let path = PathBuf::from(value("--trace").ok_or("--trace required")?);
    let mut options = OpenOptions::new();
    options.write(true).create_new(true);
    #[cfg(unix)]
    {
        use std::os::unix::fs::OpenOptionsExt;
        options.mode(0o600);
    }
    let trace = Trace {
        file: Arc::new(Mutex::new(Log {
            file: options.open(path)?,
            records: 0,
            bytes: 0,
        })),
        start: Instant::now(),
    };
    trace.event(
        "stage.begin",
        json!({"stage":stage,"capability":receipt,"keyboard_reads":false}),
    );
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../../assets/core-warden");
    let png = std::fs::read(root.join("animations/idle/96/frame_000.png"))?;
    let id = 0x00500000 + (std::process::id() % 0x000fffff);
    let wire = upload(&png, id);
    let player_stage = stage.starts_with("player");
    let uses_probe = stage.contains("probe");
    let uses_poll = stage.contains("poll");
    let raw = stage != "stdout" && stage != "stdout-alt";
    let alt = stage != "stdout" && stage != "raw";
    let mut restore = Restore {
        raw: false,
        alt: false,
        id: if player_stage { 0 } else { id },
        trace: trace.clone(),
    };
    let mut out = Out {
        trace: trace.clone(),
        bytes: 0,
        flushes: 0,
    };
    if raw {
        trace.event("raw.enable.begin", Value::Null);
        enable_raw_mode()?;
        restore.raw = true;
        trace.event("raw.enable.completed", Value::Null);
    }
    if alt {
        trace.event("alternate.enter.begin", Value::Null);
        execute!(out, EnterAlternateScreen)?;
        restore.alt = true;
        trace.event("alternate.enter.completed", Value::Null);
    }
    let player_caps = if uses_probe {
        trace.event("player.capability_probe.begin", Value::Null);
        let caps = if stage.ends_with("no-status") {
            probe_without_status(&trace)?
        } else {
            sentinel_mascot::detect(&sentinel_mascot::Environment::current())
        };
        trace.event(
            "player.capability_probe.completed",
            json!({"protocol":caps.protocol(),"reason":caps.reason}),
        );
        if caps.protocol() != "kitty" {
            return Err("additional probe did not confirm Kitty".into());
        }
        caps
    } else {
        Capabilities::from_responses(true, false, Some(cell))
    };
    if stage.starts_with("ratatui") || player_stage {
        trace.event("ratatui.terminal.new.begin", Value::Null);
        let mut terminal = Terminal::new(CrosstermBackend::new(out))?;
        trace.event("ratatui.terminal.new.completed", Value::Null);
        if stage == "ratatui-before" {
            direct(terminal.backend_mut(), &wire, id, &trace)?;
            trace.event("first_draw.not_called", Value::Null);
        } else {
            trace.event("first_draw.begin", Value::Null);
            terminal.draw(|f| {
                f.render_widget(
                    Paragraph::new("Rust terminal lifecycle control"),
                    Rect::new(0, 0, 40, 1),
                )
            })?;
            trace.event("first_draw.completed", Value::Null);
            Write::flush(terminal.backend_mut())?;
            trace.event("backend.explicit_flush.completed", Value::Null);
            if player_stage {
                let mut player =
                    Player::new(Assets::load(&root), player_caps, DisplayMode::Static, false);
                let place = sentinel_mascot::Placement {
                    area: Rect::new(5, 5, 16, 8),
                    resolution: 96,
                };
                let load = Instant::now();
                // The existing Player's bounded loader wait contract, not a new redraw timer.
                loop {
                    let dirty = player.update(State::Error, Some(place), false, Instant::now());
                    if dirty {
                        terminal.draw(|f| player.render(f, place.area, State::Error))?;
                        player.present(terminal.backend_mut())?;
                    }
                    if player.frame_count() > 0 {
                        break;
                    }
                    if load.elapsed() > Duration::from_secs(5) {
                        return Err("Player load timeout".into());
                    }
                    let wait = player
                        .next_wake(Instant::now())
                        .unwrap_or(Duration::from_millis(50));
                    if uses_poll {
                        trace.event(
                            "event.poll.begin",
                            json!({"timeout_us":wait.as_micros(),"phase":"loading"}),
                        );
                        let ready = crossterm::event::poll(wait)?;
                        trace.event(
                            "event.poll.completed",
                            json!({"ready":ready,"phase":"loading"}),
                        );
                        if ready {
                            let _ = crossterm::event::read()?;
                            trace.event("input.event.consumed", Value::Null);
                        }
                    } else {
                        std::thread::sleep(wait);
                    }
                }
                trace.event("player.cache_ready_and_presented",json!({"frames":player.frame_count(),"mode":"static","state":"disconnected neutral","encoded_bytes":player.encoded_bytes()}));
                trace.event("observation.begin",json!({"seconds":5,"no_input_reads":true,"no_draws_or_flushes_during_hold":true}));
                if uses_poll {
                    let observe = Instant::now();
                    while observe.elapsed() < Duration::from_secs(5) {
                        let wait = Duration::from_secs(5)
                            .saturating_sub(observe.elapsed())
                            .min(Duration::from_secs(1));
                        trace.event(
                            "event.poll.begin",
                            json!({"timeout_us":wait.as_micros(),"phase":"hold"}),
                        );
                        let ready = crossterm::event::poll(wait)?;
                        trace.event(
                            "event.poll.completed",
                            json!({"ready":ready,"phase":"hold"}),
                        );
                        if ready {
                            let _ = crossterm::event::read()?;
                            trace.event("input.event.consumed", Value::Null);
                        }
                    }
                } else {
                    std::thread::sleep(Duration::from_secs(5));
                }
                trace.event("observation.end", Value::Null);
                player.hide();
                trace.event("player.hide.completed", Value::Null);
                terminal.backend_mut().show_cursor()?;
                return Ok(());
            }
            direct(terminal.backend_mut(), &wire, id, &trace)?;
        }
        trace.event(
            "observation.begin",
            json!({"seconds":5,"no_input_reads":true,"no_draws_or_flushes_during_hold":true}),
        );
        std::thread::sleep(Duration::from_secs(5));
        trace.event("observation.end", Value::Null);
        terminal.backend_mut().show_cursor()?;
    } else {
        direct(&mut out, &wire, id, &trace)?;
        trace.event(
            "observation.begin",
            json!({"seconds":5,"no_input_reads":true,"no_draws_or_flushes_during_hold":true}),
        );
        std::thread::sleep(Duration::from_secs(5));
        trace.event("observation.end", Value::Null);
    }
    Ok(())
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn base64_vectors() {
        for (bytes, text) in [
            (b"".as_slice(), ""),
            (b"f", "Zg=="),
            (b"fo", "Zm8="),
            (b"foo", "Zm9v"),
            (b"foobar", "Zm9vYmFy"),
        ] {
            assert_eq!(base64(bytes), text);
        }
    }
    #[test]
    fn upload_and_placement_do_not_clear_or_invalidate_screen() {
        let wire = upload(&vec![1; 10000], 42);
        assert!(wire.starts_with("\x1b_Ga=t,f=100,t=d,i=42,q=2,m=1;"));
        assert_eq!(wire.matches("m=0;").count(), 1);
        assert!(!wire.contains("U=1") && !wire.contains("\x1b[2J"));
    }
}
