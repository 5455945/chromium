// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/zdx_miner/zdx_miner_handler.h"

#include "base/values.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
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
#include <Shlobj.h>
#include <fstream>
#pragma comment(lib, "shell32.lib")

ZdxMinerHandler::ZdxMinerHandler(content::WebUI* web_ui)
    : profile_(Profile::FromWebUI(web_ui)),
      login_ui_service_(LoginUIServiceFactory::GetForProfile(profile_)),
      result_(MinerResult::DEFAULT) {
  login_ui_service_->AddObserver(this);
}

ZdxMinerHandler::~ZdxMinerHandler() {
  login_ui_service_->RemoveObserver(this);

  // We log that an impression occurred at destruct-time. This can't be done at
  // construct-time on some platforms because this page is shown immediately
  // after a new installation of Chrome and loads while the user is deciding
  // whether or not to opt in to logging.
  signin_metrics::RecordSigninImpressionUserActionForAccessPoint(
      signin_metrics::AccessPoint::ACCESS_POINT_START_PAGE);

  UMA_HISTOGRAM_ENUMERATION("miner.SignInPromptResult", result_,
                            MinerResult::MINER_RESULT_MAX);
}

// Override from LoginUIService::Observer.
void ZdxMinerHandler::OnSyncConfirmationUIClosed(
    LoginUIService::SyncConfirmationUIClosedResult result) {
  if (result != LoginUIService::ABORT_SIGNIN) {
    result_ = MinerResult::SIGNED_IN;
    GoToNewTabPage();
  }
}

// Handles backend events necessary when user clicks "Sign in."
void ZdxMinerHandler::HandleActivateSignIn(const base::ListValue* args) {
  result_ = MinerResult::ATTEMPTED;
  base::RecordAction(base::UserMetricsAction("MinerPage_SignInClicked"));

  if (SigninManagerFactory::GetForProfile(profile_)->IsAuthenticated()) {
    // In general, this page isn't shown to signed-in users; however, if one
    // should arrive here, then opening the sign-in dialog will likely lead
    // to a crash. Thus, we just act like sign-in was "successful" and whisk
    // them away to the NTP instead.
    GoToNewTabPage();
  } else {
    Browser* browser = GetBrowser();
    browser->signin_view_controller()->ShowSignin(
        profiles::BubbleViewMode::BUBBLE_VIEW_MODE_GAIA_SIGNIN, browser,
        signin_metrics::AccessPoint::ACCESS_POINT_START_PAGE);
  }
}

// Handles backend events necessary when user clicks "No thanks."
void ZdxMinerHandler::HandleUserDecline(const base::ListValue* args) {
  // Set the appropriate decline result, based on whether or not the user
  // attempted to sign in.
  result_ = (result_ == MinerResult::ATTEMPTED)
                ? MinerResult::ATTEMPTED_DECLINED
                : MinerResult::DECLINED;
  GoToNewTabPage();
}

// Override from WebUIMessageHandler.
void ZdxMinerHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "handleActivateSignIn",
      base::BindRepeating(&ZdxMinerHandler::HandleActivateSignIn,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "handleUserDecline",
      base::BindRepeating(&ZdxMinerHandler::HandleUserDecline,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "addNumbers",
      base::Bind(&ZdxMinerHandler::AddNumbers, base::Unretained(this)));
	  
  web_ui()->RegisterMessageCallback(
      "setUserID",
      base::Bind(&ZdxMinerHandler::SetUserID, base::Unretained(this)));
}

void ZdxMinerHandler::GoToNewTabPage() {
  NavigateParams params(GetBrowser(), GURL(chrome::kChromeUINewTabURL),
                        ui::PageTransition::PAGE_TRANSITION_LINK);
  params.source_contents = web_ui()->GetWebContents();
  Navigate(&params);
}

Browser* ZdxMinerHandler::GetBrowser() {
  DCHECK(web_ui());
  content::WebContents* contents = web_ui()->GetWebContents();
  DCHECK(contents);
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  DCHECK(browser);
  return browser;
}

void ZdxMinerHandler::AddNumbers(const base::ListValue* args) {
  int term1, term2;
  if (!args->GetInteger(0, &term1) || !args->GetInteger(1, &term2)) {
    return;
  }

  base::Value result(term1 + term2);
  web_ui()->CallJavascriptFunctionUnsafe("miner.addResult", result);
}

void ZdxMinerHandler::SetUserID(const base::ListValue* args) {
  std::string UserID;
  std::string body;
  if (!args->GetString(0, &UserID)) {
    return;
  }
  //web_ui()->GetWebContents()->GetController().LoadURL(
  //    GURL("https://zdx.app/api/v1/member/login?passwd=4a92746629413c686b80e46f5bcd9cc4&phone_number=13818812913&timestamp=1534228891&type=desktop&code=691c149ba6595063ceb73594358486db"), 
  //    content::Referrer(),
  //    ui::PageTransition::PAGE_TRANSITION_LINK, std::string());

  // 把用户ID 写入指定的文件
  if (UserID.length() > 0) {
    // 从文件读取user_id

    char m_lpszDefaultDir[MAX_PATH];
    char szDocument[MAX_PATH] = {0};
    memset(m_lpszDefaultDir, 0, _MAX_PATH);

    LPITEMIDLIST pidl = NULL;
    SHGetSpecialFolderLocation(NULL, CSIDL_APPDATA, &pidl);
    if (pidl && SHGetPathFromIDListA(pidl, szDocument)) {
      GetShortPathNameA(szDocument, m_lpszDefaultDir, _MAX_PATH);
    }

    std::string appdata = m_lpszDefaultDir;
    std::string filename =
        appdata + "\\ZdxBrowser\\User Data\\Default\\user_id";
    std::ofstream out(filename, std::ios::out | std::ios::trunc);
    if (out.is_open()) {
      out << UserID;
      out.close();
    }

    typedef bool(__stdcall * pFunRunReLoad)(std::string & body, bool& ret);
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
    ::OutputDebugStringA(dllname.c_str());
    HINSTANCE hApp =
        ::LoadLibraryExA(dllname.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hApp) {
      ::OutputDebugStringA("glue.dll LoadLibraryExA error.");
    } else {
      pFunRunReLoad pRunReLoad =
          (pFunRunReLoad)::GetProcAddress(hApp, "run_reload");
      if (!pRunReLoad) {
        ::OutputDebugStringA("run_reload GetProcAddress error.");
      } else {
        bool ret = false;
        pRunReLoad(body, ret);
        if (ret) {
          ::OutputDebugStringA("^_^ setting user_id success...");
        } else {
          ::OutputDebugStringA("+_+ setting user_id error !!!");
        }
        if (hApp) {
          FreeLibrary(hApp);
          hApp = nullptr;
        }
      }
    }
  }

  //// 在这里把UserId保存到配置文件
  //base::Value result("2");
  //web_ui()->CallJavascriptFunctionUnsafe("miner.setUserIDResult", result);
}
