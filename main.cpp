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

#include <QApplication>
#include "Window.h"

// Only for static compilation
//Q_IMPORT_PLUGIN(qjpcodecs) // jp encoding

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
	app.setWindowIcon(QIcon(":/images/hyne.png"));

	Config::set();

	QTranslator translator;
	QString lang = QLocale::system().name().toLower();
	lang = Config::value("lang", lang.left(lang.indexOf("_")));
	if(translator.load("hyne_" + lang, app.applicationDirPath())) {
		app.installTranslator(&translator);
	} else if(lang != "fr") {
		lang = Window::chooseLangDialog();
		if(translator.load("hyne_" + lang, app.applicationDirPath())) {
			app.installTranslator(&translator);
			Config::setValue("lang", lang);
		} else {
			Config::setValue("lang", "fr");
		}
	} else {
		Config::setValue("lang", "fr");
	}
	Config::translator = &translator;

	Config::loadRecentFiles();

	Window window;
	window.show();

	if(argc > 1) {
		window.openFile(argv[1]);
	}

    return app.exec();
}
