// TypeART library
//
// Copyright (c) 2017-2022 TypeART Authors
// Distributed under the BSD 3-Clause license.
// (See accompanying file LICENSE.txt or copy at
// https://opensource.org/licenses/BSD-3-Clause)
//
// Project home: https://github.com/tudasc/TypeART
//
// SPDX-License-Identifier: BSD-3-Clause
//

#ifndef TYPEART_MPI_INTERCEPTOR_INTERCEPTOR_FUNCTIONS_H
#define TYPEART_MPI_INTERCEPTOR_INTERCEPTOR_FUNCTIONS_H

#include <mpi.h>

#ifdef __cplusplus
extern "C" {
#endif

void typeart_check_send(const char* name, const void* called_from, const void* sendbuf, int count, MPI_Datatype dtype);

void typeart_check_recv(const char* name, const void* called_from, void* recvbuf, int count, MPI_Datatype dtype);

void typeart_check_send_and_recv(const char* name, const void* called_from, const void* sendbuf, int sendcount,
                                 MPI_Datatype sendtype, void* recvbuf, int recvcount, MPI_Datatype recvtype);

void typeart_unsupported_mpi_call(const char* name, const void* called_from);

void typeart_exit();

#ifdef __cplusplus
}
#endif

#endif  // TYPEART_MPI_INTERCEPTOR_INTERCEPTOR_FUNCTIONS_H
