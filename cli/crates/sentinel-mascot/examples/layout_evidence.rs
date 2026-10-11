//! Ratatui TestBackend evidence. Never label this as a physical terminal screenshot.
use ratatui::{
    Terminal,
    backend::TestBackend,
    layout::{Alignment, Rect},
    widgets::{Block, Borders, Clear, Paragraph},
};
use sentinel_mascot::{Assets, Capabilities, DisplayMode, Player, State, placement};
use std::{
    path::PathBuf,
    time::{Duration, Instant},
};
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let root = PathBuf::from(env!("CARGO_MANIFEST_DIR")).join("../../..");
    let assets = root.join("assets/core-warden");
    let out = root.join("docs/reviews/core-warden-cw04/layouts");
    std::fs::create_dir_all(&out)?;
    for (width, height) in [(80, 24), (100, 30), (120, 40), (160, 48), (60, 20)] {
        for scenario in [
            "idle",
            "thinking",
            "tool_running",
            "populated",
            "overlay",
            "fallback",
        ] {
            let state = match scenario {
                "thinking" => State::Thinking,
                "tool_running" => State::ToolRunning,
                _ => State::Idle,
            };
            let caps = if scenario == "fallback" {
                Capabilities::text("Unsupported terminal")
            } else {
                Capabilities::from_responses(true, false, Some((8, 16)))
            };
            let mut player = Player::new(Assets::load(&assets), caps, DisplayMode::Auto, false);
            let screen = Rect::new(0, 0, width, height);
            let reading = Rect::new(2, 4, width - 4, height - 10);
            let area = player.cell_size().and_then(|cell| {
                placement(
                    screen,
                    reading,
                    scenario != "populated",
                    scenario == "overlay",
                    cell,
                )
            });
            player.update(state, area, true, Instant::now());
            if area.is_some() {
                let start = Instant::now();
                while player.frame_count() == 0 {
                    if start.elapsed() > Duration::from_secs(5) {
                        return Err(player.status.clone().into());
                    }
                    std::thread::sleep(Duration::from_millis(2));
                    player.update(state, area, true, Instant::now());
                }
            }
            let mut terminal = Terminal::new(TestBackend::new(width, height))?;
            terminal.draw(|f| {
    f.render_widget(Paragraph::new("SENTINEL / isolated CW-04 fixture"),Rect::new(1,0,width-2,1));
    f.render_widget(Paragraph::new(format!("{} | {} | Auto",state.label(),player.protocol())),Rect::new(1,2,width-2,1));
    if let Some(p)=area {player.render(f,p.area,state);}
    let y=area.map_or(reading.y+reading.height/2,|p|p.area.bottom()+1);
    if scenario=="populated" {f.render_widget(Paragraph::new("User\n  Inspect the mascot playback.\nAssistant\n  This is a deterministic fixture, not model output.\n  Conversation width remains available.\n\nExecution detail\n  Only explicit tool.running events select Tool Running."),reading);}
    else {f.render_widget(Paragraph::new(if area.is_none() {format!("[ S ] {}\nStart a new conversation",state.label())} else {"Start a new conversation".into()}).alignment(Alignment::Center),Rect::new(reading.x,y,reading.width,2));}
    f.render_widget(Paragraph::new("> Keyboard draft remains available").block(Block::default().borders(Borders::ALL)),Rect::new(2,height-5,width-4,3));
    f.render_widget(Paragraph::new("Ratatui TestBackend reconstruction - not Ghostty acceptance"),Rect::new(1,height-1,width-2,1));
    if scenario=="overlay" {let p=Rect::new(width/8,height/3,width*3/4,5);f.render_widget(Clear,p);f.render_widget(Paragraph::new("Approval / command palette fixture\nKeyboard focus stays here; no graphic behind it.\nEsc closes.").block(Block::default().borders(Borders::ALL)),p);}
   })?;
            let buffer = terminal.backend().buffer();
            let mut rows = Vec::new();
            for y in 0..height {
                let mut row = Vec::new();
                for x in 0..width {
                    let symbol = buffer[(x, y)].symbol();
                    row.push(
                        if symbol.contains('\x1b') || symbol.contains('\u{10eeee}') {
                            " ".to_owned()
                        } else {
                            symbol.to_owned()
                        },
                    );
                }
                rows.push(row);
            }
            let image=area.map(|p|serde_json::json!({"x":p.area.x,"y":p.area.y,"resolution":p.resolution,"width":p.area.width*8,"height":p.area.height*16,"frame":format!("animations/{}/{}/frame_000.png",state.id(),p.resolution)}));
            let data = serde_json::json!({"width":width,"height":height,"cell_pixels":[8,16],"scenario":scenario,"rows":rows,"image":image,"evidence_kind":"Ratatui TestBackend reconstruction; no physical Ghostty acceptance"});
            std::fs::write(
                out.join(format!("{scenario}-{width}x{height}.json")),
                serde_json::to_vec(&data)?,
            )?;
        }
    }
    Ok(())
}
