import sys

with open(sys.argv[1], "w") as file:
    file.write("#include \"runtime/allocator/Config.h\"\n")
    file.write("using namespace typeart::runtime::allocator::config::stack;\n")
    for i in range(2, 30):
        file.write("extern const int64_t typeart_stack_region_offset_for_size_{0} = region_offset_for({0});\n".format(pow(2, i)))
