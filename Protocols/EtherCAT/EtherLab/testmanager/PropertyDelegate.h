/*****************************************************************************
 *
 * Testmanager - Graphical Automation and Visualisation Tool
 *
 * Copyright (C) 2018  Florian Pose <fp@igh.de>
 *
 * This file is part of Testmanager.
 *
 * Testmanager is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * Testmanager is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with Testmanager. If not, see <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#ifndef PROPERTY_DELEGATE_H
#define PROPERTY_DELEGATE_H

#include <QStyledItemDelegate>

/****************************************************************************/

class Property;

/****************************************************************************/

class PropertyDelegate:
    public QStyledItemDelegate
{
    Q_OBJECT

    public:
        PropertyDelegate();

        // virtual from QStyledItemDelegate
        QWidget *createEditor(QWidget *, const QStyleOptionViewItem &,
                const QModelIndex &) const;
        void setEditorData(QWidget *, const QModelIndex &) const;
        void setModelData(QWidget *, QAbstractItemModel *,
                const QModelIndex &) const;

    private:
        const Property *getProperty(const QModelIndex &) const;

    private slots:
        void comboBoxChanged();
};

/****************************************************************************/

#endif
