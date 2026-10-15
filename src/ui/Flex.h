#pragma once

#include <cstddef>
#include <vector>

#include <raylib.h>

enum class FlexDir { Row, Column };

struct FlexItem {
    float fixed = 0.0f;
    float grow = 0.0f;
    Rectangle rect{};
};

struct Flex {
    FlexDir dir = FlexDir::Column;
    Rectangle bounds{};
    float gap = 8.0f;
    float padX = 0.0f;
    float padY = 0.0f;
    std::vector<FlexItem> items;

    void clear() { items.clear(); }

    void addFixed(float size) { items.push_back(FlexItem{ size, 0.0f, {} }); }

    void addGrow(float weight = 1.0f) {
        if (weight < 0.0f) {
            weight = 0.0f;
        }
        items.push_back(FlexItem{ 0.0f, weight, {} });
    }

    size_t size() const { return items.size(); }

    Rectangle at(size_t i) const { return items[i].rect; }

    float cursor() const {
        if (items.empty()) {
            return (dir == FlexDir::Column) ? bounds.y + padY : bounds.x + padX;
        }
        const Rectangle &r = items.back().rect;
        return (dir == FlexDir::Column) ? r.y + r.height + gap : r.x + r.width + gap;
    }

    void layout() {
        float ix = bounds.x + padX;
        float iy = bounds.y + padY;
        float iw = bounds.width - padX * 2.0f;
        float ih = bounds.height - padY * 2.0f;
        if (iw < 0.0f) {
            iw = 0.0f;
        }
        if (ih < 0.0f) {
            ih = 0.0f;
        }
        float totalWeight = 0.0f;
        float used = 0.0f;
        for (auto &it : items) {
            if (it.grow > 0.0f) {
                totalWeight += it.grow;
            } else {
                used += it.fixed;
            }
        }
        float gaps = items.empty() ? 0.0f : gap * (float)(items.size() - 1);
        float span = (dir == FlexDir::Column) ? ih : iw;
        float rest = span - used - gaps;
        if (rest < 0.0f) {
            rest = 0.0f;
        }
        float pos = (dir == FlexDir::Column) ? iy : ix;
        for (auto &it : items) {
            float s = it.fixed;
            if (it.grow > 0.0f && totalWeight > 0.0f) {
                s = rest * it.grow / totalWeight;
            }
            if (dir == FlexDir::Column) {
                it.rect = Rectangle{ ix, pos, iw, s };
                pos += s + gap;
            } else {
                it.rect = Rectangle{ pos, iy, s, ih };
                pos += s + gap;
            }
        }
    }
};
