#pragma once
/* ***********************************************************************************************************
    DESCRIPTION
*********************************************************************************************************** */
/*
*	A template class to transform imgui tables with a fixed number of tuples into scrollable tables.
*   Most relevant data of emulated hardware could easily surpass millions of entries which effectively means
*   passing huge arrays of strings/char* to the imgui backend each frame. This is a basic approach to circumvent this
*   by passing just a small snippet of the entire data to the backend.
*/

#include <vector>
#include <string>
#include <tuple>

#include "logger.h"
#include "defs.h"

using debug_instr_data = std::pair<std::string, std::string>;
using misc_output_data = std::pair<std::string, std::string>;

enum memory_data_types {
    MEM_ENTRY_ADDR,
    MEM_ENTRY_LEN,
    MEM_ENTRY_REF
};
using memory_data = std::tuple<std::string, int, u8*>;

struct bank_index {
    int bank;
    int index;

    bank_index(int _bank, int _index) : bank(_bank), index(_index) {};

    constexpr bool operator==(const bank_index& n) const {
        return (n.bank == bank) && (n.index == index);
    }
};

// index, address, data (T)
template <class T> using ScrollableTableEntry = std::tuple<int, T>;
enum entry_content_types {
    ST_ENTRY_ADDRESS,
    ST_ENTRY_DATA
};

// size, vector<entries>
template <class T> using ScrollableTableBuffer = std::vector<ScrollableTableEntry<T>>;

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

template <class T> class GuiScrollableTable : public ScrollableTableBase
{
public:
	explicit constexpr GuiScrollableTable(const int& _elements_to_show) : maxSize(_elements_to_show) {
        elementsToShow = 0;
        SetElementsToShow();
	};
	constexpr ~GuiScrollableTable() noexcept {};

    constexpr void SetElementsToShow() {
        startIndex = bank_index(0, 0);
        endIndex = bank_index(0, startIndex.index + elementsToShow);
        indexIterator = startIndex;
    }

	constexpr GuiScrollableTable& operator=(const GuiScrollableTable& _right) noexcept {
		if (this == addressof(_right)) { return *this; }
		else {
			buffer = std::vector<ScrollableTableBuffer<T>>(_right.buffer);
			startIndex = _right.startIndex;
			endIndex = _right.startIndex;
			indexIterator = _right.indexIterator;
			elementsToShow = _right.elementsToShow;
			bufferSize = _right.bufferSize;
            currentIndex = _right.currentIndex;
            maxSizeSet = _right.maxSizeSet;
            maxSize = _right.maxSize;
			return *this;
		}
	}

	void AddMemoryArea(ScrollableTableBuffer<T> _buffer) {
		buffer.emplace_back(_buffer);
		bufferSize++;

        if(!maxSizeSet){
            int elements = 0;

            for (const auto& n : buffer) {
                elements += (int)n.size();
                if (elements >= maxSize) {
                    elementsToShow = maxSize;
                    maxSizeSet = true;
                    break;
                }
            }

            if (!maxSizeSet) { elementsToShow = elements; }
            SetElementsToShow();
        }
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
    bool maxSizeSet = false;
    int maxSize;
};

template <class T> void GuiScrollableTable<T>::ScrollUp(const int& _num) {
    bool full_scroll = true;

    if (startIndex.bank > 0 || startIndex.index > 0) {
        startIndex.index -= _num;
        if (startIndex.index < 0) {
            if (startIndex.bank == 0) {
                startIndex.index = 0;
                full_scroll = false;
            }
            else {
                startIndex.bank--;
                startIndex.index += (int)buffer[startIndex.bank].size();
            }
        }

        if (full_scroll) {
            endIndex.index -= _num;
            if (endIndex.index < 1) {
                endIndex.bank--;
                endIndex.index += (int)buffer[endIndex.bank].size();
            }
        }
        else {
            endIndex.index = startIndex.index + elementsToShow;
        }
    }

    indexIterator = startIndex;
}

template <class T> void GuiScrollableTable<T>::ScrollDown(const int& _num) {
    bool full_scroll = true;
    int current_buf_size_end = (int)buffer[endIndex.bank].size();

    if (endIndex.bank < bufferSize - 1 || endIndex.index < current_buf_size_end) {
        endIndex.index += _num;
        if (endIndex.index > (int)buffer[endIndex.bank].size()) {
            if (endIndex.bank == bufferSize - 1) {
                endIndex.index = (int)buffer[endIndex.bank].size();
                full_scroll = false;
            }
            else {
                endIndex.index -= (int)buffer[endIndex.bank].size();
                endIndex.bank++;
            }
        }


        if (full_scroll) {
            int current_buf_size_start = (int)buffer[startIndex.bank].size();

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

template <class T> void GuiScrollableTable<T>::ScrollUpPage() {
    ScrollUp(elementsToShow);
}

template <class T> void GuiScrollableTable<T>::ScrollDownPage() {
    ScrollDown(elementsToShow);
}

template <class T> void GuiScrollableTable<T>::SearchBank(int& _bank) {
    if (_bank < 0) { _bank = 0; }
    else if (_bank > bufferSize - 1) { _bank = bufferSize - 1; }

    startIndex = bank_index(_bank, 0);
    endIndex = bank_index(_bank, elementsToShow);

    indexIterator = startIndex;
}

template <class T> void GuiScrollableTable<T>::SearchAddress(int& _addr) {
    ScrollableTableBuffer<T>& current_bank = buffer[startIndex.bank];

    int first_address = get<ST_ENTRY_ADDRESS>(current_bank.front());
    int last_address = get<ST_ENTRY_ADDRESS>(current_bank.back());

    if (_addr < first_address) { _addr = first_address; }
    else if (_addr > last_address) { _addr = last_address; }

    int i;
    for (i = 0; i < current_bank.size() - 1; i++) {
        if (get<ST_ENTRY_ADDRESS>(current_bank[i]) <= _addr &&
            get<ST_ENTRY_ADDRESS>(current_bank[i + 1]) > _addr)
        {
            break;
        }
    }

    int index = i - elementsToShow / 2;
    if (index < startIndex.index) { ScrollUp(startIndex.index - index); }
    else if (index > startIndex.index) { ScrollDown(index - startIndex.index); }

    indexIterator = startIndex;
}

template <class T> bool GuiScrollableTable<T>::GetNextEntry(T& _entry) {
    if (indexIterator == endIndex) {
        indexIterator = startIndex;
        return false;
    }
    else if (indexIterator.index == buffer[indexIterator.bank].size()) {
        indexIterator.bank++;
        indexIterator.index = 0;
    }

    ScrollableTableBuffer<T>& current_bank = buffer[indexIterator.bank];
    ScrollableTableEntry<T>& current_entry = current_bank[indexIterator.index];
    _entry = get<ST_ENTRY_DATA>(current_entry);
    
    currentIndex = indexIterator;
    indexIterator.index++;

    return true;
}

template <class T> T& GuiScrollableTable<T>::GetEntry(bank_index& _instr_index) {
    ScrollableTableBuffer<T>& current_bank = buffer[_instr_index.bank];
    ScrollableTableEntry<T>& current_entry = current_bank[_instr_index.index];
    return get<ST_ENTRY_DATA>(current_entry);
}

template <class T> bank_index& GuiScrollableTable<T>::GetCurrentIndex() {
    return currentIndex;
}

template <class T> bank_index GuiScrollableTable<T>::GetCurrentIndexCentre() {
    bank_index index = startIndex;
    index.index += elementsToShow / 2;

    int current_bank_size = (int)buffer[index.bank].size();

    if (index.index >= current_bank_size) {
        index.index -= current_bank_size;
        index.bank++;
    }

    return index;
}

template <class T> int& GuiScrollableTable<T>::GetAddressByIndex(const bank_index& _index) {
    return get<ST_ENTRY_ADDRESS>(buffer[_index.bank][_index.index]);
}

template <class T> bank_index GuiScrollableTable<T>::GetIndexByAddress(const int& _address) {
    ScrollableTableBuffer<T>& current_bank = buffer[startIndex.bank];

    int first_address = get<ST_ENTRY_ADDRESS>(current_bank.front());
    int last_address = get<ST_ENTRY_ADDRESS>(current_bank.back());
    int addr = _address;

    if (addr < first_address) { addr = first_address; }
    else if (addr > last_address) { addr = last_address; }

    int i;
    for (i = 0; i < current_bank.size() - 1; i++) {
        if (get<ST_ENTRY_ADDRESS>(current_bank[i]) <= addr &&
            get<ST_ENTRY_ADDRESS>(current_bank[i + 1]) > addr)
        {
            break;
        }
    }

    return bank_index(startIndex.bank, i);
}