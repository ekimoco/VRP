#include "win32/hw.hpp"
#include "win32/literals.hpp"
#include <windows.h>
#include <winternl.h>

static_assert(sizeof(uint32_t) == sizeof(DWORD), "REG_DWORD must be 32-bit");

namespace {
// NTSTATUS and type shit. VERY messy so not exposing on the public side... wtf
constexpr NTSTATUS STATUS_SUCCESS_CODE = static_cast<NTSTATUS>(0x0000'0000);
constexpr NTSTATUS STATUS_INFO_LENGTH_MISMATCH =
    static_cast<NTSTATUS>(0xC000'0004);

typedef NTSTATUS(WINAPI *PFN_NT_QUERY_SYSTEM_INFORMATION)(
    ULONG SystemInformationClass, PVOID SystemInformation,
    ULONG SystemInformationLength, PULONG ReturnLEgnth);

struct PROF_SYSTEM_PROCESS_INFORMATION {
  ULONG NextEntryOffset;
  ULONG NumberOfThreads;
  LARGE_INTEGER WorkingSetPrivateSize; // Windows 7+
  ULONG HardFaultCount;
  ULONG NumberOfThreadsHighWatermark;
  ULONGLONG CycleTime;
  LARGE_INTEGER CreateTime;
  LARGE_INTEGER UserTime;
  LARGE_INTEGER KernelTime;
  UNICODE_STRING ImageName; // holds process name length and ptr
  LONG BasePriority;
  HANDLE UniqueProcessId; // PID
  HANDLE InheritedFromUniqueProcessId;
  // remaining fields are ignored as we rely on NextEntryOffset
};
} // namespace

bool win32::hw::CaptureSnapshot(SysSnapshot &out, uint32_t err) {
  HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
  if (!hNtdll)
    return false;

  auto NtQuerySystemInformation =
      reinterpret_cast<PFN_NT_QUERY_SYSTEM_INFORMATION>(
          GetProcAddress(hNtdll, "NtQuerySystemInformation"));
  if (!NtQuerySystemInformation)
    return false;

  uint32_t bufSize = 0x100000;
  std::vector<uint8_t> buf(bufSize);
  ULONG retLen = 0;

  NTSTATUS status = NtQuerySystemInformation(
      SYSTEM_INFORMATION_CLASS::SystemProcessInformation, buf.data(), bufSize,
      &retLen);

  while (status == STATUS_INFO_LENGTH_MISMATCH) {
    bufSize = retLen /* add safe padding */ + 4096;
    buf.resize(bufSize);
    status = NtQuerySystemInformation(
        SYSTEM_INFORMATION_CLASS::SystemProcessInformation, buf.data(), bufSize,
        &retLen);
  }

  if (status != STATUS_SUCCESS_CODE) {
    err = static_cast<uint32_t>(status);
    return false;
  }

  auto pCurrent =
      reinterpret_cast<PROF_SYSTEM_PROCESS_INFORMATION *>(buf.data());
  out.processes.clear();

  while (pCurrent) {
  win32::proc::ProcessEntry entry{};
    entry.pid = HandleToULong(pCurrent->UniqueProcessId);

    /*
    By the definition of winnt.h
    ----------------------------------------------------------
    320 |  typedef wchar_t WCHAR;
    ...
    324 |  typedef CONST WCHAR *NWPSTR, *LPWSTR, *PWSTR;
    ----------------------------------------------------------
    And by definition of winternl.h:
    ----------------------------------------------------------
    37 |  typedef struct _UNICODE_STRING {
    38 |    USHORT Length;
    39 |    USHORT MaximumLength;
    40 |    PWSTR Buffer;
    41 |  } UNICODE_STRING;
    ----------------------------------------------------------
    type of ImageName.Buffer is effectively wchar_t*,
    which we can put in nw::narrow
    */

    size_t charCount = pCurrent->ImageName.Length / sizeof(wchar_t);
    if (pCurrent->ImageName.Buffer != nullptr &&
        pCurrent->ImageName.Length > 0) {

      entry.exeName = nw::narrow(pCurrent->ImageName.Buffer);
    } else {
      entry.exeName = "[System Idle Process]";
    }

    entry.key = win32::literals::NormalizeName(entry.exeName);
    out.processes.push_back(entry);

    if (pCurrent->NextEntryOffset == 0)
      break;

    pCurrent = reinterpret_cast<PROF_SYSTEM_PROCESS_INFORMATION *>(
        reinterpret_cast<uint8_t *>(pCurrent) + pCurrent->NextEntryOffset);
  }
  return true;
}