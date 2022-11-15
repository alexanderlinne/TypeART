// TypeART library
//
// Copyright (c) 2017-2021 TypeART Authors
// Distributed under the BSD 3-Clause license.
// (See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/BSD-3-Clause)
//
// Project home: https://github.com/tudasc/TypeART
//
// SPDX-License-Identifier: BSD-3-Clause
//

#ifndef TYPEART_MPI_INTERCEPTOR_UTIL_H
#define TYPEART_MPI_INTERCEPTOR_UTIL_H

#include <mpi.h>
#include <typeart/TypeART.hpp>

namespace typeart {

const char* error_message_for(Status status);

std::string error_message_for(int mpierr);

const char* combiner_name_for(int combiner);

}  // namespace typeart

#endif  // TYPEART_MPI_INTERCEPTOR_UTIL_H
