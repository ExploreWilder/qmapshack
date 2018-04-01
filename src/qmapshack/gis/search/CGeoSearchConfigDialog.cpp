/**********************************************************************************************
    Copyright (C) 2018 Norbert Truchsess <norbert.truchsess@t-online.de>

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

#include "gis/search/CGeoSearchConfigDialog.h"
#include "gis/search/CGeoSearchConfig.h"

CGeoSearchConfigDialog::CGeoSearchConfigDialog(QWidget* parent, CGeoSearchConfig *searchConfig) : QDialog(parent)
{
    setupUi(this);

    connect(buttonBox,       &QDialogButtonBox::accepted, this, &CGeoSearchConfigDialog::slotAccepted);
    connect(buttonBox,       &QDialogButtonBox::rejected, this, &QDialog::reject);

    this->searchConfig = searchConfig;

    this->checkGoogle->setChecked(searchConfig->googleApiEnabled);
    this->checkGeoSearch->setChecked(searchConfig->geonamesSearchEnabled);
    this->checkGeoAddress->setChecked(searchConfig->geonamesAddressEnabled);
    this->checkNominatim->setChecked(searchConfig->nominatimEnabled);
    this->lineGoogleKey->setText(searchConfig->googleApiKey);
    this->lineGeonamesLogin->setText(searchConfig->geonamesUsername);
    this->lineNominatimEmail->setText(searchConfig->nominatimEmail);
}

void CGeoSearchConfigDialog::slotAccepted()
{
    searchConfig->googleApiEnabled = checkGoogle->isChecked();
    searchConfig->geonamesSearchEnabled = checkGeoSearch->isChecked();
    searchConfig->geonamesAddressEnabled = checkGeoAddress->isChecked();
    searchConfig->nominatimEnabled = checkNominatim->isChecked();
    searchConfig->googleApiKey = lineGoogleKey->text();
    searchConfig->geonamesUsername = lineGeonamesLogin->text();
    searchConfig->nominatimEmail = lineNominatimEmail->text();

    accept();
}