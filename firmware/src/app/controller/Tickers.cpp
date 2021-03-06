#include "Tickers.h"

#include "../Manager.h"

Tickers::Tickers(Manager *_manager) {
    manager = _manager;

    bool tickersFileExists = manager->filesystem->exists(SPIFFS, FILE_TICKERS);

    if(!tickersFileExists) {
        Serial.print("Tickers.json is not valid, creating...");

        manager->filesystem->writeFile(SPIFFS, FILE_TICKERS, "[]");
    }
};

Tickers::~Tickers() {
};

String Tickers::get() {
    File tickersFile;
    manager->filesystem->readFile(SPIFFS, FILE_TICKERS, tickersFile);

    String contents = tickersFile.readString();

    tickersFile.close();

    return contents;
}

bool Tickers::add(const char *id, const char *coin, const char *currency) {
    String tickers = get();
    DynamicJsonBuffer jsonBuffer;
    JsonArray& tickersArray = jsonBuffer.parse(tickers);

    DynamicJsonBuffer objBuffer;
    JsonObject& obj = objBuffer.createObject();

    DynamicJsonBuffer alarmsBuffer;
    JsonArray& alarms = alarmsBuffer.createArray();

    obj["id"] = id;
    obj["coin"] = coin;
    obj["currency"] = currency;
    obj["price"] = "0";
    obj["last_update"] = "0";
    obj["change_24h"] = "0";
    obj["vol_24h"] = "0";
    obj["alarms"] = alarms;

    tickersArray.add(obj);

    String str;
    tickersArray.printTo(str);
    
    return manager->filesystem->writeFile(SPIFFS, FILE_TICKERS, str.c_str());
}

int Tickers::getIndexOf(const char *id, const char *currency) {
    String tickers = get();
    DynamicJsonBuffer jsonBuffer;
    JsonArray& tickersArray = jsonBuffer.parse(tickers);

    for(int i = 0; i < tickersArray.size(); i++) {
        JsonObject& obj = tickersArray[i].as<JsonObject>();

        if(strcmp(obj["id"], id) == 0 && strcmp(obj["currency"], currency) == 0) {
            return i;
        }
    }

    return -1;
}

bool Tickers::remove(const char *id, const char *currency) {
    int index = getIndexOf(id, currency);

    if(index == -1) {
        return false;
    }

    return remove(index);
}

bool Tickers::remove(int index) {
    String tickers = get();
    DynamicJsonBuffer jsonBuffer;
    JsonArray& tickersArray = jsonBuffer.parse(tickers);

    tickersArray.remove(index);

    String str;
    tickersArray.printTo(str);
    return manager->filesystem->writeFile(SPIFFS, FILE_TICKERS, str.c_str());
}

void Tickers::reset() {
    manager->filesystem->deleteFile(SPIFFS, FILE_TICKERS);
    manager->filesystem->writeFile(SPIFFS, FILE_TICKERS, "[]");
}

String Tickers::getTickerData(const char *ids, const char *currencies) {
    String url = URL_TICKER_DATA;
    url.concat("&ids=");
    url.concat(ids);
    url.concat("&vs_currencies=");
    url.concat(currencies);

    return manager->webserver->get(url);
}

bool Tickers::updateTickers() {
    Serial.println("Updating tickers");

    String tickers = get();
    DynamicJsonBuffer jsonBuffer;
    JsonArray& tickersArray = jsonBuffer.parse(tickers);

    String coins;
    String currencies;

    for(int i = 0; i < tickersArray.size(); i++) {
        JsonObject& obj = tickersArray[i].as<JsonObject>();

        String id = obj["id"];
        String currency = obj["currency"];

        coins += id + ",";
        currencies += currency + ",";
    }

    String response = getTickerData(coins.c_str(), currencies.c_str());
    if(response.length() == 0) {
        Serial.println("Failed to update tickers, invalid response");
        return false;
    }

    DynamicJsonBuffer responseJsonBuffer;
    JsonObject& responseJson = responseJsonBuffer.parse(response);

    for(int i = 0; i < tickersArray.size(); i++) {
        JsonObject& obj = tickersArray[i].as<JsonObject>();

        String id = obj["id"];
        String currency = obj["currency"];

        // CoinGecko responses are in lowercase
        id.toLowerCase();
        currency.toLowerCase();

        String price = responseJson[id][currency];
        String last_update = responseJson[id]["last_updated_at"];
        String change_24h = String(Utils::roundDecimals(responseJson[id][currency + "_24h_change"], 2));
        String vol_24h = Utils::numToHuman(responseJson[id][currency + "_24h_vol"], 2);

        tickersArray[i]["price"] = price;
        tickersArray[i]["last_update"] = last_update;
        tickersArray[i]["change_24h"] = change_24h;
        tickersArray[i]["vol_24h"] = vol_24h;
    }

    String str;
    tickersArray.printTo(str);
    
    return manager->filesystem->writeFile(SPIFFS, FILE_TICKERS, str.c_str());
}

bool Tickers::changeOrder(int from, int to) {
    String tickers = get();
    DynamicJsonBuffer jsonBuffer;
    JsonArray& tickersArray = jsonBuffer.parse(tickers);

    if(!tickersArray[from].is<JsonObject>() || !tickersArray[to].is<JsonObject>()) {
        return false;
    }

    JsonObject& fromTemp = tickersArray[from].as<JsonObject>();

    tickersArray[from] = tickersArray[to];
    tickersArray[to] = fromTemp;

    String str;
    tickersArray.printTo(str);
    manager->filesystem->writeFile(SPIFFS, FILE_TICKERS, str.c_str());

    return true;
}

void Tickers::requestTickers(AsyncWebServerRequest *request) {
    DynamicJsonBuffer jsonBuffer;
    JsonObject& response = jsonBuffer.createObject();

    String tickersStr = get();

    DynamicJsonBuffer buffer;
    response["status"] = "ok";
    response["message"] = buffer.parse(tickersStr);

    String str;
    response.printTo(str);
    request->send(200, "application/json", str);
};

void Tickers::requestAddTickers(AsyncWebServerRequest *request) {
    if(!request->hasParam("id", true) || !request->hasParam("coin", true) || !request->hasParam("currency", true)) {
        manager->webserver->requestInvalid(request);
        return;
    }

    const char *id = request->getParam("id", true)->value().c_str();
    const char *coin = request->getParam("coin", true)->value().c_str();
    const char *currency = request->getParam("currency", true)->value().c_str();

    int index = getIndexOf(id, currency);

    if(index != -1) {
        request->send(200, "application/json", "{\"status\":\"error\",\"message\":\"Coin already exists\"}");
        return;
    }

    add(id, coin, currency);

    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"added\"}");
};


void Tickers::requestRemoveTickers(AsyncWebServerRequest *request) {
    if(!request->hasParam("id", true) || !request->hasParam("currency", true)) {
        manager->webserver->requestInvalid(request);
        return;
    }

    const char *id = request->getParam("id", true)->value().c_str();
    const char *currency = request->getParam("currency", true)->value().c_str();

    remove(id, currency);

    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"removed\"}");
};

void Tickers::requestOrderTickers(AsyncWebServerRequest *request) {
    if(!request->hasParam("from", true) || !request->hasParam("to", true)) {
        manager->webserver->requestInvalid(request);
        return;
    }

    int from = request->getParam("from", true)->value().toInt();
    int to = request->getParam("to", true)->value().toInt();

    bool changedOrder = changeOrder(from, to);

    if(!changedOrder) {
        manager->webserver->requestInvalid(request);
        return;
    }

    request->send(200, "application/json", "{\"status\":\"ok\",\"message\":\"changed order\"}");
};