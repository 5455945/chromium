// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_ZDX_MINER_ZDX_MINER_UI_SERVICE_FACTORY_H_
#define CHROME_BROWSER_UI_WEBUI_ZDX_MINER_ZDX_MINER_UI_SERVICE_FACTORY_H_

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"

class ZdxMinerUIService;
class Profile;

// Singleton that owns all ZdxMinerUIServices and associates them with
// Profiles. Listens for the Profile's destruction notification and cleans up
// the associated ZdxMinerUIService.
class ZdxMinerUIServiceFactory : public BrowserContextKeyedServiceFactory {
 public:
  // Returns the instance of ZdxMinerUIService associated with this profile
  // (creating one if none exists). Returns NULL if this profile cannot have a
  // ZdxMinerUIService (for example, if |profile| is incognito).
  static ZdxMinerUIService* GetForProfile(Profile* profile);

  // Returns an instance of the ZdxMinerUIServiceFactory singleton.
  static ZdxMinerUIServiceFactory* GetInstance();

  // Helper method that returns a closure displaying the zdx miner popup for
  // |profile|.
  // This closure must not be called after the ZdxMinerUIService is destroyed.
  static base::Closure GetShowZdxMinerPopupCallbackForProfile(Profile* profile);

 private:
  friend struct base::DefaultSingletonTraits<ZdxMinerUIServiceFactory>;

  ZdxMinerUIServiceFactory();
  ~ZdxMinerUIServiceFactory() override;

  // BrowserContextKeyedServiceFactory:
  KeyedService* BuildServiceInstanceFor(
      content::BrowserContext* profile) const override;
  bool ServiceIsCreatedWithBrowserContext() const override;

  //DISALLOW_COPY_AND_ASSIGN(ZdxMinerUIServiceFactory);
};

#endif  // CHROME_BROWSER_UI_WEBUI_ZDX_MINER_ZDX_MINER_UI_SERVICE_FACTORY_H_
