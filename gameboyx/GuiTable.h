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

namespace GUI {
    namespace GuiTable {
        struct bank_index {
            int bank;
            int address;

            bank_index() = default;
            bank_index(int _bank, int _address) : bank(_bank), address(_address) {};

            constexpr bool operator==(const bank_index& n) const {
                return (n.bank == bank) && (n.address == address);
            }
        };

        // address, data (T)
        template <class T> using TableEntry = std::tuple<int, T>;
        enum entry_content_types {
            ST_ENTRY_ADDRESS,
            ST_ENTRY_DATA
        };

        class TableBase {
        public:
            virtual void ScrollUp(const int& _num) = 0;
            virtual void ScrollDown(const int& _num) = 0;
            virtual void ScrollUpPage() = 0;
            virtual void ScrollDownPage() = 0;
            virtual void SetToBank(bank_index& _index) = 0;
            virtual void SearchIndex(bank_index& _index, int& _offset) = 0;
            virtual bank_index GetIndexByAddress(const int& _addr) = 0;
            virtual void SetToAddress(bank_index& _index) = 0;
            virtual void SetToIndex(bank_index& _index) = 0;
            virtual bank_index GetIndexCentre() = 0;

            std::string name = "";

        protected:
            TableBase() = default;
            ~TableBase() = default;
        };

        // vector<entries>
        template <class T> using TableSection = std::vector<TableEntry<T>>;

        template <class T> class Table : public TableBase {
        public:
            explicit constexpr Table(const int& _elements_to_show) : visibleElements(_elements_to_show) {};
            constexpr ~Table() noexcept = default;
            Table(const Table& _right);

            constexpr Table& operator=(const Table& _right) noexcept;

            bool AddTableSectionDisposable(TableSection<T>& _buffer);

            void ScrollUp(const int& _num) override;
            void ScrollDown(const int& _num) override;
            void ScrollUpPage() override;
            void ScrollDownPage() override;
            void SetToIndex(bank_index& _index) override;
            void SetToBank(bank_index& _index) override;
            void SetToAddress(bank_index& _index) override;
            void SearchIndex(bank_index& _index, int& _offset) override;

            bool Next(T& _entry);
            bool IsCentre();
            bank_index GetIndexCentre() override;

            bank_index GetIndex();
            bank_index GetIndexByAddress(const int& _address) override;

        private:
            std::vector<TableSection<T>> tableSections = std::vector<TableSection<T>>();
            int visibleElements;
            int currentlyVisibleElements = 0;
            int counter = 0;
            std::vector<TableSection<T>>::iterator tableIterator;
            std::vector<TableEntry<T>>::iterator contentIterator;

            bank_index currentIndex;
            std::vector<TableSection<T>>::iterator currentTable;
            std::vector<TableEntry<T>>::iterator currentElement;

            bool centre = false;

            void ResetAllIterators();
            void ResetIterators();
            void SwitchTableUp();
            void SwitchTableDown();
        };

        template <class T> Table<T>::Table(const Table& _right) {
            name = _right.name;
            tableSections = _right.tableSections;
            visibleElements = _right.visibleElements;
            currentlyVisibleElements = _right.currentlyVisibleElements;
            counter = _right.counter;
            tableIterator = _right.tableIterator;
            contentIterator = _right.contentIterator;
            currentIndex = _right.currentIndex;
            currentTable = _right.currentTable;
            currentElement = _right.currentElement;
            centre = _right.centre;
            if (tableSections.size() > 0) {
                ResetAllIterators();
            }
        }

        template <class T> constexpr Table<T>& Table<T>::operator=(const Table<T>& _right) noexcept {
            if (this != &_right) {
                name = _right.name;
                tableSections = _right.tableSections;
                visibleElements = _right.visibleElements;
                currentlyVisibleElements = _right.currentlyVisibleElements;
                counter = _right.counter;
                tableIterator = _right.tableIterator;
                contentIterator = _right.contentIterator;
                currentIndex = _right.currentIndex;
                currentTable = _right.currentTable;
                currentElement = _right.currentElement;
                centre = _right.centre;
            }

            return *this;
        }

        template <class T> bool Table<T>::AddTableSectionDisposable(TableSection<T>& _table) {
            if (_table.size() > 0) {
                currentlyVisibleElements += _table.size();
                tableSections.emplace_back(std::move(_table));
                if (currentlyVisibleElements > visibleElements) {
                    currentlyVisibleElements = visibleElements;
                }
                ResetAllIterators();
                return true;
            } else {
                return false;
            }
        }

        template <class T> void Table<T>::ResetAllIterators() {
            tableIterator = tableSections.begin();
            contentIterator = tableIterator->begin();
            ResetIterators();
        }

        template <class T> void Table<T>::ResetIterators() {
            currentTable = tableIterator;
            currentElement = contentIterator;
            counter = 0;
            currentIndex = bank_index(std::distance(tableSections.begin(), currentTable), get<ST_ENTRY_ADDRESS>(*currentElement));
        }

        template <class T> void Table<T>::SwitchTableUp() {
            while (std::distance(tableIterator->begin(), contentIterator) < 0) {
                if (std::distance(tableSections.begin(), tableIterator) <= 0) {
                    contentIterator = tableIterator->begin();
                } else {
                    auto prev_table = tableIterator;
                    --tableIterator;
                    contentIterator = tableIterator->end() + std::distance(prev_table->begin(), contentIterator);
                }
            }
        }

        template <class T> void Table<T>::SwitchTableDown() {
            if (std::distance(tableSections.end() - 1, tableIterator) >= 0) {
                if (std::distance(tableIterator->end() - currentlyVisibleElements, contentIterator) >= 0) {
                    tableIterator = tableSections.end() - 1;
                    contentIterator = tableIterator->end() - currentlyVisibleElements;
                    SwitchTableUp();
                }
            } else {
                while (std::distance(tableIterator->end(), contentIterator) >= 0) {
                    auto prev_table = tableIterator;
                    ++tableIterator;
                    contentIterator = tableIterator->begin() + std::distance(prev_table->end(), contentIterator);
                }
            }
        }

        template <class T> void Table<T>::ScrollUp(const int& _num) {
            contentIterator -= _num;
            SwitchTableUp();
            ResetIterators();
        }

        template <class T> void Table<T>::ScrollDown(const int& _num) {
            contentIterator += _num;
            SwitchTableDown();
            ResetIterators();
        }

        template <class T> void Table<T>::ScrollUpPage() {
            ScrollUp(currentlyVisibleElements);
        }

        template <class T> void Table<T>::ScrollDownPage() {
            ScrollDown(currentlyVisibleElements);
        }

        template <class T> void Table<T>::SearchIndex(bank_index& _index, int& _offset) {
            if (_index.bank >= tableSections.size()) { _index.bank = (int)tableSections.size() - 1; }
            else if (_index.bank < 0) { _index.bank = 0; }

            TableSection<T>& current_table = tableSections[_index.bank];

            int first_address = get<ST_ENTRY_ADDRESS>(current_table.front());
            int last_address = get<ST_ENTRY_ADDRESS>(current_table.back());

            if (_index.address < first_address) { _index.address = first_address; } 
            else if (_index.address > last_address) { _index.address = last_address; }

            // binary search (sort of), as the addresses are sorted
            int size = (int)current_table.size();
            int offset = size / 2;
            int suboffset = size / 2;
            bool found = false;
            while (!found) {
                if (suboffset >= 2) {
                    suboffset /= 2;
                }

                auto cur = current_table.begin() + offset;
                int current_address = get<ST_ENTRY_ADDRESS>(*cur);
                if (_index.address < current_address) {
                    offset -= suboffset;
                } else if (current_address < _index.address &&
                    (offset + 1) < size && 
                    get<ST_ENTRY_ADDRESS>(*(cur + 1)) <= _index.address) {

                    offset += suboffset;
                } else {
                    found = true;
                }
            }

            _index.address = get<ST_ENTRY_ADDRESS>(current_table[offset]);
            _offset = offset;
        }

        template <class T> void Table<T>::SetToBank(bank_index& _index) {
            if (_index.bank < 0) { _index.bank = 0; } 
            else if (_index.bank >= (int)tableSections.size()) { _index.bank = (int)tableSections.size() - 1; }

            tableIterator = tableSections.begin() + _index.bank;
            contentIterator = tableIterator->begin();
            ResetIterators();
        }

        template <class T> void Table<T>::SetToAddress(bank_index& _index) {
            bank_index index = _index;
            int offset = 0;

            SearchIndex(index, offset);

            contentIterator = tableIterator->begin() + offset;

            ScrollUp(currentlyVisibleElements / 2);
            SwitchTableDown();
            ResetIterators();
        }

        template <class T> void Table<T>::SetToIndex(bank_index& _index) {
            SetToBank(_index);
            SetToAddress(_index);
        }

        template <class T> bool Table<T>::Next(T& _entry) {
            if (counter >= currentlyVisibleElements) {
                ResetIterators();
                return false;
            } 
            
            while (std::distance(currentTable->end(), currentElement) >= 0) {
                ++currentTable;
                currentElement = currentTable->begin();
            }

            currentIndex = bank_index(std::distance(tableSections.begin(), tableIterator), get<ST_ENTRY_ADDRESS>(*currentElement));
            _entry = get<ST_ENTRY_DATA>(*currentElement);

            centre = counter == currentlyVisibleElements / 2;

            ++currentElement;
            ++counter;

            return true;
        }

        template <class T> bank_index Table<T>::GetIndex() {
            return currentIndex;
        }

        // search correcponding index(bank,address) by address in current table section
        template <class T> bank_index Table<T>::GetIndexByAddress(const int& _addr) {
            bank_index index = bank_index(std::distance(tableSections.begin(), tableIterator), _addr);
            int offset = 0;
            SearchIndex(index, offset);

            return index;
        }

        template <class T> bank_index Table<T>::GetIndexCentre() {
            auto table = tableIterator;
            auto element = contentIterator;

            element += currentlyVisibleElements / 2;

            while (std::distance(table->end(), element) >= 0) {
                if (std::distance(tableSections.end() - 1, table) < 0) {
                    auto last_table = table;
                    ++table;
                    element = table->begin() + std::distance(last_table->end(), element);
                } else {
                    // should never occur as tables don't allow scrolling beyond the end of the last table section, but just in case
                    element = table->end() - 1;
                }
            }

            return bank_index(std::distance(tableSections.begin(), table), get<ST_ENTRY_ADDRESS>(*element));
        }

        template <class T> bool Table<T>::IsCentre() {
            return centre;
        }
    }
}