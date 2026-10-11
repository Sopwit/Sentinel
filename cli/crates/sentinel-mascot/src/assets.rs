use crate::State;
use serde::Deserialize;
use std::io::Read;
use std::{
    collections::HashMap,
    fs,
    path::{Component, Path, PathBuf},
};

#[derive(Clone, Deserialize)]
struct Mapping {
    animation: Option<String>,
    static_frame: String,
    label: String,
}
#[derive(Deserialize)]
struct Contract {
    schema_version: u32,
    asset_id: String,
    animation_manifest: String,
    states: HashMap<String, Mapping>,
}
#[derive(Clone, Deserialize)]
struct Animation {
    frame_count: usize,
    fps: u32,
    #[serde(rename = "loop")]
    looping: bool,
    frames: HashMap<String, Vec<String>>,
}
#[derive(Deserialize)]
struct Manifest {
    canvas: [u16; 2],
    resolutions: Vec<u16>,
    asset_id: String,
    states: HashMap<String, Animation>,
}
#[derive(Clone)]
pub struct Assets {
    root: PathBuf,
    states: HashMap<String, Mapping>,
    animations: HashMap<String, Animation>,
}
#[derive(Clone, Debug)]
pub struct Clip {
    pub paths: Vec<PathBuf>,
    pub fps: u32,
    pub looping: bool,
    pub resolution: u16,
}
fn read_json<T: serde::de::DeserializeOwned>(path: &Path, limit: u64) -> Result<T, String> {
    if fs::metadata(path).map_err(|e| e.to_string())?.len() > limit {
        return Err("Mascot JSON exceeds bound".into());
    }
    let mut bytes = Vec::new();
    fs::File::open(path)
        .map_err(|e| e.to_string())?
        .take(limit + 1)
        .read_to_end(&mut bytes)
        .map_err(|e| e.to_string())?;
    if bytes.len() as u64 > limit {
        return Err("Oversized JSON".into());
    }
    serde_json::from_slice(&bytes).map_err(|e| e.to_string())
}
impl Assets {
    /// Explicit installation asset root; no recursive search or arbitrary frame paths.
    pub fn load(root: &Path) -> Result<Self, String> {
        let root = root.canonicalize().map_err(|e| e.to_string())?;
        let contract: Contract = read_json(&root.join("state-contract.json"), 16384)?;
        if contract.schema_version != 1 || contract.asset_id != "sentinel_core_warden" {
            return Err("Unsupported mascot contract".into());
        }
        let path = Self::resolve(&root, &contract.animation_manifest)?;
        let manifest: Manifest = read_json(&path, 131072)?;
        if manifest.asset_id != contract.asset_id
            || !matches!(manifest.states.len(), 3 | 10)
            || manifest.canvas != [192, 192]
            || manifest.resolutions != [96, 128, 192]
        {
            return Err("Unexpected CW-03A manifest".into());
        }
        let mut required = vec![
            ("idle", 16, true),
            ("thinking", 16, true),
            ("tool_running", 12, true),
        ];
        // CW03B assets are staged for review. This extends catalog validation
        // only; the contract's seven existing static mappings remain unchanged.
        if manifest.states.len() == 10 {
            required.extend([
                ("listening", 16, true),
                ("processing", 12, true),
                ("approval", 24, true),
                ("speaking", 16, true),
                ("success", 13, false),
                ("warning", 16, true),
                ("error", 13, false),
            ]);
        }
        for (id, count, looping) in required {
            let animation = manifest.states.get(id).ok_or("Missing animation")?;
            if animation.frame_count != count || animation.fps != 8 || animation.looping != looping
            {
                return Err("Invalid animation timing".into());
            }
            for size in [96, 128, 192] {
                let paths = animation
                    .frames
                    .get(&size.to_string())
                    .ok_or("Missing resolution")?;
                if paths.len() != count {
                    return Err("Incorrect frame count".into());
                }
                for (index, path) in paths.iter().enumerate() {
                    if *path != format!("animations/{id}/{size}/frame_{index:03}.png") {
                        return Err("Noncanonical frame path".into());
                    }
                    let full = Self::resolve(&root, path)?;
                    let metadata = fs::metadata(full).map_err(|e| e.to_string())?;
                    if !metadata.is_file() || metadata.len() == 0 || metadata.len() > 524288 {
                        return Err("Invalid frame file".into());
                    }
                }
            }
        }
        if contract.states.len() != 10 {
            return Err("Expected ten shared states".into());
        }
        for state in State::ALL {
            let mapping = contract
                .states
                .get(state.id())
                .ok_or("Missing shared state")?;
            if mapping.static_frame != "animations/idle/96/frame_000.png" {
                return Err("Noncanonical static fallback".into());
            }
            if mapping.label.is_empty() {
                return Err("Missing state label".into());
            }
            Self::resolve(&root, &mapping.static_frame)?;
            let expected = match state {
                State::Idle | State::Thinking | State::ToolRunning => Some(state.id()),
                _ => None,
            };
            if mapping.animation.as_deref() != expected {
                return Err("Invalid animation/fallback mapping".into());
            }
        }
        Ok(Self {
            root,
            states: contract.states,
            animations: manifest.states,
        })
    }
    fn resolve(root: &Path, relative: &str) -> Result<PathBuf, String> {
        let path = Path::new(relative);
        if path
            .components()
            .any(|c| !matches!(c, Component::Normal(_)))
        {
            return Err("Frame path must stay within asset root".into());
        }
        let full = root.join(path).canonicalize().map_err(|e| e.to_string())?;
        if !full.starts_with(root) {
            return Err("Frame symlink escapes asset root".into());
        }
        Ok(full)
    }
    pub fn label(&self, state: State) -> &str {
        &self.states[state.id()].label
    }
    pub fn clip(&self, state: State, resolution: u16, animated: bool) -> Result<Clip, String> {
        if ![96, 128].contains(&resolution) {
            return Err("Playback supports bounded 96/128px variants".into());
        }
        let mapping = &self.states[state.id()];
        if animated && let Some(id) = &mapping.animation {
            let animation = &self.animations[id];
            return Ok(Clip {
                paths: animation.frames[&resolution.to_string()]
                    .iter()
                    .map(|p| self.root.join(p))
                    .collect(),
                fps: animation.fps,
                looping: animation.looping,
                resolution,
            });
        }
        Ok(Clip {
            paths: vec![Self::resolve(&self.root, &mapping.static_frame)?],
            fps: 8,
            looping: false,
            resolution: 96,
        })
    }
    /// Metadata validation checks all files; PNG decoding happens only for the selected clip.
    pub(crate) fn decode(path: &Path, resolution: u16) -> Result<image::DynamicImage, String> {
        let mut bytes = Vec::new();
        fs::File::open(path)
            .map_err(|e| e.to_string())?
            .take(524289)
            .read_to_end(&mut bytes)
            .map_err(|e| e.to_string())?;
        if bytes.len() > 524288 {
            return Err("Oversized PNG".into());
        }
        let reader =
            image::ImageReader::with_format(std::io::Cursor::new(&bytes), image::ImageFormat::Png);
        if reader.into_dimensions().map_err(|e| e.to_string())?
            != (resolution.into(), resolution.into())
        {
            return Err("Wrong PNG dimensions".into());
        }
        let image = image::load_from_memory_with_format(&bytes, image::ImageFormat::Png)
            .map_err(|e| e.to_string())?;
        if image.color() != image::ColorType::Rgba8 {
            return Err("Expected RGBA8 PNG".into());
        }
        Ok(image)
    }
}
