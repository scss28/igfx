#include "arena.h"
#define PAGE_SIZE 4092

Arena::Arena() {
    this->page = new Arena::Page {
        .previous = nullptr,
        .count = 0,
    };
}
