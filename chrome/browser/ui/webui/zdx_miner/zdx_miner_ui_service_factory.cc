// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/zdx_miner/zdx_miner_ui_service_factory.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/profile_sync_service_factory.h"
#include "chrome/browser/ui/webui/zdx_miner/zdx_miner_ui_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/prefs/pref_service.h"

ZdxMinerUIServiceFactory::ZdxMinerUIServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "ZdxMinerUIServiceFactory",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ProfileSyncServiceFactory::GetInstance());
}

ZdxMinerUIServiceFactory::~ZdxMinerUIServiceFactory() {}

// static
ZdxMinerUIService* ZdxMinerUIServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<ZdxMinerUIService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
ZdxMinerUIServiceFactory* ZdxMinerUIServiceFactory::GetInstance() {
  return base::Singleton<ZdxMinerUIServiceFactory>::get();
}

// static
base::Closure ZdxMinerUIServiceFactory::GetShowZdxMinerPopupCallbackForProfile(
    Profile* profile) {
  return base::Bind(
      &ZdxMinerUIService::ShowZdxMinerPopup,
      base::Unretained(ZdxMinerUIServiceFactory::GetForProfile(profile)));
}

KeyedService* ZdxMinerUIServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return new ZdxMinerUIService(static_cast<Profile*>(profile));
}

bool ZdxMinerUIServiceFactory::ServiceIsCreatedWithBrowserContext() const {
  return true;
}
