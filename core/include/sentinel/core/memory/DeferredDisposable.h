// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <optional>

namespace sentinel::core {

template<typename T>
class DeferredDisposable {
public:
    using Deleter = std::function<void(T*)>;

    DeferredDisposable() = default;
    DeferredDisposable(T* ptr, Deleter deleter) : m_ptr(ptr), m_deleter(deleter) {}

    ~DeferredDisposable() { dispose(); }

    DeferredDisposable(const DeferredDisposable&) = delete;
    DeferredDisposable& operator=(const DeferredDisposable&) = delete;

    DeferredDisposable(DeferredDisposable&& other) noexcept
        : m_ptr(other.m_ptr), m_deleter(other.m_deleter) {
        other.m_ptr = nullptr;
    }

    DeferredDisposable& operator=(DeferredDisposable&& other) noexcept {
        if (this != &other) {
            dispose();
            m_ptr = other.m_ptr;
            m_deleter = other.m_deleter;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    T* get() const { return m_ptr; }
    T* release() { T* p = m_ptr; m_ptr = nullptr; return p; }
    void dispose() {
        if (m_ptr && m_deleter) { m_deleter(m_ptr); m_ptr = nullptr; }
    }
    bool isDisposed() const { return m_ptr == nullptr; }
    explicit operator bool() const { return m_ptr != nullptr; }

private:
    T* m_ptr{nullptr};
    Deleter m_deleter;
};

} // namespace sentinel::core
