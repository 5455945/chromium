// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/help/version_updater_basic.h"

#include "base/bind.h"
#include "base/strings/string16.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/upgrade_detector/upgrade_detector.h"
#include "components/version_info/version_info.h"

#include <windows.h>
#include <shlwapi.h>
#include <memory>
#include <thread>

const char kZdxUpgradeExe[] = "zdx_upgrade.exe";
const char kZdxUpgradeSharedMemory[] = "zdx_upgrade_shared_memory_";
enum class shared_memory_size { sms_size = 4096 };

enum class zdx_upgrade_status {
  init = 0,                // 初始化状态
  upgrade_start = 1,       // 开始更新
  latest_version = 2,      // 已经是最新版本
  check_info_fail = 3,     // 检查信息失败
  check_info_success = 4,  // 检查信息完成
  downloading = 5,         // 下载中
  download_fail = 6,       // 下载失败
  download_success = 7,    // 下载完成
  installing = 8,          // 安装中
  install_fail = 9,        // 安装失败
  upgrade_success = 10,    // 更新完成
};

int UpgradeProcess(const std::string& cmdline, std::wstring& error) {
  char szPath[MAX_PATH] = {0};
  GetModuleFileNameA(NULL, szPath, MAX_PATH);
  std::string module_path = szPath;
  module_path = module_path.substr(0, module_path.rfind("\\") + 1);
  std::string upgrade_filename = module_path + kZdxUpgradeExe;
  STARTUPINFOA si = {sizeof(si)};
  PROCESS_INFORMATION pi = {0};
  char szCmdLine[1024] = {0};
  memcpy(szCmdLine, cmdline.c_str(), cmdline.length());
  BOOL bRet = ::CreateProcessA(upgrade_filename.c_str(), szCmdLine, NULL, NULL,
                               FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
  if (!bRet) {
    DWORD dwErrCode = GetLastError();
    wchar_t* lpErrMsg = NULL;
    ::FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
        dwErrCode, 0, (LPWSTR)&lpErrMsg, 0, NULL);
    ::OutputDebugStringW(lpErrMsg);
    error = lpErrMsg;
    error.erase(error.find_last_not_of(L"\r\n") + 1);
    if (lpErrMsg) {
      ::LocalFree(lpErrMsg);
      lpErrMsg = NULL;
    }
  }
  ::CloseHandle(pi.hThread);

  DWORD exit_code = ERROR_SUCCESS;
  DWORD wr = ::WaitForSingleObject(pi.hProcess, INFINITE);
  if (WAIT_OBJECT_0 != wr || !::GetExitCodeProcess(pi.hProcess, &exit_code)) {
    return -1;
  }

  ::CloseHandle(pi.hProcess);
  return exit_code;
}

void VersionUpdaterBasic::CheckUpgradeStatus(
    scoped_refptr<base::TaskRunner> task_runner,
    VersionUpdaterBasic* obj,
    HANDLE* hMap) {
  DCHECK(hMap != NULL && *hMap != NULL);
  HANDLE hmap = *hMap;
  VersionUpdater::Status status = UPDATING;
  do {
    read_write_status(zud_, hmap, true);
    std::wstring info = base::UTF8ToUTF16(zud_.zdx_upgrade_memo);
    switch ((zdx_upgrade_status)zud_.zdx_upgrade_status) {
      case zdx_upgrade_status::check_info_fail:
      case zdx_upgrade_status::download_fail:
      case zdx_upgrade_status::install_fail:
        status = FAILED;
        break;
      case zdx_upgrade_status::latest_version:
        status = UPDATED;
        break;
      case zdx_upgrade_status::upgrade_success:
        status = NEARLY_UPDATED;
        break;
      case zdx_upgrade_status::init:
      case zdx_upgrade_status::upgrade_start:
      case zdx_upgrade_status::check_info_success:
      case zdx_upgrade_status::downloading:
      case zdx_upgrade_status::download_success:
      case zdx_upgrade_status::installing:
        status = UPDATING;
        break;
      default:
        status = FAILED;
        break;
    }
    task_runner->PostTask(
        FROM_HERE, base::BindOnce(&VersionUpdaterBasic::UpdateStatus,
                                  base::Unretained(obj), status, 0, false,
                                  std::string(), 0, info));
    std::this_thread::sleep_for(std::chrono::microseconds(10));
  } while (status == UPDATING);
  // 更新成功，删除本地下载文件
  std::string filename = zud_.zdx_upgrade_filename;
  if ((zud_.zdx_upgrade_status == (int)zdx_upgrade_status::upgrade_success) &&
      filename.length() > 0) {
    char szPath[MAX_PATH] = {0};
    GetModuleFileNameA(NULL, szPath, MAX_PATH);
    std::string module_path = szPath;
    module_path = module_path.substr(0, module_path.rfind("\\") + 1);
    std::string upgrade_filename = module_path + filename;
    if (::PathFileExistsA(upgrade_filename.c_str())) {
      ::DeleteFileA(upgrade_filename.c_str());
    }
  }

  if (hmap) {
    ::CloseHandle(hmap);
    hmap = NULL;
  }
}

void VersionUpdaterBasic::UpdateStatus(VersionUpdater::Status status,
    int progress,
    bool rollback,
    const std::string& version,
    int64_t size,
    const base::string16& message) {
  callback_.Run(status, progress, rollback, version, size, message);
}

void VersionUpdaterBasic::CheckForUpdate(const StatusCallback& status_callback,
                                         const PromoteCallback&) {
  callback_ = status_callback;
  callback_.Run(CHECKING, 0, false, std::string(), 0, base::string16());

  HANDLE hMap = MapGet();
  if (!hMap) {
    callback_.Run(FAILED, 0, false, std::string(), 0, base::string16());
    return;
  }

  read_write_status(zud_, hMap, true);
  HANDLE hMap1 = hMap;
  HANDLE hMap2 = hMap;
  std::thread tCheckStatus([&](scoped_refptr<base::TaskRunner> task_runner,
                               VersionUpdaterBasic* obj, HANDLE* hMap) {
        DCHECK(hMap != NULL && *hMap != NULL);
        CheckUpgradeStatus(task_runner, obj, hMap);
      },
      base::ThreadTaskRunnerHandle::Get(), this, &hMap1);

  if (zud_.zdx_upgrade_status == (int)zdx_upgrade_status::init) {
    std::thread tUpgrade(
      [&](scoped_refptr<base::TaskRunner> task_runner,
            VersionUpdaterBasic* obj, HANDLE* hMap) {  // 启动更新线程
      DCHECK(hMap != NULL && *hMap != NULL);
      HANDLE hmap = *hMap;
       std::string upgrade_api_url = "https://zdx.app/api/v1/desktop/update";
      // 参考about_handler.cc
      std::string upgrade_type = "zdx_browser_win32_upgrade";
      if (sizeof(void*) == 8) {
        upgrade_type = "zdx_browser_win64_upgrade";
      }
      std::string upgrade_client_md5 = "";
      std::string upgrade_current_version = version_info::GetVersionNumber();
      memset(zud_.zdx_upgrade_api_url, 0, sizeof(zud_.zdx_upgrade_api_url));
      memcpy(zud_.zdx_upgrade_api_url, upgrade_api_url.c_str(), upgrade_api_url.length());
      memset(zud_.zdx_upgrade_type, 0, sizeof(zud_.zdx_upgrade_type));
      memcpy(zud_.zdx_upgrade_type, upgrade_type.c_str(), upgrade_type.length());
      memset(zud_.zdx_upgrade_client_md5, 0, sizeof(zud_.zdx_upgrade_client_md5));
      memcpy(zud_.zdx_upgrade_client_md5, upgrade_client_md5.c_str(), upgrade_client_md5.length());
      memset(zud_.zdx_upgrade_current_version, 0, sizeof(zud_.zdx_upgrade_current_version));
      memcpy(zud_.zdx_upgrade_current_version, upgrade_current_version.c_str(), upgrade_current_version.length());
      read_write_status(zud_, hmap, false);
      std::wstring error;
      UpgradeProcess("-mode 30", error);
      std::this_thread::sleep_for(std::chrono::microseconds(50));
    },
    base::ThreadTaskRunnerHandle::Get(), this, &hMap2);
    tUpgrade.detach();
  }
  tCheckStatus.detach();
}

VersionUpdater* VersionUpdater::Create(content::WebContents * web_contents) {
  return new VersionUpdaterBasic;
}

VersionUpdaterBasic::VersionUpdaterBasic() {
  hMap_ = NULL;
}

VersionUpdaterBasic::~VersionUpdaterBasic() {
}

HANDLE VersionUpdaterBasic::MapGet() {
  HANDLE hMap =
        ::OpenFileMappingA(FILE_MAP_ALL_ACCESS, 0, kZdxUpgradeSharedMemory);
  if (!hMap) {  // 已经有页面或者自动更新应用打开更新了
    hMap = ::CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,
                                  (int)shared_memory_size::sms_size,
                                  kZdxUpgradeSharedMemory);
  }
  return hMap;
}

void VersionUpdaterBasic::MapClose() {
  if (hMap_) {
    CloseHandle(hMap_);
    hMap_ = NULL;
  }
}

bool VersionUpdaterBasic::read_write_status(struct zdx_upgrade_data & zud,
                                              HANDLE hMap, bool is_read) {
  if (!hMap) {
    return false;
  }
  HANDLE pBuffer = ::MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
  if (!pBuffer) {
    return false;
  }
  if (is_read) {
    memcpy((void*)&zud, pBuffer, sizeof(zud));
  } else {
    memcpy(pBuffer, (void*)&zud, sizeof(zud));
  }
  if (pBuffer) {
    ::UnmapViewOfFile(pBuffer);
    pBuffer = NULL;
  }
  return true;
}