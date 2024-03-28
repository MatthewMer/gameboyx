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

struct bank_index {
    int bank;
    int index;

    bank_index(int _bank, int _index) : bank(_bank), index(_index) {};

    constexpr bool operator==(const bank_index& n) const {
        return (n.bank == bank) && (n.index == index);
    }
};

// index, address, data (T)
template <class T> using TableEntry = std::tuple<int, T>;
enum entry_content_types {
    ST_ENTRY_ADDRESS,
    ST_ENTRY_DATA
};

// size, vector<entries>
template <class T> using TableSection = std::vector<TableEntry<T>>;

class TableBase {
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

    std::string name = "";
    size_t size = 0;

protected:
	TableBase() = default;
	~TableBase() = default;
};

template <class T> class Table : public TableBase
{
public:
	explicit constexpr Table(const int& _elements_to_show) : visibleElements(_elements_to_show) {
        currentlyVisibleElements = 0;
        SetElementsToShow();
	};
	constexpr ~Table() noexcept = default;

    constexpr void SetElementsToShow();
    constexpr Table& operator=(const Table& _right) noexcept;

    void AddTableSectionDisposable(TableSection<T>& _buffer);

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
	std::vector<TableSection<T>> tableSections = std::vector<TableSection<T>>();
	bank_index startIndex = bank_index(0, 0);
	bank_index endIndex = bank_index(0, 0);
	bank_index indexIterator = bank_index(0, 0);
    bank_index currentIndex = bank_index(0, 0);
	int visibleElements;
    int currentlyVisibleElements;
};

template <class T> constexpr Table<T>& Table<T>::operator=(const Table<T>& _right) noexcept {
    if (this != &_right) {
        tableSections = std::move(_right.tableSections);
        startIndex = _right.startIndex;
        endIndex = _right.startIndex;
        indexIterator = _right.indexIterator;
        visibleElements = _right.visibleElements;
        currentIndex = _right.currentIndex;
        currentlyVisibleElements = _right.currentlyVisibleElements;
        size = _right.size;
    }

    return *this;
}

template <class T> constexpr void Table<T>::SetElementsToShow() {
    int elements = 0;
    for (const auto& n : tableSections) {
        elements += (int)n.size();
    }

    if (elements > visibleElements) { currentlyVisibleElements = visibleElements; } 
    else                            { currentlyVisibleElements = elements; }

    startIndex = bank_index(0, 0);
    endIndex = startIndex;
    
    int bank = 0;
    for (size_t i = 0; const auto & n : tableSections) {
        i += n.size();
        if (i >= currentlyVisibleElements) { 
            endIndex.bank = bank;
            endIndex.index = currentlyVisibleElements - (int)(i - n.size());
            break; 
        }
        else { 
            bank++; 
        }
    }

    //endIndex = bank_index(0, startIndex.index + currentlyVisibleElements);
    indexIterator = startIndex;
}

template <class T> void Table<T>::AddTableSectionDisposable(TableSection<T>& _buffer) {
    tableSections.emplace_back(std::move(_buffer));
    size = tableSections.size();

    SetElementsToShow();
}

template <class T> void Table<T>::ScrollUp(const int& _num) {
    bool full_scroll = true;

    if (startIndex.bank > 0 || startIndex.index > 0) {
        startIndex.index -= _num;
        if (startIndex.index < 0) {
            if (startIndex.bank == 0) {
                startIndex.index = 0;
                full_scroll = false;
            }
            else {
                --startIndex.bank;
                startIndex.index += (int)tableSections[startIndex.bank].size();
            }
        }

        if (full_scroll) {
            endIndex.index -= _num;
            if (endIndex.index < 1) {
                --endIndex.bank;
                endIndex.index += (int)tableSections[endIndex.bank].size();
            }
        }
        else {
            endIndex.index = startIndex.index + currentlyVisibleElements;
        }
    }

    indexIterator = startIndex;
}

template <class T> void Table<T>::ScrollDown(const int& _num) {
    bool full_scroll = true;
    int current_buf_size_end = (int)tableSections[endIndex.bank].size();

    if (endIndex.bank < tableSections.size() - 1 || endIndex.index < current_buf_size_end) {
        endIndex.index += _num;
        if (endIndex.index > (int)tableSections[endIndex.bank].size()) {
            if (endIndex.bank == tableSections.size() - 1) {
                endIndex.index = (int)tableSections[endIndex.bank].size();
                full_scroll = false;
            }
            else {
                endIndex.index -= (int)tableSections[endIndex.bank].size();
                endIndex.bank++;
            }
        }


        if (full_scroll) {
            int current_buf_size_start = (int)tableSections[startIndex.bank].size();

            startIndex.index += _num;
            if (startIndex.index >= current_buf_size_start) {
                startIndex.index -= current_buf_size_start;
                startIndex.bank++;
            }
        }
        else {
            startIndex.index = endIndex.index - currentlyVisibleElements;
        }
    }

    indexIterator = startIndex;
}

template <class T> void Table<T>::ScrollUpPage() {
    ScrollUp(currentlyVisibleElements);
}

template <class T> void Table<T>::ScrollDownPage() {
    ScrollDown(currentlyVisibleElements);
}

template <class T> void Table<T>::SearchBank(int& _bank) {
    if (_bank < 0) { _bank = 0; }
    else if (_bank >= tableSections.size()) { _bank = (int)tableSections.size() - 1; }

    startIndex = bank_index(_bank, 0);
    endIndex = bank_index(_bank, currentlyVisibleElements);

    indexIterator = startIndex;
}

template <class T> void Table<T>::SearchAddress(int& _addr) {
    TableSection<T>& current_bank = tableSections[startIndex.bank];

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

    int index = i - visibleElements / 2;
    if (index < startIndex.index) { ScrollUp(startIndex.index - index); }
    else if (index > startIndex.index) { ScrollDown(index - startIndex.index); }

    indexIterator = startIndex;
}

template <class T> bool Table<T>::GetNextEntry(T& _entry) {
    if (indexIterator == endIndex) {
        indexIterator = startIndex;
        return false;
    }
    else if (indexIterator.index == tableSections[indexIterator.bank].size()) {
        indexIterator.bank++;
        indexIterator.index = 0;
    }

    TableSection<T>& current_bank = tableSections[indexIterator.bank];
    TableEntry<T>& current_entry = current_bank[indexIterator.index];
    _entry = get<ST_ENTRY_DATA>(current_entry);
    
    currentIndex = indexIterator;
    indexIterator.index++;

    return true;
}

template <class T> T& Table<T>::GetEntry(bank_index& _instr_index) {
    TableSection<T>& current_bank = tableSections[_instr_index.bank];
    TableEntry<T>& current_entry = current_bank[_instr_index.index];
    return get<ST_ENTRY_DATA>(current_entry);
}

template <class T> bank_index& Table<T>::GetCurrentIndex() {
    return currentIndex;
}

template <class T> bank_index Table<T>::GetCurrentIndexCentre() {
    if (size > 0) {
        size_t n = (size_t)currentlyVisibleElements / 2;
        size_t m = tableSections[startIndex.bank].size() - (size_t)startIndex.index;
        int bank = startIndex.bank;
        while (m < n) {
            bank++;
            m += tableSections[bank].size();
        }

        int index = ((int)(n - (m - tableSections[bank].size())));
        return bank_index(bank, index);
    }
    else {
        return bank_index(0, 0);
    }
}

// search address in current table section
template <class T> int& Table<T>::GetAddressByIndex(const bank_index& _index) {
    return get<ST_ENTRY_ADDRESS>(tableSections[_index.bank][_index.index]);
}

// search correcponding index(bank,index) by address in current table section
template <class T> bank_index Table<T>::GetIndexByAddress(const int& _address) {
    TableSection<T>& current_bank = tableSections[startIndex.bank];

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