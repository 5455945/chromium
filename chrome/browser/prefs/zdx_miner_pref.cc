// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/prefs/zdx_miner_pref.h"

#include <stddef.h>

#include <string>
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/url_formatter/url_fixer.h"

// static
void ZdxMinerPref::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
#if defined(OS_ANDROID)
  uint32_t flags = PrefRegistry::NO_REGISTRATION_FLAGS;
#else
  uint32_t flags = user_prefs::PrefRegistrySyncable::SYNCABLE_PREF;
#endif
  registry->RegisterStringPref(prefs::kZdxMinerUserID,
                                "",
                                flags);
  registry->RegisterStringPref(prefs::kZdxMinerUUID,
                             "",
                             flags);
}

// static
void ZdxMinerPref::SetZdxMinerPref(
    Profile* profile, const ZdxMinerPref& pref) {
  DCHECK(profile);
  SetZdxMinerPref(profile->GetPrefs(), pref);
}

// static
void ZdxMinerPref::SetZdxMinerPref(PrefService* prefs,
                                   const ZdxMinerPref& pref) {
  DCHECK(prefs);
  prefs->SetString(prefs::kZdxMinerUserID, pref.m_sUserID);
  prefs->SetString(prefs::kZdxMinerUUID, pref.m_sUUID);

}

// static
ZdxMinerPref ZdxMinerPref::GetZdxMinerPref(Profile* profile) {
  DCHECK(profile);

  return GetZdxMinerPref(profile->GetPrefs());
}

// static
ZdxMinerPref ZdxMinerPref::GetZdxMinerPref(PrefService* prefs) {
  DCHECK(prefs);

  ZdxMinerPref pref(prefs->GetString(prefs::kZdxMinerUserID),
                    prefs->GetString(prefs::kZdxMinerUUID));

  return pref;
}

ZdxMinerPref::ZdxMinerPref(const std::string &user_id, const std::string &uuid)
    : m_sUserID(user_id), m_sUUID(uuid) {}

ZdxMinerPref::ZdxMinerPref(const ZdxMinerPref& other) =
    default;

ZdxMinerPref::~ZdxMinerPref() {}
