// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/proxy_service_factory.h"

#include <regex>
#include "base/base64.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/md5.h"
#include "base/path_service.h"
#include "base/task/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/login/login_handler.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/chrome_paths.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "components/proxy_config/proxy_config_dictionary.h"
#include "components/proxy_config/proxy_config_pref_names.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_resolution_service.h"

#if defined(OS_CHROMEOS)
#include "chromeos/network/proxy/proxy_config_service_impl.h"
#endif  // defined(OS_CHROMEOS)

using content::BrowserThread;

namespace {
// zhangfj 20190128 穿越功能
std::vector<std::string> string_split(const std::string& in,
                                      const std::string& delim) {
  std::regex re{delim};
  return std::vector<std::string>{
      std::sregex_token_iterator(in.begin(), in.end(), re, -1),
      std::sregex_token_iterator()};
}
void GetCrossDomain(std::string& cross_domain) {
  base::FilePath app_path;
  base::FilePath path;
  std::string user_id;
  base::FilePath zdx_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &zdx_dir);
  if (zdx_dir.empty()) {
    return;
  }
  zdx_dir = zdx_dir.AppendASCII(chrome::kInitialProfile);
  if (zdx_dir.empty()) {
    return;
  }
  base::DictionaryValue zdx_sign_info;
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  profile_manager->GetProfileAttributesStorage().GetZdxInfoCache(zdx_dir,
                                                                 zdx_sign_info);
  zdx_sign_info.GetString("zdx_login_user_id", &user_id);
  if (user_id.length() == 0)
    return;
  bool zdx_is_cross_area = false;
  bool zdx_cross_active = false;
  bool zdx_login_status = false;
  zdx_sign_info.GetBoolean("zdx_login_status", &zdx_login_status);
  if (!zdx_login_status)
    return;
  zdx_sign_info.GetBoolean("zdx_is_cross_area", &zdx_is_cross_area);
  zdx_sign_info.GetBoolean("zdx_cross_active", &zdx_cross_active);
  if (!(zdx_is_cross_area && zdx_cross_active))
    return;
  base::PathService::Get(base::DIR_APP_DATA, &app_path);
  std::string md5_web;
  std::string md5_pac;
  base::FilePath md5_web_file =
      app_path.AppendASCII("ZdxBrowser\\ZdxData\\cross_domain_md5_" + user_id);
  if (!base::PathExists(md5_web_file))
    return;
  base::ReadFileToString(md5_web_file, &md5_web);
  base::FilePath md5_pac_file =
      app_path.AppendASCII("ZdxBrowser\\ZdxData\\cross_domain_pac_" + user_id);
  if (base::PathExists(md5_pac_file)) {
    base::ReadFileToString(md5_pac_file, &md5_pac);
    if (md5_web.length() > 0 && md5_web == md5_pac) {
      return;
    }
  }
  path = app_path.AppendASCII("ZdxBrowser\\ZdxData\\cross_domain_" + user_id);
  if (!base::PathExists(path))
    return;
  std::string json;
  base::ReadFileToString(path, &json);
  if (json.length() == 0)
    return;
  std::unique_ptr<base::DictionaryValue> info = nullptr;
  base::DictionaryValue* data = nullptr;
  base::ListValue* list = nullptr;
  info = base::DictionaryValue::From(base::JSONReader::Read(json));
  if (!info)
    return;
  info->GetDictionary("data", &data);
  if (!data)
    return;
  data->GetList("list", &list);
  if (!list)
    return;
  std::string item;
  std::string reg_item;
  for (size_t i = 0; i < list->GetSize(); ++i) {
    reg_item = "";
    std::string value;
    if (list->GetString(i, &value)) {
      base::Base64Decode(value, &item);
      for (size_t j = 0; j < item.length(); ++j) {
        if (item[j] == '/') {
          reg_item += '\\';
        }
        reg_item += item[j];
      }
      if (reg_item.length() > 0) {
        reg_item = "	if (/" + reg_item + "/.test(url)) return \"+proxy\";\n";
        cross_domain += reg_item;
      }
    }
  }
  if (cross_domain.length() > 0) {
    base::WriteFile(md5_pac_file, md5_web.c_str(), md5_web.length());
  }
}
void GetLoginDomain(std::string& login_doamin) {
  login_doamin = R"(
        if (/(?:^|\.)googleapis\.com$/.test(host)) return "+proxy";
        if (/(?:^|\.)gstatic\.com$/.test(host)) return "+proxy";
        if (/(?:^|\.)googleusercontent\.com$/.test(host)) return "+proxy";
        if (/^www\.chromestatus\.com$/.test(host)) return "+proxy";
        if (/^ssl\.google-analytics\.com$/.test(host)) return "+proxy";
        if (/^accounts\.google\.com$/.test(host)) return "+proxy";
        if (/^apis\.google\.com$/.test(host)) return "+proxy";
        if (/^notifications\.google\.com$/.test(host)) return "+proxy";
        if (/^ogs\.google\.com$/.test(host)) return "+proxy";
        if (/^play\.google\.com$/.test(host)) return "+proxy";
        if (/^chrome\.google\.com$/.test(host)) return "+proxy";
        if (/^domains\.google\.com$/.test(host)) return "+proxy";
        if (/^gsuite\.google\.com$/.test(host)) return "+proxy";
        if (/^plus\.google\.com$/.test(host)) return "+proxy";
        if (/^clients.*\.google\.com$/.test(host)) return "+proxy";
)";
}
void GetOldPacScript(PrefService* profile_prefs, std::string& old_pac_script) {
  if (!profile_prefs) {
    return;
  }
  const PrefService::Preference* pref =
      profile_prefs->FindPreference(proxy_config::prefs::kProxy);
  if (!pref) {
    return;
  }
  const base::DictionaryValue* dv =
      profile_prefs->GetDictionary(proxy_config::prefs::kProxy);
  if (nullptr == dv) {
    return;
  }
  std::string mode_name;
  if (dv->GetString("mode", &mode_name)) {
    if (mode_name == "pac_script") {
      std::string str;
      dv->GetString("pac_url", &str);
      if (str.length() > 46 &&
          str.substr(0, 46).compare(
              "data:application/x-ns-proxy-autoconfig;base64,") == 0) {
        str = str.substr(46);
        base::Base64Decode(str, &old_pac_script);
      }
    }
  }
}

void AddPacScript(std::string& pac, const std::string& add_info) {
  std::string pac_script_start = R"(
var FindProxyForURL = function(init, profiles) {
    return function(url, host) {
        "use strict";
        var result = init, scheme = url.substr(0, url.indexOf(":"));
        do {
            result = profiles[result];
            if (typeof result === "function") result = result(url, host, scheme);
        } while (typeof result !== "string" || result.charCodeAt(0) === 43);
        return result;
    };
}("+auto switch", {
    "+auto switch": function(url, host, scheme) {
        "use strict";
)";
  std::string pac_script_end = R"(
        return "DIRECT";
    },
    "+proxy": function(url, host, scheme) {
        "use strict";
        return "HTTPS p.yiluzhuanqian.com:8002";
    }
});
)";
  if (pac.length() <= pac_script_start.length() + pac_script_end.length()) {
    pac = pac_script_start + add_info + pac_script_end;
    return;
  }
  std::string pac_script = pac.substr(pac_script_start.length());
  pac_script =
      pac_script.substr(0, pac_script.length() - pac_script_end.length());
  std::string new_script;
  auto s_result = string_split(add_info, "[\n]");
  for (auto it : s_result) {
    std::string item = it;
    item.erase(0, item.find_first_not_of(" \n\r\t"));
    item.erase(item.find_last_not_of(" \n\r\t") + 1);
    if (item.length() > 0 && pac_script.find(item) == std::string::npos) {
      new_script += it + "\n";
    }
  }
  new_script = pac_script + new_script;
  pac = pac_script_start + add_info + pac_script_end;
}
// zhangfj 20181219 代理设置
void CheckZdxProxyInfo(PrefService* profile_prefs) {
  std::string pac_script;
  // 获取zdx中已有配置,可能为空
  GetOldPacScript(profile_prefs, pac_script);
  // 获取登陆需要的domain信息
  std::string login_domain;
  GetLoginDomain(login_domain);
  // 获取穿越的url正则列表
  std::string cross_domain;
  GetCrossDomain(cross_domain);
  // 合并已有、登陆domain、穿越url正则
  AddPacScript(pac_script, login_domain + cross_domain);
  // base64_encode，并写入配置
  std::string pac_script_base64_encoded;
  base::Base64Encode(pac_script, &pac_script_base64_encoded);
  pac_script_base64_encoded =
      std::string("data:application/x-ns-proxy-autoconfig;base64,") +
      pac_script_base64_encoded;
  base::Value dict =
      ProxyConfigDictionary::CreatePacScript(pac_script_base64_encoded, true);
  profile_prefs->Set(proxy_config::prefs::kProxy, dict);
}

}  // namespace

// static
std::unique_ptr<net::ProxyConfigService>
ProxyServiceFactory::CreateProxyConfigService(PrefProxyConfigTracker* tracker) {
  // The linux gsettings-based proxy settings getter relies on being initialized
  // from the UI thread. The system proxy config service could also get created
  // without full browser process by launching service manager alone.
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI) ||
         !BrowserThread::IsThreadInitialized(BrowserThread::UI));

  std::unique_ptr<net::ProxyConfigService> base_service;

#if !defined(OS_CHROMEOS)
  // On ChromeOS, base service is NULL; chromeos::ProxyConfigServiceImpl
  // determines the effective proxy config to take effect in the network layer,
  // be it from prefs or system (which is network shill on chromeos).

  // For other platforms, create a baseline service that provides proxy
  // configuration in case nothing is configured through prefs (Note: prefs
  // include command line and configuration policy).

  base_service = net::ProxyResolutionService::CreateSystemProxyConfigService(
      base::ThreadTaskRunnerHandle::Get());
#endif  // !defined(OS_CHROMEOS)

  return tracker->CreateTrackingProxyConfigService(std::move(base_service));
}

// static
PrefProxyConfigTracker*
ProxyServiceFactory::CreatePrefProxyConfigTrackerOfProfile(
    PrefService* profile_prefs,
    PrefService* local_state_prefs) {
#if defined(OS_CHROMEOS)
  return new chromeos::ProxyConfigServiceImpl(profile_prefs, local_state_prefs,
                                              nullptr);
#else
  // zhangfj 20181219 代理设置
  CheckZdxProxyInfo(profile_prefs);

  return new PrefProxyConfigTrackerImpl(profile_prefs, nullptr);
#endif  // defined(OS_CHROMEOS)
}

// static
PrefProxyConfigTracker*
ProxyServiceFactory::CreatePrefProxyConfigTrackerOfLocalState(
    PrefService* local_state_prefs) {
#if defined(OS_CHROMEOS)
  return new chromeos::ProxyConfigServiceImpl(nullptr, local_state_prefs,
                                              nullptr);
#else
  return new PrefProxyConfigTrackerImpl(local_state_prefs, nullptr);
#endif  // defined(OS_CHROMEOS)
}
