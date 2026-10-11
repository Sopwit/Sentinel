//! One current presentation only; no history, daemon state or image data.
use super::{Theme, header_style, secondary_style, transcript_lines, wrap_transcript};
use ratatui::text::Line;

#[derive(Default)]
pub(super) struct TranscriptCache {
    source: String,
    width: usize,
    theme: Option<Theme>,
    empty: bool,
    rows: Vec<Line<'static>>,
    #[cfg(test)]
    pub builds: usize,
}
impl TranscriptCache {
    pub fn update(&mut self, source: String, width: usize, theme: Theme, empty: bool) {
        let width = width.max(1);
        if self.source == source
            && self.width == width
            && self.theme == Some(theme)
            && self.empty == empty
        {
            return;
        }
        let lines = if empty {
            source
                .lines()
                .map(|line| {
                    Line::styled(
                        line.to_owned(),
                        if line == "Start a new conversation" || line.contains(" S ") {
                            header_style()
                        } else {
                            secondary_style()
                        },
                    )
                })
                .collect()
        } else {
            transcript_lines(&source)
        };
        self.rows = wrap_transcript(lines, width);
        self.source = source;
        self.width = width;
        self.theme = Some(theme);
        self.empty = empty;
        #[cfg(test)]
        {
            self.builds += 1;
        }
    }
    pub fn rows(&self) -> &[Line<'static>] {
        &self.rows
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn current_presentation_is_reused_and_invalidated_by_real_inputs() {
        let mut cache = TranscriptCache::default();
        let source = "Assistant\n  **Bold** Türkçe\n  ```rust\n  let a = 1;\n  ```";
        Theme::Glacier.apply();
        cache.update(source.into(), 40, Theme::Glacier, false);
        let expected = wrap_transcript(transcript_lines(source), 40);
        assert_eq!(cache.rows(), expected);
        for _ in 0..64 {
            cache.update(source.into(), 40, Theme::Glacier, false);
        }
        assert_eq!(
            cache.builds, 1,
            "frame ticks, input and scroll must reuse unchanged rows"
        );
        cache.update(source.into(), 20, Theme::Glacier, false);
        assert_eq!(cache.builds, 2);
        Theme::Porcelain.apply();
        cache.update(source.into(), 20, Theme::Porcelain, false);
        assert_eq!(cache.rows(), wrap_transcript(transcript_lines(source), 20));
        cache.update("Different session".into(), 20, Theme::Porcelain, false);
        assert!(!cache.rows().iter().any(|r| r.to_string().contains("Bold")));
        cache.update("Different session".into(), 20, Theme::Porcelain, true);
        assert_eq!(cache.builds, 5);
        for i in 0..32 {
            cache.update(format!("Replacement {i}"), 20, Theme::Porcelain, false);
        }
        assert_eq!(cache.source, "Replacement 31");
        assert_eq!(
            cache.rows().len(),
            1,
            "replacement must not retain prior presentations"
        );
    }
}
