/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef IO_FILEREADER_LP_H_
#define IO_FILEREADER_LP_H_

#include "Filereader.h"

class FilereaderLp : public Filereader {
 public:
  FilereaderRetcode readModelFromFile(const char* filename, HighsLp& model);
};

#endif