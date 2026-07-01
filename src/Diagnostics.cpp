#include "Diagnostics.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <dbghelp.h>
#endif

#ifndef REALMZ_GIT_COMMIT
#define REALMZ_GIT_COMMIT "unknown"
#endif

#ifndef REALMZ_GIT_BRANCH
#define REALMZ_GIT_BRANCH "unknown"
#endif

namespace {

std::atomic<bool> handlers_installed{false};
std::mutex log_mutex;
std::string log_path;
std::string dump_path;
char last_context[1024] = {};

std::string timestamp_now() {
  using namespace std::chrono;
  auto now = system_clock::now();
  auto now_time = system_clock::to_time_t(now);
  auto millis = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000;

  std::tm local_time{};
#if defined(_WIN32)
  localtime_s(&local_time, &now_time);
#else
  localtime_r(&now_time, &local_time);
#endif

  char time_buf[64];
  std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &local_time);

  char ret[80];
  std::snprintf(ret, sizeof(ret), "%s.%03lld", time_buf, static_cast<long long>(millis));
  return ret;
}

std::filesystem::path diagnostics_directory() {
  std::error_code ec;
  auto dir = std::filesystem::current_path(ec);
  if (ec) {
    dir = ".";
  }
  dir /= "Realmz Crash Logs";
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    return ".";
  }
  return dir;
}

void ensure_paths() {
  if (!log_path.empty()) {
    return;
  }

  auto dir = diagnostics_directory();
  log_path = (dir / "realmz-diagnostics.log").string();
  dump_path = (dir / "realmz-crash.dmp").string();
}

void append_raw_line(const char* line) {
  ensure_paths();
  FILE* f = std::fopen(log_path.c_str(), "ab");
  if (!f) {
    return;
  }
  std::fprintf(f, "%s %s\n", timestamp_now().c_str(), line);
  std::fclose(f);
}

void append_line(const std::string& line) {
  std::lock_guard<std::mutex> g(log_mutex);
  append_raw_line(line.c_str());
}

std::string command_line_for_args(int argc, char** argv) {
  std::ostringstream out;
  for (int z = 0; z < argc; z++) {
    if (z) {
      out << ' ';
    }
    out << '"';
    if (argv && argv[z]) {
      out << argv[z];
    }
    out << '"';
  }
  return out.str();
}

std::string current_thread_id_string() {
  std::ostringstream out;
  out << std::this_thread::get_id();
  return out.str();
}

#if defined(_WIN32)
void format_realmz_location(void* address, char* output, size_t output_size) {
  HMODULE main_module = GetModuleHandleA(nullptr);
  HMODULE frame_module = nullptr;
  if (main_module &&
      GetModuleHandleExA(
          GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
          reinterpret_cast<LPCSTR>(address),
          &frame_module) &&
      frame_module == main_module) {
    std::snprintf(output, output_size, "realmz+0x%llX",
        static_cast<unsigned long long>(reinterpret_cast<uintptr_t>(address) - reinterpret_cast<uintptr_t>(main_module)));
    return;
  }
  std::snprintf(output, output_size, "external");
}

void write_stack_trace(const char* label) {
  void* frames[64] = {};
  USHORT frame_count = CaptureStackBackTrace(0, 64, frames, nullptr);

  append_raw_line(label ? label : "crash: stack");

  HANDLE process = GetCurrentProcess();
  SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
  BOOL have_symbols = SymInitialize(process, nullptr, TRUE);

  char storage[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
  auto* symbol = reinterpret_cast<SYMBOL_INFO*>(storage);
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = MAX_SYM_NAME;

  for (USHORT z = 0; z < frame_count; z++) {
    DWORD64 address = reinterpret_cast<DWORD64>(frames[z]);
    DWORD64 displacement = 0;
    char line[512];
    char location[64];

    format_realmz_location(frames[z], location, sizeof(location));

    if (have_symbols && SymFromAddr(process, address, &displacement, symbol)) {
      std::snprintf(line, sizeof(line),
          "crash: frame %02u address=%p location=%s symbol=%s+0x%llX",
          z,
          frames[z],
          location,
          symbol->Name,
          static_cast<unsigned long long>(displacement));
    } else {
      std::snprintf(line, sizeof(line),
          "crash: frame %02u address=%p location=%s",
          z,
          frames[z],
          location);
    }
    append_raw_line(line);
  }
}

void write_minidump(EXCEPTION_POINTERS* exception_info) {
  ensure_paths();

  HANDLE file = CreateFileA(
      dump_path.c_str(),
      GENERIC_WRITE,
      0,
      nullptr,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    append_raw_line("crash: failed to create minidump file");
    return;
  }

  MINIDUMP_EXCEPTION_INFORMATION minidump_exception{};
  minidump_exception.ThreadId = GetCurrentThreadId();
  minidump_exception.ExceptionPointers = exception_info;
  minidump_exception.ClientPointers = FALSE;

  BOOL ok = MiniDumpWriteDump(
      GetCurrentProcess(),
      GetCurrentProcessId(),
      file,
      MiniDumpNormal,
      exception_info ? &minidump_exception : nullptr,
      nullptr,
      nullptr);
  CloseHandle(file);

  append_raw_line(ok ? "crash: wrote minidump" : "crash: MiniDumpWriteDump failed");
}

LONG WINAPI unhandled_exception_filter(EXCEPTION_POINTERS* exception_info) {
  char line[256];
  const auto* record = exception_info ? exception_info->ExceptionRecord : nullptr;
  std::snprintf(line, sizeof(line), "crash: unhandled Windows exception code=0x%08lX address=%p",
      record ? record->ExceptionCode : 0,
      record ? record->ExceptionAddress : nullptr);
  append_raw_line(line);

  if (last_context[0]) {
    append_raw_line(last_context);
  }

  write_stack_trace("crash: stack");
  write_minidump(exception_info);
  return EXCEPTION_EXECUTE_HANDLER;
}
#endif

void signal_handler(int signal_number) {
  char line[128];
  std::snprintf(line, sizeof(line), "crash: fatal signal %d", signal_number);
  append_raw_line(line);

  if (last_context[0]) {
    append_raw_line(last_context);
  }

#if defined(_WIN32)
  write_stack_trace("crash: stack");
  write_minidump(nullptr);
#endif
  std::_Exit(128 + signal_number);
}

void terminate_handler() {
  append_raw_line("crash: std::terminate");

  if (auto exception = std::current_exception()) {
    try {
      std::rethrow_exception(exception);
    } catch (const std::exception& e) {
      std::string line = std::string("crash: exception what=") + e.what();
      append_raw_line(line.c_str());
    } catch (...) {
      append_raw_line("crash: exception type=unknown");
    }
  }

  if (last_context[0]) {
    append_raw_line(last_context);
  }

#if defined(_WIN32)
  write_stack_trace("crash: stack");
  write_minidump(nullptr);
#endif
  std::_Exit(1);
}

void install_handlers() {
  bool expected = false;
  if (!handlers_installed.compare_exchange_strong(expected, true)) {
    return;
  }

#if defined(_WIN32)
  SetUnhandledExceptionFilter(unhandled_exception_filter);
#endif
  std::signal(SIGABRT, signal_handler);
  std::signal(SIGFPE, signal_handler);
  std::signal(SIGILL, signal_handler);
  std::signal(SIGSEGV, signal_handler);
  std::signal(SIGTERM, signal_handler);
  std::set_terminate(terminate_handler);
}

} // namespace

void RealmzDiagnostics_Init(int argc, char** argv) {
  {
    std::lock_guard<std::mutex> g(log_mutex);
    ensure_paths();
  }

  install_handlers();

  append_line("diagnostics: start");
  append_line(std::string("diagnostics: log=") + log_path);
  append_line(std::string("diagnostics: dump=") + dump_path);
  append_line(std::string("diagnostics: git_commit=") + REALMZ_GIT_COMMIT);
  append_line(std::string("diagnostics: git_branch=") + REALMZ_GIT_BRANCH);
  append_line(std::string("diagnostics: cwd=") + std::filesystem::current_path().string());
  append_line(std::string("diagnostics: argv=") + command_line_for_args(argc, argv));
  append_line(std::string("diagnostics: thread=") + current_thread_id_string());
}

void RealmzDiagnostics_LogEvent(const char* event_name) {
  append_line(std::string("event: ") + (event_name ? event_name : "(null)"));
}

void RealmzDiagnostics_SetContext(const char* key, const char* value) {
  std::snprintf(last_context, sizeof(last_context), "context: %s=%s",
      key ? key : "(null)",
      value ? value : "(null)");
  append_line(last_context);
}

const char* RealmzDiagnostics_LogPath(void) {
  std::lock_guard<std::mutex> g(log_mutex);
  ensure_paths();
  return log_path.c_str();
}
