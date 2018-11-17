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

  html_source->AddResourcePath("bootstrap.min.css",
                               IDR_ZDX_CSS_BOOTSTRAP_MIN_CSS);
  html_source->AddResourcePath("bootstrap.min.css.map",
                               IDR_ZDX_CSS_BOOTSTRAP_MIN_CSS_MAP);
  html_source->AddResourcePath("bootstrap-theme.min.css",
                               IDR_ZDX_CSS_BOOTSTRAP_THEME_MIN_CSS);
  html_source->AddResourcePath("bootstrap-theme.min.css.map",
                               IDR_ZDX_CSS_BOOTSTRAP_THEME_MIN_CSS_MAP);
  html_source->AddResourcePath("glyphicons-halflings-regular.eot", IDR_ZDX_FONTS_BOOTSTRAP_EOT);
  html_source->AddResourcePath("glyphicons-halflings-regular.svg",
                               IDR_ZDX_FONTS_BOOTSTRAP_SVG);
  html_source->AddResourcePath("glyphicons-halflings-regular.ttf",
                               IDR_ZDX_FONTS_BOOTSTRAP_TTF);
  html_source->AddResourcePath("glyphicons-halflings-regular.woff",
                               IDR_ZDX_FONTS_BOOTSTRAP_WOFF);
  html_source->AddResourcePath("glyphicons-halflings-regular.woff2",
                               IDR_ZDX_FONTS_BOOTSTRAP_WOFF2);
  html_source->AddResourcePath("zdx_miner.css", IDR_ZDX_MINER_CSS);

  html_source->AddResourcePath("bootstrap.min.js", IDR_ZDX_JS_BOOTSTRAP_MIN_JS);
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
