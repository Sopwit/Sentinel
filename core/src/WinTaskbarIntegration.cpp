#include "sentinel/core/WinTaskbarIntegration.h"

#include <QCoreApplication>
#include <QDebug>

#if defined(Q_OS_WIN)
// clang-format off
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
// clang-format on
#endif

namespace sentinel::core {

#if defined(Q_OS_WIN)

namespace {

// ── Helper: Get HRESULT description ──────────────────────────────────────

QString hresultToString(HRESULT hr) {
    wchar_t* msg = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   nullptr, hr, 0, reinterpret_cast<wchar_t*>(&msg), 0, nullptr);
    QString result = msg ? QString::fromWCharArray(msg) : QStringLiteral("(unknown)");
    LocalFree(msg);
    return result.trimmed();
}

// ── COM GUIDs (defined locally to avoid linker dependency on uuid.lib) ───

static const GUID CLSID_TaskbarList_Local = {0x56fdf344, 0xfd6d, 0x11d0,
    {0x95, 0x8a, 0x00, 0x60, 0x97, 0xc9, 0x90, 0x4b}};
static const GUID CLSID_DestinationList_Local = {0x77f10cf0, 0x3db5, 0x4966,
    {0xb5, 0x20, 0xb7, 0xc5, 0x4f, 0xd3, 0x5e, 0xd6}};
static const GUID CLSID_EnumerableCollection_Local = {0x2d3468c1, 0x36a7, 0x43b6,
    {0xac, 0x24, 0xd3, 0xf0, 0x2f, 0xd9, 0x60, 0x7a}};
static const GUID IID_ITaskbarList3_Local = {0xea1afb91, 0x9e28, 0x4b86,
    {0x90, 0xe9, 0x9e, 0x9f, 0x8a, 0x5e, 0xef, 0xaf}};
static const GUID IID_ICustomDestinationList_Local = {0x6332debf, 0x87b5, 0x4670,
    {0x90, 0x0c, 0x5d, 0xb3, 0x4b, 0x82, 0x06, 0x42}};
static const GUID IID_IObjectCollection_Local = {0x5632b1a4, 0xe38a, 0x400a,
    {0x92, 0x8a, 0xd4, 0xcd, 0x63, 0x23, 0x02, 0x95}};

} // namespace

// ── Private data ──────────────────────────────────────────────────────────

struct WinTaskbarIntegration::Private {
    HWND hwnd = nullptr;
    bool comInitialized = false;

    IUnknown* taskbarList = nullptr;
    IUnknown* destList = nullptr;

    bool initCOM() {
        if (comInitialized) return true;
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE || hr == S_FALSE) {
            comInitialized = true;
            return true;
        }
        return false;
    }

    void ensureTaskbarList() {
        if (taskbarList) return;
        if (!hwnd || !initCOM()) return;

        IUnknown* unk = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_TaskbarList_Local, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_IUnknown, reinterpret_cast<void**>(&unk));
        if (FAILED(hr) || !unk) {
            qWarning().noquote()
                << "WinTaskbar: CoCreateInstance(TaskbarList) failed:" << hresultToString(hr);
            return;
        }

        ITaskbarList3* tlb3 = nullptr;
        hr = unk->QueryInterface(IID_ITaskbarList3_Local, reinterpret_cast<void**>(&tlb3));
        if (SUCCEEDED(hr) && tlb3) {
            tlb3->Release();
            taskbarList = unk;
            ITaskbarList* base = nullptr;
            if (SUCCEEDED(unk->QueryInterface(IID_ITaskbarList, reinterpret_cast<void**>(&base)))) {
                base->HrInit();
                base->Release();
            }
        } else {
            unk->Release();
            qWarning().noquote() << "WinTaskbar: ITaskbarList3 not supported";
        }
    }

    void ensureDestList() {
        if (destList) return;
        if (!hwnd || !initCOM()) return;

        HRESULT hr = CoCreateInstance(CLSID_DestinationList_Local, nullptr, CLSCTX_INPROC_SERVER,
                                       IID_IUnknown, reinterpret_cast<void**>(&destList));
        if (FAILED(hr) || !destList) {
            qWarning().noquote()
                << "WinTaskbar: CoCreateInstance(DestinationList) failed:" << hresultToString(hr);
            destList = nullptr;
        }
    }

    void releaseCOM() {
        if (taskbarList) { taskbarList->Release(); taskbarList = nullptr; }
        if (destList) { destList->Release(); destList = nullptr; }
        if (comInitialized) { CoUninitialize(); comInitialized = false; }
    }

    static IShellLinkW* createShellLink(const QString& title, const QString& appPath,
                                        const QString& arguments, const QString& iconPath,
                                        int iconIndex) {
        IShellLinkW* link = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                      IID_IShellLinkW, reinterpret_cast<void**>(&link));
        if (FAILED(hr) || !link) return nullptr;

        link->SetPath(appPath.toStdWString().c_str());
        if (!arguments.isEmpty())
            link->SetArguments(arguments.toStdWString().c_str());
        if (!title.isEmpty())
            link->SetDescription(title.toStdWString().c_str());
        if (!iconPath.isEmpty())
            link->SetIconLocation(iconPath.toStdWString().c_str(), iconIndex);

        return link;
    }

    IObjectCollection* createCollection() {
        IObjectCollection* collection = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_EnumerableCollection_Local, nullptr,
                                       CLSCTX_INPROC_SERVER, IID_IObjectCollection_Local,
                                       reinterpret_cast<void**>(&collection));
        if (FAILED(hr)) {
            qWarning().noquote()
                << "WinTaskbar: CoCreateInstance(Collection) failed:" << hresultToString(hr);
        }
        return collection;
    }

    bool appendCategory(const QString& name, IObjectCollection* collection) {
        if (!destList || !collection) return false;

        ICustomDestinationList* cd = nullptr;
        HRESULT hr = destList->QueryInterface(IID_ICustomDestinationList_Local,
                                               reinterpret_cast<void**>(&cd));
        if (FAILED(hr) || !cd) return false;

        cd->SetAppID(L"dev.sentinel.Sentinel");
        UINT maxSlots = 0;
        IObjectArray* removed = nullptr;
        hr = cd->BeginList(&maxSlots, IID_IObjectArray, reinterpret_cast<void**>(&removed));
        if (SUCCEEDED(hr)) {
            hr = cd->AppendCategory(name.toStdWString().c_str(), collection);
            if (FAILED(hr))
                qWarning().noquote() << "WinTaskbar: AppendCategory failed:" << hresultToString(hr);
            cd->CommitList();
        } else {
            qWarning().noquote() << "WinTaskbar: BeginList failed:" << hresultToString(hr);
        }
        if (removed) removed->Release();
        cd->Release();
        return SUCCEEDED(hr);
    }
};

// ── Constructor / Destructor ──────────────────────────────────────────────

WinTaskbarIntegration::WinTaskbarIntegration(QObject* parent)
    : QObject(parent), d(std::make_unique<Private>()) {}

WinTaskbarIntegration::~WinTaskbarIntegration() {
    d->releaseCOM();
}

void WinTaskbarIntegration::setWindowHandle(quintptr hwnd) {
    d->hwnd = reinterpret_cast<HWND>(hwnd);
}

// ── Taskbar Progress ──────────────────────────────────────────────────────

namespace {
ITaskbarList3* queryTaskbarList(IUnknown* unk) {
    if (!unk) return nullptr;
    ITaskbarList3* tlb3 = nullptr;
    if (SUCCEEDED(unk->QueryInterface(IID_ITaskbarList3_Local, reinterpret_cast<void**>(&tlb3))))
        return tlb3;
    return nullptr;
}
} // namespace

void WinTaskbarIntegration::setProgressValue(quint64 completed, quint64 total) {
    d->ensureTaskbarList();
    if (auto* tlb3 = queryTaskbarList(d->taskbarList)) {
        tlb3->SetProgressValue(d->hwnd, completed, total);
        tlb3->Release();
    }
}

void WinTaskbarIntegration::setProgressIndeterminate(bool indeterminate) {
    d->ensureTaskbarList();
    if (auto* tlb3 = queryTaskbarList(d->taskbarList)) {
        tlb3->SetProgressState(d->hwnd, indeterminate ? TBPF_INDETERMINATE : TBPF_NORMAL);
        tlb3->Release();
    }
}

void WinTaskbarIntegration::setProgressPaused(bool paused) {
    d->ensureTaskbarList();
    if (auto* tlb3 = queryTaskbarList(d->taskbarList)) {
        tlb3->SetProgressState(d->hwnd, paused ? TBPF_PAUSED : TBPF_NORMAL);
        tlb3->Release();
    }
}

void WinTaskbarIntegration::setProgressError(bool error) {
    d->ensureTaskbarList();
    if (auto* tlb3 = queryTaskbarList(d->taskbarList)) {
        tlb3->SetProgressState(d->hwnd, error ? TBPF_ERROR : TBPF_NORMAL);
        tlb3->Release();
    }
}

void WinTaskbarIntegration::clearProgress() {
    d->ensureTaskbarList();
    if (auto* tlb3 = queryTaskbarList(d->taskbarList)) {
        tlb3->SetProgressState(d->hwnd, TBPF_NOPROGRESS);
        tlb3->Release();
    }
}

// ── JumpList ──────────────────────────────────────────────────────────────

void WinTaskbarIntegration::setUserTasks(const QList<WinTaskbarTask>& tasks) {
    if (!d->hwnd || tasks.isEmpty()) return;
    d->ensureDestList();
    if (!d->destList) return;

    auto* collection = d->createCollection();
    if (!collection) return;

    const QString appPath = QCoreApplication::applicationFilePath();
    for (const auto& task : tasks) {
        auto* link = Private::createShellLink(
            task.title, task.appPath.isEmpty() ? appPath : task.appPath,
            task.arguments, task.iconPath, task.iconIndex);
        if (link) {
            collection->AddObject(link);
            link->Release();
        }
    }

    d->appendCategory(QStringLiteral("Tasks"), collection);
    collection->Release();
}

void WinTaskbarIntegration::addRecentItem(const QString& filePath, const QString& title) {
    if (!d->hwnd) return;
    auto* link = Private::createShellLink(title, filePath, {}, {}, 0);
    if (!link) return;
    SHAddToRecentDocs(SHARD_LINK, link);
    link->Release();
}

void WinTaskbarIntegration::setRecentItems(const QList<WinTaskbarJumpListItem>& items) {
    if (!d->hwnd || items.isEmpty()) return;
    d->ensureDestList();
    if (!d->destList) return;

    auto* collection = d->createCollection();
    if (!collection) return;

    for (const auto& item : items) {
        auto* link = Private::createShellLink(
            item.title, item.filePath, {}, item.iconPath, item.iconIndex);
        if (link) {
            collection->AddObject(link);
            link->Release();
        }
    }

    d->appendCategory(QStringLiteral("Recent"), collection);
    collection->Release();
}

void WinTaskbarIntegration::clearJumpList() {
    if (!d->destList) return;

    ICustomDestinationList* cd = nullptr;
    if (SUCCEEDED(d->destList->QueryInterface(IID_ICustomDestinationList_Local,
                                               reinterpret_cast<void**>(&cd)))) {
        cd->DeleteList(L"dev.sentinel.Sentinel");
        cd->Release();
    }
}

#else // !Q_OS_WIN

// ── Stub implementation for non-Windows ──────────────────────────────────

struct WinTaskbarIntegration::Private {};

WinTaskbarIntegration::WinTaskbarIntegration(QObject* parent) : QObject(parent), d(std::make_unique<Private>()) {}
WinTaskbarIntegration::~WinTaskbarIntegration() = default;
void WinTaskbarIntegration::setWindowHandle(quintptr) {}
void WinTaskbarIntegration::setProgressValue(quint64, quint64) {}
void WinTaskbarIntegration::setProgressIndeterminate(bool) {}
void WinTaskbarIntegration::setProgressPaused(bool) {}
void WinTaskbarIntegration::setProgressError(bool) {}
void WinTaskbarIntegration::clearProgress() {}
void WinTaskbarIntegration::setUserTasks(const QList<WinTaskbarTask>&) {}
void WinTaskbarIntegration::addRecentItem(const QString&, const QString&) {}
void WinTaskbarIntegration::setRecentItems(const QList<WinTaskbarJumpListItem>&) {}
void WinTaskbarIntegration::clearJumpList() {}

#endif // Q_OS_WIN

} // namespace sentinel::core
