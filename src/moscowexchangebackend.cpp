/*
 * harbour-watchlist - Sailfish OS Version
 * Copyright © 2019 Andreas Wüst (andreas.wuest.freelancer@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "moscowexchangebackend.h"
#include "chartdatacalculator.h"

#include <QDebug>
#include <QFile>
#include <QUrl>
#include <QUrlQuery>
#include <QUuid>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QVariantMap>
#include <QJsonDocument>

// const QString MoscowExchangeBackend::MIME_TYPE_JSON = QString("application/json");

MoscowExchangeBackend::MoscowExchangeBackend(QNetworkAccessManager *manager, const QString &applicationName, const QString applicationVersion, QObject *parent)
    : AbstractDataBackend(manager, applicationName, applicationVersion, parent) {
    qDebug() << "Initializing Moscow Exchange Backend...";
    // debug mode when we want to see everything in english
    // debugMode = true;
}

MoscowExchangeBackend::~MoscowExchangeBackend() {
    qDebug() << "Shutting down Moscow Exchange Backend...";
}

void MoscowExchangeBackend::searchName(const QString &searchString) {
    qDebug() << "MoscowExchangeBackend::searchName";
    QNetworkReply *reply = executeGetRequest(QUrl(QString(MOSCOW_EXCHANGE_API_SEARCH).arg(searchString).arg(getLanguage())));

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleRequestError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), this, SLOT(handleSearchNameFinished()));
}

void MoscowExchangeBackend::searchQuoteForNameSearch(const QString &searchString) {
    // TODO check if needed
    qDebug() << "MoscowExchangeBackend::searchQuoteForNameSearch";
    QNetworkReply *reply = executeGetRequest(QUrl(QString(MOSCOW_EXCHANGE_QUOTE).arg(searchString).arg(getLanguage())));

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleRequestError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), this, SLOT(handleSearchQuoteForNameFinished()));
}

void MoscowExchangeBackend::fetchPricesForChart(const QString &extRefId, const int chartType) {
    qDebug() << "MoscowExchangeBackend::fetchClosePrices";

    if (!isChartTypeSupported(chartType)) {
        qDebug() << "MoscowExchangeBackend::fetchClosePrices - chart type " << chartType << " not supported!";
        return;
    }

    QString startDateString = getStartDateForChart(chartType).toString("yyyy-MM-dd");

    QNetworkReply *reply;
    if (chartType > 0) {
        reply = executeGetRequest(QUrl(QString(MOSCOW_EXCHANGE_API_CLOSE_PRICES).arg(extRefId).arg(startDateString).arg(getLanguage())));
    } else {
        // TODO implement
        reply = executeGetRequest(QUrl(QString(MAPI_INTRADAY_PRICES).arg(extRefId)));
    }

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleRequestError(QNetworkReply::NetworkError)));
   //  connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &MoscowExchangeBackend::handleRequestError);
//    connect(reply, SIGNAL(finished()), this, SLOT(handleFetchPricesForChartFinished()));
    connect(reply, &QNetworkReply::finished, this, &MoscowExchangeBackend::handleFetchPricesForChartFinished);

    reply->setProperty("type", chartType);
}

void MoscowExchangeBackend::searchQuote(const QString &searchString) {
    // TODO check if needed
    qDebug() << "MoscowExchangeBackend::searchQuote";
    QNetworkReply *reply = executeGetRequest(QUrl(QString(MOSCOW_EXCHANGE_QUOTE).arg(searchString).arg(getLanguage())));

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleRequestError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), this, SLOT(handleSearchQuoteFinished()));
}

void MoscowExchangeBackend::handleSearchNameFinished() {
    qDebug() << "MoscowExchangeBackend::handleSearchNameFinished";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    QByteArray searchReply = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(searchReply);
    if (jsonDocument.isObject()) {
        emit searchResultAvailable(processSearchResult(searchReply));
    } else {
        qDebug() << "not a json object !";
    }
}

void MoscowExchangeBackend::handleSearchQuoteForNameFinished() {
    qDebug() << "MoscowExchangeBackend::handleSearchQuoteForNameFinished";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    emit searchResultAvailable(processSearchResult(reply->readAll()));
}

void MoscowExchangeBackend::handleSearchQuoteFinished() {
    qDebug() << "MoscowExchangeBackend::handleSearchQuoteForNameFinished";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    emit quoteResultAvailable(processQuoteResult(reply->readAll()));
}

void MoscowExchangeBackend::handleFetchPricesForChartFinished() {
    qDebug() << "MoscowExchangeBackend::handleFetchPricesForChartFinished";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    QByteArray resultByteArray = reply->readAll();
    QString result = QString(resultByteArray);

    qDebug() << "MoscowExchangeBackend::handleFetchPricesForChartFinished result " << result;

    QString jsonResponseString = parsePriceResponse(resultByteArray);

    if (!jsonResponseString.isNull()) {
        emit fetchPricesForChartAvailable(jsonResponseString, reply->property("type").toInt());
    }
}

QString MoscowExchangeBackend::parsePriceResponse(QByteArray reply) {
    QJsonDocument jsonDocument = QJsonDocument::fromJson(reply);
    if (!jsonDocument.isObject()) {
        qDebug() << "not a json object!";
        return QString();
    }

    QJsonObject responseObject = jsonDocument.object();
    QJsonObject historyObject = responseObject["history"].toObject();
    QJsonArray responseArray = historyObject["data"].toArray();

    QJsonDocument resultDocument;
    QJsonArray resultArray;

    ChartDataCalculator chartDataCalculator;

    foreach (const QJsonValue & value, responseArray) {
        QJsonArray valueArray = value.toArray();
        QJsonObject resultObject;

        QString tradeDate = valueArray.at(1).toString(); // TRADEDATE
        // artifical time - irrelevant - since we do not display the time for these history entries
        QString tradeDateTime = tradeDate + " 18:00:00";
        QDateTime dateTimeTradeDate = QDateTime::fromString(tradeDateTime, Qt::ISODate);

        QJsonValue closeObject = valueArray.at(11); // CLOSE
        double closeValue = closeObject.toDouble();

        chartDataCalculator.checkCloseValue(closeValue);

        resultObject.insert("x", dateTimeTradeDate.toMSecsSinceEpoch() / 1000);
        resultObject.insert("y", closeValue);

        resultArray.push_back(resultObject);
    }

    // resultDocument.setArray(resultArray);
    QJsonObject resultObject;
    resultObject.insert("min", chartDataCalculator.getMinValue());
    resultObject.insert("max", chartDataCalculator.getMaxValue());
    resultObject.insert("fractionDigits", chartDataCalculator.getFractionDigits());
    resultObject.insert("data", resultArray);

    resultDocument.setObject(resultObject);

    QString dataToString(resultDocument.toJson());
    return dataToString;
}

QString MoscowExchangeBackend::processSearchResult(QByteArray searchReply) {
    qDebug() << "MoscowExchangeBackend::processSearchResult";
    QJsonDocument jsonDocument = QJsonDocument::fromJson(searchReply);
    if (!jsonDocument.isObject()) {
        qDebug() << "not a json object!";
    }

    QJsonObject responseObject = jsonDocument.object();
    QJsonObject securitiesObject = responseObject["securities"].toObject();
    QJsonArray dataArray = securitiesObject["data"].toArray();

//    QJsonObject columnsObject = securitiesObject["columns"].toObject();
//    QJsonObject metadataObject = securitiesObject["metadata"].toObject();

    QJsonDocument resultDocument;
    QJsonArray resultArray;

    foreach (const QJsonValue & value, dataArray) {
        QJsonArray resultDataArray = value.toArray();

        // id is not mapped so far - is it used ??
        QJsonObject resultObject;
        resultObject.insert("extRefId", resultDataArray.at(1)); // secId
        resultObject.insert("symbol1", resultDataArray.at(1)); // secId
        resultObject.insert("name", resultDataArray.at(4)); // name
        resultObject.insert("isin", resultDataArray.at(5)); // isin
        resultObject.insert("stockMarketName", resultDataArray.at(14)); // primary_boardid
        resultObject.insert("currency", "-"); // dummy for currency

        resultArray.push_back(resultObject);
    }

    resultDocument.setArray(resultArray);
    QString dataToString(resultDocument.toJson());

    return dataToString;
}

QString MoscowExchangeBackend::processQuoteResult(QByteArray searchReply) {
    qDebug() << "MoscowExchangeBackend::processQuoteResult";
    QJsonDocument jsonDocument = QJsonDocument::fromJson(searchReply);
    if (!jsonDocument.isObject()) {
        qDebug() << "not a json object!";
    }

    QJsonObject responseObject = jsonDocument.object();
    QJsonObject securitiesObject = responseObject["securities"].toObject();
    QJsonObject marketDataObject = responseObject["marketdata"].toObject();
    QJsonArray dataArray = securitiesObject["data"].toArray();
    QJsonArray marketDataArray = marketDataObject["data"].toArray();

//    QJsonObject columnsObject = securitiesObject["columns"].toObject();
//    QJsonObject metadataObject = securitiesObject["metadata"].toObject();

    QJsonDocument resultDocument;
    QJsonArray resultArray;

    if (dataArray.size() != marketDataArray.size()) {
        qDebug() << "data arrays do not have the same size!";
    }

    int dataLength = dataArray.size();

    for (int i = 0; i < dataLength; i++) {
        QJsonArray tmpMarketDataArray = marketDataArray.at(i).toArray();
        QJsonArray tmpDataArray = dataArray.at(i).toArray();

        // id is not mapped so far - is it used ??
        QJsonObject resultObject;

        // read from securities data
        resultObject.insert("name", tmpDataArray.at(2)); // name
        resultObject.insert("isin", tmpDataArray.at(19)); // isin
        resultObject.insert("currency", convertCurrency(tmpDataArray.at(24).toString())); // CURRENCYID

        // read from marketdata data
        resultObject.insert("extRefId", tmpMarketDataArray.at(0)); // secId
        resultObject.insert("symbol1", tmpMarketDataArray.at(0)); // secId
        resultObject.insert("stockMarketName", tmpMarketDataArray.at(1)); // primary_boardid

        resultObject.insert("price", tmpMarketDataArray.at(36)); // LCLOSEPRICE
        resultObject.insert("high", tmpMarketDataArray.at(11)); // HIGH
        resultObject.insert("low", tmpMarketDataArray.at(10)); // LOW
        resultObject.insert("volume", tmpMarketDataArray.at(27)); // VOLTODAY
        resultObject.insert("changeAbsolute", tmpMarketDataArray.at(41)); // CHANGE
        resultObject.insert("changeRelative", tmpMarketDataArray.at(25)); // LASTTOPREVPRICE

        QString timestampString = QString("");
        QString systimeString = tmpMarketDataArray.at(48).toString();
        if (!systimeString.isEmpty() && systimeString.length() > 10) {
            timestampString.append(systimeString.mid(0, 10));
            QString updateTimeString = tmpMarketDataArray.at(32).toString();
            if (!updateTimeString.isEmpty()) {
                timestampString.append(" ");
                timestampString.append(updateTimeString);
            }
            resultObject.insert("quoteTimestamp", timestampString);
        }

        QDateTime dateTimeNow = QDateTime::currentDateTime();
        QString nowString = dateTimeNow.toString("yyyy-MM-dd") + " " + dateTimeNow.toString("hh:mm:ss");
        resultObject.insert("lastChangeTimestamp", nowString);

        resultArray.push_back(resultObject);
    }

    resultDocument.setArray(resultArray);
    QString dataToString(resultDocument.toJson());

    return dataToString;
}

QString MoscowExchangeBackend::convertCurrency(const QString &currencyString) {
    if (QString("SUR").compare(currencyString, Qt::CaseInsensitive) == 0) {
        return tr("RUB"); // "\u20BD"
    }
    return currencyString;
}

bool MoscowExchangeBackend::isChartTypeSupported(const int chartType) {
    switch(chartType) {
        case ChartType::MONTH:
        case ChartType::THREE_MONTHS:
            return true;
        case ChartType::INTRADAY:
        case ChartType::FIVE_YEARS:
        case ChartType::YEAR:
        case ChartType::THREE_YEARS:
            return false;
        default:
            qDebug() << "EuroinvestorBackend::isChartTypeSupported : illegal chartType received " << chartType;
            return false;
    }
}

QString MoscowExchangeBackend::getLanguage() {
    return (debugMode ? QString(LANG_EN) : QString());
}
