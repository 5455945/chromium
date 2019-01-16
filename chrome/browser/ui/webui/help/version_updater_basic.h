// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_BASIC_H_
#define CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_BASIC_H_

#include "base/task_runner.h"
#include "base/compiler_specific.h"
#include "base/macros.h"
#include "chrome/browser/ui/webui/help/version_updater.h"

namespace{

struct zdx_upgrade_data {
  int zdx_upgrade_status;               // 更新状态
  long long zdx_upgrade_max_file_size;  // 服务端文件(zdx_installer.exe)总大小
  long long zdx_upgrade_download_size;  // 已经下载大小
  char zdx_upgrade_md5[32 + 1];         // 服务端文件md5码
  char zdx_upgrade_version[32];   // 服务端返回版本号
  char zdx_upgrade_memo[256];  // 更新描述，如果超过255个字符，截断
  char zdx_upgrade_filename[MAX_PATH];  // 服务端返回文件名称(只是文件名:zdx_install.exe,或
                   // zdx_install_1.0.0.16.exe，不包含url)
  char zdx_upgrade_api_url[MAX_PATH];  // 获取自动更新包api地址
  char zdx_upgrade_type[64];  // 更新包的类型，zdx_browser_win32_upgrade，zdx_browser_win64_upgrade
  char zdx_upgrade_client_md5[32 + 1];  // 上次更新包的md5码，保存在配置文件中
  char zdx_upgrade_current_version[32];  // 客户端版本
  char zdx_upgrade_client_path[MAX_PATH];  // 客户端安装路径
  char zdx_upgrade_url[512];               // 安装包下载url地址
  int zdx_upgrade_mode;  // 更新调用模式, 1:只获取服务端信息,10:下载更新,
                         // 20:本地安装, 30:执行全部过程
  zdx_upgrade_data();
};
zdx_upgrade_data::zdx_upgrade_data()
    : zdx_upgrade_status(0),
      zdx_upgrade_max_file_size(0),
      zdx_upgrade_download_size(0),
      zdx_upgrade_md5(""),
      zdx_upgrade_version(""),
      zdx_upgrade_memo(""),
      zdx_upgrade_filename(""),
      zdx_upgrade_api_url(""),
      zdx_upgrade_type(""),
      zdx_upgrade_client_md5(""),
      zdx_upgrade_current_version(""),
      zdx_upgrade_client_path(""),
      zdx_upgrade_url(""),
      zdx_upgrade_mode(30){};
}

// Bare bones implementation just checks if a new version is ready.
class VersionUpdaterBasic : public VersionUpdater {
 public:
  // VersionUpdater implementation.
  void CheckForUpdate(const StatusCallback& callback,
                      const PromoteCallback&) override;
 protected:
  friend class VersionUpdater;

  // Clients must use VersionUpdater::Create().
  VersionUpdaterBasic();
  ~VersionUpdaterBasic() override;

 private:
  void CheckUpgradeStatus(scoped_refptr<base::TaskRunner> task_runner,
                          VersionUpdaterBasic* obj,
                          HANDLE* hMap);
  void UpdateStatus(VersionUpdater::Status status,
                    int progress,
                    bool rollback,
                    const std::string& version,
                    int64_t size,
                    const base::string16& message);
  bool read_write_status(struct zdx_upgrade_data& zud,
                         HANDLE hMap,
                         bool is_read = true);
   StatusCallback callback_;

  DISALLOW_COPY_AND_ASSIGN(VersionUpdaterBasic);
};

#endif  // CHROME_BROWSER_UI_WEBUI_HELP_VERSION_UPDATER_BASIC_H_
