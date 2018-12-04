#include <iostream>

#include <unistd.h> // unlink, rmdir

#include "common.hpp"
#include "errors.hpp"
#include "components/config.hpp"
#include "components/logger.hpp"
#include "utils/file.hpp"

#include "common/test.hpp"

using namespace polybar;

namespace {

  class temp_directory {
  public:
    temp_directory(string directory) {
      if (file_util::is_absolute(directory)) {
        m_directory = directory;
      } else {
        m_directory = "/tmp/" + directory;
      }

      if (m_directory.length() < 6 || m_directory.compare(m_directory.length() - 6, 6, "XXXXXX") != 0) {
        m_directory += "_XXXXXX";
      }

      if(mkdtemp(&m_directory[0]) == nullptr) {
        throw system_error("mkdtemp failed");
      }
    }

    ~temp_directory() {
      // this will fail if directory is not empty: tests should clear their own files
      rmdir(m_directory.c_str());
    }

    const string& directory() const {
      return m_directory;
    }

  private:
    string m_directory;
  };

  class temp_filestream : public fd_stream<std::iostream> {
  public:
    explicit temp_filestream(string directory)
      : temp_filestream(directory, "filename_XXXXXX")
    {}

    ~temp_filestream() {
      unlink(m_full_path.c_str());
    }

    const string& filename() const {
      return m_filename;
    }

    const string& directory() const {
      return m_directory;
    }

    const string& full_path() const {
      return m_full_path;
    }

  private:
    temp_filestream(string directory, string filename)
      : fd_stream<std::iostream>(open_temp_file(directory, filename))
      , m_directory(std::move(directory))
      , m_filename(std::move(filename))
      , m_full_path(m_directory + "/" + m_filename)
    {}

    static int open_temp_file(string& directory, string& filename) {
      directory += "/";
      directory += filename;

      auto fd = mkstemp(&directory[0]);
      if (fd == -1) {
        throw system_error("mkstemp failed");
      }

      filename = directory.substr(directory.length() - filename.length());
      directory.erase(directory.length() - (filename.length() + 1));

      return fd;
    }

  private:
    string m_directory;
    string m_filename;
    string m_full_path;
  };

} // (anonymous namespace)

TEST(Config, IncludeFileAbsolutePath) {
  temp_directory temp_dir("polybar_unit_test");

  temp_filestream base_file(temp_dir.directory());
  temp_filestream include_file(temp_dir.directory());

  // log file needs section [bar/BARNAME]
  base_file << "[bar/test_bar]\n";
  base_file << "include-file = " << include_file.full_path() << "\n";
  base_file << std::flush;

  include_file << "key = value\n";
  include_file << std::flush;

  auto const& null_logger = logger::make(loglevel::NONE); // everything is discarded

  config test_config(null_logger, string{ base_file.full_path() }, "test_bar");

  ASSERT_TRUE(test_config.has("bar/test_bar", "key"));
  ASSERT_EQ(test_config.get("bar/test_bar", "key"), "value");
}

TEST(Config, IncludeFileRelativePath) {
  temp_directory temp_dir("polybar_unit_test");

  temp_filestream base_file(temp_dir.directory());
  temp_filestream include_file(temp_dir.directory());

  // log file needs section [bar/BARNAME]
  base_file << "[bar/test_bar]\n";
  base_file << "include-file = " << include_file.filename() << "\n";
  base_file << std::flush;

  include_file << "key = value\n";
  include_file << std::flush;

  auto const& null_logger = logger::make(loglevel::NONE); // everything is discarded

  config test_config(null_logger, string{ base_file.full_path() }, "test_bar");

  ASSERT_TRUE(test_config.has("bar/test_bar", "key"));
  ASSERT_EQ(test_config.get("bar/test_bar", "key"), "value");
}
