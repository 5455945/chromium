// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CHROME_BROWSER_UI_WEBUI_ZDX_MINER_ZDX_MINER_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_ZDX_MINER_ZDX_MINER_HANDLER_H_
#include "base/macros.h"
#include "content/public/browser/web_ui_message_handler.h"

class Browser;
class Profile;

// Handles actions on miner page.
class ZdxMinerHandler : public content::WebUIMessageHandler {
 public:
  explicit ZdxMinerHandler(content::WebUI* web_ui);
  ~ZdxMinerHandler() override;

  void RegisterMessages() override;

 private:

  const std::string GetDllName();
  const std::string GetAppDataPath();
  void SetUserID(const base::ListValue* args);
  void UnSetUserID(const base::ListValue* args);
  void MinerStop(const base::ListValue* args);
  void GetUserID(const base::ListValue* args);
  void GetMinerRate(const base::ListValue* args);
  void GetMinerYieldRate(const base::ListValue* args);
  void GetRemoveMinerRate(bool save = false);  // 到远程服务器获取算力
  double GetCalc(const std::string &miner_type,
                      const std::string &device_type,
                      int recapture = 0);
  void ShowMinerYieldRate();

  Profile* profile_;
  double m_xmr_rate;
  double m_eth_rate;
  bool   m_running;

  DISALLOW_COPY_AND_ASSIGN(ZdxMinerHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_ZDX_MINER_ZDX_MINER_HANDLER_H_
