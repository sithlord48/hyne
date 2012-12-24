#include "QTaskBarButton.h"

#ifdef Q_WS_WIN

QTaskBarButton::QTaskBarButton(QWidget *mainWindow) :
	QObject(mainWindow), pITask(0), destinationList(0),
	_minimum(0), _maximum(100),
	_value(0), _state(Invisible)
{
	_winId = mainWindow->window()->winId();

	initTaskBar();
}

QTaskBarButton::~QTaskBarButton()
{
	if(pITask) {
		pITask->Release();
		pITask = 0;
		CoUninitialize();
	}

	if(destinationList) {
		removedItems->Release();
		destinationList->Release();
		destinationList = 0;
//		CoUninitialize();
	}
}

void QTaskBarButton::initTaskBar()
{
	HRESULT hRes = CoInitialize(NULL);

	if(FAILED(hRes)) {
		return;
	}

	hRes = CoCreateInstance(CLSID_TaskbarList,
							NULL, CLSCTX_INPROC_SERVER,
							IID_ITaskbarList3, (LPVOID *)&pITask);

	if(FAILED(hRes)) {
		pITask = 0;
		CoUninitialize();
		qWarning() << "error TaskBar" << hRes;
		return;
	}

	hRes = pITask->HrInit();

	if(FAILED(hRes)) {
		pITask->Release();
		pITask = 0;
		CoUninitialize();
		qWarning() << "error TaskBar" << hRes;
		return;
	}
}

void QTaskBarButton::initDestinationList()
{
	if(destinationList)	return;

//	CoInitialize(NULL);
	HRESULT hRes = CoCreateInstance(CLSID_DestinationList,
									NULL, CLSCTX_INPROC_SERVER,
									IID_ICustomDestinationList,
									reinterpret_cast<void **>(&destinationList));

	if(FAILED(hRes)) {
		destinationList = 0;
//		CoUninitialize();
		return;
	}

	UINT max_count = 0;

	hRes = destinationList->BeginList(&max_count, IID_IObjectArray,
									  reinterpret_cast<void **>(&removedItems));

	if(FAILED(hRes)) {
		qWarning() << "error BeginList" << hRes;
		return;
	}

	//	removedItems->Release();
	//	destinationList->Release();
}

void QTaskBarButton::addList(ListCategories category)
{
	QList<KNOWNDESTCATEGORY> knownCats;

	if(category.testFlag(Frequent)) {
		knownCats.append(KDC_FREQUENT);
	}
	if(category.testFlag(Recent)) {
		knownCats.append(KDC_RECENT);
	}

	if(knownCats.isEmpty()) {
		qWarning() << "QTaskBarButton::addList no category!";
		return;
	}

	initDestinationList();
	if(!destinationList)	return;

	HRESULT hRes;

	foreach(KNOWNDESTCATEGORY knownCat, knownCats) {
		hRes = destinationList->AppendKnownCategory(knownCat);

		if(FAILED(hRes)) {
			qWarning() << "error AppendKnownCategory" << hRes;
		}
	}

	hRes = destinationList->CommitList();

	if(FAILED(hRes)) {
		qWarning() << "error CommitList" << hRes;
		return;
	}
}

void QTaskBarButton::clearList()
{
	if(!destinationList)	return;
	destinationList->DeleteList(NULL);
}

void QTaskBarButton::addToRecentDocs(const QString &path)
{
	SHAddToRecentDocs(SHARD_PATHW, QDir::toNativeSeparators(QDir::cleanPath(path)).utf16());
}

void QTaskBarButton::setOverlayIcon(const QPixmap &pixmap, const QString &text)
{
	if(!pITask)	return;

	if(pixmap.isNull()) {
		pITask->SetOverlayIcon(_winId, NULL, NULL);
	} else {
		const HICON icon = pixmap.toWinHICON();
		pITask->SetOverlayIcon(_winId, icon, (wchar_t *)text.utf16());
		DestroyIcon(icon);
	}
}

void QTaskBarButton::setState(State state)
{
	if(!pITask)	return;

	TBPFLAG flag;
	switch(state) {
	case Invisible:		flag = TBPF_NOPROGRESS;
		break;
	case Indeterminate:	flag = TBPF_INDETERMINATE;
		break;
	case Paused:		flag = TBPF_PAUSED;
		break;
	case Error:			flag = TBPF_ERROR;
		break;
	default:			flag = TBPF_NORMAL;
		break;
	}

	if(SUCCEEDED(pITask->SetProgressState(_winId, flag))) {
		_state = state;
	}
}

void QTaskBarButton::setValue(int value)
{
	if(!pITask)	return;

	int completed = value - _minimum, total = _maximum - _minimum;
	if(completed < 0 || total <= 0) {
		return;
	}

	if(SUCCEEDED(pITask->SetProgressValue(_winId, completed, total))) {
		_value = value;
		emit valueChanged(value);
	}
}
#else

QTaskBarButton::QTaskBarButton(QWidget *parent) :
	QObject(parent)
{
}

QTaskBarButton::~QTaskBarButton()
{
}

void QTaskBarButton::addList(ListCategories category)
{
	Q_UNUSED(category);
}

void QTaskBarButton::clearList()
{
}

void QTaskBarButton::addToRecentDocs(const QString &path)
{
	Q_UNUSED(path);
}

void QTaskBarButton::setOverlayIcon(const QPixmap &pixmap, const QString &text)
{
	Q_UNUSED(pixmap);
	Q_UNUSED(text);
}

void QTaskBarButton::setState(State state)
{
	Q_UNUSED(state);
}

void QTaskBarButton::setValue(int value)
{
	Q_UNUSED(value);
}

#endif

int QTaskBarButton::maximum() const
{
	return _minimum;
}

int QTaskBarButton::minimum() const
{
	return _maximum;
}

QTaskBarButton::State QTaskBarButton::state() const
{
	return _state;
}

int QTaskBarButton::value() const
{
	return _value;
}

void QTaskBarButton::reset()
{
	setState(Normal);
	setValue(0);
}

void QTaskBarButton::setMaximum(int maximum)
{
	_maximum = maximum;
}

void QTaskBarButton::setMinimum(int minimum)
{
	_minimum = minimum;
}

void QTaskBarButton::setRange(int minimum, int maximum)
{
	setMinimum(minimum);
	setMaximum(maximum);
}
