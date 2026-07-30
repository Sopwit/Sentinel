#ifndef SENTINEL_DESKTOP_VIEWMODELS_RAGVIEWMODEL_H
#define SENTINEL_DESKTOP_VIEWMODELS_RAGVIEWMODEL_H

#include <QObject>
#include <QString>

namespace sentinel::desktop::viewmodels {

class RagViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isIndexing READ isIndexing NOTIFY isIndexingChanged)
    Q_PROPERTY(int indexedDocumentCount READ indexedDocumentCount NOTIFY indexedDocumentCountChanged)

public:
    explicit RagViewModel(QObject* parent = nullptr);
    ~RagViewModel() override = default;

    [[nodiscard]] bool isIndexing() const { return m_isIndexing; }
    void setIsIndexing(bool indexing);

    [[nodiscard]] int indexedDocumentCount() const { return m_indexedDocumentCount; }
    void setIndexedDocumentCount(int count);

public Q_SLOTS:
    void rebuildIndex();

Q_SIGNALS:
    void isIndexingChanged();
    void indexedDocumentCountChanged();

private:
    bool m_isIndexing{false};
    int m_indexedDocumentCount{0};
};

} // namespace sentinel::desktop::viewmodels

#endif // SENTINEL_DESKTOP_VIEWMODELS_RAGVIEWMODEL_H
