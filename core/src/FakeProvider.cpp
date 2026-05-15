#include "sentinel/core/FakeProvider.h"

namespace sentinel::core {

QString FakeProvider::name() const {
    return QStringLiteral("FakeProvider");
}

QString FakeProvider::generateReply(const QString& prompt) {
    Q_UNUSED(prompt);
    return QStringLiteral("Sentinel Core online. Provider bridge is active.");
}

} // namespace sentinel::core
