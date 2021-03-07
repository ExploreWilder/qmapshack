/**********************************************************************************************
    Copyright (C) 2021 Clement <explorewilder.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

**********************************************************************************************/

#include "CAuth.h"
#include "helpers/CSettings.h"

#include <QtWidgets>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUuid>
#include <QJsonDocument>
#include <iostream>

CAuth::CAuth()
    : QDialog(), isRememberMe(true)
{
    SETTINGS;
    hashedToken = cfg.value("User/token", "").toString();
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    manager = new QNetworkAccessManager();
    connect(manager, &QNetworkAccessManager::finished, this, &CAuth::slotFinishedPreRequest);

    if (!hashedToken.isEmpty())
    {
        QEventLoop loop;
        connect(this, &CAuth::sigLoopDone, &loop, &QEventLoop::quit);
        requestCheck();
        loop.exec(); // wait without GUI
    }
    else
    {
        openDialog();
    }
}

CAuth::~CAuth()
{
    delete manager;
}

bool CAuth::readReply(const QByteArray &json)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json);

    if (!jsonDoc.isObject())
    {
        return false;
    }

    QJsonValue v = jsonDoc["success"];
    if (v == QJsonValue::Undefined)
    {
        return false;
    }
    if (!v.toBool(false))
    {
        return false;
    }

    v = jsonDoc["username"];
    if (v == QJsonValue::Undefined)
    {
        return false;
    }
    username = v.toString();

    v = jsonDoc["bing_url"];
    if (v == QJsonValue::Undefined)
    {
        return false;
    }
    bingUrl = v.toString();

    v = jsonDoc["mapbox_token"];
    if (v == QJsonValue::Undefined)
    {
        return false;
    }
    mapboxToken = v.toString();

    return true;
}

void CAuth::closeEvent(QCloseEvent *e)
{
    emit sigLoopDone();
}

void CAuth::openDialog()
{
    setupUi(this);

    connect(tokenLineEdit, &QLineEdit::textChanged,          this, &CAuth::slotTokenChanged);
    connect(submitToken,   &QPushButton::pressed,            this, &CAuth::slotCheckToken);
    disconnect(manager,    &QNetworkAccessManager::finished, this, &CAuth::slotFinishedPreRequest);
    connect(manager,       &QNetworkAccessManager::finished, this, &CAuth::slotFinishedGuiRequest);

    exec();
}

void CAuth::requestCheck()
{
    uuid = QUuid::createUuid().toString(QUuid::Id128);
    request.setUrl(QUrl(URL_CHECK_TOKEN));
    QString args = "hashed_token=" + hashedToken + "&uuid=" + uuid;
    manager->post(request, args.toUtf8());
}

void CAuth::slotFinishedPreRequest(QNetworkReply *reply)
{
    const QNetworkReply::NetworkError errorCode = reply->error();

    switch (errorCode)
    {
    case QNetworkReply::NoError:
    {
        readReply(reply->readAll());
        break;
    }

    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::AuthenticationRequiredError:
    {
        openDialog();
        break;
    }

    default:
    {
        QString error = "Unexpected server error or network error. You cannot be authenticated to the ExploreWilder server. Please try again later.";
        QMessageBox::critical(nullptr, QCoreApplication::applicationName(), error);
    }
    }

    emit sigLoopDone();
}

void CAuth::slotFinishedGuiRequest(QNetworkReply *reply)
{
    QString error;
    const QNetworkReply::NetworkError errorCode = reply->error();

    switch (errorCode)
    {
    case QNetworkReply::NoError:
    {
        readReply(reply->readAll());
        close();

        if (isRememberMe)
        {
            SETTINGS;
            cfg.setValue("User/token", hashedToken);
        }
        break;
    }

    case QNetworkReply::ContentAccessDenied:
    case QNetworkReply::AuthenticationRequiredError:
    {
        errorMessage->setText("Bad credentials");
        submitToken->setEnabled(true);
        break;
    }

    default:
    {
        errorMessage->setText("Unexpected server error or network error");
        submitToken->setEnabled(true);
    }
    }

    emit sigLoopDone();
}

void CAuth::slotTokenChanged(const QString &newToken)
{
    submitToken->setEnabled(!newToken.isEmpty());
    errorMessage->setText("");
}

void CAuth::slotCheckToken()
{
    submitToken->setEnabled(false);
    isRememberMe = rememberMe->isChecked();
    QByteArray newToken = tokenLineEdit->text().toUtf8();
    hashedToken = QCryptographicHash::hash(newToken, QCryptographicHash::Sha512).toHex();
    requestCheck();
}

