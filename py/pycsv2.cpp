
#include <boost/python/iterator.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/noncopyable.hpp>
#include "csv2/reader.hpp"
using namespace boost::python;
using namespace csv2;
template<typename T>
bool mmap_wraper(T* pSelf, const std::string& p)
{
    return pSelf->mmap(p);
}

template<typename C>
std::string to_string_wraper(C* pSelf)
{
    auto sv = pSelf->as_string();
    return std::string(sv.begin(), sv.end());
}

template <class delimiter = delimiter<','>, class quote_character = quote_character<'"'>,
          class first_row_is_header = first_row_is_header<true>,
          class trim_policy = trim_policy::trim_whitespace>
void export_reader(const std::string& name
    , const csv2::Reader<delimiter, quote_character, first_row_is_header, trim_policy>& c)
{
    using CSVT = csv2::Reader<delimiter, quote_character, first_row_is_header, trim_policy>;
    using CellT = typename CSVT::Cell;
    using RowT = typename CSVT::Row;
    using RowIter = typename CSVT::RowIterator;

    auto cellName = name+"Cell";
    class_<CellT>(cellName.c_str())
        .def("cell_no", &CellT::cell_no)
        .def("__str__", to_string_wraper<CellT>);

    auto rowName = name+"Row"; 
    class_<RowT>(rowName.c_str())
        .def("__iter__", boost::python::iterator<RowT>())
        .def("line_no", &RowT::line_no)
        .def("__str__", to_string_wraper<RowT>)
        .def("__len__", &RowT::cols);
    
    // auto rowIter = name+"RowIter";
    // class_<RowIter>(rowIter.c_str(), no_init);
    using RowVec = std::vector<RowT>;
    auto rowVecName = name+"RowVec";
    class_<RowVec>(rowVecName.c_str())
        .def(boost::python::vector_indexing_suite<RowVec>());

    class_<CSVT, boost::noncopyable>(name.c_str())
        .def("mmap", mmap_wraper<CSVT>)
        .def("header", &CSVT::header, return_internal_reference<>())
        .def("cols", &CSVT::cols)
        .def("rows", &CSVT::rows)
        .def("__len__", &CSVT::rows)
        .def("__iter__", boost::python::iterator<CSVT>());

}

BOOST_PYTHON_MODULE(libpycsv2)
{
    csv2::CommaHeaderCSV chc;
    export_reader("CommaHeaderCSV", chc);

    csv2::CommaNoneHeaderCSV cnhc;
    export_reader("CommaNoneHeaderCSV", cnhc);

    csv2::TabHeaderCSV thc;
    export_reader("TabHeaderCSV", thc);

    csv2::TabNoneHeaderCSV tnhc;
    export_reader("TableNoneHeaderCSV", tnhc);
}
