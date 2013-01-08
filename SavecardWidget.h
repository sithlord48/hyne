/****************************************************************************
 ** Hyne Final Fantasy VIII Save Editor
 ** Copyright (C) 2009-2012 Arzel Jérôme <myst6re@gmail.com>
 **
 ** This program is free software: you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation, either version 3 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/

#ifndef DEF_SAVECARDWIDGET
#define DEF_SAVECARDWIDGET

#include <QtWidgets>
#include "SavecardView.h"

class SavecardView;

class SavecardWidget : public QScrollArea
{
	Q_OBJECT

public:
	SavecardWidget(QWidget *parent=0);
	virtual ~SavecardWidget();

	SavecardView *view() const;

	SavecardData *savecard() const;
	void setSavecard(SavecardData *save);

	QRect visualRect() const;
	void scrollToDrag(const QPoint &pos);
};

#endif // DEF_SAVECARDWIDGET
