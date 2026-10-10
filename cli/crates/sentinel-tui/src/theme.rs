//! Process-local presentation preferences; never daemon permissions or settings.
use ratatui::style::{Color, Style};
use std::cell::Cell;
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub(crate) enum Theme {
    Terminal,
    Obsidian,
    Glacier,
    Porcelain,
}
impl Default for Theme {
    fn default() -> Self {
        Self::parse(&std::env::var("SENTINEL_TUI_THEME").unwrap_or_default())
            .unwrap_or(Self::Terminal)
    }
}
impl Theme {
    pub fn parse(name: &str) -> Option<Self> {
        match name {
            "terminal" => Some(Self::Terminal),
            "obsidian" | "dark" => Some(Self::Obsidian),
            "glacier" => Some(Self::Glacier),
            "porcelain" | "light" => Some(Self::Porcelain),
            _ => None,
        }
    }
    pub fn name(self) -> &'static str {
        match self {
            Self::Terminal => "terminal",
            Self::Obsidian => "obsidian",
            Self::Glacier => "glacier",
            Self::Porcelain => "porcelain",
        }
    }
    pub fn apply(self) {
        CURRENT.set(self);
    }
    fn colors(self) -> Option<(Color, Color, Color, Color)> {
        match self {
            Self::Terminal => None,
            Self::Obsidian => Some((
                Color::Rgb(21, 24, 31),
                Color::Rgb(232, 235, 240),
                Color::Rgb(156, 204, 218),
                Color::Rgb(173, 184, 197),
            )),
            Self::Glacier => Some((
                Color::Rgb(28, 40, 51),
                Color::Rgb(231, 241, 248),
                Color::Rgb(126, 214, 232),
                Color::Rgb(172, 198, 213),
            )),
            Self::Porcelain => Some((
                Color::Rgb(244, 242, 237),
                Color::Rgb(36, 43, 49),
                Color::Rgb(22, 88, 110),
                Color::Rgb(75, 89, 101),
            )),
        }
    }
}
thread_local! { static CURRENT: Cell<Theme> = const { Cell::new(Theme::Terminal) }; }
fn colors() -> Option<(Color, Color, Color, Color)> {
    if std::env::var_os("NO_COLOR").is_some() {
        None
    } else {
        CURRENT.get().colors()
    }
}
pub(crate) fn base() -> Style {
    colors().map_or(Style::default(), |(bg, fg, _, _)| {
        Style::default().bg(bg).fg(fg)
    })
}
pub(crate) fn accent() -> Style {
    colors().map_or(Style::default(), |(_, _, accent, _)| {
        Style::default().fg(accent)
    })
}
pub(crate) fn secondary() -> Style {
    colors().map_or(Style::default(), |(_, _, _, muted)| {
        Style::default().fg(muted)
    })
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn aliases_and_distinct_palettes() {
        assert_eq!(Theme::parse("dark"), Some(Theme::Obsidian));
        assert_eq!(Theme::parse("light"), Some(Theme::Porcelain));
        assert_eq!(Theme::parse("bad"), None);
        assert_ne!(Theme::Obsidian.colors(), Theme::Glacier.colors());
        assert_ne!(Theme::Porcelain.colors(), Theme::Glacier.colors());
    }
}
