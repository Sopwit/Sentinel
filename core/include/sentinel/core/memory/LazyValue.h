// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
#include <memory>

namespace sentinel::core {

template <typename T> class LazyValue {
public:
    using Factory = std::function<T()>;

    explicit LazyValue(Factory factory) : m_factory(factory) {}

    T& value() {
        if (!m_cached) {
            m_value = m_factory();
            m_cached = true;
        }
        return m_value;
    }

    const T& value() const {
        if (!m_cached) {
            m_value = m_factory();
            m_cached = true;
        }
        return m_value;
    }

    bool isComputed() const {
        return m_cached;
    }
    void invalidate() {
        m_cached = false;
    }
    explicit operator bool() const {
        return m_cached;
    }

private:
    Factory m_factory;
    mutable T m_value{};
    mutable bool m_cached{false};
};

} // namespace sentinel::core
