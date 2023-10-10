#pragma once

#include <vector>
#include <string>
#include <tuple>

#include "logger.h"

// index, address, data (T)
template <class T> using ScrollableTableEntry = std::tuple<int, int, T>;
enum entry_content_types {
    ST_ENTRY_INDEX,
    ST_ENTRY_ADDRESS,
    ST_ENTRY_DATA
};

// size, offset, vector<entries>
template <class T> using ScrollableTableBuffer = std::tuple<int ,int, std::vector<ScrollableTableEntry<T>>>;
enum buffer_content_types {
    ST_BUF_SIZE,
    ST_BUF_OFFSET,
    ST_BUF_BUFFER
};

class ScrollableTableBase {
public:
	virtual void ScrollUp(const int& _num) = 0;
	virtual void ScrollDown(const int& _num) = 0;
	virtual void ScrollUpPage() = 0;
	virtual void ScrollDownPage() = 0;

protected:
	ScrollableTableBase() = default;
	~ScrollableTableBase() = default;
};

template <class T> class ScrollableTable : public ScrollableTableBase
{
public:
	explicit constexpr ScrollableTable(const int& _elements_to_show) : elementsToShow(_elements_to_show) {
		endIndex.y = startIndex.y + (elementsToShow - 1);
        currentIndex = startIndex;
	};
	constexpr ~ScrollableTable() noexcept {};

	constexpr ScrollableTable& operator=(ScrollableTable&& _right) noexcept {
		if (this == addressof(_right)) { return *this; }
		else {
			buffer = std::vector<ScrollableTableBuffer<T>>(_right.buffer);
			memcpy(&startIndex, &_right.startIndex, sizeof(_right.startIndex));
			memcpy(&endIndex, &_right.startIndex, sizeof(_right.startIndex));
			memcpy(&currentIndex, &_right.currentIndex, sizeof(_right.currentIndex));
			memcpy(&elementsToShow, &_right.elementsToShow, sizeof(_right.elementsToShow));
			memcpy(&bufferSize, &_right.bufferSize, sizeof(_right.bufferSize));
			return *this;
		}
	}

	void AddMemoryArea(ScrollableTableBuffer<T> _buffer) {
		buffer.emplace_back(_buffer);
		bufferSize++;
	}

	void ScrollUp(const int& _num) override;
	void ScrollDown(const int& _num) override;
	void ScrollUpPage() override;
	void ScrollDownPage() override;

	bool GetNextEntry(T& _entry);
	Vec2 GetCurrentIndex();
	bool CompareIndex(const Vec2& _index) const;
	void SwitchBank(int& _bank);
    void ScrollToAddress(Vec2& _index, const int& _addr);

	void SearchAddress(int& _addr);

private:

	// size, offset <index, address,  template type T>
	std::vector<ScrollableTableBuffer<T>> buffer = std::vector<ScrollableTableBuffer<T>>();
	Vec2 startIndex = Vec2(0, 0);
	Vec2 endIndex = Vec2(0, 0);
	Vec2 currentIndex = Vec2(0, 0);
	int bufferSize = 0;
	int elementsToShow;
};

template <class T> void ScrollableTable<T>::ScrollUp(const int& _num) {
    static bool full_scroll = true;

    if (startIndex.x > 0 || startIndex.y > 0) {
        startIndex.y -= _num;
        if (startIndex.y < 0) {
            if (startIndex.x == 0) {
                startIndex.y = 0;
                full_scroll = false;
            }
            else {
                startIndex.x--;
                startIndex.y += get<ST_BUF_SIZE>(buffer[startIndex.x]);
            }
        }

        if (full_scroll) {
            endIndex.y -= _num;
            if (endIndex.y < 0) {
                endIndex.x--;
                endIndex.y += get<ST_BUF_SIZE>(buffer[endIndex.x]);
            }
        }
        else {
            endIndex.y = startIndex.y + elementsToShow - 1;
        }
    }
}

template <class T> void ScrollableTable<T>::ScrollDown(const int& _num) {
    static bool full_scroll = true;

    if (endIndex.x < bufferSize - 1 || endIndex.y < get<ST_BUF_SIZE>(buffer[endIndex.x]) - 1) {
        endIndex.y += _num;
        if (endIndex.y >= get<ST_BUF_SIZE>(buffer[endIndex.x])) {
            if (endIndex.x == bufferSize - 1) {
                endIndex.y = get<ST_BUF_SIZE>(buffer[endIndex.x]) - 1;
                full_scroll = false;
            }
            else {
                endIndex.y -= get<ST_BUF_SIZE>(buffer[endIndex.x]);
                endIndex.x++;
            }
        }


        if (full_scroll) {
            startIndex.y += _num;
            if (startIndex.y >= get<ST_BUF_SIZE>(buffer[startIndex.x])) {
                startIndex.y -= get<ST_BUF_SIZE>(buffer[startIndex.x]);
                startIndex.x++;
            }
        }
        else {
            startIndex.y = endIndex.y - (elementsToShow - 1);
        }
    }
}

template <class T> void ScrollableTable<T>::ScrollUpPage() {
    ScrollUp(elementsToShow);
}

template <class T> void ScrollableTable<T>::ScrollDownPage() {
    ScrollDown(elementsToShow);
}

template <class T> void ScrollableTable<T>::SwitchBank(int& _bank) {
    return;
}

template <class T> void ScrollableTable<T>::SearchAddress(int& _addr) {
    // limit given address to address space of current bank
    ScrollableTableBuffer<T>& current_bank = buffer[startIndex.x];
    std::vector<ScrollableTableEntry<T>>& entries = get<ST_BUF_BUFFER>(current_bank);

    int first_address = get<ST_ENTRY_ADDRESS>(entries.front());
    int last_address = get<ST_ENTRY_ADDRESS>(entries.back());

    if (_addr < first_address) { _addr = first_address; }
    if (_addr > last_address) { _addr = last_address; }

    int index;
    int prev_addr;
    int next_addr;
    int bank_size = get<ST_BUF_SIZE>(current_bank);
    ScrollableTableEntry<T> current_entry, next_entry;

    for (index = 0; index < bank_size - 1; index++) {
        current_entry = entries[index];
        prev_addr = get<ST_ENTRY_ADDRESS>(current_entry);

        if (index + 1 == bank_size) { next_addr = bank_size; }
        else {
            next_entry = entries[index + 1];
            next_addr = get<ST_ENTRY_ADDRESS>(next_entry);
        }

        if (prev_addr <= _addr &&
            next_addr > _addr)
        {
            break;
        }
    }

    if (index > startIndex.y) {
        ScrollUp(index - startIndex.y);
    }
    else if (index < startIndex.y) {
        ScrollDown(startIndex.y - index);
    }

    _addr = startIndex.y;
}

template <class T> void ScrollableTable<T>::ScrollToAddress(Vec2& _index, const int& _addr) {

}

template <class T> bool ScrollableTable<T>::CompareIndex(const Vec2& _index) const {
    return currentIndex == _index;
}

template <class T> bool ScrollableTable<T>::GetNextEntry(T& _entry) {

    if (currentIndex > endIndex) { 
        currentIndex = startIndex; 
        return false;
    }

    ScrollableTableBuffer<T>& current_bank = buffer[currentIndex.x];
    ScrollableTableEntry<T>& current_entry = get<ST_BUF_BUFFER>(current_bank)[currentIndex.y];
    _entry = get<ST_ENTRY_DATA>(current_entry);
    
    currentIndex.y++;
    if (currentIndex.y > get<ST_BUF_SIZE>(current_bank) - 1) {
        currentIndex.y = 0;
        currentIndex.x++;
    }

    return true;
}

template <class T> Vec2 ScrollableTable<T>::GetCurrentIndex() {
    return currentIndex;
}