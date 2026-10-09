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
            .filter(|item| {
                fuzzy(
                    &self.query,
                    &format!("{} {} {}", item.label, item.id, item.extra),
                )
            })
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
    fn identities_and_long_details_are_searchable() {
        let p = Picker {
            kind: "session",
            query: "session-42".into(),
            selected: 0,
            items: vec![Item {
                label: "Untitled".into(),
                id: "session-42".into(),
                extra: "long model / workspace detail".into(),
                enabled: true,
            }],
        };
        assert_eq!(p.visible().len(), 1);
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
