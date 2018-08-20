// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/zdx_miner/zdx_miner_handler.h"

#include <Shlobj.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>
#include "base/json/json_reader.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_navigator.h"
#include "chrome/browser/ui/profile_chooser_constants.h"
#include "chrome/browser/ui/webui/signin/login_ui_service_factory.h"
#include "chrome/common/url_constants.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/signin/core/browser/signin_metrics.h"
#include "ui/base/page_transition_types.h"
#pragma comment(lib, "shell32.lib")

ZdxMinerHandler::ZdxMinerHandler(content::WebUI* web_ui)
    : profile_(Profile::FromWebUI(web_ui)),
      m_xmr_rate(0),
      m_eth_rate(0),
      m_running(true) {}

ZdxMinerHandler::~ZdxMinerHandler() {
  m_running = false;
}

void ZdxMinerHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "setUserID",
      base::Bind(&ZdxMinerHandler::SetUserID, base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getMinerRate",
      base::Bind(&ZdxMinerHandler::GetMinerRate, base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getMinerYieldRate",
      base::Bind(&ZdxMinerHandler::GetMinerYieldRate, base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "getUserID",
      base::Bind(&ZdxMinerHandler::GetUserID, base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "unsetUserID",
      base::Bind(&ZdxMinerHandler::UnSetUserID, base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "minerStop",
      base::Bind(&ZdxMinerHandler::MinerStop, base::Unretained(this)));

  ShowMinerYieldRate();
  GetUserID(nullptr);
}

void ZdxMinerHandler::SetUserID(const base::ListValue* args) {
  std::string UserID;
  std::string body("{\"rt\":-1,\"error\":\"UserID error\"}");
  int ret = 0;
  if (!args->GetString(0, &UserID)) {
    base::Value result(body);
    web_ui()->CallJavascriptFunctionUnsafe("miner.setUserIDResult", result);
    return;
  }

  if (UserID.length() > 0) {
    std::string filename =
        GetAppDataPath() + "\\ZdxBrowser\\User Data\\Default\\user_id";
    std::ofstream out(filename, std::ios::out | std::ios::trunc);
    if (out.is_open()) {
      out << UserID;
      out.close();
    }

    typedef int(__stdcall * pFunRunReLoad)(const char* url, char* body,
                                           size_t max_size);
    HINSTANCE hApp = ::LoadLibraryExA(GetDllName().c_str(), NULL,
                                      LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hApp) {
      pFunRunReLoad pRunReLoad =
          (pFunRunReLoad)::GetProcAddress(hApp, "RunReload");
      if (pRunReLoad) {
        char sbody[4096];
        memset(sbody, 0, 4096);
        size_t sbody_len = 4096;
        ret = pRunReLoad("http://127.0.0.1:2492/api/reload?dev=cpu", sbody,
                         sbody_len);
        if (ret > 0) {
          body = sbody;
        }
      }
      FreeLibrary(hApp);
      hApp = nullptr;
    }
  }

  base::Value result(body);
  web_ui()->CallJavascriptFunctionUnsafe("miner.setUserIDResult", result);
}

void ZdxMinerHandler::GetUserID(const base::ListValue* args) {
  std::string user_id="";
  std::string filename =
      GetAppDataPath() + "\\ZdxBrowser\\User Data\\Default\\user_id";
  std::ifstream in(filename);
  if (in.is_open()) {
    in >> user_id;
    in.close();
  }
  if (user_id.length() > 0) {
    base::Value result(user_id);
    web_ui()->CallJavascriptFunctionUnsafe("miner.getUserIDResult", result);
  }
}

void ZdxMinerHandler::GetMinerRate(const base::ListValue* args) {
  double calc = 0;   // 算力
  std::string type;  // type = "xmr" || "eth"
  int mode =
      0;  // mode = 0,不访问服务器; mode = 1,读本地文件; mode = 2,访问服务器.
  if (!args->GetDouble(0, &calc) || !args->GetString(1, &type) ||
      !args->GetInteger(2, &mode)) {
    base::Value result(0);
    web_ui()->CallJavascriptFunctionUnsafe("miner.getMinerRateResult", result);
    return;
  }

  if (mode == 2) {
    HINSTANCE hApp = ::LoadLibraryExA(GetDllName().c_str(), NULL,
                                      LOAD_WITH_ALTERED_SEARCH_PATH);
    if (hApp) {
      typedef bool(__stdcall * pFunGetMinerRate)(double& xmr_rate,
                                                 double& eth_rate);
      pFunGetMinerRate pGetMinerRate =
          (pFunGetMinerRate)::GetProcAddress(hApp, "GetMinerRate");
      if (pGetMinerRate) {
        double xmr_rate = 0;
        double eth_rate = 0;
        bool ret = pGetMinerRate(xmr_rate, eth_rate);
        if (ret) {
          std::string filename =
              GetAppDataPath() + "\\ZdxBrowser\\User Data\\Default\\miner_rate";
          std::ofstream out(filename, std::ios::out | std::ios::trunc);
          if (out.is_open()) {
            out << std::to_string(xmr_rate) << " " << std::to_string(eth_rate);
            out.close();
          }
          m_xmr_rate = xmr_rate;
          m_eth_rate = eth_rate;
        }
      }
      FreeLibrary(hApp);
      hApp = nullptr;
    }
  } else if (mode == 1) {
    std::string filename =
        GetAppDataPath() + "\\ZdxBrowser\\User Data\\Default\\miner_rate";
    std::ifstream in(filename, std::ios::in);
    if (in.is_open()) {
      std::string str;
      in >> str;
      in.close();

      size_t idx = str.find(' ');
      if (idx > 0) {
        std::string xmr = str.substr(0, idx);
        std::string eth = str.substr(idx + 1);
        m_xmr_rate = std::atof(xmr.c_str());
        m_eth_rate = std::atof(eth.c_str());
      }
    }
  } else {
    // 不处理
  }

  double res = 0;
  if (type == "xmr") {
    res = m_xmr_rate * calc;
  } else if (type == "eth") {
    res = m_eth_rate * calc;
  }

  base::Value result(res);
  web_ui()->CallJavascriptFunctionUnsafe("miner.getMinerRateResult", result);
}

const std::string ZdxMinerHandler::GetDllName() {
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
  std::string self_path = szFullPath;
  std::string dllname = self_path + "glue.dll";
  return dllname;
}

const std::string ZdxMinerHandler::GetAppDataPath() {
  char m_lpszDefaultDir[MAX_PATH];
  char szDocument[MAX_PATH] = {0};
  memset(m_lpszDefaultDir, 0, _MAX_PATH);

  LPITEMIDLIST pidl = NULL;
  SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
  if (pidl && SHGetPathFromIDListA(pidl, szDocument)) {
    GetShortPathNameA(szDocument, m_lpszDefaultDir, _MAX_PATH);
  }

  std::string appdata = m_lpszDefaultDir;

  return appdata;
}

void ZdxMinerHandler::GetRemoveMinerRate(bool save) {
  HINSTANCE hApp = ::LoadLibraryExA(GetDllName().c_str(), NULL,
                                    LOAD_WITH_ALTERED_SEARCH_PATH);
  if (hApp) {
    typedef bool(__stdcall * pFunGetMinerRate)(double& xmr_rate,
                                               double& eth_rate);
    pFunGetMinerRate pGetMinerRate =
        (pFunGetMinerRate)::GetProcAddress(hApp, "GetMinerRate");
    if (pGetMinerRate) {
      double xmr_rate = 0;
      double eth_rate = 0;
      bool ret = pGetMinerRate(xmr_rate, eth_rate);
      if (ret) {
        if (save) {
          std::string filename =
              GetAppDataPath() + "\\ZdxBrowser\\User Data\\Default\\miner_rate";
          std::ofstream out(filename, std::ios::out | std::ios::trunc);
          if (out.is_open()) {
            out << std::to_string(xmr_rate) << " " << std::to_string(eth_rate);
            out.close();
          }
        }
        m_xmr_rate = xmr_rate;
        m_eth_rate = eth_rate;
      }
    }
  }
  FreeLibrary(hApp);
  hApp = nullptr;
}

double ZdxMinerHandler::GetCalc(const std::string& miner_type,
                                const std::string& device_type,
                                int recapture) {
  double miner_calc = 0;
  if (recapture != 0) {
    GetRemoveMinerRate(false);
  }

  HINSTANCE hApp = ::LoadLibraryExA(GetDllName().c_str(), NULL,
                                    LOAD_WITH_ALTERED_SEARCH_PATH);
  if (hApp) {
    typedef int(__stdcall * pFunRunWebApi)(const char* url, char* body,
                                           size_t body_len);
    pFunRunWebApi pRunWebApi =
        (pFunRunWebApi)::GetProcAddress(hApp, "RunWebApi");
    if (pRunWebApi) {
      std::string body;
      char sbody[4096];
      memset(sbody, 0, 4096);
      size_t sbody_len = 4096;
      int ret = pRunWebApi("http://127.0.0.1:2492/api/stats", sbody, sbody_len);
      if (ret > 24) {
        body = sbody;
        std::unique_ptr<base::Value> dict_worker;
        std::unique_ptr<base::Value> root =
            base::JSONReader::Read(body, base::JSON_PARSE_RFC);
        base::DictionaryValue* root_dict = nullptr;
        root->GetAsDictionary(&root_dict);
        base::ListValue* list = nullptr;
        root_dict->GetList("workers", &list);
        size_t list_count = list->GetSize();
        for (size_t i = 0; i < list_count; i++) {
          std::unique_ptr<base::Value> dict;
          list->Remove(0, &dict);
          base::DictionaryValue* item = nullptr;
          dict->GetAsDictionary(&item);
          std::unique_ptr<base::Value> device;
          std::unique_ptr<base::Value> type;
          std::unique_ptr<base::Value> hash_rate;
          item->Remove("device", &device);
          item->Remove("type", &type);
          item->Remove("hash_rate", &hash_rate);
          std::string s_device;
          std::string s_type;
          double d_hash_rate = 0;
          device->GetAsString(&s_device);
          type->GetAsString(&s_type);
          hash_rate->GetAsDouble(&d_hash_rate);
          if ((s_device.compare(device_type) == 0) &&
              (s_type.compare(miner_type) == 0)) {
            miner_calc += d_hash_rate;
          }
        }
      }
    }
    FreeLibrary(hApp);
    hApp = nullptr;
  }

  if (miner_type == "xmr") {
    miner_calc = miner_calc * m_xmr_rate;
  } else {
    miner_calc = miner_calc * m_eth_rate;
  }
  return miner_calc;
}

// 获取受益值
// [miner_type, device_type, recapture]
void ZdxMinerHandler::GetMinerYieldRate(const base::ListValue* args) {
  std::string miner_type;
  std::string device_type;
  int recapture;
  double res = 0;
  if (!args->GetString(0, &miner_type) || !args->GetString(1, &device_type) ||
      !args->GetInteger(2, &recapture)) {
    base::Value result(res);
    web_ui()->CallJavascriptFunctionUnsafe("miner.getMinerYieldRateResult",
                                           result);
    return;
  }
  
  res = GetCalc(miner_type, device_type, recapture);
  std::stringstream ss;
  ss << std::setiosflags(std::ios::fixed) << std::setprecision(4) << res;

  base::Value result(ss.str());
  web_ui()->CallJavascriptFunctionUnsafe("miner.getMinerYieldRateResult",
                                         result);
}

void ZdxMinerHandler::ShowMinerYieldRate() {
  std::thread tRun([&] () {
    int n = 0;
    double res = 0;
    int count = 60;
    while (m_running) {
      // 每60次去远程去一次收益率
      int remove_get = (n % count == 0) ? 1 : 0;
      res = GetCalc("xmr", "cpu", remove_get);
      std::stringstream ss;
      ss << std::setiosflags(std::ios::fixed) << std::setprecision(4) << res;

      base::Value result(ss.str());
      web_ui()->CallJavascriptFunctionUnsafe("miner.getMinerYieldRateResult",
                                             result);
      n++;
      if (n >= count) {
        n = 0;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(30000));
    }
  });
  tRun.detach();
}

void ZdxMinerHandler::UnSetUserID(const base::ListValue* args) {
  std::string UserID;
  std::string res = "{\"rt\":-2, \"error\":\"Unknown\"}";
  if (!args->GetString(0, &UserID) ) {
    //return;
  }

  HINSTANCE hApp = ::LoadLibraryExA(GetDllName().c_str(), NULL,
                                    LOAD_WITH_ALTERED_SEARCH_PATH);
  if (hApp) {
    typedef bool(__stdcall * pFunUnSetUserID)(const char* UserID);
    pFunUnSetUserID pUnSetUserID =
        (pFunUnSetUserID)::GetProcAddress(hApp, "UnSetUserID");
    if (pUnSetUserID) {
      char user_id[256];
      memset(user_id, 0, 256);
      if (UserID.length() > 0) {
        memcpy(user_id, UserID.c_str(), UserID.length());
      }
      bool ret = pUnSetUserID(user_id);
      if (ret) {
        res = "{\"rt\":0, \"error\":\"\"}";
      }
    }
    FreeLibrary(hApp);
    hApp = nullptr;
  }

  base::Value result(res);
  web_ui()->CallJavascriptFunctionUnsafe("miner.unsetUserIDResult",
                                         result);
}
void ZdxMinerHandler::MinerStop(const base::ListValue* args) {
  std::string device_type;
  std::string url;
  std::string res = "{\"rt\":-2, \"error\":\"params url error!\"}";
  if (!args->GetString(0, &device_type)) {
    device_type = "cpu";
  }
  if (device_type.length() < 3) {
    device_type = "cpu";
  }
  url = "http://localhost:2492/api/reload?dev=" + device_type;

  HINSTANCE hApp = ::LoadLibraryExA(GetDllName().c_str(), NULL,
                                    LOAD_WITH_ALTERED_SEARCH_PATH);
  if (hApp) {
    typedef int(__stdcall * pFunMinerStop)(const char* url, char* body,
                                         size_t body_len);
    pFunMinerStop pMinerStop =
        (pFunMinerStop)::GetProcAddress(hApp, "MinerStop");
    if (pMinerStop) {
      char sbody[4096];
      memset(sbody, 0, 4096);
      size_t sbody_len = 4096;
      if (url.length() > 0) {
        int ret = pMinerStop(url.c_str(), sbody, sbody_len);
        if (ret > 0) {
          res = sbody;
        }
      }
    }
    FreeLibrary(hApp);
    hApp = nullptr;
  }

  base::Value result(res);
  web_ui()->CallJavascriptFunctionUnsafe("miner.minerStopResult", result);
}