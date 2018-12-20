// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/net/proxy_service_factory.h"

#include "base/base64.h"
#include "base/task/post_task.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/browser/ui/login/login_handler.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "components/prefs/pref_service.h"
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
// zhangfj 20181219 代理设置
void CheckLoginProxyInfo(PrefService* profile_prefs) {
  if (profile_prefs) {
    const PrefService::Preference* pref =
      profile_prefs->FindPreference(proxy_config::prefs::kProxy);
    DCHECK(pref);

    const base::DictionaryValue* dv =
      profile_prefs->GetDictionary(proxy_config::prefs::kProxy);
    if (nullptr == dv) {
      return;
    }
    std::string mode_name;
    if (dv->GetString("mode", &mode_name)) {
      if (mode_name == "pac_script") {
        return;
      }
    }
    std::string pac_script = R"(
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
        if (/^accounts\.google\.com$/.test(host)) return "+proxy";
        if (/^www\.googleapis\.com$/.test(host)) return "+proxy";
        return "DIRECT";
    },
    "+proxy": function(url, host, scheme) {
        "use strict";
        return "HTTPS p.yiluzhuanqian.com:8002";
    }
});
)";
    std::string pac_script_base64_encoded;
    base::Base64Encode(pac_script, &pac_script_base64_encoded);

    pac_script_base64_encoded =
      std::string("data:application/x-ns-proxy-autoconfig;base64,") +
      pac_script_base64_encoded;

    base::Value dict =
      ProxyConfigDictionary::CreatePacScript(pac_script_base64_encoded, true);
      profile_prefs->Set(proxy_config::prefs::kProxy, dict);
  }
}

}

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
  CheckLoginProxyInfo(profile_prefs);

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
