# Sentinel Roadmap

## Phase 1: Desktop Alpha Foundation

Create the native Qt/QML desktop shell, core interfaces, fake provider bridge, runtime memory store, mode switching, and documentation.

## Phase 2: Real Provider Integration

Add provider adapters for real AI APIs behind `IProvider`. Keep provider selection local and explicit.

## Phase 3: SQLite Memory

Replace runtime-only memory with a SQLite-backed implementation of `IMemoryStore`.

## Phase 4: Voice System

Add local-first voice input and output behind clean interfaces. Avoid coupling voice code to QML.

## Phase 5: Automation Engine

Introduce a safe local automation layer with permissions, auditability, and explicit user control.

## Phase 6: Edge Device Prototype

Prototype deployment on Raspberry Pi or Jetson-class devices with constrained runtime profiles.

## Phase 7: Wearable Ecosystem

Explore companion wearable clients after the desktop and edge foundations are stable.
