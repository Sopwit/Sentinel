# Architecture Decision Records (ADR Index)

This index summarizes architectural decision records and phase progression milestones for Sentinel.

---

## Decision Log Summary

- **ADR-001: Qt 6 & C++20 Core Selection**  
  *Decision*: Selected Qt 6 and C++20 over Web/Electron to guarantee native desktop performance, minimal memory footprint, and native Linux/KDE Plasma integration.

- **ADR-002: SQLite Database Separation**  
  *Decision*: Separated key-value memory (`memory.sqlite3`), conversation history (`chat_history.sqlite3`), and Local RAG metadata (`local_rag.sqlite3`) into distinct database files rather than a single monolithic database.

- **ADR-003: Ollama Endpoint Boundary**  
  *Decision*: Established Ollama local health and discovery boundary (`http://127.0.0.1:11434`) as an isolated inspection layer, keeping model prompt execution disabled until explicit execution phases.

- **ADR-004: Liquid Glass UI Architecture**  
  *Decision*: Adopted a transparent matte liquid glass UI theme in QML, removing generic driver controls and tailoring the design for modern KDE Plasma desktop styling.
