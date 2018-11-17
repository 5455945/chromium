#include "chrome/browser/ui/webui/zdx_login/zdx_login_ui.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/zdx_login/zdx_login_handler.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui_data_source.h"

ZdxLoginUI::ZdxLoginUI(content::WebUI* web_ui)
    : content::WebUIController(web_ui) {
  // Set up the chrome://login source.

  web_ui->AddMessageHandler(std::make_unique<ZdxLoginHandler>(web_ui));

  content::WebUIDataSource* html_source =
      content::WebUIDataSource::Create(chrome::kChromeUIZdxLoginHost);

  // Localized strings.
  html_source->AddLocalizedString("minerTitle", IDS_ZDX_MINER_TITLE);
  html_source->AddLocalizedString("minerText1", IDS_ZDX_MINER_TEXT1);
  html_source->AddLocalizedString("minerText2", IDS_ZDX_MINER_TEXT2);
  html_source->AddLocalizedString("minerText3", IDS_ZDX_MINER_TEXT3);
  html_source->AddLocalizedString("minerMessage", IDS_ZDX_MINER_TEXT);
  html_source->AddLocalizedString("acceptText", IDS_WELCOME_ACCEPT_BUTTON);
  html_source->AddLocalizedString("declineText", IDS_WELCOME_DECLINE_BUTTON);


  // As a demonstration of passing a variable for JS to use we pass in the name "Bob".
  html_source->AddString("userName", "Bob");
  html_source->SetJsonPath("strings.js");

  // Add required resources.
  html_source->AddResourcePath("bootstrap.min.css",
                               IDR_ZDX_CSS_BOOTSTRAP_MIN_CSS);
  html_source->AddResourcePath("bootstrap.min.css.map",
                               IDR_ZDX_CSS_BOOTSTRAP_MIN_CSS_MAP);
  html_source->AddResourcePath("bootstrap-theme.min.css",
                               IDR_ZDX_CSS_BOOTSTRAP_THEME_MIN_CSS);
  html_source->AddResourcePath("bootstrap-theme.min.css.map",
                               IDR_ZDX_CSS_BOOTSTRAP_THEME_MIN_CSS_MAP);
  html_source->AddResourcePath("glyphicons-halflings-regular.eot",
                               IDR_ZDX_FONTS_BOOTSTRAP_EOT);
  html_source->AddResourcePath("glyphicons-halflings-regular.svg",
                               IDR_ZDX_FONTS_BOOTSTRAP_SVG);
  html_source->AddResourcePath("glyphicons-halflings-regular.ttf",
                               IDR_ZDX_FONTS_BOOTSTRAP_TTF);
  html_source->AddResourcePath("glyphicons-halflings-regular.woff",
                               IDR_ZDX_FONTS_BOOTSTRAP_WOFF);
  html_source->AddResourcePath("glyphicons-halflings-regular.woff2",
                               IDR_ZDX_FONTS_BOOTSTRAP_WOFF2);

  html_source->AddResourcePath("bootstrap.min.js", IDR_ZDX_JS_BOOTSTRAP_MIN_JS);
  html_source->AddResourcePath("jquery-3.3.1.min.js", IDR_ZDX_JS_JQUERY_MIN_JS);

  html_source->AddResourcePath("common.js", IDR_ZDX_LOGIN_COMMON_JS);
  html_source->AddResourcePath("login-api.js", IDR_ZDX_LOGIN_API_JS);
  html_source->SetDefaultResource(IDR_ZDX_LOGIN_HTML);

  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource::Add(profile, html_source);
}

ZdxLoginUI::~ZdxLoginUI() {}
