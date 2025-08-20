struct Arena {
    static constexpr usize pageBytesCount = 4092 - sizeof(usize) * 2;
    struct Page {
        Page* previous;
        usize count;
        u8 bytes[pageBytesCount];
    };

    Page* page;

    Arena();
    ~Arena();

    // TODO
    template <typename T>
    T* alloc(usize count) {
        return nullptr;
    }
};
