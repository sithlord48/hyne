#ifndef QTASKBARBUTTON_H
#define QTASKBARBUTTON_H

#include <QtCore>
#include <QWidget>
#ifdef Q_OS_WIN32_AI
#include "shobjidl.h"
#include "shlobj.h"
#endif

class QTaskBarButton : public QObject
{
    Q_OBJECT
public:
	enum State {
		Invisible, Normal, Indeterminate, Paused, Error
	};

	enum ListCategory {
		Frequent=0x01, Recent=0x02
	};
	Q_DECLARE_FLAGS(ListCategories, ListCategory)

	explicit QTaskBarButton(QWidget *mainWindow);
	virtual ~QTaskBarButton();
	void addList(ListCategories category);
	void clearList();
	static void addToRecentDocs(const QString &path);
	void setOverlayIcon(const QPixmap &pixmap, const QString &text=QString());
	void setState(State state);
	int maximum() const;
	int minimum() const;
	State state() const;
	int value() const;
signals:
	void valueChanged(int value);
public slots:
	void reset();
	void setMaximum(int maximum);
	void setMinimum(int minimum);
	void setRange(int minimum, int maximum);
	void setValue(int value);
private:
	void initTaskBar();
	void initDestinationList();
#ifdef Q_OS_WIN32_AI
	WId _winId;
	ITaskbarList3 *pITask;
	ICustomDestinationList *destinationList;
	IObjectArray *removedItems;
#endif // Q_OS_WIN32
	int _minimum, _maximum, _value;
	State _state;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(QTaskBarButton::ListCategories)

#endif // QTASKBARBUTTON_H
