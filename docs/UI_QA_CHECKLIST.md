# UI QA Checklist

Phase 5.3 records manual visual QA expectations for the current Qt/QML desktop shell. It does not
add automated UI driving, provider execution, tool execution, or platform integration.

## Screen States

- Dashboard: metric cards, read-only agent runtime section, current posture, and chat panel.
- Chat: initial local transcript, typed prompt, sent message, local echo response, and clear-chat
  confirmation dialog.
- Memory: empty list, key/value entry row, stored memory row, and compact stacked form.
- Settings: theme/profile fields, local data maintenance statuses, and clear confirmation dialogs.
- Sidebar/Header/StatusBar: navigation selection, hover/focus states, mode selector, compact
  status truncation, and provider summary.

## Width Checks

- Compact: about 760 px wide. Sidebar should stay usable, labels may elide, forms stack, and no
  control text should overlap.
- Normal: about 1000 px wide. Main navigation, header, pages, and footer should remain balanced.
- Wide: 1200 px or wider. Dashboard overview and chat should sit side by side without excessive
  crowding.

## Platform Notes

- Fedora KDE Plasma: verify ordinary window resizing, keyboard focus rings, and Basic Qt controls
  remain readable under the default desktop compositor.
- macOS: verify bundled app launch, title/header sizing, and text elision behave consistently.
- Avoid assessing future assistant visuals in this checklist; particle systems, assistant-face
  rendering, and custom OpenGL/Vulkan rendering remain out of scope.

## Pass Criteria

- Shared spacing, card padding, button height, input height, and status rows look consistent.
- Compact layouts wrap or elide instead of overlapping.
- Dashboard runtime visibility remains read-only.
- No UI element implies real execution, approval, networking, provider/model execution, plugin
  loading, or filesystem/system action capability.
