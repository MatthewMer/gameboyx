#pragma once

#include <vector>
#include <string>
#include <tuple>

#include "logger.h"

// index, address, data (T)
template <class T> using ScrollableTableEntry = std::tuple<int, T>;
enum entry_content_types {
    ST_ENTRY_ADDRESS,
    ST_ENTRY_DATA
};

// size, vector<entries>
template <class T> using ScrollableTableBuffer = std::tuple<int ,std::vector<ScrollableTableEntry<T>>>;
enum buffer_content_types {
    ST_BUF_SIZE,
    ST_BUF_BUFFER
};

class ScrollableTableBase {
public:
	virtual void ScrollUp(const int& _num) = 0;
	virtual void ScrollDown(const int& _num) = 0;
	virtual void ScrollUpPage() = 0;
	virtual void ScrollDownPage() = 0;
    virtual void SearchBank(int& _bank) = 0;
    virtual bank_index GetCurrentIndexCentre() = 0;
    virtual int& GetAddressByIndex(const bank_index& _index) = 0;
    virtual bank_index GetIndexByAddress(const int& _address) = 0;
    virtual void SearchAddress(int& _addr) = 0;

protected:
	ScrollableTableBase() = default;
	~ScrollableTableBase() = default;
};

template <class T> class ScrollableTable : public ScrollableTableBase
{
public:
	explicit constexpr ScrollableTable(const int& _elements_to_show) : elementsToShow(_elements_to_show) {
		endIndex.index = startIndex.index + elementsToShow;
        indexIterator = startIndex;
	};
	constexpr ~ScrollableTable() noexcept {};

	constexpr ScrollableTable& operator=(const ScrollableTable& _right) noexcept {
		if (this == addressof(_right)) { return *this; }
		else {
			buffer = std::vector<ScrollableTableBuffer<T>>(_right.buffer);
			startIndex = _right.startIndex;
			endIndex = _right.startIndex;
			indexIterator = _right.indexIterator;
			elementsToShow = _right.elementsToShow;
			bufferSize = _right.bufferSize;
            currentIndex = _right.currentIndex;
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
    void SearchBank(int& _bank) override;
    void SearchAddress(int& _addr) override;

	bool GetNextEntry(T& _entry);
    T& GetEntry(bank_index& _instr_index);

    bank_index& GetCurrentIndex();
    bank_index GetCurrentIndexCentre() override;
    int& GetAddressByIndex(const bank_index& _index) override;
    bank_index GetIndexByAddress(const int& _address) override;

private:

	// size, offset <index, address,  template type T>
	std::vector<ScrollableTableBuffer<T>> buffer = std::vector<ScrollableTableBuffer<T>>();
	bank_index startIndex = bank_index(0, 0);
	bank_index endIndex = bank_index(0, 0);
	bank_index indexIterator = bank_index(0, 0);
    bank_index currentIndex = bank_index(0, 0);
	int bufferSize = 0;
	int elementsToShow;
};

template <class T> void ScrollableTable<T>::ScrollUp(const int& _num) {
    static bool full_scroll = true;

    if (startIndex.bank > 0 || startIndex.index > 0) {
        startIndex.index -= _num;
        if (startIndex.index < 0) {
            if (startIndex.bank == 0) {
                startIndex.index = 0;
                full_scroll = false;
            }
            else {
                startIndex.bank--;
                startIndex.index += get<ST_BUF_SIZE>(buffer[startIndex.bank]);
            }
        }

        if (full_scroll) {
            endIndex.index -= _num;
            if (endIndex.index < 1) {
                endIndex.bank--;
                endIndex.index += get<ST_BUF_SIZE>(buffer[endIndex.bank]);
            }
        }
        else {
            endIndex.index = startIndex.index + elementsToShow;
        }
    }

    indexIterator = startIndex;
}

template <class T> void ScrollableTable<T>::ScrollDown(const int& _num) {
    static bool full_scroll = true;
    int current_buf_size_end = get<ST_BUF_SIZE>(buffer[endIndex.bank]);

    if (endIndex.bank < bufferSize - 1 || endIndex.index < current_buf_size_end) {
        endIndex.index += _num;
        if (endIndex.index > get<ST_BUF_SIZE>(buffer[endIndex.bank])) {
            if (endIndex.bank == bufferSize - 1) {
                endIndex.index = get<ST_BUF_SIZE>(buffer[endIndex.bank]);
                full_scroll = false;
            }
            else {
                endIndex.index -= get<ST_BUF_SIZE>(buffer[endIndex.bank]);
                endIndex.bank++;
            }
        }


        if (full_scroll) {
            int current_buf_size_start = get<ST_BUF_SIZE>(buffer[startIndex.bank]);

            startIndex.index += _num;
            if (startIndex.index >= current_buf_size_start) {
                startIndex.index -= current_buf_size_start;
                startIndex.bank++;
            }
        }
        else {
            startIndex.index = endIndex.index - elementsToShow;
        }
    }

    indexIterator = startIndex;
}

template <class T> void ScrollableTable<T>::ScrollUpPage() {
    ScrollUp(elementsToShow);
}

template <class T> void ScrollableTable<T>::ScrollDownPage() {
    ScrollDown(elementsToShow);
}

template <class T> void ScrollableTable<T>::SearchBank(int& _bank) {
    if (_bank < 0) { _bank = 0; }
    else if (_bank > bufferSize - 1) { _bank = bufferSize - 1; }

    startIndex = bank_index(_bank, 0);
    endIndex = bank_index(_bank, elementsToShow);

    ScrollUp(elementsToShow / 2);
}

template <class T> void ScrollableTable<T>::SearchAddress(int& _addr) {
    ScrollableTableBuffer<T>& current_bank = buffer[startIndex.bank];
    std::vector<ScrollableTableEntry<T>>& entries = get<ST_BUF_BUFFER>(current_bank);

    int first_address = get<ST_ENTRY_ADDRESS>(entries.front());
    int last_address = get<ST_ENTRY_ADDRESS>(entries.back());

    if (_addr < first_address) { _addr = first_address; }
    else if (_addr > last_address) { _addr = last_address; }

    int i;
    for (i = 0; i < get<ST_BUF_SIZE>(current_bank) - 1; i++) {
        if (get<ST_ENTRY_ADDRESS>(entries[i]) <= _addr &&
            get<ST_ENTRY_ADDRESS>(entries[i + 1]) > _addr) 
        {
            break;
        }
    }

    int index = i - elementsToShow / 2;
    if (index < startIndex.index) { ScrollUp(startIndex.index - index); }
    else if (index > startIndex.index) { ScrollDown(index - startIndex.index); }

    indexIterator = startIndex;
}

template <class T> bool ScrollableTable<T>::GetNextEntry(T& _entry) {
    if (indexIterator == endIndex) {
        indexIterator = startIndex;
        return false;
    }
    else if (indexIterator.index == get<ST_BUF_SIZE>(buffer[indexIterator.bank])) {
        indexIterator.bank++;
        indexIterator.index = 0;
    }

    ScrollableTableBuffer<T>& current_bank = buffer[indexIterator.bank];
    ScrollableTableEntry<T>& current_entry = get<ST_BUF_BUFFER>(current_bank)[indexIterator.index];
    _entry = get<ST_ENTRY_DATA>(current_entry);
    
    currentIndex = indexIterator;
    indexIterator.index++;

    return true;
}

template <class T> T& ScrollableTable<T>::GetEntry(bank_index& _instr_index) {
    ScrollableTableBuffer<T>& current_bank = buffer[_instr_index.bank];
    ScrollableTableEntry<T>& current_entry = get<ST_BUF_BUFFER>(current_bank)[_instr_index.index];
    return get<ST_ENTRY_DATA>(current_entry);
}

template <class T> bank_index& ScrollableTable<T>::GetCurrentIndex() {
    return currentIndex;
}

template <class T> bank_index ScrollableTable<T>::GetCurrentIndexCentre() {
    bank_index index = startIndex;
    index.index += elementsToShow / 2;

    int current_bank_size = get<ST_BUF_SIZE>(buffer[index.bank]);

    if (index.index >= current_bank_size) {
        index.index -= current_bank_size;
        index.bank++;
    }

    return index;
}

template <class T> int& ScrollableTable<T>::GetAddressByIndex(const bank_index& _index) {
    return get<ST_ENTRY_ADDRESS>(get<ST_BUF_BUFFER>(buffer[_index.bank])[_index.index]);
}

template <class T> bank_index ScrollableTable<T>::GetIndexByAddress(const int& _address) {
    ScrollableTableBuffer<T>& current_bank = buffer[startIndex.bank];
    std::vector<ScrollableTableEntry<T>>& entries = get<ST_BUF_BUFFER>(current_bank);

    int first_address = get<ST_ENTRY_ADDRESS>(entries.front());
    int last_address = get<ST_ENTRY_ADDRESS>(entries.back());
    int addr = _address;

    if (addr < first_address) { addr = first_address; }
    else if (addr > last_address) { addr = last_address; }

    int i;
    for (i = 0; i < get<ST_BUF_SIZE>(current_bank) - 1; i++) {
        if (get<ST_ENTRY_ADDRESS>(entries[i]) <= addr &&
            get<ST_ENTRY_ADDRESS>(entries[i + 1]) > addr)
        {
            break;
        }
    }

    return bank_index(startIndex.bank, i);
}