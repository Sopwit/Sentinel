#[derive(Clone)]
pub(crate) struct Item {
    pub label: String,
    pub id: String,
    pub extra: String,
    pub enabled: bool,
}
pub(crate) struct Picker {
    pub kind: &'static str,
    pub query: String,
    pub selected: usize,
    pub items: Vec<Item>,
}
pub(crate) fn fuzzy(query: &str, label: &str) -> bool {
    let label = label.to_lowercase();
    let mut chars = label.chars();
    query
        .to_lowercase()
        .chars()
        .all(|needle| chars.any(|c| c == needle))
}
impl Picker {
    pub fn visible(&self) -> Vec<&Item> {
        self.items
            .iter()
            .filter(|item| fuzzy(&self.query, &item.label))
            .collect()
    }
    pub fn move_by(&mut self, down: bool) {
        let len = self.visible().len();
        self.selected = if down {
            (self.selected + 1).min(len.saturating_sub(1))
        } else {
            self.selected.saturating_sub(1)
        };
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn subsequence_and_case() {
        assert!(fuzzy("olma", "Ollama / model"));
        assert!(!fuzzy("xyz", "Ollama"));
    }
    #[test]
    fn empty_picker_safe() {
        let mut p = Picker {
            kind: "model",
            query: String::new(),
            selected: 0,
            items: vec![],
        };
        p.move_by(true);
        assert_eq!(p.selected, 0);
    }
}
