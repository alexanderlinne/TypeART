/*
 * Logger.h
 *
 *  Created on: Jan 3, 2018
 *      Author: ahueck
 */

#ifndef LIB_LOGGER_H_
#define LIB_LOGGER_H_

#include <llvm/Support/raw_ostream.h>

#ifndef LOG_LEVEL
/*
 * Usually set at compile time: -DLOG_LEVEL=<N>, N in [0, 3] for output
 * 3 being most verbose
 */
#define LOG_LEVEL 2
#endif

#ifndef LOG_BASENAME_FILE
#define LOG_BASENAME_FILE __FILE__
#endif

// clang-format off
#define OO_LOG_LEVEL_MSG(LEVEL_NUM, LEVEL, MSG) \
  if ((LEVEL_NUM) <= LOG_LEVEL) { \
    llvm::errs() << (LEVEL) << " " << LOG_BASENAME_FILE << ":" << __func__ << ":" << __LINE__ << ": " << MSG << "\n"; /* NOLINT */ \
  }

#define LOG_DEBUG(MSG) OO_LOG_LEVEL_MSG(3, "[Debug]", MSG)
#define LOG_INFO(MSG) OO_LOG_LEVEL_MSG(2, "[Info]", MSG)
#define LOG_WARNING(MSG) OO_LOG_LEVEL_MSG(1, "[Warning]", MSG)
#define LOG_ERROR(MSG) OO_LOG_LEVEL_MSG(1, "[Error]", MSG)
#define LOG_FATAL(MSG) OO_LOG_LEVEL_MSG(0, "[Fatal]", MSG)
#define LOG_MSG(MSG) llvm::outs() << MSG << "\n"; /* NOLINT */
// clang-format on

#endif /* LIB_LOGGER_H_ */
