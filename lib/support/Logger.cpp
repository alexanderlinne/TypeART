#include "support/Logger.hpp"

#include <spdlog/pattern_formatter.h>

#ifdef TYPEART_MPI_LOGGER
#include <mpi.h>
#endif

class LogLevelFlag : public spdlog::custom_flag_formatter {
 public:
  void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override {
    std::string level;

    switch (msg.level) {
      case spdlog::level::trace:
        level = "Trace";
        break;
      case spdlog::level::debug:
        level = "Debug";
        break;
      case spdlog::level::info:
        level = "Info";
        break;
      case spdlog::level::warn:
        level = "Warning";
        break;
      case spdlog::level::err:
        level = "Error";
        break;
      case spdlog::level::critical:
        level = "Critical";
        break;
      default:
        level = "Unknown";
        break;
    }
    dest.append(level.data(), level.data() + level.size());
  }

  std::unique_ptr<custom_flag_formatter> clone() const override {
    return spdlog::details::make_unique<LogLevelFlag>();
  }
};

#ifdef TYPEART_MPI_LOGGER
class MPIRankPrefixFlag : public spdlog::custom_flag_formatter {
  std::string rank = "";

 public:
  void format(const spdlog::details::log_msg&, const std::tm&, spdlog::memory_buf_t& dest) override {
    int initialized, finalized;
    MPI_Initialized(&initialized);
    MPI_Finalized(&finalized);

    if (initialized != 0 && finalized == 0) {
      int mpi_rank;
      MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
      rank = fmt::format("R[{}]", mpi_rank);
    }

    dest.append(rank.data(), rank.data() + rank.size());
  }

  std::unique_ptr<custom_flag_formatter> clone() const override {
    return spdlog::details::make_unique<MPIRankPrefixFlag>();
  }
};
#endif

namespace typeart {

std::shared_ptr<spdlog::logger> logger = []() {
  auto result = spdlog::stderr_logger_mt("typeart");
  result->set_level(spdlog::level::trace);
  auto formatter = std::make_unique<spdlog::pattern_formatter>();
#ifdef TYPEART_MPI_LOGGER
  formatter->add_flag<MPIRankPrefixFlag>('M').add_flag<LogLevelFlag>('L').set_pattern("[Typeart][%s:%#]%MT[%t][%L] %v");
#else
  formatter->add_flag<LogLevelFlag>('L').set_pattern("[Typeart][%s:%#]T[%t][%L] %v");
#endif
  result->set_formatter(std::move(formatter));
  return result;
}();

}  // namespace typeart
