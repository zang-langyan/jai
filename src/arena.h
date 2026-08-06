#ifndef JAI_ARENA_H
#define JAI_ARENA_H
#include <cstdlib>
#include <cstring>
#include <algorithm>

class Arena {
public:
    static constexpr size_t DEFAULT_BLOCK_SIZE = 4096;
    static constexpr size_t DEFAULT_ALIGN = alignof(std::max_align_t);

    Arena(size_t initialSize = DEFAULT_BLOCK_SIZE)
        : _head(nullptr), _tail(nullptr), _current(nullptr),
          _offset(0), _capacity(0), _blockSize(initialSize) {
        newBlock(initialSize);
    }

    ~Arena() {
        Block* block = _head;
        while (block) {
            Block* next = block->next;
            ::free(block->data);
            ::free(block);
            block = next;
        }
    }

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;
    Arena(Arena&&) = delete;
    Arena& operator=(Arena&&) = delete;

    template<typename T, typename... Args>
    T* New(Args&&... args) {
        void* mem = alloc(sizeof(T), alignof(T));
        return ::new(mem) T(std::forward<Args>(args)...);
    }

    void* alloc(size_t size, size_t alignment = DEFAULT_ALIGN) {
        size_t alignedOffset = alignUp(_offset, alignment);
        if (alignedOffset + size > _capacity) {
            size_t newBlockSize = std::max(_blockSize, size + alignment);
            newBlock(newBlockSize);
            alignedOffset = _offset;
        }
        void* ptr = _current->data + alignedOffset;
        _offset = alignedOffset + size;
        return ptr;
    }

    char* strdup(const char* src, size_t len) {
        char* p = static_cast<char*>(alloc(len + 1));
        std::memcpy(p, src, len);
        p[len] = '\0';
        return p;
    }

    char* strdup(const char* src) {
        return strdup(src, std::strlen(src));
    }

    void reset() {
        Block* block = _head->next;
        while (block) {
            Block* next = block->next;
            ::free(block->data);
            ::free(block);
            block = next;
        }
        _head->next = nullptr;
        _tail = _head;
        _current = _head;
        _offset = 0;
        _capacity = _current->capacity;
    }

    size_t totalUsed() const {
        size_t total = 0;
        Block* block = _head;
        while (block) {
            if (block == _current) {
                total += _offset;
                break;
            } else {
                total += block->capacity;
            }
            block = block->next;
        }
        return total;
    }

private:
    struct Block {
        char* data;
        size_t capacity;
        Block* next;
    };

    Block* _head;
    Block* _tail;
    Block* _current;
    size_t _offset;
    size_t _capacity;
    size_t _blockSize;

    void newBlock(size_t capacity) {
        Block* block = static_cast<Block*>(::malloc(sizeof(Block)));
        block->data = static_cast<char*>(::malloc(capacity));
        block->capacity = capacity;
        block->next = nullptr;

        if (!_head) {
            _head = _tail = block;
        } else {
            _tail->next = block;
            _tail = block;
        }
        _current = block;
        _offset = 0;
        _capacity = capacity;
    }

    static size_t alignUp(size_t n, size_t alignment) {
        return (n + alignment - 1) & ~(alignment - 1);
    }
};

#endif