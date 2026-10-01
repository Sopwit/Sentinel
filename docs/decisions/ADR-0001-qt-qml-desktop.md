# ADR-0001: Qt/QML desktop presentation

Status: Accepted

## Context

Sentinel is a native cross-platform desktop application with a C++ core and QML user interface.

## Decision

Use Qt/QML for Desktop presentation and expose QML-safe view models rather than core objects.

## Consequences

Business logic remains testable in C++ and platform UI work does not become core runtime behavior.
