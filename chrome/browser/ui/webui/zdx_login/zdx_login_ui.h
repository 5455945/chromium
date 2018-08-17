#ifndef CHROME_BROWSER_UI_WEBUI_ZDX_LOGIN_ZDX_LOGIN_UI_H_
#define CHROME_BROWSER_UI_WEBUI_ZDX_LOGIN_ZDX_LOGIN_UI_H_
#pragma once

#include "content/public/browser/web_ui_controller.h"

namespace content {
class WebUI;
}

// The WebUI for chrome://login
class ZdxLoginUI : public content::WebUIController {
 public:
  ZdxLoginUI(content::WebUI* web_ui);
  ~ZdxLoginUI() override;
};

#endif  // CHROME_BROWSER_UI_WEBUI_ZDX_LOGIN_ZDX_LOGIN_UI_H_
