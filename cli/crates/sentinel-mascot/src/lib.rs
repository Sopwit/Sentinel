//! Bounded presentation-only playback. The daemon retains all run authority.
mod assets;
mod capability;
mod playback;
mod state;
pub use assets::{Assets, Clip};
pub use capability::{Capabilities, Environment, detect};
pub use playback::{DisplayMode, Placement, Player, frame_index, placement};
pub use state::{Projection, State};
#[cfg(test)]
mod tests;

mod terminal_input;
pub use terminal_input::poll_terminal_event;
