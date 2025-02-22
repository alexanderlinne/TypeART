// REQUIRES: mpi
// UNSUPPORTED: asan
// UNSUPPORTED: tsan
// clang-format off
// RUN: TYPEART_SOURCE_LOCATION=error %run %s --mpi_intercept --compile_flags "-g" --executable %s.exe --command "%mpi-exec -n 2 --output-filename %s.log %s.exe"
// RUN: cat "%s.log/1/rank.0/stderr" | %filecheck --check-prefixes CHECK,RANK0 %s
// RUN: cat "%s.log/1/rank.1/stderr" | %filecheck --check-prefixes CHECK,RANK1 %s
// clang-format on

#include "Util.hpp"

#include <mpi.h>

constexpr auto n = 16;

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

  // CHECK: [Trace] TypeART Runtime Trace

  padded_array<n> data;

  // clang-format off
  // RANK0: R[0]T[{{[0-9]*}}][Info] at 0x{{.*}}: MPI_Send: successfully checked send-buffer 0x{{.*}} of type [1 x double[16]] against 16 elements of MPI type "MPI_DOUBLE"
  // RANK1: R[1]T[{{[0-9]*}}][Info] at 0x{{.*}}: MPI_Recv: successfully checked recv-buffer 0x{{.*}} of type [1 x double[16]] against 16 elements of MPI type "MPI_DOUBLE"
  // CHECK-NOT: R[{{0|1}}][Error]{{.*}}
  // clang-format on
  run_test(data, n, MPI_DOUBLE);

  // clang-format off
  // RANK0: R[0]T[{{[0-9]*}}][Error] run_test(void*, int, ompi_datatype_t*)[0x{{.*}}] at {{(/.*)*}}/Util.hpp:{{[0-9?]*}}: MPI_Send: type error while checking send-buffer 0x{{.*}} of type [1 x double[16]] against 17 elements of MPI type "MPI_DOUBLE": buffer too small (16 elements, 17 required)
  // RANK1: R[1]T[{{[0-9]*}}][Error] run_test(void*, int, ompi_datatype_t*)[0x{{.*}}] at {{(/.*)*}}/Util.hpp:{{[0-9?]*}}: MPI_Recv: type error while checking recv-buffer 0x{{.*}} of type [1 x double[16]] against 17 elements of MPI type "MPI_DOUBLE": buffer too small (16 elements, 17 required)
  // clang-format on
  run_test(data, n + 1, MPI_DOUBLE);

  // RANK0: R[0]T[{{[0-9]*}}][Info] CCounter { Send: 2 Recv: 0 Send_Recv: 0 Unsupported: 0 MAX RSS[KBytes]: {{[0-9]+}} }
  // RANK1: R[1]T[{{[0-9]*}}][Info] CCounter { Send: 0 Recv: 2 Send_Recv: 0 Unsupported: 0 MAX RSS[KBytes]: {{[0-9]+}} }
  // CHECK: R[{{0|1}}]T[{{[0-9]*}}][Info] MCounter { Error: 0 Null_Buf: 0 Null_Count: 0 Type_Error: 1 }
  MPI_Finalize();
  return 0;
}
