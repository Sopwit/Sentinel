use super::*;
use ratatui::{Terminal, backend::TestBackend, layout::Rect};
use std::{
    path::PathBuf,
    time::{Duration, Instant},
};
fn root() -> PathBuf {
    PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../../assets/core-warden")
}
fn capabilities() -> Capabilities {
    Capabilities::from_responses(true, false, Some((8, 16)))
}
fn loaded(mode: DisplayMode, reduced: bool) -> Player {
    Player::new(Assets::load(&root()), capabilities(), mode, reduced)
}
fn area() -> Placement {
    Placement {
        area: Rect::new(10, 5, 12, 6),
        resolution: 96,
    }
}
fn await_cache(player: &mut Player, state: State) {
    let start = Instant::now();
    while player.frame_count() == 0 {
        assert!(
            start.elapsed() < Duration::from_secs(5),
            "{}",
            player.status
        );
        player.update(state, Some(area()), true, Instant::now());
        std::thread::sleep(Duration::from_millis(2));
    }
}
#[test]
fn manifest_and_all_required_frames_validate() {
    let assets = Assets::load(&root()).unwrap();
    for (state, n) in [
        (State::Idle, 16),
        (State::Thinking, 16),
        (State::ToolRunning, 12),
    ] {
        for size in [96, 128] {
            let clip = assets.clip(state, size, true).unwrap();
            assert_eq!(clip.paths.len(), n);
            assert_eq!(clip.fps, 8);
            assert!(clip.looping);
        }
    }
    for state in [
        State::Listening,
        State::Processing,
        State::Approval,
        State::Speaking,
        State::Success,
        State::Warning,
        State::Error,
    ] {
        let clip = assets.clip(state, 128, true).unwrap();
        assert_eq!(clip.paths.len(), 1);
        assert!(!clip.looping);
        assert_eq!(clip.resolution, 96);
    }
    assert!(assets.clip(State::Idle, 192, true).is_err());
}
#[test]
fn timing_uses_elapsed_time_and_wraps_without_tick_queue() {
    assert_eq!(frame_index(Duration::from_millis(124), 8, 16, true), 0);
    assert_eq!(frame_index(Duration::from_millis(125), 8, 16, true), 1);
    assert_eq!(frame_index(Duration::from_millis(1999), 8, 16, true), 15);
    assert_eq!(frame_index(Duration::from_millis(2000), 8, 16, true), 0);
    assert_eq!(frame_index(Duration::from_millis(1500), 8, 12, true), 0);
    assert_eq!(frame_index(Duration::from_secs(100_000), 8, 16, true), 0);
    assert_eq!(frame_index(Duration::from_secs(5), 8, 1, false), 0);
    assert_eq!(frame_index(Duration::MAX, 8, 0, true), 0);
}
#[test]
fn mapping_never_infers_tools_or_accepted_success_from_text() {
    let mut p = Projection::default();
    p.event("tool.running");
    assert_eq!(p.state, State::Idle);
    p.ready();
    p.model_pending();
    assert_eq!(p.state, State::Thinking);
    for event in ["output.delta", "tool.requested", "agent.activity"] {
        p.event(event);
        assert_eq!(p.state, State::Thinking);
    }
    p.event("tool.running");
    assert_eq!(p.state, State::ToolRunning);
    p.event("tool.result");
    assert_eq!(p.state, State::Thinking);
    p.event("approval.requested");
    assert_eq!(p.state, State::Approval);
    p.event("run.completed");
    assert_eq!(p.state, State::Idle);
    p.event("tool.result");
    assert_eq!(p.state, State::Idle);
    p.snapshot(&serde_json::json!({"state":"completed"}));
    assert_eq!(p.state, State::Idle);
    p.disconnected();
    p.event("run.started");
    assert_eq!(p.state, State::Error);
}
#[test]
fn capability_results_not_term_hints_control_graphics() {
    assert_eq!(
        Capabilities::from_responses(false, false, Some((8, 16))).protocol(),
        "text"
    );
    assert_eq!(
        Capabilities::from_responses(true, true, Some((8, 16))).protocol(),
        "kitty"
    );
    assert_eq!(
        Capabilities::from_responses(false, true, Some((8, 16))).protocol(),
        "sixel"
    );
    assert_eq!(
        Capabilities::from_responses(true, false, None).protocol(),
        "text"
    );
    assert_eq!(
        Capabilities::from_responses(true, false, Some((0, 16))).protocol(),
        "text"
    );
    for env in [
        Environment::default(),
        Environment {
            tty: true,
            ssh: true,
            ..Default::default()
        },
        Environment {
            tty: true,
            multiplexed: true,
            ..Default::default()
        },
        Environment {
            tty: true,
            ascii: true,
            ..Default::default()
        },
        Environment {
            tty: true,
            no_color: true,
            ..Default::default()
        },
    ] {
        assert!(!env.permits_probe());
        assert_eq!(detect(&env).protocol(), "text");
    }
}
#[test]
fn responsive_placement_and_overlay_priority() {
    for (w, h) in [(80, 24), (100, 30), (120, 40), (160, 48)] {
        let screen = Rect::new(0, 0, w, h);
        let reading = Rect::new(2, 4, w - 4, h - 10);
        let p = placement(screen, reading, true, false, (8, 16)).unwrap();
        assert!(reading.contains((p.area.x, p.area.y).into()));
        assert!(p.area.bottom() + 5 <= reading.bottom());
        assert!(
            (i32::from(p.area.x) * 2 + i32::from(p.area.width)
                - i32::from(reading.x) * 2
                - i32::from(reading.width))
            .abs()
                <= 1
        );
        assert!(placement(screen, reading, true, true, (8, 16)).is_none());
        assert_eq!(
            placement(screen, reading, false, false, (8, 16)).is_some(),
            w >= 120
        );
    }
    assert!(
        placement(
            Rect::new(0, 0, 79, 24),
            Rect::new(0, 0, 79, 14),
            true,
            false,
            (8, 16)
        )
        .is_none()
    );
}
#[test]
fn cache_is_one_state_and_resets_when_hidden_or_disabled() {
    let mut player = loaded(DisplayMode::Animated, false);
    await_cache(&mut player, State::Idle);
    assert_eq!(player.frame_count(), 16);
    assert_eq!(player.decoded_bytes, 16 * 96 * 96 * 4);
    let time = player.last_load_time;
    for _ in 0..10 {
        player.update(State::Idle, Some(area()), true, Instant::now());
        assert_eq!(player.last_load_time, time);
    }
    player.update(State::ToolRunning, Some(area()), true, Instant::now());
    assert_eq!(player.frame_count(), 0);
    await_cache(&mut player, State::ToolRunning);
    assert_eq!(player.frame_count(), 12);
    player.hide();
    assert_eq!(player.frame_count(), 0);
    assert_eq!(player.decoded_bytes, 0);
    assert!(player.next_wake(Instant::now()).is_none());
    player.set_mode(DisplayMode::Off);
    player.update(State::Idle, Some(area()), true, Instant::now());
    assert_eq!(player.frame_count(), 0);
}
#[test]
fn reduced_motion_static_and_disconnect_do_not_animate() {
    for (mode, reduced) in [(DisplayMode::Static, false), (DisplayMode::Animated, true)] {
        let mut player = loaded(mode, reduced);
        await_cache(&mut player, State::Thinking);
        assert_eq!(player.frame_count(), 1);
        assert!(player.next_wake(Instant::now()).is_none());
    }
    let mut player = loaded(DisplayMode::Animated, false);
    player.update(State::Thinking, Some(area()), false, Instant::now());
    let start = Instant::now();
    while player.frame_count() == 0 {
        assert!(start.elapsed() < Duration::from_secs(5));
        player.update(State::Thinking, Some(area()), false, Instant::now());
        std::thread::sleep(Duration::from_millis(2));
    }
    assert_eq!(player.frame_count(), 1);
    assert!(player.next_wake(Instant::now()).is_none());
}
#[test]
fn missing_assets_and_unsupported_terminals_render_readable_text() {
    for (assets, caps) in [
        (Assets::load(&root().join("missing")), capabilities()),
        (
            Assets::load(&root()),
            Capabilities::text("No confirmed protocol"),
        ),
    ] {
        let mut player = Player::new(assets, caps, DisplayMode::Auto, false);
        player.update(State::Thinking, Some(area()), true, Instant::now());
        let mut terminal = Terminal::new(TestBackend::new(80, 24)).unwrap();
        terminal
            .draw(|f| player.render(f, area().area, State::Thinking))
            .unwrap();
        let text = terminal
            .backend()
            .buffer()
            .content
            .iter()
            .map(|c| c.symbol())
            .collect::<String>();
        assert!(text.contains("Thinking"));
        assert_eq!(player.frame_count(), 0);
        assert!(player.next_wake(Instant::now()).is_none());
    }
}

#[test]
fn all_132_pngs_decode_as_expected_rgba() {
    for (state, count) in [("idle", 16), ("thinking", 16), ("tool_running", 12)] {
        for size in [96u16, 128, 192] {
            for index in 0..count {
                let path = root().join(format!("animations/{state}/{size}/frame_{index:03}.png"));
                let image = crate::assets::Assets::decode(&path, size).unwrap();
                assert_eq!(image.as_rgba8().unwrap().get_pixel(0, 0).0[3], 0);
            }
        }
    }
}
#[test]
fn resize_cancels_old_cache_and_reload_stays_bounded() {
    let mut player = loaded(DisplayMode::Animated, false);
    await_cache(&mut player, State::Idle);
    let resized = Placement {
        area: Rect::new(10, 5, 16, 8),
        resolution: 128,
    };
    player.update(State::Idle, Some(resized), true, Instant::now());
    assert_eq!(player.frame_count(), 0);
    let start = Instant::now();
    while player.frame_count() == 0 {
        assert!(start.elapsed() < Duration::from_secs(5));
        player.update(State::Idle, Some(resized), true, Instant::now());
        std::thread::sleep(Duration::from_millis(2));
    }
    assert_eq!(player.decoded_bytes, 16 * 128 * 128 * 4);
    player.update(State::Thinking, None, true, Instant::now());
    assert_eq!(player.frame_count(), 0);
    assert!(player.next_wake(Instant::now()).is_none());
}
#[test]
fn dropping_during_load_joins_worker_without_a_background_input_reader() {
    let start = Instant::now();
    let mut player = loaded(DisplayMode::Animated, false);
    player.update(State::Idle, Some(area()), true, Instant::now());
    drop(player);
    assert!(start.elapsed() < Duration::from_secs(2));
}
#[test]
fn strict_manifest_rejects_missing_files_and_path_escape() {
    let nonce = std::time::SystemTime::now()
        .duration_since(std::time::UNIX_EPOCH)
        .unwrap()
        .as_nanos();
    let dir = std::env::temp_dir().join(format!("sentinel-cw04-{}-{nonce}", std::process::id()));
    std::fs::create_dir(&dir).unwrap();
    struct Remove(PathBuf);
    impl Drop for Remove {
        fn drop(&mut self) {
            let _ = std::fs::remove_dir_all(&self.0);
        }
    }
    let _remove = Remove(dir.clone());
    std::fs::copy(
        root().join("state-contract.json"),
        dir.join("state-contract.json"),
    )
    .unwrap();
    std::fs::create_dir(dir.join("animations")).unwrap();
    let original = std::fs::read(root().join("animations/manifest.json")).unwrap();
    let mut manifest: serde_json::Value = serde_json::from_slice(&original).unwrap();
    manifest["states"]["idle"]["frames"]["96"][0] = serde_json::json!("../../outside.png");
    std::fs::write(
        dir.join("animations/manifest.json"),
        serde_json::to_vec(&manifest).unwrap(),
    )
    .unwrap();
    assert!(Assets::load(&dir).is_err());
    std::fs::write(dir.join("animations/manifest.json"), original).unwrap();
    assert!(Assets::load(&dir).is_err());
    assert!(crate::assets::Assets::decode(&root().join("state-contract.json"), 96).is_err());
}

#[test]
fn sixel_auto_is_static_and_requests_full_terminal_redraw_on_release() {
    let mut player = Player::new(
        Assets::load(&root()),
        Capabilities::from_responses(false, true, Some((8, 16))),
        DisplayMode::Auto,
        false,
    );
    await_cache(&mut player, State::Idle);
    assert_eq!(player.frame_count(), 1);
    assert!(player.next_wake(Instant::now()).is_none());
    player.hide();
    assert!(player.take_full_redraw());
    assert!(!player.take_full_redraw());
}

#[test]
fn offline_first_paint_places_directly_without_input_or_followup_timer() {
    for mode in [
        DisplayMode::Static,
        DisplayMode::Animated,
        DisplayMode::Auto,
    ] {
        for _warm_repeat in 0..2 {
            let mut player = Player::new(
                Assets::load(&root()),
                Capabilities::from_responses(true, false, Some((16, 34))),
                mode,
                false,
            );
            let place = placement(
                Rect::new(0, 0, 180, 50),
                Rect::new(4, 4, 172, 40),
                true,
                false,
                (16, 34),
            )
            .unwrap();
            let mut terminal = Terminal::new(TestBackend::new(180, 50)).unwrap();
            let start = Instant::now();
            let mut draws = 0;
            let mut output = Vec::new();
            // Same dirty contract as production, with no input/IPC events.
            loop {
                let dirty = player.update(State::Error, Some(place), false, Instant::now());
                if dirty {
                    terminal
                        .draw(|f| player.render(f, place.area, State::Error))
                        .unwrap();
                    player.present(&mut output).unwrap();
                    draws += 1;
                }
                if player.frame_count() == 1 && !player.placement_refresh_pending() {
                    break;
                }
                assert!(start.elapsed() < Duration::from_secs(5));
                std::thread::sleep(
                    player
                        .next_wake(Instant::now())
                        .unwrap_or(Duration::from_millis(50))
                        .min(Duration::from_millis(50)),
                );
            }
            assert!(draws >= 2);
            assert_eq!(player.redraw_reason, "cache_ready");
            assert!(player.next_wake(Instant::now()).is_none());
            let symbols: String = terminal
                .backend()
                .buffer()
                .content
                .iter()
                .map(|c| c.symbol())
                .collect();
            assert!(!symbols.contains('\u{10eeee}'));
            let wire = String::from_utf8(output.clone()).unwrap();
            assert!(wire.contains("a=t,"));
            assert!(wire.contains("a=p,"));
            assert!(!wire.contains("U=1"));
            let length = output.len();
            terminal
                .draw(|f| player.render(f, place.area, State::Error))
                .unwrap();
            player.present(&mut output).unwrap();
            assert_eq!(
                output.len(),
                length,
                "unchanged frame must not upload or place again"
            );
            let bytes = player.decoded_bytes;
            for _ in 0..10 {
                assert!(!player.update(State::Error, Some(place), false, Instant::now()));
            }
            assert_eq!(player.decoded_bytes, bytes);
        }
    }
}

#[test]
fn large_font_empty_placement_scales_above_header_and_keeps_labels_whole() {
    let screen = Rect::new(0, 0, 180, 50);
    let reading = Rect::new(4, 4, 172, 40);
    let empty = placement(screen, reading, true, false, (16, 34)).unwrap();
    let header = placement(screen, reading, false, false, (16, 34)).unwrap();
    assert_eq!((empty.area.width, empty.area.height), (16, 8));
    assert!(empty.area.height > header.area.height * 2);
    assert!(empty.area.bottom() + 6 <= reading.bottom());
    for width in [3, 6, 11, 16] {
        let mut player = Player::new(
            Err("missing".into()),
            capabilities(),
            DisplayMode::Static,
            false,
        );
        let mut terminal = Terminal::new(TestBackend::new(20, 5)).unwrap();
        terminal
            .draw(|f| player.render(f, Rect::new(0, 0, width, 3), State::Error))
            .unwrap();
        let row: String = (0..20)
            .map(|x| terminal.backend().buffer()[(x, 1)].symbol())
            .collect();
        if width >= 11 {
            assert!(row.contains("Unavailable"));
        } else {
            assert!(!row.contains("Unav"));
        }
    }
}

#[test]
fn terminal_safety_gates_and_cell_bounds_remain_conservative() {
    let allowed = Environment {
        tty: true,
        ..Environment::default()
    };
    assert!(allowed.permits_probe());
    assert!(
        Environment {
            reduced_motion: true,
            ..allowed.clone()
        }
        .permits_probe()
    );
    for blocked in [
        Environment {
            tty: false,
            ..allowed.clone()
        },
        Environment {
            ascii: true,
            ..allowed.clone()
        },
        Environment {
            no_color: true,
            ..allowed.clone()
        },
        Environment {
            ssh: true,
            ..allowed.clone()
        },
        Environment {
            multiplexed: true,
            ..allowed.clone()
        },
    ] {
        assert!(!blocked.permits_probe());
        assert_eq!(crate::detect(&blocked).protocol(), "text");
    }
    for cell in [
        None,
        Some((0, 16)),
        Some((8, 0)),
        Some((129, 16)),
        Some((8, 257)),
    ] {
        assert_eq!(
            Capabilities::from_responses(true, true, cell).protocol(),
            "text"
        );
    }
    assert_eq!(
        Capabilities::from_responses(false, true, Some((128, 256))).protocol(),
        "sixel"
    );
}

#[test]
fn fragmented_sixel_and_negative_graphics_responses_do_not_grant_kitty() {
    use ratatui_image::picker::cap_parser::{Parser, Response};
    let mut parser = Parser::new();
    let mut sixel = false;
    let mut kitty = false;
    let mut cell = None;
    for fragment in [
        "\x1b_Gi=31;",
        "ENOTSUP\x1b\\",
        "\x1b[?1;",
        "4c",
        "\x1b[6;16;",
        "8t",
    ] {
        for byte in fragment.bytes() {
            for response in parser.push(char::from(byte)) {
                match response {
                    Response::Kitty => kitty = true,
                    Response::Sixel => sixel = true,
                    Response::CellSize(size) => cell = size,
                    _ => {}
                }
            }
        }
    }
    assert!(!kitty);
    assert!(sixel);
    assert_eq!(
        Capabilities::from_responses(kitty, sixel, cell).protocol(),
        "sixel"
    );
}

#[test]
fn review_catalog_accepts_legacy_manifest_and_rejects_invalid_staged_timing() {
    use std::fs;
    let original: serde_json::Value =
        serde_json::from_slice(&fs::read(root().join("animations/manifest.json")).unwrap())
            .unwrap();
    assert_eq!(original["states"].as_object().unwrap().len(), 10);
    let temp = std::env::temp_dir().join(format!(
        "warden-review-{}-{}",
        std::process::id(),
        std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
            .as_nanos()
    ));
    struct Cleanup(PathBuf);
    impl Drop for Cleanup {
        fn drop(&mut self) {
            let _ = std::fs::remove_dir_all(&self.0);
        }
    }
    let _cleanup = Cleanup(temp.clone());
    fs::create_dir_all(temp.join("animations")).unwrap();
    fs::copy(
        root().join("state-contract.json"),
        temp.join("state-contract.json"),
    )
    .unwrap();
    for info in original["states"].as_object().unwrap().values() {
        for paths in info["frames"].as_object().unwrap().values() {
            for path in paths.as_array().unwrap() {
                let relative = path.as_str().unwrap();
                let target = temp.join(relative);
                fs::create_dir_all(target.parent().unwrap()).unwrap();
                fs::hard_link(root().join(relative), target).unwrap();
            }
        }
    }
    let manifest = temp.join("animations/manifest.json");
    fs::write(&manifest, serde_json::to_vec(&original).unwrap()).unwrap();
    assert!(Assets::load(&temp).is_ok());
    let mut invalid = original.clone();
    invalid["states"]["speaking"]["fps"] = serde_json::json!(16);
    fs::write(&manifest, serde_json::to_vec(&invalid).unwrap()).unwrap();
    assert!(Assets::load(&temp).is_err());
    let mut legacy = original;
    legacy["states"]
        .as_object_mut()
        .unwrap()
        .retain(|name, _| ["idle", "thinking", "tool_running"].contains(&name.as_str()));
    fs::write(&manifest, serde_json::to_vec(&legacy).unwrap()).unwrap();
    assert!(Assets::load(&temp).is_ok());
}
