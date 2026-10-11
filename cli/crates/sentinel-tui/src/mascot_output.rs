//! Opt-in byte counters at the actual Crossterm writer boundary. Never stores terminal text.
use serde_json::{Value, json};
use std::{
    io::{self, Write},
    sync::{Arc, Mutex},
    time::Instant,
};
#[derive(Default)]
struct Counts {
    bytes: u64,
    flushes: u64,
    uploads: u64,
    virtuals: u64,
    placements: u64,
    placeholders: u64,
    deletes: u64,
    last_flush_us: Option<u128>,
    ids: Vec<u32>,
    header: Vec<u8>,
    in_header: bool,
    prefix: usize,
    unicode: usize,
}
impl Counts {
    fn accept(&mut self, bytes: &[u8]) {
        const APC: &[u8] = b"\x1b_G";
        let placeholder = "\u{10eeee}".as_bytes();
        self.bytes += bytes.len() as u64;
        for &byte in bytes {
            if byte == placeholder[self.unicode] {
                self.unicode += 1;
            } else {
                self.unicode = usize::from(byte == placeholder[0]);
            }
            if self.unicode == placeholder.len() {
                self.placeholders += 1;
                self.unicode = 0;
            }
            if self.in_header {
                if byte == b';' || byte == 0x1b {
                    let text = String::from_utf8_lossy(&self.header);
                    if text.split(',').any(|s| s == "a=T" || s == "a=t") {
                        self.uploads += 1;
                    }
                    if text.split(',').any(|s| s == "a=p") {
                        self.placements += 1;
                    }
                    if text.split(',').any(|s| s == "U=1") {
                        self.virtuals += 1;
                    }
                    if text.split(',').any(|s| s == "a=d") {
                        self.deletes += 1;
                    }
                    if let Some(id) = text
                        .split(',')
                        .find_map(|s| s.strip_prefix("i=").and_then(|s| s.parse().ok()))
                        && self.ids.len() < 64
                        && !self.ids.contains(&id)
                    {
                        self.ids.push(id);
                    }
                    self.header.clear();
                    self.in_header = false;
                } else if self.header.len() < 256 {
                    self.header.push(byte);
                } else {
                    self.header.clear();
                    self.in_header = false;
                }
            }
            if byte == APC[self.prefix] {
                self.prefix += 1;
            } else {
                self.prefix = usize::from(byte == APC[0]);
            }
            if self.prefix == APC.len() {
                self.prefix = 0;
                self.in_header = true;
                self.header.clear();
            }
        }
    }
}
#[derive(Clone)]
pub(super) struct Trace {
    start: Instant,
    counts: Option<Arc<Mutex<Counts>>>,
}
impl Trace {
    pub(super) fn new(enabled: bool) -> Self {
        Self {
            start: Instant::now(),
            counts: enabled.then(|| Arc::new(Mutex::new(Counts::default()))),
        }
    }
    pub(super) fn enabled(&self) -> bool {
        self.counts.is_some()
    }
    pub(super) fn snapshot(&self) -> Value {
        let Some(counts) = &self.counts else {
            return Value::Null;
        };
        let c = counts.lock().unwrap();
        json!({"monotonic_us":self.start.elapsed().as_micros(),"actual_bytes_written":c.bytes,"backend_flushes_completed":c.flushes,"last_flush_us":c.last_flush_us,"kitty_uploads":c.uploads,"virtual_placement_declarations":c.virtuals,"direct_placements":c.placements,"unicode_placeholders_written":c.placeholders,"owned_deletes":c.deletes,"image_ids":c.ids})
    }
}
pub(super) struct Output<W> {
    inner: W,
    trace: Trace,
}
impl<W> Output<W> {
    pub(super) fn new(inner: W, trace: Trace) -> Self {
        Self { inner, trace }
    }
}
impl<W: Write> Write for Output<W> {
    fn write(&mut self, bytes: &[u8]) -> io::Result<usize> {
        let n = self.inner.write(bytes)?;
        if let Some(counts) = &self.trace.counts {
            counts.lock().unwrap().accept(&bytes[..n]);
        }
        Ok(n)
    }
    fn flush(&mut self) -> io::Result<()> {
        self.inner.flush()?;
        if let Some(counts) = &self.trace.counts {
            let mut c = counts.lock().unwrap();
            c.flushes += 1;
            c.last_flush_us = Some(self.trace.start.elapsed().as_micros());
        }
        Ok(())
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn background_ready_emits_upload_and_placement_without_input() {
        use ratatui::{
            Terminal, TerminalOptions, Viewport, backend::CrosstermBackend, layout::Rect,
        };
        use sentinel_mascot::{Assets, Capabilities, DisplayMode, Placement, Player, State};
        use std::time::Duration;
        for connected in [false, true] {
            for mode in [
                DisplayMode::Static,
                DisplayMode::Animated,
                DisplayMode::Auto,
            ] {
                let trace = Trace::new(true);
                let mut terminal = Terminal::with_options(
                    CrosstermBackend::new(Output::new(Vec::<u8>::new(), trace.clone())),
                    TerminalOptions {
                        viewport: Viewport::Fixed(Rect::new(0, 0, 100, 30)),
                    },
                )
                .unwrap();
                let mut player = Player::new(
                    Assets::load(
                        &std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
                            .join("../../../assets/core-warden"),
                    ),
                    Capabilities::from_responses(true, false, Some((8, 16))),
                    mode,
                    false,
                );
                let place = Placement {
                    area: Rect::new(42, 5, 16, 8),
                    resolution: 128,
                };
                let state = if connected { State::Idle } else { State::Error };
                let start = Instant::now();
                loop {
                    if player.update(state, Some(place), connected, Instant::now()) {
                        terminal
                            .draw(|f| player.render(f, place.area, state))
                            .unwrap();
                    }
                    if player.frame_count() > 0 {
                        assert_eq!(
                            trace.snapshot()["direct_placements"],
                            0,
                            "placement must follow terminal draw/flush"
                        );
                        assert!(
                            trace.snapshot()["backend_flushes_completed"]
                                .as_u64()
                                .unwrap()
                                > 0
                        );
                    }
                    player.present(terminal.backend_mut()).unwrap();
                    if trace.snapshot()["kitty_uploads"].as_u64().unwrap() > 0 {
                        break;
                    }
                    assert!(start.elapsed() < Duration::from_secs(5));
                    std::thread::sleep(
                        player
                            .next_wake(Instant::now())
                            .unwrap_or(Duration::from_millis(50)),
                    );
                }
                let result = trace.snapshot();
                assert_eq!(result["virtual_placement_declarations"], 0);
                assert_eq!(result["direct_placements"], 1);
                assert_eq!(result["unicode_placeholders_written"], 0);
                assert!(result["backend_flushes_completed"].as_u64().unwrap() > 0);
                assert!(result["last_flush_us"].is_number());
                player.hide();
            }
        }
    }
    #[test]
    fn counts_split_writes_and_flush_without_retaining_text_or_payload() {
        let trace = Trace::new(true);
        let mut output = Output::new(Vec::<u8>::new(), trace.clone());
        let text =
            "private prompt must not be retained\x1b_Gi=42,a=T,U=1;PRIVATE_BASE64\x1b\\\u{10eeee}";
        for byte in text.as_bytes() {
            output.write_all(&[*byte]).unwrap();
        }
        output.flush().unwrap();
        let snapshot = trace.snapshot();
        assert_eq!(snapshot["kitty_uploads"], 1);
        assert_eq!(snapshot["unicode_placeholders_written"], 1);
        assert_eq!(snapshot["image_ids"], json!([42]));
        assert_eq!(snapshot["backend_flushes_completed"], 1);
        assert!(!snapshot.to_string().contains("private"));
        assert!(!snapshot.to_string().contains("BASE64"));
    }
}
