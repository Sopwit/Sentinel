//! Diagnostic: direct Ratatui/Kitty first paint, without Player, IPC or keyboard reads.
use crossterm::{
    execute,
    terminal::{EnterAlternateScreen, LeaveAlternateScreen, disable_raw_mode, enable_raw_mode},
};
use ratatui::{Terminal, backend::CrosstermBackend, layout::Rect};
use ratatui_image::{Resize, ResizeEncodeRender};
use sentinel_mascot::{Environment, detect};
use std::{
    io::{self, Write},
    sync::mpsc,
    time::{Duration, Instant},
};
struct Restore;
impl Drop for Restore {
    fn drop(&mut self) {
        let _ = disable_raw_mode();
        let _ = execute!(io::stdout(), LeaveAlternateScreen);
    }
}
fn main() -> Result<(), Box<dyn std::error::Error>> {
    enable_raw_mode()?;
    let _restore = Restore;
    execute!(io::stdout(), EnterAlternateScreen)?;
    let Some(picker) = detect(&Environment::current()).picker else {
        return Err("no confirmed graphics support".into());
    };
    let mut terminal = Terminal::new(CrosstermBackend::new(io::stdout()))?;
    let area = Rect::new(5, 5, 16, 8);
    terminal.draw(|_| {})?;
    let (tx, rx) = mpsc::sync_channel(1);
    std::thread::spawn(move || {
        let result = (|| -> Result<_, String> {
            let img = image::open(
                std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
                    .join("../../../assets/core-warden/animations/idle/96/frame_000.png"),
            )
            .map_err(|e| e.to_string())?;
            let mut protocol = picker.new_resize_protocol(img);
            let resize = Resize::Scale(Some(image::imageops::FilterType::Triangle));
            let desired = protocol.size_for(resize.clone(), area);
            protocol.resize_encode(&resize, desired);
            Ok(protocol)
        })();
        let _ = tx.send(result);
    });
    let mut protocol = rx.recv_timeout(Duration::from_secs(5))??;
    let ratatui_image::protocol::StatefulProtocolType::Kitty(kitty) = protocol.protocol_type()
    else {
        return Err("A/B requires confirmed Kitty".into());
    };
    let id = kitty.unique_id;
    if std::env::args().any(|arg| arg == "--direct") {
        let mut buffer = ratatui::buffer::Buffer::empty(area);
        protocol.render(area, &mut buffer);
        let upload = buffer[(area.x, area.y)]
            .symbol()
            .split_once("\x1b[s")
            .ok_or("upload boundary")?
            .0
            .replace("a=T,U=1,", "a=t,");
        terminal.draw(|_| {})?;
        let out = terminal.backend_mut();
        out.write_all(upload.as_bytes())?;
        write!(
            out,
            "\x1b7\x1b[{};{}H\x1b_Ga=p,i={id},p=1,c={},r={},C=1,z=1,q=2\x1b\\\x1b8",
            area.y + 1,
            area.x + 1,
            area.width,
            area.height
        )?;
        out.flush()?;
    } else {
        terminal.draw(|f| protocol.render(area, f.buffer_mut()))?;
    }
    let start = Instant::now();
    while start.elapsed() < Duration::from_secs(5) {
        std::thread::sleep(Duration::from_millis(50));
    }
    write!(terminal.backend_mut(), "\x1b_Ga=d,d=I,i={id},q=2\x1b\\")?;
    terminal.backend_mut().flush()?;
    Ok(())
}
