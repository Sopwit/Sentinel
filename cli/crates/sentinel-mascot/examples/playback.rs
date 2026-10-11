//! Isolated deterministic presentation fixture, not a live Agent or the production TUI.
use crossterm::{
    event::{self, Event, KeyCode, KeyEventKind},
    execute,
    terminal::{EnterAlternateScreen, LeaveAlternateScreen, disable_raw_mode, enable_raw_mode},
};
use ratatui::{
    Terminal,
    backend::CrosstermBackend,
    layout::{Alignment, Rect},
    widgets::{Block, Borders, Clear, Paragraph},
};
use sentinel_mascot::{Assets, DisplayMode, Environment, Player, Projection, detect, placement};
use std::{
    io,
    path::PathBuf,
    time::{Duration, Instant},
};
struct Restore;
impl Drop for Restore {
    fn drop(&mut self) {
        let _ = disable_raw_mode();
        let _ = execute!(io::stdout(), LeaveAlternateScreen);
    }
}
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = std::env::args().collect::<Vec<_>>();
    let value = |name: &str| {
        args.iter()
            .position(|x| x == name)
            .and_then(|i| args.get(i + 1))
            .cloned()
    };
    let root = value("--assets").map(PathBuf::from).unwrap_or_else(|| {
        PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../../assets/core-warden")
    });
    let assets = Assets::load(&root);
    let env = Environment::current();
    enable_raw_mode()?;
    let _restore = Restore;
    execute!(io::stdout(), EnterAlternateScreen)?;
    let caps = if DisplayMode::from_env() == DisplayMode::Off {
        sentinel_mascot::Capabilities::text("Display off; no probe")
    } else {
        detect(&env)
    };
    let protocol = caps.protocol().to_owned();
    let probe_ms = caps.probe_elapsed.as_secs_f64() * 1000.;
    let mut terminal = Terminal::new(CrosstermBackend::new(io::stdout()))?;
    let mut player = Player::new(assets, caps, DisplayMode::from_env(), env.reduced_motion);
    let mut projection = Projection::default();
    projection.ready();
    let mut overlay = false;
    let mut populated = false;
    let mut reduced = env.reduced_motion;
    let mut draft = String::new();
    let mut scroll = 0u16;
    let start = Instant::now();
    let seconds = value("--seconds")
        .and_then(|s| s.parse::<f64>().ok())
        .unwrap_or(f64::INFINITY);
    let autoplay = args.iter().any(|s| s == "--fixture");
    let mut phase = usize::MAX;
    let mut dirty = true;
    let mut draws = 0;
    let mut input_count = 0;
    let mut max_input_us = 0;
    let mut max_decoded = 0;
    let mut state_log = Vec::new();
    let mut observed = None;
    let mut cache_log = Vec::new();
    let mut observed_cache = None;
    loop {
        let now = Instant::now();
        if start.elapsed().as_secs_f64() >= seconds {
            break;
        }
        if autoplay {
            let next = (start.elapsed().as_secs() / 3) as usize;
            if next != phase {
                phase = next;
                overlay = false;
                populated = false;
                projection.ready();
                match next % 8 {
                    0 => {}
                    1 => projection.model_pending(),
                    2 => {
                        projection.event("run.started");
                        projection.event("tool.running")
                    }
                    3 => {
                        projection.event("run.started");
                        projection.event("approval.requested");
                        overlay = true;
                    }
                    4 => projection.disconnected(),
                    5 => {
                        populated = true;
                        {
                            projection.event("run.started");
                            projection.event("tool.running")
                        };
                    }
                    6 => {
                        reduced = true;
                        player.set_reduced_motion(true);
                    }
                    _ => {
                        reduced = false;
                        player.set_reduced_motion(false);
                    }
                }

                dirty = true;
            }
        }
        let snapshot = (
            projection.state,
            projection.connected,
            overlay,
            populated,
            reduced,
            player.mode(),
        );
        if observed != Some(snapshot) {
            state_log.push(serde_json::json!({"seconds":start.elapsed().as_secs_f64(),"state":projection.state.id(),"connected":projection.connected,"overlay":overlay,"populated":populated,"reduced_motion":reduced,"mode":player.mode().name()}));
            observed = Some(snapshot);
        }
        let screen = terminal.size()?;
        let screen = Rect::new(0, 0, screen.width, screen.height);
        let reading = Rect::new(
            2,
            4,
            screen.width.saturating_sub(4),
            screen.height.saturating_sub(10),
        );
        let image_area = player
            .cell_size()
            .filter(|_| player.mode() != DisplayMode::Off)
            .and_then(|cell| placement(screen, reading, !populated, overlay, cell));
        dirty |= player.update(projection.state, image_area, projection.connected, now);
        max_decoded = max_decoded.max(player.decoded_bytes);
        let cached = (projection.state, player.frame_count());
        if observed_cache != Some(cached) {
            cache_log.push(serde_json::json!({"seconds":start.elapsed().as_secs_f64(),"state":projection.state.id(),"frames":player.frame_count()}));
            observed_cache = Some(cached);
        }
        if player.take_full_redraw() {
            terminal.clear()?;
            dirty = true;
        }
        if dirty {
            terminal.draw(|f| {
                let a=f.area();f.render_widget(Paragraph::new("SENTINEL / CW-04 isolated playback fixture"),Rect::new(1,0,a.width.saturating_sub(2),1));
                f.render_widget(Paragraph::new(format!("{} · protocol {} · {} · {}",projection.state.label(),player.protocol(),player.mode().name(),if reduced {"reduced motion"} else {"motion allowed"})),Rect::new(1,2,a.width.saturating_sub(2),1));
                if let Some(p)=image_area {player.render(f,p.area,projection.state);}
                let welcome_y=image_area.map_or(reading.y+reading.height/2,|p|p.area.bottom()+1);
                if populated {
                    f.render_widget(Paragraph::new("User\n  Deterministic fixture; no model request.\nAssistant\n  Conversation width is preserved.\n\nTool activity comes only from the explicit fixture event.\nPgUp/PgDn and overlays remain keyboard responsive.").scroll((scroll,0)),reading);
                } else {
                    f.render_widget(Paragraph::new(if image_area.is_none() {if player.mode()==DisplayMode::Off {"Start a new conversation".into()} else {format!("[ S ] {}\nStart a new conversation",projection.state.label())}} else {"Start a new conversation".into()}).alignment(Alignment::Center),Rect::new(reading.x,welcome_y,reading.width,2));
                }
                f.render_widget(Paragraph::new(format!("> {draft}")).block(Block::default().borders(Borders::ALL)),Rect::new(2,a.height.saturating_sub(5),a.width.saturating_sub(4),3));
                f.render_widget(Paragraph::new("1 Idle · 2 Thinking · 3 Tool · 4 Offline · m Overlay · p Transcript · s Static · a Animated · o Off · u Auto · r Reduced · q Exit"),Rect::new(1,a.height.saturating_sub(1),a.width.saturating_sub(2),1));
                if overlay {
                    let popup=Rect::new(a.width/8,a.height/3,a.width*3/4,5);f.render_widget(Clear,popup);f.render_widget(Paragraph::new("Approval / palette fixture\nKeyboard focus stays here. Esc closes.\nNo graphics are rendered behind this overlay.").block(Block::default().borders(Borders::ALL)),popup);
                }
            })?;
            player.present(terminal.backend_mut())?;
            draws += 1;
            dirty = false;
        }
        let timeout = player
            .next_wake(Instant::now())
            .unwrap_or(Duration::from_secs(1))
            .min(Duration::from_secs_f64(
                (seconds - start.elapsed().as_secs_f64()).max(0.001),
            ));
        if sentinel_mascot::poll_terminal_event(timeout)? {
            let input_start = Instant::now();
            match event::read()? {
                Event::Key(key) if key.kind == KeyEventKind::Press => {
                    input_count += 1;
                    dirty = true;
                    if key.code == KeyCode::Char('q') {
                        break;
                    }
                    if overlay {
                        if key.code == KeyCode::Esc {
                            overlay = false;
                        }
                    } else {
                        match key.code {
                            KeyCode::Char('1') => projection.ready(),
                            KeyCode::Char('2') => {
                                projection.ready();
                                projection.model_pending();
                            }
                            KeyCode::Char('3') => {
                                projection.ready();
                                {
                                    projection.event("run.started");
                                    projection.event("tool.running")
                                };
                            }
                            KeyCode::Char('4') => projection.disconnected(),
                            KeyCode::Char('m') => overlay = true,
                            KeyCode::Char('p') => populated = !populated,
                            KeyCode::Char('s') => player.set_mode(DisplayMode::Static),
                            KeyCode::Char('a') => player.set_mode(DisplayMode::Animated),
                            KeyCode::Char('o') => player.set_mode(DisplayMode::Off),
                            KeyCode::Char('u') => player.set_mode(DisplayMode::Auto),
                            KeyCode::Char('r') => {
                                reduced = !reduced;
                                player.set_reduced_motion(reduced);
                            }
                            KeyCode::PageUp => scroll = scroll.saturating_add(2),
                            KeyCode::PageDown => scroll = scroll.saturating_sub(2),
                            KeyCode::Backspace => {
                                draft.pop();
                            }
                            KeyCode::Char(c) => draft.push(c),
                            _ => {}
                        }
                    }
                }
                Event::Resize(_, _) => dirty = true,
                _ => {}
            }
            max_input_us = max_input_us.max(input_start.elapsed().as_micros());
        }
    }
    let metrics = serde_json::json!({"fixture_only":true,"protocol":protocol,"probe_ms":probe_ms,"wall_seconds":start.elapsed().as_secs_f64(),"draws":draws,"inputs":input_count,"max_input_dispatch_us":max_input_us,"peak_retained_source_rgba_bytes":max_decoded,"last_clip_load_ms":player.last_load_time.as_secs_f64()*1000.,"state_log":state_log,"cache_log":cache_log,"asset_status":player.status});
    player.hide();
    drop(player);
    drop(terminal);
    drop(_restore);
    if let Some(path) = value("--metrics") {
        std::fs::write(path, serde_json::to_vec_pretty(&metrics)?)?;
    }
    eprintln!("{metrics}");
    Ok(())
}
