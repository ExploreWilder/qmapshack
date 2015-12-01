/**********************************************************************************************
    Copyright (C) 2015 Christian Eichler code@christian-eichler.de

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

#ifndef CSLFPROJECT_H
#define CSLFPROJECT_H

#include "gis/prj/IGisProject.h"

class CSlfProject : public IGisProject
{
public:
    CSlfProject(const QString &filename, bool readFile = true);
    virtual ~CSlfProject();

    virtual const QString getFileDialogFilter() override
    {
        return IGisProject::filedialogFilterSLF;
    }

    virtual const QString getFileExtension() override
    {
        return "slf";
    }

private:
    void loadSlf(const QString &filename);

    friend class CSlfReader;
};

#endif // CSLFPROJECT_H

