#include "chrome/browser/ui/webui/zdx_miner/zdx_miner_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/zdx_miner/zdx_miner_handler.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui_data_source.h"

ZdxMinerUI::ZdxMinerUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {

  web_ui->AddMessageHandler(std::make_unique<ZdxMinerHandler>(web_ui));

  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(chrome::kChromeUIZdxMinerHost);

  html_source->AddLocalizedString("minerTitle", IDS_ZDX_MINER_TITLE);
  html_source->AddResourcePath("zdx_miner.css", IDR_ZDX_MINER_CSS);
  html_source->AddResourcePath("jquery-3.3.1.min.js", IDR_ZDX_JS_JQUERY_MIN_JS);

  html_source->AddResourcePath("zdx_miner.js", IDR_ZDX_MINER_JS);
  html_source->AddResourcePath("zdx_miner.ico", IDR_ZDX_MINER_ICO);
  html_source->AddString("sUserID", "");
  html_source->AddString("sUUID", "");
  html_source->AddString("sMinerYieldRate", "");

  html_source->SetJsonPath("strings.js");
  html_source->SetDefaultResource(IDR_ZDX_MINER_HTML);

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, html_source);
}

ZdxMinerUI::~ZdxMinerUI() {}
