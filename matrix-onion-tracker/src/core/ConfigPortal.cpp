#include "ConfigPortal.h"

#include <WebServer.h>

#include "Net.h"
#include "NetworkStore.h"

namespace {

WebServer server(80);

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
  server.begin();
}

void ConfigPortal::poll() { server.handleClient(); }
