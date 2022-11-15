#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <typeart/TypeART.h>

struct test {
  int a;
  double b;
  unsigned int c;
  void* d;
  int e;
  struct test* f;
};

int main(int argc, char** argv) {
  MPI_Init(&argc, &argv);

#ifdef NOSTACK
  struct test* mystruct = malloc(sizeof(struct test) * 5);
#else
  struct test mystruct[5];
#endif

  typeart_pointer_info pointer_info;

#ifdef NOSTACK
  int* buffer = malloc(sizeof(int) * 50);
#else
  int buffer[50];
#endif

  typeart_status status = typeart_get_pointer_info(mystruct, &pointer_info);

  if (status == TYPEART_OK)
    printf("type (id=%i), count=%lu\n", pointer_info.type_id, pointer_info.count);
  else
    printf("[Demo] Toy Error: %i\n", status);

  status = typeart_get_pointer_info(&(mystruct[2].e), &pointer_info);

  if (status == TYPEART_OK)
    printf("(sub) type (id=%i), count=%lu\n", pointer_info.type_id, pointer_info.count);
  else
    printf("[Demo] Toy Error: %i\n", status);

  status = typeart_get_pointer_info(&(mystruct[2].e), &pointer_info);

  if (status == TYPEART_OK)
    printf("type (id=%i), count=%lu\n", pointer_info.type_id, pointer_info.count);
  else
    printf("[Demo] Toy Error: %i\n", status);

  status = typeart_get_pointer_info(&(mystruct[2].c), &pointer_info);

  if (status == TYPEART_OK)
    printf("(sub) type (id=%i), count=%lu\n", pointer_info.type_id, pointer_info.count);
  else
    printf("[Demo] Toy Error: %i\n", status);

  status = typeart_get_pointer_info(&(mystruct[2].c), &pointer_info);

  if (status == TYPEART_OK)
    printf("type (id=%i), count=%lu\n", pointer_info.type_id, pointer_info.count);
  else
    printf("[Demo] Toy Error: %i\n", status);

  size_t offset;
  status = typeart_get_containing_info(&(mystruct[2].c), &pointer_info, &offset);

  if (status == TYPEART_OK)
    printf("containing_type (id=%i), count=%lu, %p, %lu\n", pointer_info.type_id, pointer_info.count,
           pointer_info.base_addr, offset);
  else
    printf("[Demo] Toy Error: %i\n", status);

  printf("buffer\n");

  status = typeart_get_containing_info(&(buffer[20]), &pointer_info, &offset);

  if (status == TYPEART_OK)
    printf("containing_type (id=%i), count=%lu, %p, %lu\n", pointer_info.type_id, pointer_info.count,
           pointer_info.base_addr, offset);
  else
    printf("[Demo] Toy Error: %i\n", status);

  MPI_Sendrecv(mystruct, 1, MPI_INT, 0, 0, buffer + 20, 1, MPI_INT, 0, 0, MPI_COMM_SELF, MPI_STATUS_IGNORE);

  MPI_Finalize();

#ifdef NOSTACK
  free(mystruct);
#else
#endif

#ifdef NOSTACK
  free(buffer);
#else
#endif
}
