#ifndef SERVER_H
#define SERVER_H

#define WEB_NAME_WIFI_SSID "ssid"
#define WEB_NAME_WIFI_PASS "password"

static char web_page[] = {
    "<header>"
    "</header>"
    "<body>"
        "<form method=\"post\" action=\"/\">"
        "<label for=\"fname\">Wi-Fi SSID:</label><br>"
        "<input type=\"text\" name=\"ssid\"><br>"
        "<label for=\"lname\">Password:</label><br>"
        "<input type=\"password\" name=\"password\">"
        "<input type=\"submit\" value=\"Submit\">"
        "</form>"
        "</body>"
};

void server_init();

#endif //SERVER_H