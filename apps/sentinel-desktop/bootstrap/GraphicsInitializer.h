// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENTINEL_DESKTOP_GRAPHICSINITIALIZER_H
#define SENTINEL_DESKTOP_GRAPHICSINITIALIZER_H

class QQuickWindow;

namespace sentinel::desktop {

void configureGraphicsBackend();
void installGraphicsDiagnostics(QQuickWindow& window);

} // namespace sentinel::desktop

#endif // SENTINEL_DESKTOP_GRAPHICSINITIALIZER_H
