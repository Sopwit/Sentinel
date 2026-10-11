//! Keep Crossterm's Unix drain loop bounded even for unrecognized terminal replies.
use std::{io, time::Duration};

/// Poll with nonblocking reads only for the duration of polling. Restore flags
/// before any rendering: stdin and stdout can share an open file description.
pub fn poll_terminal_event(timeout: Duration) -> io::Result<bool> {
    #[cfg(unix)]
    {
        use rustix::{
            fs::{OFlags, fcntl_getfl, fcntl_setfl},
            stdio::stdin,
        };
        let input = stdin();
        if !rustix::termios::isatty(input) {
            return crossterm::event::poll(timeout);
        }
        struct Restore(Option<OFlags>);
        impl Drop for Restore {
            fn drop(&mut self) {
                if let Some(flags) = self.0 {
                    let _ = fcntl_setfl(stdin(), flags);
                }
            }
        }
        let original = fcntl_getfl(input)?;
        let mut restore = Restore(Some(original));
        fcntl_setfl(input, original | OFlags::NONBLOCK)?;
        let result = crossterm::event::poll(timeout);
        let restored = fcntl_setfl(input, original);
        restored?;
        restore.0 = None;
        result
    }
    #[cfg(not(unix))]
    {
        crossterm::event::poll(timeout)
    }
}
