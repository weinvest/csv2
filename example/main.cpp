#include "csv2/reader.hpp"
#include <iostream>

int main() {
  csv2::Reader<csv2::delimiter<','>, 
    csv2::quote_character<'"'>, 
    csv2::first_row_is_header<true>,
    csv2::trim_policy::trim_whitespace> csv;
               
  if (csv.mmap("/tmp/o.csv")) {
    std::cout << "rows:" << csv.rows() << std::endl;
    const auto& header = csv.header();
    for(auto& h : header) {
      for(auto c : h) {
        std::cout << c.as_string() << ",";
      }

      std::cout << std::endl;
    }

    auto h2 = csv(2);
    auto h3 = csv(30000);
    for(; h2 != h3; ++h2) {
      std::cout << h2.line_no();
      for(auto c : *h2) {
          std::cout << c.as_string() << ",";
      }

      std::cout << std::endl;
    }
    // for (const auto row: csv) {
    //   std::cout << row.line_no();
    //   for (const auto cell: row) {
    //     auto value = cell.as_string();
    //     std::cout << value << ",";
    //   }
      
    //   std::cout << std::endl;
    // }
    // std::cout << "rxx" << std::endl;
    // for(auto it = csv.rbegin(); it != csv.rend(); ++it)
    // {
    //   auto row = *it;
    //   std::cout << row.line_no();
    //   for (const auto cell: row) {
    //     auto value = cell.as_string();
    //     std::cout << value << ",";
    //   }
      
    //   std::cout << std::endl;      
    // }
  }
}
