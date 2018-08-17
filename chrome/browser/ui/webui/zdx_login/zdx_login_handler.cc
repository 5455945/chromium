// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/zdx_login/zdx_login_handler.h"

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

ZdxLoginHandler::ZdxLoginHandler(content::WebUI* web_ui)
    : profile_(Profile::FromWebUI(web_ui)),
      login_ui_service_(LoginUIServiceFactory::GetForProfile(profile_)),
      result_(MinerResult::DEFAULT) {
  login_ui_service_->AddObserver(this);
}

ZdxLoginHandler::~ZdxLoginHandler() {
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
void ZdxLoginHandler::OnSyncConfirmationUIClosed(
    LoginUIService::SyncConfirmationUIClosedResult result) {
  if (result != LoginUIService::ABORT_SIGNIN) {
    result_ = MinerResult::SIGNED_IN;
    GoToNewTabPage();
  }
}

// Handles backend events necessary when user clicks "Sign in."
void ZdxLoginHandler::HandleActivateSignIn(const base::ListValue* args) {
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
void ZdxLoginHandler::HandleUserDecline(const base::ListValue* args) {
  // Set the appropriate decline result, based on whether or not the user
  // attempted to sign in.
  result_ = (result_ == MinerResult::ATTEMPTED)
                ? MinerResult::ATTEMPTED_DECLINED
                : MinerResult::DECLINED;
  GoToNewTabPage();
}

// Override from WebUIMessageHandler.
void ZdxLoginHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "handleActivateSignIn",
      base::BindRepeating(&ZdxLoginHandler::HandleActivateSignIn,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "handleUserDecline",
      base::BindRepeating(&ZdxLoginHandler::HandleUserDecline,
                          base::Unretained(this)));

  web_ui()->RegisterMessageCallback(
      "addNumbers",
      base::Bind(&ZdxLoginHandler::AddNumbers, base::Unretained(this)));
}

void ZdxLoginHandler::GoToNewTabPage() {
  NavigateParams params(GetBrowser(), GURL(chrome::kChromeUINewTabURL),
                        ui::PageTransition::PAGE_TRANSITION_LINK);
  params.source_contents = web_ui()->GetWebContents();
  Navigate(&params);
}

Browser* ZdxLoginHandler::GetBrowser() {
  DCHECK(web_ui());
  content::WebContents* contents = web_ui()->GetWebContents();
  DCHECK(contents);
  Browser* browser = chrome::FindBrowserWithWebContents(contents);
  DCHECK(browser);
  return browser;
}

void ZdxLoginHandler::AddNumbers(const base::ListValue* args) {
  int term1, term2;
  if (!args->GetInteger(0, &term1) || !args->GetInteger(1, &term2)) {
    return;
  }

  base::Value result(term1 + term2);
  web_ui()->CallJavascriptFunctionUnsafe("miner.addResult", result);
}