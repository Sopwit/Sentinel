//! Disposable PTY control: compare upstream polling with the scoped adapter.
use std::{
    io::{self, Write},
    time::{Duration, Instant},
};
fn main() -> io::Result<()> {
    crossterm::terminal::enable_raw_mode()?;
    struct Restore;
    impl Drop for Restore {
        fn drop(&mut self) {
            let _ = crossterm::terminal::disable_raw_mode();
        }
    }
    let _restore = Restore;
    print!("READY\r\n");
    io::stdout().flush()?;
    let start = Instant::now();
    let bounded = std::env::args().any(|a| a == "--bounded");
    let ready = if bounded {
        sentinel_mascot::poll_terminal_event(Duration::from_millis(50))?
    } else {
        crossterm::event::poll(Duration::from_millis(50))?
    };
    println!("elapsed_ms={} ready={ready}", start.elapsed().as_millis());
    Ok(())
}
