
#include <boost/python/iterator.hpp>
#include <boost/python/module.hpp>
#include <boost/python/class.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <boost/python/slice.hpp>
#include <boost/noncopyable.hpp>
#include <boost/python/return_arg.hpp>
#include "csv2/reader.hpp"
namespace bp=boost::python;
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

template<typename IT>
struct RowRange {
    IT beg, ed;
    IT begin() const { return beg; }
    IT end() const { return ed; }
    using iterator = IT;
};

template<typename C>
RowRange<typename C::iterator> get_item_wraper(C* pSelf, bp::slice s)
{
    auto start = s.start();
    auto stop = s.stop();
    auto step = s.step();
    
    auto startv = start.is_none() ? 0LU : (size_t)bp::extract<size_t>(start);
    auto endv = stop.is_none() ? pSelf->size() : (size_t)bp::extract<size_t>(stop);

    typename C::iterator beg = (*pSelf)(startv);
    typename C::iterator end = (*pSelf)(endv);
    
    return RowRange<typename C::iterator>{beg, end};
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
    using RowRangeT = RowRange<typename CSVT::RowIterator>;
    using RowIterT = typename CSVT::RowIterator;

    auto cellName = name+"Cell";
    bp::class_<CellT>(cellName.c_str())
        .def("cell_no", &CellT::cell_no)
        .def("__str__", to_string_wraper<CellT>);

    auto rowName = name+"Row"; 
    bp::class_<RowT>(rowName.c_str())
        .def("__iter__", bp::iterator<RowT>())
        .def("line_no", &RowT::line_no)
        .def("__str__", to_string_wraper<RowT>)
        .def("__len__", &RowT::cols);
    
    auto rowRange = name+"RowRange";
    bp::class_<RowRangeT>(rowRange.c_str(), bp::no_init)
        .def("__iter__", bp::iterator<RowRangeT>());

    using RowVec = std::vector<RowT>;
    auto rowVecName = name+"RowVec";
    bp::class_<RowVec>(rowVecName.c_str())
        .def(bp::vector_indexing_suite<RowVec>());
    
    auto rowIterName = name+"Iter";
    bp::class_<RowIterT>(rowIterName.c_str(), bp::no_init)
        .def("next", &RowIterT::operator+=, bp::return_self<>())
        .def("prev", &RowIterT::operator-=, bp::return_self<>())
        .def("get", &RowIterT::operator*);

    bp::class_<CSVT, boost::noncopyable>(name.c_str())
        .def("mmap", mmap_wraper<CSVT>)
        .def("header", &CSVT::header, bp::return_internal_reference<>())
        .def("cols", &CSVT::cols)
        .def("rows", &CSVT::rows)
        .def("begin", &CSVT::begin)
        .def("end", &CSVT::end)
        .def("get_iter", &CSVT::operator())
        .def("__len__", &CSVT::size)
        .def("__iter__", bp::iterator<CSVT>())
        .def("__getitem__", get_item_wraper<CSVT>);

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
