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
#include "euroinvestorbackend.h"
#include "chartdatacalculator.h"

#include <QDebug>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QJsonDocument>

EuroinvestorBackend::EuroinvestorBackend(QNetworkAccessManager *manager, const QString &applicationName, const QString applicationVersion, QObject *parent)
    : AbstractDataBackend(manager, applicationName, applicationVersion, parent) {
    qDebug() << "Initializing Euroinvestor Backend...";
}

EuroinvestorBackend::~EuroinvestorBackend() {
    qDebug() << "Shutting down Euroinvestor Backend...";
}

void EuroinvestorBackend::searchName(const QString &searchString) {
    qDebug() << "EuroinvestorBackend::searchName";
    QNetworkReply *reply = executeGetRequest(QUrl(API_SEARCH + searchString));

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleRequestError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), this, SLOT(handleSearchNameFinished()));
}

void EuroinvestorBackend::searchQuoteForNameSearch(const QString &searchString) {
    qDebug() << "EuroinvestorBackend::searchQuoteForNameSearch";
    QNetworkReply *reply = executeGetRequest(QUrl(API_QUOTE + searchString));

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleRequestError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), this, SLOT(handleSearchQuoteForNameFinished()));
}

void EuroinvestorBackend::fetchPricesForChart(const QString &extRefId, const int chartType) {
    qDebug() << "EuroinvestorBackend::fetchClosePrices";

    if (!isChartTypeSupported(chartType)) {
        qDebug() << "EuroinvestorBackend::fetchClosePrices - chart type " << chartType << " not supported!";
        return;
    }

    QString startDateString = getStartDateForChart(chartType).toString("yyyy-MM-dd");

    QNetworkReply *reply;
    if (chartType > 0) {
        reply = executeGetRequest(QUrl(QString(API_CLOSE_PRICES).arg(extRefId).arg(startDateString)));
    } else {
        reply = executeGetRequest(QUrl(QString(API_INTRADAY_PRICES).arg(extRefId)));
    }

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleRequestError(QNetworkReply::NetworkError)));
   //  connect(reply, static_cast<void (QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error), this, &EuroinvestorBackend::handleRequestError);
//    connect(reply, SIGNAL(finished()), this, SLOT(handleFetchPricesForChartFinished()));
    connect(reply, &QNetworkReply::finished, this, &EuroinvestorBackend::handleFetchPricesForChartFinished);

    reply->setProperty("type", chartType);
}

void EuroinvestorBackend::searchQuote(const QString &searchString) {
    qDebug() << "EuroinvestorBackend::searchQuote";
    QNetworkReply *reply = executeGetRequest(QUrl(API_QUOTE + searchString));

    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(handleRequestError(QNetworkReply::NetworkError)));
    connect(reply, SIGNAL(finished()), this, SLOT(handleSearchQuoteFinished()));
}

void EuroinvestorBackend::handleSearchNameFinished() {
    qDebug() << "EuroinvestorBackend::handleSearchNameFinished";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    QByteArray searchReply = reply->readAll();
    QJsonDocument jsonDocument = QJsonDocument::fromJson(searchReply);
    if (jsonDocument.isArray()) {
        QJsonArray responseArray = jsonDocument.array();
        qDebug() << "array size : " << responseArray.size();

        QStringList idList;

        foreach (const QJsonValue & value, responseArray) {
            QJsonObject rootObject = value.toObject();
            QJsonObject sourceObject = rootObject["_source"].toObject();
            idList.append(QString::number(sourceObject.value("id").toInt()));
        }

        QString quoteQueryIds = idList.join(",");

        qDebug() << "EuroinvestorBackend::handleSearchNameFinished - quoteQueryIds : " << quoteQueryIds;

        searchQuoteForNameSearch(quoteQueryIds);

    } else {
        qDebug() << "not a json object !";
    }
}

void EuroinvestorBackend::handleSearchQuoteForNameFinished() {
    qDebug() << "EuroinvestorBackend::handleSearchQuoteForNameFinished";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    emit searchResultAvailable(processQuoteSearchResult(reply->readAll()));
}

void EuroinvestorBackend::handleSearchQuoteFinished() {
    qDebug() << "EuroinvestorBackend::handleSearchQuoteForNameFinished";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    emit quoteResultAvailable(processQuoteSearchResult(reply->readAll()));
}

void EuroinvestorBackend::handleFetchPricesForChartFinished() {
    qDebug() << "EuroinvestorBackend::handleFetchPricesForChartFinished";
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(sender());
    reply->deleteLater();
    if (reply->error() != QNetworkReply::NoError) {
        return;
    }

    QByteArray resultByteArray = reply->readAll();
    QString result = QString(resultByteArray);

    qDebug() << "EuroinvestorBackend::handleFetchPricesForChartFinished result " << result;

    QString jsonResponseString = parsePriceResponse(resultByteArray);

    if (!jsonResponseString.isNull()) {
        emit fetchPricesForChartAvailable(jsonResponseString, reply->property("type").toInt());
    }
}

QString EuroinvestorBackend::parsePriceResponse(QByteArray reply) {
    QJsonDocument jsonDocument = QJsonDocument::fromJson(reply);
    if (!jsonDocument.isArray()) {
        qDebug() << "not a json array!";
        return QString();
    }

    QJsonArray responseArray = jsonDocument.array();
    QJsonDocument resultDocument;
    QJsonArray resultArray;

    ChartDataCalculator chartDataCalculator;

    foreach (const QJsonValue & value, responseArray) {
        QJsonObject rootObject = value.toObject();
        QJsonObject resultObject;

        QJsonValue jsonUpdatedAt = rootObject.value("timestamp");
//        QDateTime dateTimeUpdatedAt = QDateTime::fromString(jsonUpdatedAt.toString(), Qt::ISODate);

        QDateTime updatedAtLocalTime = convertUTCDateTimeToLocalDateTime(jsonUpdatedAt.toString());

//        qDebug() << dateTimeUpdatedAt << " - " << updatedAtLocalTime;


        double closeValue = rootObject.value("close").toDouble();

        chartDataCalculator.checkCloseValue(closeValue);

        resultObject.insert("x", updatedAtLocalTime.toMSecsSinceEpoch() / 1000);
        resultObject.insert("y", closeValue);

        resultArray.push_back(resultObject);
    }

    QJsonObject resultObject;
    resultObject.insert("min", chartDataCalculator.getMinValue());
    resultObject.insert("max", chartDataCalculator.getMaxValue());
    resultObject.insert("fractionDigits", chartDataCalculator.getFractionDigits());
    resultObject.insert("data", resultArray);

    resultDocument.setObject(resultObject);

    QString dataToString(resultDocument.toJson());
    return dataToString;
}

QString EuroinvestorBackend::processQuoteSearchResult(QByteArray searchReply) {
    qDebug() << "EuroinvestorBackend::processQuoteSearchResult";
    QJsonDocument jsonDocument = QJsonDocument::fromJson(searchReply);
    if (!jsonDocument.isArray()) {
        qDebug() << "not a json array!";
    }

    QJsonArray responseArray = jsonDocument.array();
    QJsonDocument resultDocument;
    QJsonArray resultArray;

    foreach (const QJsonValue & value, responseArray) {
        QJsonObject rootObject = value.toObject();
        QJsonObject exchangeObject = rootObject["exchange"].toObject();

        QJsonObject resultObject;
        resultObject.insert("extRefId", rootObject.value("id"));
        resultObject.insert("name", rootObject.value("name"));
        resultObject.insert("currency", convertCurrency(rootObject.value("currency").toString()));
        resultObject.insert("price", rootObject.value("last"));
        resultObject.insert("symbol1", rootObject.value("symbol"));
        resultObject.insert("isin", rootObject.value("isin"));
        resultObject.insert("stockMarketName", exchangeObject.value("name"));
        resultObject.insert("changeAbsolute", rootObject.value("change"));
        resultObject.insert("changeRelative", rootObject.value("changeInPercentage"));
        resultObject.insert("high", rootObject.value("high"));
        resultObject.insert("low", rootObject.value("low"));
        resultObject.insert("ask", rootObject.value("ask"));
        resultObject.insert("bid", rootObject.value("bid"));
        resultObject.insert("volume", rootObject.value("volume"));
        resultObject.insert("numberOfStocks", rootObject.value("numberOfStocks"));

        QJsonValue jsonUpdatedAt = rootObject.value("updatedAt");
        QDateTime updatedAtLocalTime = convertUTCDateTimeToLocalDateTime(jsonUpdatedAt.toString());
        resultObject.insert("quoteTimestamp", convertToDatabaseDateTimeFormat(updatedAtLocalTime));

        resultObject.insert("lastChangeTimestamp", convertToDatabaseDateTimeFormat(QDateTime::currentDateTime()));

        resultArray.push_back(resultObject);
    }

    resultDocument.setArray(resultArray);
    QString dataToString(resultDocument.toJson());

    return dataToString;
}

QDateTime EuroinvestorBackend::convertUTCDateTimeToLocalDateTime(const QString utcDateTimeString) {
    QDateTime utcDateTime = QDateTime::fromString(utcDateTimeString, Qt::ISODate);
    QDateTime localDateTime = QDateTime(utcDateTime.date(), utcDateTime.time(), Qt::UTC).toLocalTime();

    // qDebug() << " converted date from " << utcDateTimeString << " to " << localDateTime;

    return localDateTime;
}

QString EuroinvestorBackend::convertCurrency(const QString &currencyString) {
    if (QString("EUR").compare(currencyString, Qt::CaseInsensitive) == 0) {
        return QString("\u20AC");
    }
    if (QString("USD").compare(currencyString, Qt::CaseInsensitive) == 0) {
        return QString("$");
    }
    return currencyString;
}

bool EuroinvestorBackend::isChartTypeSupported(const int chartType) {
    switch(chartType) {
        case ChartType::INTRADAY:
        case ChartType::MONTH:
        case ChartType::THREE_MONTHS:
        case ChartType::YEAR:
        case ChartType::THREE_YEARS:
            return true;
        case ChartType::FIVE_YEARS:
            return false;
        default:
            qDebug() << "EuroinvestorBackend::isChartTypeSupported : illegal chartType received " << chartType;
            return false;
    }
}
