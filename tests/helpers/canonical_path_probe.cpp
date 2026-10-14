// SPDX-License-Identifier: GPL-3.0-or-later
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>

// Exercise Qt's canonicalization inside the actual restricted child process.
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (argc != 2) {
        return 1;
    }
    const auto canonical = QFileInfo(QString::fromLocal8Bit(argv[1])).canonicalFilePath();
    if (canonical.isEmpty()) {
        return 2;
    }
    QFile file(canonical);
    if (!file.open(QIODevice::ReadOnly)) {
        return 3;
    }
    return file.readAll() == QByteArray("CANONICAL_OK") ? 0 : 4;
}
