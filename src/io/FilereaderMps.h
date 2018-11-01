#ifndef IO_FILEREADER_MPS_H_
#define IO_FILEREADER_MPS_H_

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>
#include "Filereader.h"

#ifdef BOOST_Found
#include <boost/algorithm/string/predicate.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/utility/string_ref.hpp>
#endif

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>

#include "HConst.h"
#include "Hash.hpp"
#include "HighsLp.h"
#include "pdqsort.h"

#include "HMPSIO.h"

using Triplet = std::tuple<int, int, double>;

const double infinity();

bool operator==(boost::string_ref word, std::string str);

int readMpsFreeFormatParser(const HighsOptions &options, HighsLp &lp);
int readMpsFixedFormatParser(const HighsOptions &options, HighsLp &lp);

// Free format parser class. The old fixed parser is in HMPSIO.h
class MpsParser {
 private:
  int status;

  int numRow;
  int numCol;
  int objSense;
  double objOffset;
  std::vector<int> Astart;
  std::vector<int> Aindex;
  std::vector<double> Avalue;
  std::vector<double> colCost;
  std::vector<double> colLower;
  std::vector<double> colUpper;
  std::vector<double> rowLower;
  std::vector<double> rowUpper;

 public:
  int loadProblem(const HighsOptions &options, HighsLp &lp);
  int loadProblem(const char *filename_, int &numRow_, int &numCol_,
                  int &objSense_, double &objOffset_, std::vector<int> &Astart_,
                  std::vector<int> &Aindex_, std::vector<double> &Avalue_,
                  std::vector<double> &colCost_, std::vector<double> &colLower_,
                  std::vector<double> &colUpper_,
                  std::vector<double> &rowLower_,
                  std::vector<double> &rowUpper_);

  int getProb();

  MpsParser() : status(-1) {}

  int getStatus() { return status; };

  /// load LP from MPS file as transposed triplet matrix
  int parseFile(std::string filename);

  int fillArrays();

  int fillMatrix(std::vector<Triplet> entries, int nRows_in, int nCols_in);

  enum class parsekey {
    ROWS,
    COLS,
    RHS,
    BOUNDS,
    RANGES,
    NONE,
    END,
    FAIL,
    COMMENT
  };

 private:
  int parse(boost::iostreams::filtering_istream &file);

  enum class boundtype { LE, EQ, GE, FR };
  /*
   * data for mps problem
   */

  std::vector<Triplet> entries;
  std::vector<std::pair<int, double>> coeffobj;
  std::vector<double> rowlhs;
  std::vector<double> rowrhs;
  std::vector<std::string> rownames;
  std::vector<std::string> colnames;

  HashMap<std::string, int> rowname2idx;
  HashMap<std::string, int> colname2idx;
  std::vector<double> lb4cols;
  std::vector<double> ub4cols;
  std::vector<boundtype> row_type;
  boost::dynamic_bitset<> col_integrality;

  int nCols = 0;
  int nRows = 0;
  int nnz = -1;

  /// checks first word of strline and wraps it by it_begin and it_end
  MpsParser::parsekey checkFirstWord(std::string &strline,
                                     std::string::iterator &it,
                                     boost::string_ref &word_ref) const;

  MpsParser::parsekey parseDefault(
      boost::iostreams::filtering_istream &file) const;

  MpsParser::parsekey parseRows(boost::iostreams::filtering_istream &file,
                                std::vector<boundtype> &rowtype);

  MpsParser::parsekey parseCols(boost::iostreams::filtering_istream &file,
                                const std::vector<boundtype> &rowtype);

  MpsParser::parsekey parseRhs(boost::iostreams::filtering_istream &file);

  MpsParser::parsekey parseRanges(boost::iostreams::filtering_istream &file);

  MpsParser::parsekey parseBounds(boost::iostreams::filtering_istream &file);
};

class FilereaderMps : public Filereader {
 public:
  FilereaderRetcode readModelFromFile(const HighsOptions options, HighsLp &lp);
  FilereaderRetcode readModelFromFile(const char *filename, HighsLp &model) {
    return FilereaderRetcode::OKAY;
  }
};

#endif