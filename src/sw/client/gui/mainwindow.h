/*
 * SW - Build System and Package Manager
 * Copyright (C) 2019-2020 Egor Pugin
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <qabstractitemmodel.h>
#include <qmainwindow.h>

struct SwClientContext;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(SwClientContext &swctx, QWidget *parent = 0);

private:
    SwClientContext &swctx;

    void setupUi();

    void setupGeneral(QWidget *parent);
    void setupConfiguration(QWidget *parent);
};
