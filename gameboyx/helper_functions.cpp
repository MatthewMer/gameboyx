#include "helper_functions.h"

#include <vector>
#include <string>
#include <fstream>

#include "logger.h"

using namespace std;
namespace fs = filesystem;

namespace Helpers {
    const string WHITESPACE = " \n\r\t\f\v" + (char)0x00;

    vector<string> split_string(const string& _in_string, const string& _delimiter) {
        vector<string> vec_in_string;
        string in_string_copy = _in_string;

        while (in_string_copy.find(_delimiter) != string::npos) {
            vec_in_string.push_back(in_string_copy.substr(0, in_string_copy.find(_delimiter)));
            in_string_copy.erase(0, in_string_copy.find(_delimiter) + _delimiter.length());
        }
        vec_in_string.push_back(in_string_copy);

        return vec_in_string;
    }

    string trim(const string& _in_string) {
        return rtrim(ltrim(_in_string));
    }

    string ltrim(const string& _in_string) {
        size_t start = _in_string.find_first_not_of(WHITESPACE);
        return (start == string::npos) ? "" : _in_string.substr(start);
    }

    string rtrim(const string& _in_string) {
        size_t end = _in_string.find_last_not_of(WHITESPACE);
        return (end == string::npos) ? "" : _in_string.substr(0, end + 1);
    }
}