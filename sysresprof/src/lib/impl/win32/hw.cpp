#include "win32/hw.hpp"
#include "win32/literals.hpp"
#include "win32/reg.hpp"
#include <boost/nowide/config.hpp>
#include <d3d12.h>
#include <dxgi.h>
#include <format>
#include <windows.h>
#include <winternl.h>

static_assert(sizeof(uint32_t) == sizeof(DWORD), "REG_DWORD must be 32-bit");

namespace hwss = win32::hw::snapshot;
namespace hwpf = win32::hw::profile;
namespace nw = boost::nowide;

namespace {
hwpf::GpuKind DetectKind(IDXGIAdapter1 *adapter, const DXGI_ADAPTER_DESC1 &d) {
  ID3D12Device *device = nullptr;
  if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0,
                                  IID_PPV_ARGS(&device)))) {
    D3D12_FEATURE_DATA_ARCHITECTURE arch{};
    arch.NodeIndex = 0;
    HRESULT hr = device->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE, &arch,
                                             sizeof(arch));
    device->Release();
    if (SUCCEEDED(hr))
      return arch.UMA ? hwpf::GpuKind::Integrated : hwpf::GpuKind::Discrete;
  }

  // Reached when D3D12 isn't available or the query failed
  constexpr uint64_t kOneGiB = 1ull << 30;
  if (d.DedicatedVideoMemory < kOneGiB)
    return hwpf::GpuKind::Integrated;
  return hwpf::GpuKind::Unknown;
}

std::string ReadCpuName(std::string &err) {
  std::string regVal;

  LSTATUS regSetResult = win32::reg::ReadRegValWithStatus(
      HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
      "ProcessorNameString", regVal);

  if (regSetResult != ERROR_SUCCESS) {
    err += "CPU name registry read failed (" + std::to_string(regSetResult) +
           "); ";
    return "Unknown CPU";
  }
  return regVal;
}

std::vector<hwpf::GpuProfile> ReadGpus(std::string &err) {
  std::vector<hwpf::GpuProfile> out;

  IDXGIFactory1 *factory = nullptr;
  HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
  if (FAILED(hr)) {
    err += "CreatedDXGIFactory1 failed; ";
    return out;
  }

  for (UINT i = 0;; ++i) {
    IDXGIAdapter1 *adapter = nullptr;
    hr = factory->EnumAdapters1(i, &adapter);
    if (hr == DXGI_ERROR_NOT_FOUND)
      break; // no more adapters
    if (FAILED(hr)) {
      err += "EnumAdapters1 failed; ";
      break;
    }

    DXGI_ADAPTER_DESC1 d{};
    if (SUCCEEDED(adapter->GetDesc1(&d)) &&
        !(d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) {
      uint64_t luid = (uint64_t(uint32_t(d.AdapterLuid.HighPart)) << 32) |
                      d.AdapterLuid.LowPart;

      // Same hardware IDs = same physical card seen through a virtual display
      auto existing = std::find_if(out.begin(), out.end(), [&](const auto &g) {
        return g.vendorId == d.VendorId && g.deviceId == d.DeviceId &&
               g.subSysId == d.SubSysId && g.revision == d.Revision;
      });

      if (existing != out.end()) {
        existing->luids.push_back(luid);
      } else {
        hwpf::GpuProfile g;
        g.index = static_cast<uint8_t>(out.size());
        g.luids.push_back(luid);
        g.vendorId = d.VendorId;
        g.deviceId = d.DeviceId;
        g.subSysId = d.SubSysId;
        g.revision = d.Revision;
        g.gpuName = nw::narrow(d.Description);
        g.kind = DetectKind(adapter, d);
        g.capDedicatedRam = d.DedicatedVideoMemory;
        g.capDedicatedSystemRam = d.DedicatedSystemMemory;
        g.capSharedRam = d.SharedSystemMemory;
        out.push_back(std::move(g));
      }
    }
    adapter->Release();
  }
  factory->Release();
  return out;
}

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

bool hwss::CaptureSnapshot(SysSnapshot &out, uint32_t err) {
  HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
  if (!hNtdll)
    return false;

  auto NtQuerySystemInformation =
      reinterpret_cast<PFN_NT_QUERY_SYSTEM_INFORMATION>(
          reinterpret_cast<void (*)()>(
              GetProcAddress(hNtdll, "NtQuerySystemInformation")));

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

const hwpf::SystemProfile &hwpf::GetProfile(std::string &err) {
  static std::string initErr;
  static const SystemProfile profile = [] {
    SystemProfile p;
    p.cpuName = ReadCpuName(initErr);
    p.logicalCores = GetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    p.gpuProfiles = ReadGpus(initErr);
    return p;
  }();
  err = initErr;
  return profile;
}