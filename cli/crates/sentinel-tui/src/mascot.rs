use super::*;
use sentinel_mascot::{Assets, Capabilities, Environment, Placement};

// Runtime paths only: installed prefix or the bounded standard Cargo checkout layout.
// An explicit override is authoritative even when missing/invalid.
fn asset_root(exe: Option<&Path>, explicit: Option<std::path::PathBuf>) -> std::path::PathBuf {
    if let Some(root) = explicit {
        return root;
    }
    let directory = exe.and_then(Path::parent).unwrap_or(Path::new("."));
    let installed = directory.join("../share/sentinel/core-warden");
    if installed.is_dir() {
        return installed;
    }
    if matches!(
        directory.file_name().and_then(|s| s.to_str()),
        Some("release" | "debug")
    ) && let Some(target) = directory.parent()
        && target.file_name().is_some_and(|s| s == "target")
        && let Some(cli) = target.parent()
        && cli.file_name().is_some_and(|s| s == "cli")
        && let Some(repository) = cli.parent()
    {
        let development = repository.join("assets/core-warden");
        if development.join("state-contract.json").is_file() {
            return development;
        }
    }
    installed
}

#[cfg(test)]
mod release_paths {
    use super::*;
    #[test]
    fn relocatable_assets_and_missing_override_do_not_use_build_home() {
        let temp = std::env::temp_dir().join(format!(
            "sentinel-assets-{}-{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        let development = temp.join("checkout/assets/core-warden");
        std::fs::create_dir_all(&development).unwrap();
        std::fs::write(development.join("state-contract.json"), "{}").unwrap();
        let executable = temp.join("checkout/cli/target/release/sentinel");
        assert_eq!(asset_root(Some(&executable), None), development);
        let installed = temp.join("checkout/cli/target/release/../share/sentinel/core-warden");
        std::fs::create_dir_all(&installed).unwrap();
        assert_eq!(asset_root(Some(&executable), None), installed);
        let override_path = temp.join("missing-override");
        assert_eq!(
            asset_root(Some(&executable), Some(override_path.clone())),
            override_path
        );
        let relocated = temp.join("standalone/sentinel");
        assert_eq!(
            asset_root(Some(&relocated), None),
            temp.join("standalone/../share/sentinel/core-warden")
        );
        std::fs::remove_dir_all(temp).unwrap();
    }
}

pub(super) fn player(mode: DisplayMode) -> (Player, Diagnostics) {
    let mut diagnostics = Diagnostics::open();
    let environment = Environment::current();
    let capabilities = if mode == DisplayMode::Off {
        Capabilities::text("Disabled at launch; restart to negotiate graphics")
    } else {
        sentinel_mascot::detect(&environment)
    };
    let root = asset_root(
        std::env::current_exe().ok().as_deref(),
        std::env::var_os("SENTINEL_CORE_WARDEN_ASSETS").map(std::path::PathBuf::from),
    );
    let assets = Assets::load(&root);
    diagnostics.write(json!({"kind":"startup", "executable":std::env::current_exe().ok(), "mode":mode.name(), "environment":format!("{environment:?}"), "asset_root":root, "assets":assets.as_ref().map(|_| "validated manifest and 132 required PNG files").map_err(Clone::clone), "protocol":capabilities.protocol(), "capability_reason":capabilities.reason, "probe_ms":capabilities.probe_elapsed.as_millis()}));
    (
        Player::new(assets, capabilities, mode, environment.reduced_motion),
        diagnostics,
    )
}

pub(super) fn slot(app: &App, screen: Rect, cell: Option<(u16, u16)>) -> Option<Placement> {
    let cell = cell?;
    if app.mascot_mode == DisplayMode::Off {
        return None;
    }
    let composer = app
        .editor
        .text
        .split('\n')
        .count()
        .saturating_add(2)
        .min((screen.height / 3).max(3) as usize) as u16;
    let areas = Layout::vertical([
        Constraint::Length(3),
        Constraint::Min(1),
        Constraint::Length(composer),
        Constraint::Length(2),
    ])
    .split(screen);
    let grid = content_grid(areas[1]);
    let reading = Rect::new(
        grid.x + 2,
        grid.y + 1,
        grid.width.saturating_sub(4).max(1),
        grid.height.saturating_sub(1),
    );
    let empty = app.view.is_none() && app.output.is_empty() && !app.active() && !app.turn_visible;
    let overlay = app.archive_confirm.is_some()
        || app.picker.is_some()
        || app.approval.is_some()
        || app.search.is_some()
        || app.view.is_some()
        || app.slash_open();
    let mut slot = sentinel_mascot::placement(screen, reading, empty, overlay, cell)?;
    if !empty {
        slot.area.x = screen
            .right()
            .saturating_sub(slot.area.width + readiness_label(app).len() as u16 + 3);
    }
    Some(slot)
}

/// Explicit file opt-in; never logs prompts, output, IPC payloads or arbitrary input.
/// A new file is required, and at most 64 records / 64 KiB are written per launch.
pub(super) struct Diagnostics {
    file: Option<std::fs::File>,
    records: usize,
    bytes: usize,
    last: String,
    first_key_recorded: bool,
}
impl Diagnostics {
    fn open() -> Self {
        use std::fs::OpenOptions;
        let file = std::env::var_os("SENTINEL_TUI_MASCOT_DIAGNOSTICS").and_then(|path| {
            let mut options = OpenOptions::new();
            options.write(true).create_new(true);
            #[cfg(unix)]
            {
                use std::os::unix::fs::OpenOptionsExt;
                options.mode(0o600);
            }
            options.open(path).ok()
        });
        Self {
            file,
            records: 0,
            bytes: 0,
            last: String::new(),
            first_key_recorded: false,
        }
    }
    fn write(&mut self, value: Value) {
        use std::io::Write;
        let keyboard = value["kind"] == "first_keyboard_event";
        // Reserve one final record and 2 KiB for input timing even after long animation.
        let record_limit = if keyboard { 64 } else { 63 };
        let byte_limit = if keyboard { 65536 } else { 63488 };
        if self.file.is_none() || self.records >= record_limit {
            return;
        }
        let line = format!("{value}\n");
        if self.bytes + line.len() > byte_limit {
            return;
        }
        if let Some(file) = &mut self.file
            && file
                .write_all(line.as_bytes())
                .and_then(|_| file.flush())
                .is_err()
        {
            self.file = None;
            return;
        }
        self.bytes += line.len();
        self.records += 1;
    }
    pub(super) fn boundary(
        &mut self,
        phase: &str,
        trace: &super::mascot_output::Trace,
        image_cells: Option<usize>,
        buffer_changes: Option<usize>,
    ) {
        self.write(
            json!({"kind":phase,"output":trace.snapshot(),"image_buffer_cells":image_cells,"terminal_buffer_changes":buffer_changes}),
        );
    }
    pub(super) fn first_key(&mut self, trace: &super::mascot_output::Trace) {
        if !self.first_key_recorded {
            self.first_key_recorded = true;
            self.boundary("first_keyboard_event", trace, None, None);
        }
    }
    pub(super) fn observe(&mut self, app: &App, player: &Player, screen: Rect, rendered: bool) {
        if self.file.is_none() || self.records >= 64 {
            return;
        }
        let place = slot(app, screen, player.cell_size());
        let record = json!({"kind":"presentation", "mode":player.mode().name(), "state":app.mascot.state.id(), "connected":app.connected,"empty_conversation":app.view.is_none() && app.output.is_empty() && !app.active() && !app.turn_visible, "screen":[screen.width,screen.height], "cell":player.cell_size(), "placement":place.map(|p| [p.area.x,p.area.y,p.area.width,p.area.height]), "resolution":place.map(|p|p.resolution), "loading":player.is_loading(), "cache_ready":player.frame_count()>0, "redraw_requested":rendered, "redraw_reason":player.redraw_reason, "frame_index":player.selected_frame(), "placement_refresh_pending":player.placement_refresh_pending(), "frames_loaded":player.frame_count(), "decoded_bytes":player.decoded_bytes,"encoded_cache_bytes":player.encoded_bytes(),"presentation_path":if player.protocol()=="kitty" {"direct_post_draw"} else {"upstream_widget"}, "load_ms":player.last_load_time.as_millis(), "status":player.status, "render_invoked":rendered && place.is_some(), "hidden_reason":if player.mode()==DisplayMode::Off {"off"} else if player.cell_size().is_none() {"no confirmed graphics/cell response"} else if place.is_none() {"overlay or insufficient layout space"} else {"visible slot"}});
        let key = record.to_string();
        if key != self.last {
            self.last = key;
            self.write(record);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn diagnostics_have_record_and_byte_bounds() {
        let path = std::env::temp_dir().join(format!(
            "sentinel-mascot-diagnostics-{}-{}.jsonl",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        let file = std::fs::OpenOptions::new()
            .write(true)
            .create_new(true)
            .open(&path)
            .unwrap();
        let mut diagnostics = Diagnostics {
            file: Some(file),
            records: 0,
            bytes: 0,
            last: String::new(),
            first_key_recorded: false,
        };
        diagnostics.write(json!({"oversized":"x".repeat(65536)}));
        assert_eq!(diagnostics.bytes, 0);
        for i in 0..100 {
            diagnostics.write(json!({"record":i}));
        }
        assert_eq!(diagnostics.records, 63);
        drop(diagnostics);
        let text = std::fs::read_to_string(&path).unwrap();
        assert_eq!(text.lines().count(), 63);
        assert!(text.len() <= 65536);
        std::fs::remove_file(path).unwrap();
    }
}
