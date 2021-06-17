#include "web_server.assets.js.h"
#include "web_server.config.js.h"
#include "web_server.home.html.h"
#include "web_server.lib.js.h"
#include "web_server.split_term.html.h"
#include "web_server.style.css.h"
#include "web_server.term.html.h"
#include "web_server.term.js.h"
#include "web_server.wifi_signal_1.svg.h"
#include "web_server.wifi_signal_2.svg.h"
#include "web_server.wifi_signal_3.svg.h"
#include "web_server.wifi_signal_4.svg.h"
#include "web_server.wifi_signal_5.svg.h"
StaticFile staticFiles[] = {
  { "/assets.js", CONTENT_ASSETS_JS, sizeof(CONTENT_ASSETS_JS) - 1, _CONTENT_TYPE_JS },
  { "/config.js", CONTENT_CONFIG_JS, sizeof(CONTENT_CONFIG_JS) - 1, _CONTENT_TYPE_JS },
  { "/home.html", CONTENT_HOME_HTML, sizeof(CONTENT_HOME_HTML) - 1, _CONTENT_TYPE_HTML },
  { "/lib.js", CONTENT_LIB_JS, sizeof(CONTENT_LIB_JS) - 1, _CONTENT_TYPE_JS },
  { "/split_term.html", CONTENT_SPLIT_TERM_HTML, sizeof(CONTENT_SPLIT_TERM_HTML) - 1, _CONTENT_TYPE_HTML },
  { "/style.css", CONTENT_STYLE_CSS, sizeof(CONTENT_STYLE_CSS) - 1, _CONTENT_TYPE_CSS },
  { "/term.html", CONTENT_TERM_HTML, sizeof(CONTENT_TERM_HTML) - 1, _CONTENT_TYPE_HTML },
  { "/term.js", CONTENT_TERM_JS, sizeof(CONTENT_TERM_JS) - 1, _CONTENT_TYPE_JS },
  { "/wifi_signal_1.svg", CONTENT_WIFI_SIGNAL_1_SVG, sizeof(CONTENT_WIFI_SIGNAL_1_SVG) - 1, _CONTENT_TYPE_SVG },
  { "/wifi_signal_2.svg", CONTENT_WIFI_SIGNAL_2_SVG, sizeof(CONTENT_WIFI_SIGNAL_2_SVG) - 1, _CONTENT_TYPE_SVG },
  { "/wifi_signal_3.svg", CONTENT_WIFI_SIGNAL_3_SVG, sizeof(CONTENT_WIFI_SIGNAL_3_SVG) - 1, _CONTENT_TYPE_SVG },
  { "/wifi_signal_4.svg", CONTENT_WIFI_SIGNAL_4_SVG, sizeof(CONTENT_WIFI_SIGNAL_4_SVG) - 1, _CONTENT_TYPE_SVG },
  { "/wifi_signal_5.svg", CONTENT_WIFI_SIGNAL_5_SVG, sizeof(CONTENT_WIFI_SIGNAL_5_SVG) - 1, _CONTENT_TYPE_SVG },
};
