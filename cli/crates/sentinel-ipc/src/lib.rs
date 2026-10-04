//! Bounded local-user IPC client. Disconnect never implies cancellation.
#[rustfmt::skip]
pub mod contract;
pub const APP_VERSION: &str = env!("SENTINEL_APP_VERSION");
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
#[cfg(unix)]
use std::os::unix::net::UnixStream;
use std::{
    collections::VecDeque,
    io::{BufRead, BufReader, Write},
    path::{Path, PathBuf},
};

#[derive(Debug, Serialize, Deserialize, PartialEq, Eq)]
#[serde(rename_all = "lowercase")]
pub enum MessageType {
    Request,
    Response,
    Event,
    Error,
}
#[derive(Debug, Serialize, Deserialize)]
pub struct Version {
    pub major: usize,
    pub minor: usize,
}
#[derive(Debug, Serialize, Deserialize)]
pub struct Envelope {
    pub version: Version,
    #[serde(rename = "type")]
    pub kind: MessageType,
    pub id: String,
    pub name: String,
    pub payload: Value,
}
#[derive(Debug)]
pub enum Error {
    Unavailable(String),
    Disconnected,
    Protocol(String),
    Incompatible,
    Remote(String),
}
impl std::fmt::Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{self:?}")
    }
}
impl std::error::Error for Error {}
impl Error {
    pub fn exit_code(&self) -> i32 {
        match self {
            Self::Unavailable(_) | Self::Disconnected => 3,
            Self::Incompatible => 4,
            _ => 1,
        }
    }
}
pub fn default_socket_path() -> PathBuf {
    PathBuf::from(std::env::var_os("HOME").unwrap_or_default()).join(".sentinel/run/daemon.sock")
}
pub fn decode(bytes: &[u8]) -> Result<Envelope, Error> {
    if bytes.len() > contract::MAX_FRAME_BYTES {
        return Err(Error::Protocol("oversized frame".into()));
    }
    let message: Envelope =
        serde_json::from_slice(bytes).map_err(|e| Error::Protocol(e.to_string()))?;
    if message.version.major != contract::MAJOR {
        return Err(Error::Incompatible);
    }
    if !message.payload.is_object() {
        return Err(Error::Protocol("payload must be an object".into()));
    }
    if message.kind == MessageType::Event {
        if contract::EVENTS.contains(&message.name.as_str()) {
            let _: contract::EventPayload =
                serde_json::from_value(json!({"name":message.name,"payload":message.payload}))
                    .map_err(|e| Error::Protocol(e.to_string()))?;
        } else if message.version.minor.saturating_sub(contract::MINOR) == 0 {
            return Err(Error::Protocol("unknown event".into()));
        }
    } else if message.kind == MessageType::Response {
        let _: contract::ResponsePayload =
            serde_json::from_value(json!({"name":message.name,"payload":message.payload}))
                .map_err(|e| Error::Protocol(e.to_string()))?;
    }
    Ok(message)
}
#[cfg(unix)]
pub struct Client {
    reader: BufReader<UnixStream>,
    writer: UnixStream,
    sequence: u64,
    events: VecDeque<Envelope>,
}
#[cfg(unix)]
impl Client {
    pub fn connect(path: &Path, identity: &str) -> Result<Self, Error> {
        let stream = UnixStream::connect(path).map_err(|e| Error::Unavailable(e.to_string()))?;
        let writer = stream
            .try_clone()
            .map_err(|e| Error::Unavailable(e.to_string()))?;
        let mut client = Self {
            reader: BufReader::new(stream),
            writer,
            sequence: 0,
            events: VecDeque::new(),
        };
        client.request(
            "hello",
            json!({"client_id":identity,"major":contract::MAJOR,"minor":contract::MINOR,"capabilities":contract::CAPABILITIES}),
        )?;
        Ok(client)
    }
    pub fn set_read_timeout(&self, timeout: Option<std::time::Duration>) -> Result<(), Error> {
        self.reader
            .get_ref()
            .set_read_timeout(timeout)
            .map_err(|e| Error::Protocol(e.to_string()))
    }
    pub fn request(&mut self, name: &str, payload: Value) -> Result<Value, Error> {
        let _: contract::RequestPayload =
            serde_json::from_value(json!({"name":name,"payload":payload}))
                .map_err(|e| Error::Protocol(e.to_string()))?;
        self.sequence += 1;
        let id = self.sequence.to_string();
        let message = Envelope {
            version: Version {
                major: contract::MAJOR,
                minor: contract::MINOR,
            },
            kind: MessageType::Request,
            id: id.clone(),
            name: name.into(),
            payload,
        };
        let mut frame = serde_json::to_vec(&message).map_err(|e| Error::Protocol(e.to_string()))?;
        if frame.len() + 1 > contract::MAX_FRAME_BYTES {
            return Err(Error::Protocol("oversized request".into()));
        }
        frame.push(b'\n');
        self.writer
            .write_all(&frame)
            .map_err(|_| Error::Disconnected)?;
        loop {
            let message = self.read()?;
            if message.kind == MessageType::Event {
                if self.events.len() >= 256 {
                    return Err(Error::Protocol("event backlog exceeded".into()));
                }
                self.events.push_back(message);
                continue;
            }
            if message.id != id {
                return Err(Error::Protocol("response correlation mismatch".into()));
            }
            if message.kind == MessageType::Error {
                let code = message.payload["code"].as_str().unwrap_or("unknown");
                return Err(if code == "incompatible-major" {
                    Error::Incompatible
                } else {
                    Error::Remote(code.into())
                });
            }
            if message.kind != MessageType::Response || message.name != name {
                return Err(Error::Protocol("invalid response".into()));
            }
            return Ok(message.payload);
        }
    }
    fn read(&mut self) -> Result<Envelope, Error> {
        let mut bytes = Vec::new();
        loop {
            let available = self.reader.fill_buf().map_err(|_| Error::Disconnected)?;
            if available.is_empty() {
                return Err(Error::Disconnected);
            }
            let end = available.iter().position(|b| *b == b'\n');
            let count = end.map_or(available.len(), |n| n + 1);
            if bytes.len() + count > contract::MAX_FRAME_BYTES {
                return Err(Error::Protocol("oversized frame".into()));
            }
            bytes.extend_from_slice(&available[..count]);
            self.reader.consume(count);
            if end.is_some() {
                return decode(&bytes);
            }
        }
    }
    pub fn next_event(&mut self) -> Result<Envelope, Error> {
        if let Some(event) = self.events.pop_front() {
            return Ok(event);
        }
        let event = self.read()?;
        if event.kind == MessageType::Error {
            return Err(Error::Remote(
                event.payload["code"]
                    .as_str()
                    .unwrap_or("protocol error")
                    .into(),
            ));
        }
        if event.kind != MessageType::Event {
            return Err(Error::Protocol("expected event".into()));
        }
        Ok(event)
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn rejects_malformed() {
        assert!(decode(b"invalid").is_err());
    }
    #[test]
    fn rejects_oversized() {
        assert!(decode(&vec![0; contract::MAX_FRAME_BYTES + 1]).is_err());
    }
    #[test]
    fn event_decoding() {
        let e=decode(br#"{"version":{"major":1,"minor":0},"type":"event","id":"","name":"run.cancelled","payload":{"run_id":"r","session_id":"s","text":"","state":"cancelled"}}"#).unwrap();
        assert_eq!(e.kind, MessageType::Event);
    }
    #[test]
    fn incompatible() {
        assert!(matches!(decode(br#"{"version":{"major":2,"minor":0},"type":"error","id":"1","name":"ProtocolError","payload":{}}"#),Err(Error::Incompatible)));
    }
    #[test]
    fn exit_codes() {
        assert_eq!(Error::Disconnected.exit_code(), 3);
        assert_eq!(Error::Incompatible.exit_code(), 4);
    }
    #[cfg(unix)]
    #[test]
    fn missing_daemon() {
        assert!(matches!(
            Client::connect(Path::new("/nonexistent/sentinel.sock"), "test"),
            Err(Error::Unavailable(_))
        ));
    }
}

#[cfg(not(unix))]
pub struct Client;
#[cfg(not(unix))]
impl Client {
    pub fn connect(_: &Path, _: &str) -> Result<Self, Error> {
        Err(Error::Unavailable(
            "Windows named-pipe transport is not implemented yet".into(),
        ))
    }
    pub fn request(&mut self, _: &str, _: Value) -> Result<Value, Error> {
        Err(Error::Disconnected)
    }
    pub fn next_event(&mut self) -> Result<Envelope, Error> {
        Err(Error::Disconnected)
    }
}

#[cfg(all(test, unix))]
mod transport_tests {
    use super::*;
    use std::os::unix::net::UnixListener;
    #[test]
    fn handshake_correlation_and_event_queue() {
        let directory =
            std::env::temp_dir().join(format!("sentinel-rust-ipc-{}", std::process::id()));
        std::fs::create_dir_all(&directory).unwrap();
        let path = directory.join("mock.sock");
        let listener = UnixListener::bind(&path).unwrap();
        let handle = std::thread::spawn(move || {
            let (mut stream, _) = listener.accept().unwrap();
            let mut reader = BufReader::new(stream.try_clone().unwrap());
            for _ in 0..2 {
                let mut line = String::new();
                reader.read_line(&mut line).unwrap();
                let request: Envelope = serde_json::from_str(&line).unwrap();
                if request.name == "daemon.status" {
                    writeln!(stream,"{}",json!({"version":{"major":1,"minor":0},"type":"event","id":"","name":"output.delta","payload":{"text":"test","run_id":"r","session_id":"s"}})).unwrap();
                }
                writeln!(stream,"{}",json!({"version":{"major":1,"minor":0},"type":"response","id":request.id,"name":request.name,"payload":if request.name == "hello" { json!({"major":1,"minor":0,"daemon_version":APP_VERSION,"capabilities":[]}) } else { json!({"running":true,"daemon_version":APP_VERSION,"uptime_ms":0,"active_runs":0,"sessions":0}) }})).unwrap();
            }
        });
        let mut client = Client::connect(&path, "rust-test").unwrap();
        assert_eq!(
            client.request("daemon.status", json!({})).unwrap()["running"],
            true
        );
        assert_eq!(client.next_event().unwrap().payload["text"], "test");
        assert!(matches!(client.next_event(), Err(Error::Disconnected)));
        handle.join().unwrap();
        std::fs::remove_dir_all(directory).unwrap();
    }
    #[test]
    fn typed_request_invalid_enum_or_fields() {
        assert!(
            serde_json::from_value::<contract::RequestPayload>(
                json!({"name":"invented","payload":{}})
            )
            .is_err()
        );
        assert!(serde_json::from_value::<contract::RequestPayload>(json!({"name":"approval.respond","payload":{"run_id":"x","approval_id":"y","allow":"yes"}})).is_err());
    }
}
