#pragma once

struct Size {
    Size(float width = 0.f, float height = 0.f);
    Size(Size const&) = default;
    Size(Size&&) = default;

    Size& operator=(Size const&) = default;
    Size& operator=(Size&&) = default;

    Size operator*(float) const;
    Size operator/(float) const;

    float getWidth() const;
    float getHeight() const;

    float width = 0.f;
    float height = 0.f;
};
