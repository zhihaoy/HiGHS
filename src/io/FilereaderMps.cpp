#include "FilereaderMps.h"
#include "HMpsFF.h"

FilereaderRetcode FilereaderMps::readModelFromFile(const HighsOptions options,
                                                  HighsLp& lp) {

  // todo
  // Which parser
  // will be (options.getValue("parser") == MpsParser::new)
  // For the moment use new one only, until you fix the old one too.

  MpsParser parser;
  parser.loadProblem(options, lp);
  // call MPSParser::loadProblem(arrays of HighsLp object)

  return FilereaderRetcode::OKAY;
}