# ADR-0004: Tool execution gateway

Status: Accepted

## Context

Built-in, plugin, and MCP tools need consistent validation and user-authority checks.

## Decision

Route tool invocations through `ToolExecutionGateway` using authoritative `ToolDescriptor` contracts.

## Consequences

Schema validation, approval, permission, sandbox, hooks, and result handling are shared rather than reimplemented by each tool source.
