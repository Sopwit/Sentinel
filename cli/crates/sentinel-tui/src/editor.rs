//! In-memory composer: UTF-8 boundaries, multiline navigation and bounded history.
#[derive(Default)]
pub(crate) struct Editor {
    pub text: String,
    pub cursor: usize,
    pub literal: bool,
    history: Vec<(String, bool)>,
    index: Option<usize>,
    draft: (String, bool),
}
impl Editor {
    pub fn insert(&mut self, text: &str) {
        if self.text.is_empty() {
            self.literal = false;
        }
        if self.text.len() + text.len() <= 65536 {
            self.text.insert_str(self.cursor, text);
            self.cursor += text.len();
        }
    }
    pub fn paste(&mut self, text: &str) {
        self.insert(text);
        if self.text.starts_with('/') {
            self.literal = true;
        }
    }
    pub fn replace(&mut self, text: String, literal: bool) {
        self.text = text;
        self.cursor = self.text.len();
        self.literal = literal;
        self.index = None;
    }
    pub fn left(&mut self) {
        self.cursor = self.text[..self.cursor]
            .char_indices()
            .last()
            .map_or(0, |(i, _)| i);
    }
    pub fn right(&mut self) {
        if let Some(c) = self.text[self.cursor..].chars().next() {
            self.cursor += c.len_utf8();
        }
    }
    pub fn backspace(&mut self) {
        let old = self.cursor;
        self.left();
        self.text.drain(self.cursor..old);
    }
    pub fn delete(&mut self) {
        if let Some(c) = self.text[self.cursor..].chars().next() {
            self.text.drain(self.cursor..self.cursor + c.len_utf8());
        }
    }
    pub fn home(&mut self) {
        self.cursor = self.text[..self.cursor].rfind('\n').map_or(0, |i| i + 1);
    }
    pub fn end(&mut self) {
        self.cursor += self.text[self.cursor..]
            .find('\n')
            .unwrap_or(self.text.len() - self.cursor);
    }
    pub fn kill_to_end(&mut self) {
        let end = self.cursor
            + self.text[self.cursor..]
                .find('\n')
                .unwrap_or(self.text.len() - self.cursor);
        let end = if end == self.cursor && end < self.text.len() {
            end + 1
        } else {
            end
        };
        self.text.drain(self.cursor..end);
    }
    pub fn kill_to_start(&mut self) {
        let end = self.cursor;
        self.home();
        self.text.drain(self.cursor..end);
    }
    pub fn word(&mut self, forward: bool) {
        if forward {
            while self.cursor < self.text.len()
                && !self.text[self.cursor..].starts_with(char::is_whitespace)
            {
                self.right();
            }
            while self.cursor < self.text.len()
                && self.text[self.cursor..].starts_with(char::is_whitespace)
            {
                self.right();
            }
        } else {
            self.left();
            while self.cursor > 0 && self.text[self.cursor..].starts_with(char::is_whitespace) {
                self.left();
            }
            while self.cursor > 0 {
                let before = self.text[..self.cursor].chars().next_back().unwrap();
                if before.is_whitespace() {
                    break;
                }
                self.left();
            }
        }
    }
    pub fn vertical(&mut self, down: bool) {
        let start = self.text[..self.cursor].rfind('\n').map_or(0, |i| i + 1);
        let column = self.text[start..self.cursor].chars().count();
        let target = if down {
            match self.text[self.cursor..].find('\n') {
                Some(i) => self.cursor + i + 1,
                None => return,
            }
        } else {
            if start == 0 {
                return;
            }
            self.text[..start - 1].rfind('\n').map_or(0, |i| i + 1)
        };
        let line = self.text[target..].split('\n').next().unwrap_or("");
        self.cursor = target
            + line
                .char_indices()
                .nth(column)
                .map_or(line.len(), |(i, _)| i);
    }
    pub fn history(&mut self, down: bool) {
        if self.history.is_empty() {
            return;
        }
        if down {
            if let Some(i) = self.index {
                if i + 1 == self.history.len() {
                    self.index = None;
                    self.text = self.draft.0.clone();
                    self.literal = self.draft.1;
                } else {
                    self.index = Some(i + 1);
                    self.text = self.history[i + 1].0.clone();
                    self.literal = self.history[i + 1].1;
                }
            }
        } else {
            let i = if let Some(i) = self.index {
                i.saturating_sub(1)
            } else {
                self.draft = (self.text.clone(), self.literal);
                self.history.len() - 1
            };
            self.index = Some(i);
            self.text = self.history[i].0.clone();
            self.literal = self.history[i].1;
        }
        self.cursor = self.text.len();
    }
    pub fn take(&mut self) -> String {
        let text = std::mem::take(&mut self.text);
        let entry = (text.clone(), self.literal);
        if self.history.last() != Some(&entry) {
            self.history.push(entry);
        }
        if self.history.len() > 100 {
            self.history.remove(0);
        }
        self.index = None;
        self.draft = (String::new(), false);
        self.literal = false;
        self.cursor = 0;
        text
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn pasted_command_recall_stays_literal_and_restores_draft() {
        let mut e = Editor::default();
        e.paste("/exit");
        assert_eq!(e.take(), "/exit");
        e.insert("unfinished");
        e.history(false);
        assert_eq!(e.text, "/exit");
        assert!(e.literal);
        e.history(true);
        assert_eq!(e.text, "unfinished");
        assert!(!e.literal);
        e.replace(String::new(), true);
        e.insert("/help");
        assert!(
            !e.literal,
            "clearing a pasted draft must allow fresh commands"
        );
    }
    #[test]
    fn unicode_edits() {
        let mut e = Editor::default();
        e.insert("ağ🙂");
        e.left();
        e.backspace();
        assert_eq!(e.text, "a🙂");
        e.delete();
        assert_eq!(e.text, "a");
    }
    #[test]
    fn draft_restored() {
        let mut e = Editor::default();
        e.insert("sent");
        e.take();
        e.insert("draft");
        e.history(false);
        assert_eq!(e.text, "sent");
        e.history(true);
        assert_eq!(e.text, "draft");
    }
    #[test]
    fn multiline_navigation() {
        let mut e = Editor::default();
        e.insert("abc\nx\nüzz");
        e.vertical(false);
        assert_eq!(e.cursor, 5);
        e.home();
        assert_eq!(e.cursor, 4);
        e.vertical(false);
        assert_eq!(e.cursor, 0);
        e.end();
        assert_eq!(e.cursor, 3);
    }
    #[test]
    fn readline_kills_respect_lines_and_utf8() {
        let mut e = Editor::default();
        e.insert("first\n界ü rest");
        e.home();
        e.right();
        e.kill_to_end();
        assert_eq!(e.text, "first\n界");
        e.kill_to_start();
        assert_eq!(e.text, "first\n");
        e.home();
        e.kill_to_end();
        assert_eq!(e.text, "first\n");
    }
    #[test]
    fn bounded_paste() {
        let mut e = Editor::default();
        e.insert(&"a".repeat(65537));
        assert!(e.text.is_empty());
    }
}
