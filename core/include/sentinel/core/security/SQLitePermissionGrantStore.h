#pragma once

#include "sentinel/core/security/IPermissionGrantStore.h"

namespace sentinel::core {

class SQLitePermissionGrantStore final : public IPermissionGrantStore {
public:
    explicit SQLitePermissionGrantStore(QString databasePath);
    bool load(QList<PersistentPermissionGrant>& grants) override;
    bool save(const QList<PersistentPermissionGrant>& grants) override;
    bool remove(const QString& id) override;
    bool clear() override;
    QString lastError() const override { return lastError_; }

private:
    bool initialize();
    QString databasePath_;
    QString lastError_;
    bool ready_ = false;
};

} // namespace sentinel::core
