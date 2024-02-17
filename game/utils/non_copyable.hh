#pragma once

struct NonCopyable {
    NonCopyable(NonCopyable const&) = delete;

    NonCopyable& operator=(NonCopyable const&) = delete;

    NonCopyable() = default;
};
