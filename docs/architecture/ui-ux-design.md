# UI/UX Design System: Liquid Glass

Sentinel implements a custom **Transparent / Matte Liquid Glass** design system in QML.

---

## Design System Tokens

### Color Palette (Tailored Dark / Light HSL)
- **Background Base**: Deep translucent dark `#0d1117` with 85% opacity.
- **Glass Panel Surface**: `#161b22` with 60% opacity and subtle backdrop blur filter.
- **Accent Primary**: Vibrant cyan/teal `#38bdf8` for active selections, highlights, and focus states.
- **Border & Dividers**: Thin translucent borders `#30363d` (1px) for visual structure.

### Typography
- Inter, Roboto, or System UI font family.
- Clear hierarchy: Headers (20-24px), Body (14px), Captions & Metadata (12px).

---

## Component Architecture

- **QML Shell**: Responsive main window frame supporting header controls, sidebar workspace navigation, and command palette overlays.
- **Micro-Animations**: Smooth opacity and scale transitions (150ms - 250ms) for interactive elements and modal popovers.
