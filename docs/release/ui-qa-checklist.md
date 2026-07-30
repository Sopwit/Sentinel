# UI / UX Quality Assurance Checklist

Checklist for visual inspection and interactive testing of Sentinel's QML presentation layer.

---

## 1. Visual Styling & Themes

- [ ] Verify Liquid Glass translucent panel background renders cleanly without graphical glitches.
- [ ] Confirm dark mode palette contrast meets WCAG AA standards for legibility.
- [ ] Confirm light mode theme toggle works smoothly without lingering color artifacts.
- [ ] Verify window controls (minimize, maximize, close) conform to active desktop environment standards.

---

## 2. Interactive Navigation & Micro-Animations

- [ ] Press `Ctrl+K` to open Command Palette; verify smooth popover animation (150ms).
- [ ] Ensure Command Palette focus is placed automatically in search input field.
- [ ] Verify keybindings for workspace navigation (`Ctrl+W`) and Settings (`Ctrl+,`).
- [ ] Confirm focus indicators are visible when navigating UI elements via `Tab` / `Shift+Tab`.
