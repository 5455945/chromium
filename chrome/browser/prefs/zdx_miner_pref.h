// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREFS_ZDX_MINER_PREF_H__
#define CHROME_BROWSER_PREFS_ZDX_MINER_PREF_H__

#include <vector>

#include "url/gurl.h"

class PrefService;
class Profile;

namespace user_prefs {
class PrefRegistrySyncable;
}


struct ZdxMinerPref {


  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // What should happen on startup for the specified profile.
  static void SetZdxMinerPref(Profile* profile, const ZdxMinerPref& pref);
  static void SetZdxMinerPref(PrefService* prefs, const ZdxMinerPref& pref);
  static ZdxMinerPref GetZdxMinerPref(Profile* profile);
  static ZdxMinerPref GetZdxMinerPref(PrefService* prefs);

  ZdxMinerPref(const ZdxMinerPref& other);

  ZdxMinerPref(const std::string &user_id, const std::string &uuid);

  ~ZdxMinerPref();

  std::string m_sUserID;   // 挖矿注册需要用户填UserID
  std::string m_sUUID;     // 挖矿注册需要设备UUID
};

#endif  // CHROME_BROWSER_PREFS_SESSION_STARTUP_PREF_H__
