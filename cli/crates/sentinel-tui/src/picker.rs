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
        let mut items = self
            .items
            .iter()
            .filter(|item| {
                if self.kind == "file" {
                    // Relative names are the useful fuzzy target; common absolute roots
                    // and explanatory prose must not crowd out a requested filename.
                    fuzzy(&self.query, &item.label)
                        || item.id.to_lowercase().contains(&self.query.to_lowercase())
                } else {
                    fuzzy(
                        &self.query,
                        &format!("{} {} {}", item.label, item.id, item.extra),
                    )
                }
            })
            .collect::<Vec<_>>();
        if self.kind == "file" {
            items.sort_by_key(|item| !fuzzy(&self.query, &item.label));
        }
        items
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
    fn file_search_does_not_match_explanations_or_common_roots() {
        let p = Picker {
            kind: "file",
            query: "README".into(),
            selected: 0,
            items: vec![
                Item {
                    label: "CMakeLists.txt".into(),
                    id: "/root/README-project/CMakeLists.txt".into(),
                    extra: "reference requires reading metadata".into(),
                    enabled: true,
                },
                Item {
                    label: "nested/README.md".into(),
                    id: "/root/README-project/nested/README.md".into(),
                    extra: "reference only".into(),
                    enabled: true,
                },
            ],
        };
        // An exact substring in an absolute path remains discoverable, but
        // filename matches should come first when selecting the first row.
        assert_eq!(p.visible()[0].label, "nested/README.md");
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
