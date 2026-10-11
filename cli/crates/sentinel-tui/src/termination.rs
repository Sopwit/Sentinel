//! Scoped TUI-only notification; handlers only store an atomic value.
#[cfg(unix)]
pub struct Termination {
    pending: std::sync::Arc<std::sync::atomic::AtomicUsize>,
    registrations: Vec<signal_hook::SigId>,
}

#[cfg(unix)]
impl Termination {
    pub fn new() -> std::io::Result<Self> {
        let mut this = Self {
            pending: Default::default(),
            registrations: Vec::new(),
        };
        for signal in [signal_hook::consts::SIGTERM, signal_hook::consts::SIGHUP] {
            this.registrations.push(signal_hook::flag::register_usize(
                signal,
                this.pending.clone(),
                signal as usize,
            )?);
        }
        Ok(this)
    }

    pub fn check(&self) -> Result<(), sentinel_ipc::Error> {
        use std::sync::atomic::Ordering;
        let signal = self.pending.load(Ordering::SeqCst);
        let name = match signal as i32 {
            signal_hook::consts::SIGTERM => "SIGTERM",
            signal_hook::consts::SIGHUP => "SIGHUP",
            _ => return Ok(()),
        };
        Err(sentinel_ipc::Error::Failed(format!(
            "TUI terminated by {name} ({signal})"
        )))
    }
}

#[cfg(unix)]
impl Drop for Termination {
    fn drop(&mut self) {
        for id in self.registrations.drain(..) {
            signal_hook::low_level::unregister(id);
        }
    }
}

#[cfg(not(unix))]
pub struct Termination;
#[cfg(not(unix))]
impl Termination {
    pub fn new() -> std::io::Result<Self> {
        Ok(Self)
    }
    pub fn check(&self) -> Result<(), sentinel_ipc::Error> {
        Ok(())
    }
}

#[cfg(all(test, unix))]
mod tests {
    use super::*;
    #[test]
    fn notification_is_sticky_and_reports_signal_without_cancelling_a_run() {
        let termination = Termination::new().unwrap();
        assert!(termination.check().is_ok());
        for signal in [signal_hook::consts::SIGTERM, signal_hook::consts::SIGHUP] {
            termination
                .pending
                .store(signal as usize, std::sync::atomic::Ordering::SeqCst);
            for _ in 0..2 {
                let error = termination.check().unwrap_err();
                assert_eq!(error.exit_code(), 1);
                assert!(error.to_string().contains("TUI terminated by"));
            }
        }
    }
}
