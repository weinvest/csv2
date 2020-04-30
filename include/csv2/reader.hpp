#pragma once
#include <cassert>
#include <cstring>
#include <csv2/mio.hpp>
#include <istream>
#include <string>
#include <utility>
#include <vector>
#include <set>

namespace csv2 {

namespace trim_policy {
struct no_trimming {
public:
  static std::pair<size_t, size_t> trim(const char *buffer, size_t start, size_t end) {
    (void)(buffer); // to silence unused parameter warning
    return {start, end};
  }
};

template <char... character_list> struct trim_characters {
private:
  constexpr static bool is_trim_char(char) { return false; }

  template <class... Tail> constexpr static bool is_trim_char(char c, char head, Tail... tail) {
    return c == head || is_trim_char(c, tail...);
  }

public:
  static std::pair<size_t, size_t> trim(const char *buffer, size_t start, size_t end) {
    size_t new_start = start, new_end = end;
    while (new_start != new_end && is_trim_char(buffer[new_start], character_list...))
      ++new_start;
    while (new_start != new_end && is_trim_char(buffer[new_end - 1], character_list...))
      --new_end;
    return {new_start, new_end};
  }
};

using trim_whitespace = trim_characters<' ', '\t'>;
} // namespace trim_policy

template <char character> struct delimiter {
  constexpr static char value = character;
};

template <char character> struct quote_character {
  constexpr static char value = character;
};

template <bool flag> struct first_row_is_header {
  constexpr static bool value = flag;
};

template <class delimiter = delimiter<','>, class quote_character = quote_character<'"'>,
          class first_row_is_header = first_row_is_header<true>,
          class trim_policy = trim_policy::trim_whitespace>
class Reader {
  mio::mmap_source mmap_;          // mmap source
  const char *buffer_{nullptr};    // pointer to memory-mapped data
  size_t buffer_size_{0};          // mapped length of buffer
  size_t header_start_{0};         // start index of header (cache)
  size_t header_end_{0};           // end index of header (cache)

public:
  Reader() {
    init_header_();
    row_cnt_ = init_rows_();
    col_cnt_ = init_cols_();
  }

  // Use this if you'd like to mmap the CSV file
  template <typename StringType> bool mmap(StringType &&filename) {
    mmap_ = mio::mmap_source(filename);
    if (!mmap_.is_open() || !mmap_.is_mapped())
      return false;
    buffer_ = mmap_.data();
    buffer_size_ = mmap_.mapped_length();
    return true;
  }

  // Use this if you have the CSV contents
  // in an std::string already
  template <typename StringType> bool parse(StringType &&contents) {
    buffer_ = std::forward<StringType>(contents).c_str();
    buffer_size_ = contents.size();
    return buffer_size_ > 0;
  }

  class RowIterator;
  class Row;
  class CellIterator;

  class Cell {
    const char *buffer_{nullptr}; // Pointer to memory-mapped buffer
    size_t start_{0};             // Start index of cell content
    size_t end_{0};               // End index of cell content
    bool escaped_{false};         // Does the cell have escaped content?
    friend class Row;
    friend class CellIterator;

  public:
    auto as_string() const { return std::string_view(buffer_+start_, end_-start_); }
    // Returns the raw_value of the cell without handling escaped
    // content, e.g., cell containing """foo""" will be returned
    // as is
    template <typename Container> void read_raw_value(Container &result) const {
      if (start_ >= end_)
        return;
      result.reserve(end_ - start_);
      for (size_t i = start_; i < end_; ++i)
        result.push_back(buffer_[i]);
    }

    // If cell is escaped, convert and return correct cell contents,
    // e.g., """foo""" => ""foo""
    template <typename Container> void read_value(Container &result) const {
      if (start_ >= end_)
        return;
      result.reserve(end_ - start_);
      const auto new_start_end = trim_policy::trim(buffer_, start_, end_);
      for (size_t i = new_start_end.first; i < new_start_end.second; ++i)
        result.push_back(buffer_[i]);
      for (size_t i = 1; i < result.size(); ++i) {
        if (result[i] == quote_character::value && result[i - 1] == quote_character::value) {
          result.erase(i - 1, 1);
        }
      }
    }

    std::string_view get_preffix(char c) {
      auto  preffix_end = start_;
      while(preffix_end < end_ && c != buffer_[preffix_end]) {
        ++preffix_end;
      }

      return std::string_view(&buffer_[start_], preffix_end-start_);
    }
  };

  class Row {
    const char *buffer_{nullptr}; // Pointer to memory-mapped buffer
    size_t start_{0};             // Start index of row content
    size_t end_{0};               // End index of row content
    size_t line_no_{0};
    friend class RowIterator;
    friend class Reader;

  public:
    auto as_string() const { return std::string_view(buffer_+start_, end_-start_); }
    // Returns the raw_value of the row
    template <typename Container> void read_raw_value(Container &result) const {
      if (start_ >= end_)
        return;
      result.reserve(end_ - start_);
      for (size_t i = start_; i < end_; ++i)
        result.push_back(buffer_[i]);
    }

    class CellIterator {
      friend class Row;
      const char *buffer_;
      size_t row_start_;
      size_t row_end_;
      size_t cur_start_;
      size_t cur_end_;
      bool escaped_;
    public:
      CellIterator(const char *buffer, size_t start, size_t end)
          : buffer_(buffer), row_start_(start), cur_start_(row_start_), row_end_(end) {
        find_cell_end();
      }

      CellIterator &operator++() {
        cur_start_ = cur_end_;
        find_cell_end();
        return *this;
      }

      Cell operator*() {
        class Cell cell;
        cell.buffer_ = buffer_;
        cell.start_ = cur_start_;
        cell.end_ = cur_end_;
        cell.escaped_ = escaped_;
        return cell;       
      }

      void find_cell_end() {
        escaped_ = false;

        size_t last_quote_location = 0;
        bool quote_opened = false;
        for (auto i = cur_start_; i < row_end_; i++) {
          cur_end_ = i;
          if (buffer_[i] == delimiter::value && !quote_opened) {
            // actual delimiter
            // end of cell
            cur_end_ = cur_end_;
            return;
          } else {
            if (buffer_[i] == quote_character::value) {
              if (!quote_opened) {
                // first quote for this cell
                quote_opened = true;
                last_quote_location = i;
              } else {
                escaped_ = (last_quote_location == i - 1);
                last_quote_location += (i - last_quote_location) * size_t(!escaped_);
                quote_opened = escaped_ || (buffer_[i + 1] != delimiter::value);
              }
            }
          }
        }
        
        cur_end_ += 1;
      }

      bool operator!=(const CellIterator &rhs) { return cur_start_ != rhs.cur_start_; }
    };

    CellIterator begin() const { return CellIterator(buffer_, start_, end_); }
    CellIterator end() const { return CellIterator(buffer_, end_, end_); }
  };

  class RowIterator {
    friend class Reader;
    const char *buffer_;
    size_t buffer_size_;
    size_t start_;
    size_t end_;
    int64_t line_no_;

  public:
    RowIterator(const char *buffer, size_t buffer_size, size_t start, int64_t line_no)
        : buffer_(buffer), buffer_size_(buffer_size), start_(start), end_(start_), line_no_(line_no) {
          end_ = find_next(start_);
        }

    auto find_next(size_t s) {
      s = std::min(s, buffer_size_);
      auto e = s;
      if (const char *ptr =
              static_cast<const char *>(memchr(buffer_+s, '\n', (buffer_size_ - s)))) {
        e = ptr - buffer_;
      } else {
        // last row
        e = buffer_size_;
      }

      return e;
    }
    
    auto find_prev(size_t e) {
      auto s = e;
      if (const char *ptr =
              static_cast<const char *>(memrchr(buffer_, '\n', (buffer_size_ - e)))) {
        s = ptr - buffer_ + 1;
      } else {
        // last row
        s = 0;
      }

      return s;
    }

    RowIterator &operator++() {
      start_ = end_ + 1;
      end_ = find_next(start_);
      
      line_no_ = start_ > end_ ? line_no_ : (line_no_+1);
      return *this;
    }

    RowIterator &operator--() {
      end_ = start_ - 1;
      start_ = find_prev(end_);
      line_no_ = 0 >= line_no_ ? 0 : (line_no_-1);
      return *this;
    }

    Row operator*() {
      Row result;
      result.buffer_ = buffer_;
      result.start_ = start_;
      result.end_ = end_;
      result.line_no_ = line_no_;

      return result;
    }

    bool operator!=(const RowIterator &rhs) { return start_ != rhs.start_; }
  };

  class RRowIterator : RowIterator {
  public:
    using Impl = RowIterator;
    RRowIterator(const char *buffer, size_t buffer_size, size_t start, int64_t line_no)
        : Impl(buffer, buffer_size, start, line_no) {}
    
    RRowIterator(const Impl& impl): Impl(impl) {}

    RRowIterator& operator++() { Impl::operator--(); return *this; }
    RRowIterator& operator--() { Impl::operator++(); return *this; }

    bool operator != (const RRowIterator& rhs) { return Impl::operator != (rhs) ; }
    using Impl::operator*;
  };

  RowIterator begin() const {
    if (buffer_size_ == 0)
      return end();
    if (first_row_is_header::value) {
      const auto header_indices = header_indices_();
      return RowIterator(buffer_, buffer_size_, header_indices.second  > 0 ? header_indices.second + 1 : 0, headers_.size());
    } else {
      return RowIterator(buffer_, buffer_size_, 0, 0);
    }
  }

  RowIterator end() const { return RowIterator(buffer_, buffer_size_, buffer_size_ + 1, rows()-1); }

  RRowIterator rbegin() const { return --end(); }
  RRowIterator rend() const { return --begin(); }

  RowIterator operator[] (int64_t irow) {
    if(irow < rows()/2) {
      RowIterator it(buffer_size_, buffer_size_, 0, 0);
      while(it.line_no_ < irow) {
        ++it;
      }

      return it;
    }
    else {
      RowIterator it = end();
      while(it.line_no_ > irow) {
        --it;
      }

      return it;
    }
  }

private:
  std::pair<size_t, size_t> header_indices_() const {
    
    return {0, headers_.empty() ? 0 : headers_.back().end_};
  }

public:
  auto header() const { return headers_; }
  auto rows() const { return row_cnt_; }
  auto cols() const { return col_cnt_; }

private:
  void init_header_() {
    if (!first_row_is_header::value) return;

    std::set<std::string_view> header_names;
    size_t start = 0, end = 0;
    bool continue_next = true;
    do
    {
      Row result;
      result.buffer_ = buffer_;
      result.start_ = start;
      result.end_ = end;

      if (const char *ptr =
              static_cast<const char *>(memchr(&buffer_[start], '\n', (buffer_size_ - start)))) {
        end = start + (ptr - &buffer_[start]);
        result.end_ = end;
        headers_.push_back(result);
        
        auto first_cell = *result.begin();
        auto preffix = first_cell.get_preffix(':');
        continue_next = false;
        if(preffix.empty()) {
          assert(header_names.empty());
          headers_.push_back(result);
        }
        else if (0 == header_names.count(preffix)) {
          headers_.push_back(result);
          header_names.insert(preffix);
          start = end+1;
          end = start;
          continue_next = true;
        }
      }
      else {
        continue_next = false;
      }
      
    } while (continue_next);
  }

  size_t init_rows_() {
    size_t result{0};
    if (!buffer_ || buffer_size_ == 0)
      return result;
    for (const char *p = buffer_; (p = (char *)memchr(p, '\n', (buffer_ + buffer_size_) - p)); ++p)
      ++result;
    return result;
  }

  size_t init_cols_() {
    size_t result{0};
    for(auto& row : headers_) {
      size_t cols{0};
      for (const auto cell : header())
        cols += 1;
      
      result = std::max(result, cols);
    }

    return result;
  }

private:
  std::vector<Row> headers_;
  static constexpr size_t invalid_size_value = std::numeric_limits<size_t>::max();
  mutable size_t row_cnt_{invalid_size_value};
  mutable size_t col_cnt_{invalid_size_value};
};
} // namespace csv2
