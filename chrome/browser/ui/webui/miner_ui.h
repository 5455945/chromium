#ifndef CHROME_BROWSER_UI_WEBUI_MINER_UI_H_
#define CHROME_BROWSER_UI_WEBUI_MINER_UI_H_
#pragma once

#include "content/public/browser/web_ui_controller.h"

namespace content {
class WebUI;
}

// The WebUI for chrome://miner
class MinerUI : public content::WebUIController {
 public:
  MinerUI(content::WebUI* web_ui);
  ~MinerUI() override;
};

#endif  // CHROME_BROWSER_UI_WEBUI_MINER_UI_H_
