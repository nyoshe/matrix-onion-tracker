#include "ConfigPortal.h"

#include <DNSServer.h>
#include <WebServer.h>

#include "CoinStore.h"
#include "Net.h"
#include "NetworkStore.h"
#include "PriceFeed.h"

namespace {

WebServer server(80);
DNSServer dnsServer;
bool dnsRunning = false;

// Phones/laptops probe a known URL to tell whether a network has real
// internet; any answer other than the expected one (a plain 204, or a
// specific fixed page) makes them treat it as a captive portal and pop a
// sign-in browser straight to whatever we redirect to here. Combined with
// the DNS server resolving every hostname to us, that covers both the
// probe-by-domain-name and probe-by-fixed-IP cases.
void handleNotFound() {
  server.sendHeader("Location", "http://" + server.client().localIP().toString() + "/");
  server.send(302, "text/plain", "");
}

// Thresholds roughly match what phone WiFi pickers treat as 4/3/2/1 bars.
const char* signalQuality(int32_t rssi) {
  if (rssi >= -55) return "Excellent";
  if (rssi >= -67) return "Good";
  if (rssi >= -75) return "Fair";
  return "Weak";
}

void appendEscaped(String& out, const char* s) {
  for (const char* p = s; *p; p++) {
    switch (*p) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      default: out += *p;
    }
  }
}

// Percent-encodes for use in a query string -- SSIDs can contain spaces and
// other characters a raw URL can't. WebServer's arg() decodes this back on
// the way in.
void appendUrlEncoded(String& out, const char* s) {
  const char* hex = "0123456789ABCDEF";
  for (const char* p = s; *p; p++) {
    unsigned char c = (unsigned char)*p;
    bool safe = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' ||
                c == '_' || c == '.' || c == '~';
    if (safe) {
      out += (char)c;
    } else {
      out += '%';
      out += hex[c >> 4];
      out += hex[c & 0xF];
    }
  }
}

// Tabs are two radio inputs + a CSS sibling selector -- no JS, so the page
// stays servable from an ESP32 with no extra requests. For this to work, the
// radios, the ".tabs" nav and the two ".panel"s must all be direct siblings
// in that order (general sibling selectors only look forward).
const char kStyle[] =
    "*{box-sizing:border-box}"
    "body{font-family:system-ui,sans-serif;margin:0;padding:1rem;"
    "background:#111;color:#eee}"
    "h1{margin:0 0 1rem;font-size:1.3rem}"
    "h2{font-size:1rem;color:#9cf;margin:1.25rem 0 .5rem}"
    "input[name=view]{display:none}"
    ".tabs{display:flex;gap:.25rem;margin-bottom:1rem;border-bottom:1px solid #333}"
    ".tabs label{padding:.5rem 1rem;cursor:pointer;color:#aaa;"
    "border-radius:.25rem .25rem 0 0}"
    "#view-network:checked ~ .tabs label[for=view-network],"
    "#view-coins:checked ~ .tabs label[for=view-coins]{background:#222;color:#fff}"
    ".panel{display:none}"
    "#view-network:checked ~ .panel-network,"
    "#view-coins:checked ~ .panel-coins{display:block}"
    ".status{padding:.5rem .75rem;border-radius:.25rem;background:#1a1a1a;margin:0}"
    "ul{list-style:none;margin:0;padding:0}"
    "li{display:flex;justify-content:space-between;align-items:center;gap:.5rem;"
    "padding:.4rem 0;border-bottom:1px solid #222}"
    ".net-pick{display:flex;flex:1;justify-content:space-between;align-items:center;"
    "gap:.5rem;color:inherit;text-decoration:none}"
    ".net-pick:hover{color:#fff}"
    "input:not([type]),input[type=password]{background:#1a1a1a;border:1px solid #333;"
    "color:#eee;padding:.3rem .5rem;border-radius:.2rem}"
    "button{background:#2a6;border:none;color:#fff;padding:.3rem .7rem;"
    "border-radius:.2rem;cursor:pointer}"
    "button:hover{background:#3b7}"
    "li button{background:#a33}"
    "li button:hover{background:#c44}"
    "small{color:#888}";

void handleRoot() {
  // Set when the user clicked a scanned network below -- pre-fills the add
  // form's SSID so they only have to type the password.
  String selectedSsid = server.arg("ssid");

  String page =
      "<!DOCTYPE html><html><head>"
      "<meta name=viewport content='width=device-width,initial-scale=1'>"
      "<title>matrix-onion-tracker</title><style>" + String(kStyle) +
      "</style></head><body>"
      "<h1>matrix-onion-tracker</h1>"
      "<input type=radio name=view id=view-network checked>"
      "<input type=radio name=view id=view-coins>"
      "<div class=tabs>"
      "<label for=view-network>Network</label>"
      "<label for=view-coins>Coins</label>"
      "</div>";

  page += "<div class='panel panel-network'>";
  if (Net::isConnected()) {
    page += "<p class=status>Connected to <b>";
    appendEscaped(page, Net::connectedSsid());
    page += "</b> (" + String(Net::staIp()) + ")</p>";
  } else {
    // No auto-refresh here: this page is reached while filling out the add
    // form below, and a full reload mid-typing would wipe it out. Reload
    // manually to check whether it connected.
    page +=
        "<p class=status>Not connected to a network yet -- trying any saved networks. "
        "Add one below, or reload this page to check status.</p>";
  }

  page += "<h2>Available networks</h2><ul>";
  int sn = Net::scanCount();
  if (sn < 0) {
    page += "<li>Scanning&hellip;</li>";
  } else if (sn == 0) {
    page += "<li>(none found)</li>";
  } else {
    for (int i = 0; i < sn; i++) {
      const char* ssid = Net::scanSsid(i);
      int32_t rssi = Net::scanRssi(i);
      page += "<li><a class=net-pick href='/?ssid=";
      appendUrlEncoded(page, ssid);
      page += "#add-network'><span>";
      appendEscaped(page, ssid);
      if (Net::isConnected() && strcmp(ssid, Net::connectedSsid()) == 0) page += " <small>(connected)</small>";
      if (Net::scanIsOpen(i)) page += " <small>(open)</small>";
      page += "</span><small>" + String(rssi) + " dBm &middot; " + signalQuality(rssi) + "</small></a></li>";
    }
  }
  page += "</ul>";

  page += "<h2>Saved networks</h2><ul>";
  int n = NetworkStore::count();
  if (n == 0) page += "<li>(none)</li>";
  for (int i = 0; i < n; i++) {
    page += "<li>";
    appendEscaped(page, NetworkStore::ssid(i));
    page += " <form style='display:inline' method=POST action=/delete>"
            "<input type=hidden name=index value='" +
            String(i) + "'><button type=submit>Delete</button></form></li>";
  }
  page += "</ul>";

  page += "<h2 id=add-network>Add a network</h2>"
          "<form method=POST action=/add>"
          "SSID: <input name=ssid maxlength=32 required value=\"";
  appendEscaped(page, selectedSsid.c_str());
  page += "\"><br>"
          "Password: <input name=password type=password maxlength=63";
  if (selectedSsid.length() > 0) page += " autofocus";
  page += "><br>"
          "<button type=submit>Save &amp; connect</button>"
          "</form>"
          "</div>";

  page += "<div class='panel panel-coins'>";
  page += "<h2>Tracked coins</h2><ul>";
  int cn = CoinStore::count();
  if (cn == 0) page += "<li>(none)</li>";
  for (int i = 0; i < cn; i++) {
    page += "<li>";
    appendEscaped(page, CoinStore::symbol(i));
    page += " (";
    appendEscaped(page, CoinStore::id(i));
    page += ")";
    if (!PriceFeed::known(CoinStore::id(i))) page += " <b>not found on CoinGecko?</b>";
    page += " <form style='display:inline' method=POST action=/coin/delete>"
            "<input type=hidden name=index value='" +
            String(i) + "'><button type=submit>Delete</button></form></li>";
  }
  page += "</ul>";

  page +=
      "<h2>Add a coin</h2>"
      "<form method=POST action=/coin/add>"
      "CoinGecko id: <input name=id maxlength=23 required> "
      "(the slug in coingecko.com/en/coins/&lt;id&gt;)<br>"
      "Symbol: <input name=symbol maxlength=5 placeholder=optional><br>"
      "<button type=submit>Save</button>"
      "</form>"
      "</div>"
      "</body></html>";

  server.send(200, "text/html", page);
}

void handleAdd() {
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  int index = NetworkStore::add(ssid.c_str(), password.c_str());
  if (index >= 0) Net::tryIndex(index);
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleDelete() {
  NetworkStore::remove(server.arg("index").toInt());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleCoinAdd() {
  String id = server.arg("id");
  id.toLowerCase();
  CoinStore::add(id.c_str(), server.arg("symbol").c_str());
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleCoinDelete() {
  CoinStore::remove(server.arg("index").toInt());
  server.sendHeader("Location", "/");
  server.send(303);
}

}  // namespace

void ConfigPortal::begin() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/add", HTTP_POST, handleAdd);
  server.on("/delete", HTTP_POST, handleDelete);
  server.on("/coin/add", HTTP_POST, handleCoinAdd);
  server.on("/coin/delete", HTTP_POST, handleCoinDelete);
  server.onNotFound(handleNotFound);
  server.begin();
}

void ConfigPortal::poll() {
  server.handleClient();

  // DNS hijacking only makes sense while the AP is what phones are actually
  // using; start/stop it on the AP's own transitions rather than once at
  // boot, so it always resolves to whatever the AP's current IP is.
  bool apActive = Net::isApActive();
  if (apActive && !dnsRunning) {
    dnsServer.start();  // defaults: port 53, catch-all domain, resolves to the AP's IP
    dnsRunning = true;
  } else if (!apActive && dnsRunning) {
    dnsServer.stop();
    dnsRunning = false;
  }
}
