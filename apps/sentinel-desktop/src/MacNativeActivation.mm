// SPDX-License-Identifier: GPL-3.0-or-later
#import <AppKit/AppKit.h>

namespace sentinel::desktop {
// QWindow::requestActivate makes the NSWindow key but does not activate NSApp.
// Invoke only for an explicit tray/shortcut action, without showing the main window.
void activateMacQuickPanelApplication() {
    if (@available(macOS 14.0, *)) {
        [NSApp activate];
    } else {
        [NSApp activateIgnoringOtherApps:YES];
    }
}
} // namespace sentinel::desktop
