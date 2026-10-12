// SPDX-License-Identifier: GPL-3.0-or-later
#include "../helpers/ProviderCertificationRoot.h"
#include "sentinel/core/security/ExternalDirectoryGate.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <cstdio>
#include <cstdlib>
using namespace sentinel::core;
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    int checks = 0;
    auto check = [&](bool ok) { ++checks; if (!ok) { std::fprintf(stderr, "root/gate check %d failed\n", checks); std::exit(1); } };
    QTemporaryDir fixture, outside;
    check(fixture.isValid() && outside.isValid());
    const auto root = QFileInfo(fixture.path()).canonicalFilePath();
    check(validatedProviderCertificationRoot(root) == root);
    for (const auto& invalid : QStringList{QString(), " ", ".", "relative", root + "/missing", QDir::rootPath(), QDir::homePath()})
        check(validatedProviderCertificationRoot(invalid).isEmpty());
    QFile inside(root + "/inside.txt"); check(inside.open(QIODevice::WriteOnly)); inside.write("fixture"); inside.close();
    check(validatedProviderCertificationRoot(inside.fileName()).isEmpty());
    const auto other = QFileInfo(outside.path()).canonicalFilePath();
    QFile external(other + "/outside.txt"); check(external.open(QIODevice::WriteOnly)); external.write("outside"); external.close();
    const auto link = root + "/escape";
    const bool linked = QFile::link(other, link);
#ifdef Q_OS_UNIX
    check(linked);
#endif
    if (linked) check(validatedProviderCertificationRoot(link).isEmpty());
    ExternalDirectoryGate gate;
    gate.setWorkingDirectory(root);
    check(gate.isAccessAllowed(inside.fileName(), root, false));
    check(gate.isAccessAllowed(root + "/new.txt", root, true));
    check(!gate.isAccessAllowed(external.fileName(), root, false));
    check(!gate.isAccessAllowed(external.fileName(), root, true));
    check(!gate.isAccessAllowed(root + "/../" + QFileInfo(other).fileName() + "/outside.txt", root, false));
    if (linked) check(!gate.isAccessAllowed(link + "/outside.txt", root, false));
    check(!gate.isAccessAllowed(root + "/.env", root, false));
    if (gate.isAccessAllowed(inside.fileName(), root, false)) { QFile read(inside.fileName()); check(read.open(QIODevice::ReadOnly)); check(read.readAll() == "fixture"); }
    check(!gate.isAccessAllowed(external.fileName(), root, false));
    std::printf("PASS: %d explicit-root and production-gate checks\n", checks);
    return 0;
}
