#include "chrome/browser/ui/webui/zdx_miner/zdx_miner_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/zdx_miner/zdx_miner_handler.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui_data_source.h"

ZdxMinerUI::ZdxMinerUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  // Set up the chrome://miner source.

  web_ui->AddMessageHandler(std::make_unique<ZdxMinerHandler>(web_ui));

  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(chrome::kChromeUIZdxMinerHost);

  // Localized strings.
  html_source->AddLocalizedString("minerTitle", IDS_ZDX_MINER_TITLE);
  html_source->AddLocalizedString("minerText1", IDS_ZDX_MINER_TEXT1);
  html_source->AddLocalizedString("minerText2", IDS_ZDX_MINER_TEXT2);
  html_source->AddLocalizedString("minerText3", IDS_ZDX_MINER_TEXT3);
  html_source->AddLocalizedString("minerMessage", IDS_ZDX_MINER_TEXT);
  html_source->AddLocalizedString("acceptText", IDS_WELCOME_ACCEPT_BUTTON);
  html_source->AddLocalizedString("declineText", IDS_WELCOME_DECLINE_BUTTON);

  html_source->AddString("UserID", "");
  html_source->AddString("UUID", "");
  html_source->SetJsonPath("strings.js");

  // Add required resources.
  html_source->AddResourcePath("zdx_miner.css", IDR_ZDX_MINER_CSS);
  html_source->AddResourcePath("zdx_miner.js", IDR_ZDX_MINER_JS);
  html_source->SetDefaultResource(IDR_ZDX_MINER_HTML);

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, html_source);
}

ZdxMinerUI::~ZdxMinerUI() {}
