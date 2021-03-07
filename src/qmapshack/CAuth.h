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

#ifndef CAUTH_H
#define CAUTH_H

#include "ui_IAuth.h"
#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

#define URL_CHECK_TOKEN "https://explorewilder.com/qmapshack/token/check"

class CAuth : public QDialog, private Ui::IAuth
{
    Q_OBJECT
public:
    CAuth();
    virtual ~CAuth();

    const QString& getHashedToken() const
    {
        return hashedToken;
    }

    const QString& getUuid() const
    {
        return uuid;
    }

    const QString& getUsername() const
    {
        return username;
    }

    const QString& getBingUrl() const
    {
        return bingUrl;
    }

    const QString& getMapboxToken() const
    {
        return mapboxToken;
    }

protected:
    /// Interpret the received JSON data.
    bool readReply(const QByteArray &json);

    /// Generate an UUID and send a POST request to check the credentials.
    void requestCheck();

    /// Actual dialog openner.
    void openDialog();

    /// Exit this dialog without authorized credentials.
    void closeEvent(QCloseEvent *e) override;

signals:
    /// Emit to stop the main window blocker that would start as soon as this dialog closes.
    void sigLoopDone();

private slots:
    /// Handle the information received from the server when requested before launching the GUI.
    void slotFinishedPreRequest(QNetworkReply *reply);

    /// Handle the information received from the server when requested through the GUI.
    void slotFinishedGuiRequest(QNetworkReply *reply);

    // Update the submit button status and reset the error message.
    void slotTokenChanged(const QString &newToken);

    // Disable the submit button, update the member information, send a request to check.
    void slotCheckToken();

private:
    QString hashedToken;
    QNetworkAccessManager *manager;
    QNetworkRequest request;

    // True to save the hashed token into the disk, false to keep it in RAM.
    bool isRememberMe;

    // Unique session ID.
    QString uuid;

    // Username of authenticated member.
    QString username;

    // URL in the VTS format for the mapConfig.
    QString bingUrl;

    // Mapbox access token.
    QString mapboxToken;
};

#endif //CAUTH_H

