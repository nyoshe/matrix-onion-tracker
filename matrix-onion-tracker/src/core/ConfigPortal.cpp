#include "ConfigPortal.h"

#include <DNSServer.h>
#include <WebServer.h>

#include "Net.h"
#include "NetworkStore.h"

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

void handleRoot() {
  String page =
      "<!DOCTYPE html><html><head>"
      "<meta name=viewport content='width=device-width,initial-scale=1'>"
      "<title>matrix-onion-tracker</title></head><body>"
      "<h1>matrix-onion-tracker</h1>";

  if (Net::isConnected()) {
    page += "<p>Connected to <b>";
    appendEscaped(page, Net::connectedSsid());
    page += "</b> (" + String(Net::staIp()) + ")</p>";
  } else {
    page +=
        "<p>Not connected to a network yet -- trying any saved networks. "
        "Add one below once this page is reachable again.</p>"
        "<meta http-equiv=refresh content=5>";
  }

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

  page +=
      "<h2>Add a network</h2>"
      "<form method=POST action=/add>"
      "SSID: <input name=ssid maxlength=32 required><br>"
      "Password: <input name=password type=password maxlength=63><br>"
      "<button type=submit>Save &amp; connect</button>"
      "</form></body></html>";

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

}  // namespace

void ConfigPortal::begin() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/add", HTTP_POST, handleAdd);
  server.on("/delete", HTTP_POST, handleDelete);
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
