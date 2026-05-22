//
// Created by liuyics15 on 2026/5/19.
//

#ifndef OHOS_SLAB_H
#define OHOS_SLAB_H

#include <mutex>
#include <vector>
#include <atomic>
#include <optional>

namespace OHOS {
template <typename T>
class SlabMT {
public:
    static constexpr uint64_t InvalidId = std::numeric_limits<uint64_t>::max();

    SlabMT(): groupId_(0) {}
    explicit SlabMT(uint32_t groupId): groupId_(groupId) {}

    uint64_t Append(T data)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (last_ >= 0) {
            auto& item = items_[last_];
            auto lastPrev = item.prev;
            const auto result = last_;
            item.data = std::move(data);
            item.prev = -1;
            last_ = lastPrev;
            return ToId(result);
        }
        auto result = items_.size();
        if (result >= 0x1'0000'0000) {
            return InvalidId;
        }
        items_.emplace_back(std::move(data));
        return ToId(result);
    }

    std::optional<T> Remove(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t index = -1;
        if (!ToIndex(id, index)) {
            return std::nullopt;
        }
        auto& item = items_[index];
        if (!item.data.has_value()) {
            return std::nullopt;
        }
        auto result = std::move(item.data.value());
        item.data = std::nullopt;
        item.prev = last_;
        last_ = index;
        return std::move(result);
    }

    using OptReference = std::conditional_t<std::is_copy_assignable_v<T>, std::optional<T>, std::optional<T&>>;

    OptReference Get(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        uint32_t index = -1;
        if (!ToIndex(id, index)) {
            return std::nullopt;
        }
        return items_[index].data;
    }

    bool Contains(uint64_t id)
    {
        uint32_t index = -1;
        if (!ToIndex(id, index)) {
            return false;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= items_.size()) {
            return false;
        }
        return items_[index].data.has_value();
    }

    // for debug, not thread safe.
    size_t Size() const
    {
        size_t size = 0;
        for (const auto& item : items_) {
            if (item.data.has_value()) {
                size++;
            }
        }
        return size;
    }

private:
    template <typename >
    friend class MultSlabMT;

    uint64_t ToId(uint32_t index) const
    {
        return (static_cast<uint64_t>(groupId_) << 32) | index;
    }

    bool ToIndex(uint64_t id, uint32_t& index) const
    {
        if ((id >> 32) != groupId_) {
            return false;
        }
        index = id & 0xFFFF'FFFF;
        return true;
    }

    struct SlabItem {
        std::optional<T> data;
        int64_t prev;
        SlabItem(T data): data(std::move(data)), prev(-1) {}
    };
    std::mutex mutex_;
    uint32_t groupId_;
    int64_t last_ = -1;
    std::vector<SlabItem> items_;
};

template <typename T>
class MultSlabMT {
public:
    MultSlabMT()
    {
        for (uint32_t i = 0;i < std::size(items_);i++) {
            items_[i].groupId_ = i;
        }
    }

    uint64_t Append(T data)
    {
        auto groupId = (++nextGroupId_) & 0x0F;
        return items_[groupId].Append(std::move(data));
    }

    std::optional<T> Remove(uint64_t id)
    {
        auto groupId = id >> 32;
        if (groupId < std::size(items_)) {
            return items_[groupId].Remove(id);
        }
        return std::nullopt;
    }

    bool Contains(uint64_t id)
    {
        auto groupId = id >> 32;
        if (groupId >= std::size(items_)) {
            return false;
        }
        return items_[groupId].Contains(id);
    }

    using OptReference = typename SlabMT<T>::OptReference;

    OptReference Get(uint64_t id)
    {
        auto groupId = id >> 32;
        if (groupId < std::size(items_)) {
            return items_[groupId].Get(id);
        }
        return std::nullopt;
    }

    // for debugger, not thread safe
    size_t Size() const
    {
        size_t size = 0;
        for (const auto& item : items_) {
            size += item.Size();
        }
        return size;
    }
private:
    SlabMT<T> items_[16];
    std::atomic<uint8_t> nextGroupId_ {0};
};
}

#endif //OHOS_SLAB_H
