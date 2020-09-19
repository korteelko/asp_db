#include "gtest/gtest.h"

#include "Common.h"
#include "Logging.h"

#include <filesystem>

namespace fs = std::filesystem;


/**
 * \brief Тест логирования
 * \note Логгер ассинхронный, так что ставим `sleep`
 *   после каждого обновления логгера
 * */
TEST(Logging, Full) {
  /* обычное логирование */
  EXPECT_EQ(Logging::InitDefault(), ERROR_SUCCESS_T);
  Logging::Append("long message for logging in default file, manual test");

  /* перерегистрируем логифайл */
  std::string lfile = "testolog";
  logging_cfg lcfg("main_logger", io_loglvl::err_logs, lfile,
      DEFAULT_MAXLEN_LOGFILE, 2, false);
  ASSERT_EQ(Logging::ResetInstance(lcfg), ERROR_SUCCESS_T);
  Logging::ClearLogFile();
  try {
    EXPECT_EQ(fs::file_size(lfile), 0);
  } catch (fs::filesystem_error &e) {
    std::cerr << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
  std::stringstream sstr;
  sstr << "nologable error";
  Logging::Append(io_loglvl::debug_logs, sstr);
  sleep(lcfg.flush_rate_sec + 1);
  EXPECT_EQ(fs::file_size(lfile), 0);
  Logging::Append(sstr);
  sleep(lcfg.flush_rate_sec + 1);
  EXPECT_NE(fs::file_size(lfile), 0);
  Logging::ClearLogFile();
  Logging::Append(io_loglvl::err_logs, std::string("Логируемая ошибка"));
  sleep(lcfg.flush_rate_sec + 1);
  EXPECT_NE(fs::file_size(lfile), 0);
  /* логирование ошибки */
  Logging::ClearLogFile();
  EXPECT_EQ(Logging::GetErrorCode(), ERROR_SUCCESS_T);
  ErrorWrap ew(ERROR_FILE_LOGGING_ST, "Тест логирования ошибки");
  ew.LogIt(io_loglvl::err_logs);
  EXPECT_TRUE(ew.IsLogged());
  sleep(lcfg.flush_rate_sec + 1);
  EXPECT_NE(fs::file_size(lfile), 0);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
