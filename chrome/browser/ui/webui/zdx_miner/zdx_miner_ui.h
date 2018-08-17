#ifndef CHROME_BROWSER_UI_WEBUI_ZDX_MINER_ZDX_MINER_UI_H_
#define CHROME_BROWSER_UI_WEBUI_ZDX_MINER_ZDX_MINER_UI_H_
#pragma once

#include "content/public/browser/web_ui_controller.h"

namespace content {
class WebUI;
}

// The WebUI for chrome://miner
class ZdxMinerUI : public content::WebUIController {
 public:
  ZdxMinerUI(content::WebUI* web_ui);
  ~ZdxMinerUI() override;
};

#endif  // CHROME_BROWSER_UI_WEBUI_ZDX_MINER_ZDX_MINER_UI_H_
