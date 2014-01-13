/****************************************************************************
 ** Hyne Final Fantasy VIII Save Editor
 ** Copyright (C) 2009-2013 Arzel Jérôme <myst6re@gmail.com>
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

#include "SavecardData.h"
#include "GZIP.h"
#include "Parameters.h"
#include "LZS.h"

SavecardData::SavecardData(const QString &path, quint8 slot, const FF8Installation &ff8Installation) :
	_ok(true), start(0), _isModified(false), _slot(slot), _ff8Installation(ff8Installation)
{
	open(path, slot);
}

SavecardData::SavecardData(int saveCount) :
	_ok(true), start(0), _isModified(false)
{
	for(int i=0 ; i<saveCount ; ++i) {
		addSave();
	}

	setType(Undefined);
}

SavecardData::~SavecardData()
{
	foreach(SaveData *save, saves)	delete save;
}

bool SavecardData::open(const QString &path, quint8 slot)
{
	if(slot)
	{
		setPath(QDir::fromNativeSeparators(QDir::cleanPath(path)) + "/");
		setType(PcSlot);

		directory(_ff8Installation.saveNamePattern(_slot));
		_ok = !saves.isEmpty();
	}
	else
	{
		setPath(QDir::fromNativeSeparators(QDir::cleanPath(path)));
		QString extension = this->extension();

		if(extension.isEmpty() || extension == "ff8")
		{
			setType(Pc);
			if(!pc())	addSave(QByteArray());
		}
		else if(extension == "mcr" || extension == "ddf" || extension == "mc"
		   || extension == "mcd"|| extension == "mci" || extension == "ps"
		   || extension == "psm" || extension == "vm1")
		{
			setType(Ps);
			_ok = ps();
		}
		else if(extension == "mem" || extension == "vgs")
		{
			setType(Vgs);
			_ok = ps();
		}
		else if(extension == "gme")
		{
			setType(Gme);
			_ok = ps();
		}
		else if(extension == "vmp")
		{
			setType(Vmp);
			_ok = ps();
		}
		else if(extension == "psv")
		{
			setType(Psv);
			_ok = ps3();
			if(!_ok) {
				_ok = sstate_pSX();
				setType(Undefined);
			}
		}
		else if(extension == "000"
				|| extension == "001"
				|| extension == "002"
				|| extension == "003"
				|| extension == "004")
		{
			_ok = sstate_ePSXe();
			setType(Undefined);
		}
		else
		{
			_ok = false;
			setType(Unknown);
		}
	}

//	compare(saves.at(0)->save(), saves.at(1)->save());
//	compare(saves.at(10)->save(), saves.at(11)->save());
	return _ok;
}

const QFileSystemWatcher *SavecardData::watcher() const
{
	return &fileWatcher;
}

QString SavecardData::description() const
{
	return QString(_description);
}

void SavecardData::setDescription(const QString &desc)
{
	_description = desc.toLatin1();
}

QString SavecardData::dirname() const
{
	int index = _path.lastIndexOf('/');
	return index != -1 ? _path.left(index + 1) : QString();
}

QString SavecardData::name() const
{
	return _path.mid(_path.lastIndexOf('/') + 1);
}

QString SavecardData::extension() const
{
	int index = _path.lastIndexOf('.');
	return index != -1 ? _path.mid(index + 1).toLower() : QString();
}

void SavecardData::setName(const QString &name)
{
	_path = dirname() + name;
}

void SavecardData::setType(Type type)
{
	switch(type) {
	case Vgs:
		start = 64;
		break;
	case Gme:
		start = 3904;
		break;
	case Vmp:
		start = 128;
		break;
	case Undefined:
		start = 0;
		setPath(QObject::tr("Sans nom"));
		setModified(true);
		break;
	default:
		start = 0;
		break;
	}

	_type = type;
}

bool SavecardData::hasPath() const
{
	return !dirname().isEmpty();
}

bool SavecardData::isModified() const
{
	if(_isModified)	return true;

	foreach(SaveData *save, saves) {
		if(save->isModified())	return true;
	}
	return false;
}

void SavecardData::setModified(bool modified)
{
	_isModified = modified;

	if(modified == false) {
		foreach(SaveData *save, saves) {
			save->setModified(false);
		}
	}
}

void SavecardData::setIsTheLastEdited(int saveID)
{
	foreach(SaveData *save, saves) {
		if(save->isTheLastEdited()) {
			save->setIsTheLastEdited(false);
		}
	}
	saves.at(saveID)->setIsTheLastEdited(true);
}

bool SavecardData::ps()
{
	setErrorString(QString());
	QFile fic(_path);

	if(!fic.exists())
	{
		setErrorString(QObject::tr("Le fichier n'existe plus.\n%1").arg(_path));
		return false;
	}
	if(!fic.open(QIODevice::ReadOnly))
	{
		setErrorString(QObject::tr("Le fichier est protégé en lecture."));
		return false;
	}
	if(fic.size() < start+131072)//start+8192*16
	{
		setErrorString(QObject::tr("Fichier trop court"));
		return false;
	}

	if(_type == Gme) {
		fic.seek(64);
		_description = fic.read(3840);
	}

	fic.seek(start+128);
	QByteArray header = fic.read(1920);//(128*15)
	fic.seek(start+SAVE_SIZE);

	quint8 blockCount = 1;
	bool occupied;

	for(quint8 i=0 ; i<15 ; ++i) {
		if(blockCount <= 1 && !saves.isEmpty()) {
			blockCount = saves.last()->blockCount();
		}

		if(blockCount > 1) {
			blockCount--;
			occupied = true;
		} else {
			occupied = false;
		}

		addSave(fic.read(SAVE_SIZE), header.mid(128*i, 127), occupied);
	}

	if(fileWatcher.files().size()<30)
		fileWatcher.addPath(_path);

	return true;
}

bool SavecardData::ps3()
{
	QFile fic(_path);
	setErrorString(QString());

	if(!fic.exists())
	{
		setErrorString(QObject::tr("Le fichier n'existe plus.\n%1").arg(_path));
		return false;
	}
	if(!fic.open(QIODevice::ReadOnly))
	{
		setErrorString(QObject::tr("Le fichier est protégé en lecture."));
		return false;
	}

	// Check format
	fic.seek(1);
	if(QString(fic.peek(3)) != "VSP" && QString(fic.peek(6)) == "RS2CPU")
		return false;

	if(fic.size() < 8324)
	{
		setErrorString(QObject::tr("Fichier invalide"));
		return false;
	}

	fic.seek(100);
	QByteArray MCHeader("\x51\x00\x00\x00\x00\x20\x00\x00\xff\xff", 10);
	MCHeader.append(fic.read(20));

	fic.seek(132);

	addSave(fic.read(SAVE_SIZE), MCHeader);

	if(saves.first()->isDelete())	return false;

	if(fileWatcher.files().size()<30)
		fileWatcher.addPath(_path);

	return true;
}

bool SavecardData::pc(const QString &path)
{
	int tailleC;
	QFile f(path.isEmpty() ? _path : path);

	if(!f.exists() || !f.open(QIODevice::ReadOnly))
		return false;

	if(f.size() > SAVE_SIZE * 8)		return false;

	f.read((char *)&tailleC, 4);
	if(tailleC != f.size()-4)		return false;

	addSave(LZS::decompress(f.readAll(), FF8SAVE_SIZE));

//	if(fileWatcher.files().size()<30)
//		fileWatcher.addPath(_path);

	return true;
}

bool SavecardData::getFormatFromRaw()
{
	QFile f(_path);
	start = 0;
	_ok = false;

	if(!f.exists() || !f.open(QIODevice::ReadOnly))
		return false;

	QByteArray data = f.read(10000);

	int indexMC = data.indexOf("MC");
	if(indexMC != -1) { // Maybe Memory Card format
		start = indexMC;
		switch(indexMC) {
		case 0:
			_type = Ps;
			break;
		case 64:
			_type = Vgs;
			break;
		case 128:
			_type = Vmp;
			break;
		case 3904:
			_type = Gme;
			break;
		default:
			setType(Undefined);
			start = indexMC;
			break;
		}
		return _ok = ps();
	} else { // Maybe PC format
		// compressed?
		quint32 lzsSize;
		memcpy(&lzsSize, data.constData(), 4);
		if(lzsSize + 4 == f.size()) {
			const QByteArray &unLzsed = LZS::decompress(data.mid(4), 2);
			if(unLzsed.startsWith("SC")) {
				_type = Pc;
				return _ok = pc();
			}
			return false;
		}

		if(data.indexOf("VSP") == 1) {
			_type = Psv;
			return _ok = ps3();
		}
	}

	return false;
}

bool SavecardData::sstate_ePSXe()
{
	QTemporaryFile temp;
	setErrorString(QString());

	if(!temp.open()) {
		setErrorString(QObject::tr("Impossible de créer le fichier temporaire."));
		return false;
	}
	if(!GZIP::decompress(_path, temp.fileName())) {
		setErrorString(QObject::tr("Impossible de décompresser le fichier."));
		return false;
	}
//	temp.seek(7);
//	QString serial(temp.read(11));

	temp.seek(0xBC42);
	QByteArray MCHeader = temp.read(0x20);

	temp.seek(0x779CC);

	return sstate(temp.read(0x139E), MCHeader);
}

bool SavecardData::sstate_pSX()
{
	QFile f(_path);
	setErrorString(QString());

	if(!f.open(QIODevice::ReadOnly)) {
		setErrorString(QObject::tr("Le fichier est protégé en lecture."));
		return false;
	}

	f.seek(0xBD38);
	QByteArray MCHeader = f.read(0x20);

	f.seek(0x77AC2);

	return sstate(f.read(0x139E), MCHeader);
}

bool SavecardData::sstate(const QByteArray &fdata, const QByteArray &MCHeader)
{
	QByteArray squallIcon;

	QFile iconFile(":/data/icon0.psico");
	if(iconFile.open(QIODevice::ReadOnly)) {
		squallIcon = iconFile.readAll();
		iconFile.close();
	}

	QByteArray data;
	data.append("SC");
	data.append('\x11'); // icon (1 frame)
	data.append('\x01'); // 1 save slot per save
	data.append(QByteArray(0x4C, '\0')); // generic desc
	data.append("\x01\x00CRD0", 6);
	data.append(QByteArray(0x0A, '\0'));
	data.append(squallIcon.leftJustified(288, '\0', true));
	data.append("\x00\x00", 2); // CRC
	data.append(fdata);

	if(data.size() != 0x139E + 0x182) {
		setErrorString(QObject::tr("Format invalide."));
		return false;
	}

	addSave(data, MCHeader);

	if(!saves.isEmpty()) {
		SaveData *saveData = saves.last();
		if(!saveData->isFF8()) {
			saves.removeLast();
			delete saveData;
			setErrorString(QObject::tr("La sauvegarde trouvée n'est pas de Final Fantasy VIII."));
			return false;
		}
	}

	return true;
}

void SavecardData::directory(const QString &filePattern)
{
	QString dirname = this->dirname();

	for(quint8 i=0 ; i<30 ; ++i) {
		QString path = filePattern;
		QString num = QString("%1").arg(i + 1, 2, 10, QChar('0'));
		path = dirname + path.replace("{num}", num);
		if(!pc(path)) {
			addSave(); // Empty save
		}
	}

	LZS::clear();
}

void SavecardData::addSave(const QByteArray &data, const QByteArray &header, bool occupied)
{
	saves.append(new SaveData(saves.size(), data, header, type()!=Psv, occupied));
}

void SavecardData::moveSave(int sourceID, int targetID)
{
	SaveData *saveData = saves.takeAt(sourceID);
	saves.insert(targetID, saveData);

	// Rebuild ids
	int saveID = 0;
	foreach(saveData, saves) {
		saveData->setId(saveID);
		saveID++;
	}

	setModified(true);
}

SaveData *SavecardData::getSave(int id) const
{
	return saves.value(id, 0);
}

int SavecardData::saveCount() const
{
	return saves.size();
}

bool SavecardData::save(const QString &saveAs, Type newType)
{
	const QString path = saveAs.isEmpty() ? _path : saveAs;
	QTemporaryFile temp;
	QFile fic(_path);
	setErrorString(QString());

	if(saveAs.isEmpty()) {
		newType = _type;
	}

	if(!fic.exists())
	{
		setErrorString(QObject::tr("Le fichier n'existe plus.\n%1").arg(_path));
		return false;
	}
	if(!fic.open(QIODevice::ReadOnly))
	{
		setErrorString(QObject::tr("Le fichier est protégé en lecture.\n%1").arg(_path));
		return false;
	}
	if(!temp.open())
	{
		fic.close();
		setErrorString(QObject::tr("Impossible de créer un fichier temporaire"));
		return false;
	}

	if(_type == Psv)
	{
		temp.write(fic.read(100));
		temp.write(saves.first()->MCHeader().mid(10, 20));// B + country + code + id
		fic.seek(120);
		temp.write(fic.read(12));
//		compare(fic.peek(FF8SAVE_SIZE), saves.first()->save());
		temp.write(saves.first()->save());
	}
	else if(_type != Pc && _type != PcSlot)
	{
		quint8 i;
		SaveData *save;

		temp.write(header(&fic, newType, !saveAs.isEmpty()));

		temp.write("MC", 2);//MC
		temp.write(QByteArray(125,'\x00'));
		temp.putChar('\x0E');//xor byte

		fic.seek(start+128);

		for(i=0 ; i<15 ; ++i)
		{
			save = saves.at(i);

			if(save->hasMCHeader())
			{
				temp.write(save->saveMCHeader());//128
				fic.seek(fic.pos() + 128);
			}
			else
			{
				temp.write(fic.read(128));//Main header
			}
		}

		temp.write(fic.read(6144));//Padding (8192-16*128)

		for(i=0 ; i<15 ; ++i)
		{
			save = saves.at(i);

//			compare(fic.peek(FF8SAVE_SIZE), save->save());
			temp.write(save->save());
			fic.seek(fic.pos() + SAVE_SIZE);
		}
	}

	fic.close();

	bool readdPath = false;
	if(fileWatcher.files().contains(path))
	{
		readdPath = true;
		fileWatcher.removePath(path);
	}

	if(QFile::exists(path) && !QFile::remove(path))
	{
		temp.close();
		setErrorString(QObject::tr("Impossible de supprimer le fichier !\n%1\nÉchec de la sauvegarde.\nVérifiez que le fichier n'est pas utilisé par un autre programme.").arg(path));
		if(readdPath)	fileWatcher.addPath(path);
		return false;
	}
	if(!temp.copy(path))
	{
		setErrorString(QObject::tr("Échec de la sauvegarde."));
	}
	if(readdPath)	fileWatcher.addPath(path);

	return true;
}

bool SavecardData::save2PC(const quint8 id, const QString &saveAs)
{
	const SaveData *save = saves.at(id);
	setErrorString(QString());

	if(!save->isFF8() && !save->isDelete()) {
		setErrorString(QObject::tr("Cette sauvegarde ne provient pas de Final Fantasy VIII."));
		return false;
	}

	const QString path = saveAs.isEmpty() ? _path : saveAs;

	QTemporaryFile temp;
	if(!temp.open()) {
		setErrorString(QObject::tr("Impossible de créer un fichier temporaire"));
		return false;
	}

	bool readdPath = false;
	if(fileWatcher.files().contains(path)) {
		readdPath = true;
		fileWatcher.removePath(path);
	}

	// Rerelease 2013
	UserDirectory userDirectory;
	quint8 slot=0, num=0;

	QString filename = path.mid(path.lastIndexOf('/') + 1);
	QRegExp regExp("slot([12])_save(\\d\\d).ff8");

	if(regExp.exactMatch(filename)) {
		QString dirname = path.left(path.lastIndexOf('/'));
		userDirectory.setDirname(dirname);

		if(userDirectory.hasMetadata() && userDirectory.openMetadata()) {
			QStringList capturedTexts = regExp.capturedTexts();
			slot = capturedTexts.at(1).toInt();
			num = capturedTexts.at(2).toInt();
		} else if(!userDirectory.hasMetadata()) {
			setErrorString(QObject::tr("Le fichier 'metadata.xml' n'a pas été trouvé dans le dossier '%1'.\n"
									   "Essayez de signer vos sauvegardes manuellement (Fichier > Signer les sauv. pour le Cloud).").arg(dirname));
		} else {
			setErrorString(QObject::tr("Le fichier 'metadata.xml' n'a pas pu être ouvert.\n%1").arg(userDirectory.errorString()));
		}
	}

	if(save->isDelete()) {
		QFile::remove(path);

		// Rerelease 2013: removing from metadata file
		if(slot > 0) {
			userDirectory.updateMetadata(slot, num);
			if(!userDirectory.saveMetadata()) {
				setErrorString(QObject::tr("Le fichier 'metadata.xml' n'a pas pu être mis à jour.\n%1").arg(userDirectory.errorString()));
			}
		}

		return true;
	}

	QByteArray result = LZS::compress(save->save());
	int size = result.size();
	result.prepend((char *)&size, 4);

	// Rerelease 2013: updating signature in metadata file
	if(slot > 0) {
		userDirectory.updateMetadata(slot, num, result);
		if(!userDirectory.saveMetadata()) {
			setErrorString(QObject::tr("Le fichier 'metadata.xml' n'a pas pu être mis à jour.\n%1").arg(userDirectory.errorString()));
		}
	}

	temp.write(result);

	if(QFile::exists(path) && !QFile::remove(path))
	{
		setErrorString(QObject::tr("Impossible de supprimer le fichier !\n%1\nÉchec de la sauvegarde.\nEssayez de lancer %2 en tant qu'administrateur.")
				.arg(path).arg(PROG_NAME));
		if(readdPath)	fileWatcher.addPath(path);
		return false;
	}
	if(!temp.copy(path))
	{
		setErrorString(QObject::tr("Échec de la sauvegarde."));
	}
	if(readdPath)	fileWatcher.addPath(path);

	if(_type == Undefined) {
		setPath(path);
		setType(Pc);
	}

	return true;
}

bool SavecardData::save2PSV(const quint8 id, const QString &saveAs, const QByteArray &MCHeader)
{
	const SaveData *save = saves.at(id);
	const QString path = saveAs.isEmpty()
			? _path
			: saveAs;
	setErrorString(QString());

	QTemporaryFile temp;
	if(!temp.open())
	{
		setErrorString(QObject::tr("Impossible de créer un fichier temporaire"));
		return false;
	}

	bool readdPath = false;
	if(fileWatcher.files().contains(path))
	{
		readdPath = true;
		fileWatcher.removePath(path);
	}

	if(save->isDelete()) {
		return QFile::remove(path);
	}

	QByteArray result;
	result.append("\0VSP\0\0\0\0", 8);
	result.append(QByteArray(40, '\0')); // TODO: checksum
	result.append("\x00\x00\x00\x00\x00\x00\x00\x00\x14\x00\x00\x00"
				  "\x01\x00\x00\x00\x00\x20\x00\x00\x84\x00\x00\x00"
				  "\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
				  "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x20\x00\x00"
				  "\x03\x90\x00\x00", 52); // unknown
	result.append(MCHeader.mid(10, 32)); // Country + prod code + identifier
	result.append(save->save());
	temp.write(result);

	if(QFile::exists(path) && !QFile::remove(path))
	{
		setErrorString(QObject::tr("Impossible de supprimer le fichier !\n%1\nÉchec de la sauvegarde.\nVérifiez que le fichier n'est pas utilisé par un autre programme.").arg(path));
		if(readdPath)	fileWatcher.addPath(path);
		return false;
	}
	if(!temp.copy(path))
	{
		setErrorString(QObject::tr("Échec de la sauvegarde."));
	}
	if(readdPath)	fileWatcher.addPath(path);

	if(_type == Undefined) {
		setPath(path);
		setType(Psv);
	}

	return true;
}

bool SavecardData::save2PS(const QList<int> &ids, const QString &path, const Type newType, const QByteArray &MCHeader)
{
	QTemporaryFile temp;
	quint8 i;
	setErrorString(QString());

	if(!temp.open())
	{
		setErrorString(QObject::tr("Impossible de créer un fichier temporaire"));
		return false;
	}

	temp.write(header(0, newType, true));

	temp.write("MC", 2);//MC
	temp.write(QByteArray(125,'\x00'));
	temp.putChar('\x0E');//xor byte

	for(i=0 ; i<15 ; ++i)
	{
		// 128 bytes
		if(i >= ids.size())
		{
			temp.write(SaveData::emptyMCHeader());
		}
		else
		{
			if(!MCHeader.isEmpty()) {
				QByteArray MCHeaderCpy = MCHeader;
				MCHeaderCpy.replace(26, 2, QString("%1").arg(i, 2, 10, QChar('0')).toLatin1());
				MCHeaderCpy[127] = (char)SaveData::xorByte(MCHeaderCpy.constData());
				temp.write(MCHeaderCpy);
			} else {
				temp.write(saves.at(ids.at(i))->saveMCHeader());
			}
		}
	}

	temp.write(QByteArray(6144, '\x00'));//Padding

	for(i=0 ; i<15 ; ++i)
	{
		// 8192 bytes
		if(i >= ids.size() || !saves.at(ids.at(i))->isFF8())
		{
			temp.write(QByteArray(SAVE_SIZE, '\x00'));
		}
		else
		{
			temp.write(saves.at(ids.at(i))->save());
		}
	}

	bool readdPath = false;
	if(fileWatcher.files().contains(path))
	{
		readdPath = true;
		fileWatcher.removePath(path);
	}

	if(QFile::exists(path) && !QFile::remove(path))
	{
		setErrorString(QObject::tr("Impossible de supprimer le fichier !"
								 "\n%1\nÉchec de la sauvegarde."
								 "\nVérifiez que le fichier n'est pas utilisé"
								 " par un autre programme.").arg(path));
		if(readdPath)	fileWatcher.addPath(path);
		return false;
	}
	if(!temp.copy(path))
	{
		setErrorString(QObject::tr("Échec de la sauvegarde."));
	}
	if(readdPath)	fileWatcher.addPath(path);

	if(_type == Undefined) {
		setPath(path);
		setType(newType);
		for(i=saves.size() ; i<15 ; ++i) {
			addSave();
		}
		i=0;
		foreach(SaveData *save, saves) {
			if(save->isFF8()) {
				if(!MCHeader.isEmpty()) {
					QByteArray MCHeaderCpy = MCHeader;
					save->setMCHeader(MCHeaderCpy.replace(26, 2, QString("%1").arg(i, 2, 10, QChar('0')).toLatin1()));
				}
			}
			save->setModified(false);
			++i;
		}
	}

	return true;
}

QByteArray SavecardData::header(QFile *srcFile, Type newType, bool saveAs)
{
	if(newType==Vgs)
	{
		// header : 64 bytes
		if(_type==Vgs)
		{
			return srcFile->read(64);
		}
		else
		{
			return QByteArray("\x56\x67\x73\x4d\x01\x00\x00\x00\x01\x00\x00\x00"
							  "\x01\x00\x00\x00\x00\x02", 18)
					.append(QByteArray(46, '\x00'));
		}
	}
	else if(newType==Gme)
	{
		QByteArray header;
		// header : 64 bytes
		if(_type==Gme)
		{
			header = srcFile->read(64);
		}
		else
		{
			header = QByteArray("\x31\x32\x33\x2d\x34\x35\x36\x2d\x53\x54\x44\x00"
								"\x00\x00\x00\x00\x00\x00\x01\x00\x01\x4d\x51\xa0"
								"\xa0\xa0\xa0\xa0\xa0\xa0\xa0\xa0\xa0\xa0\xa0\xa0"
								"\xa0\x00\xff\x00\x00\x00\x00\x00\x00\x00\x00\x00"
								"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
								"\x00\x00\x00\x00", 64);
		}

		// description : 3840 bytes
		if(saveAs)
		{
			return header.append(_description.leftJustified(3840, '\0', true));
		}
		else
		{
			//srcFile->seek(64);
			return header.append(srcFile->read(3840));
		}
	}
	else if(newType==Vmp)
	{
		// header : 128 bytes
		if(_type==Vmp)
		{
			return srcFile->read(128);
		}
		else
		{
			//TODO: Unknown crc
			return QByteArray("\x00\x50\x4d\x56\x80", 5)
					.append(QByteArray(123, '\x00'));
		}
	}
	else if(newType==_type)
	{
		return srcFile->read(start);
	}
	else
		return QByteArray();
}

bool SavecardData::saveDirectory()
{
	QString dirname = this->dirname(), filePattern;
	bool ok = true;
	int i = 0;

	filePattern = _ff8Installation.saveNamePattern(_slot);

	foreach(const SaveData *save, saves) {
		if(save->isModified()) {
			QString num = QString("%1").arg(i + 1, 2, 10, QChar('0'));
			QString path = filePattern;

			if(!save2PC(i, dirname + path.replace("{num}", num))) {
				ok = false;
			}
		}
		++i;
	}

	return ok;
}

void SavecardData::compare(const QByteArray &oldData, const QByteArray &newData)
{
	qDebug() << "Compare ----";
	for(int i=386 ; i<FF8SAVE_SIZE ; ++i) {
		switch(i) {
		case 0:
			qDebug() << "\tHeader1";break;
		case 386:
			qDebug() << "\tHeader2";break;
		case 464:
			qDebug() << "\tGF";break;
		case 1552:
			qDebug() << "\tPerso";break;
		case 2768:
			qDebug() << "\tBoutiques";break;
		case 3168:
			qDebug() << "\tConfig";break;
		case 3188:
			qDebug() << "\tDivers 1";break;
		case 3220:
			qDebug() << "\tLimit Break";break;
		case 3236:
			qDebug() << "\tObjets";break;
		case 3664:
			qDebug() << "\tStats";break;
		case 3808:
			qDebug() << "\tVar map 1";break;
		case 4064:
			qDebug() << "\tVar map 2";break;
		case 5088:
			qDebug() << "\tMappemonde";break;
		case 5216:
			qDebug() << "\tTriple Triad Cards";break;
		case 5344:
			qDebug() << "\tChocobo";break;
		}

		if(oldData.at(i)!=newData.at(i)) {
			qDebug() << QString("Difference at %1: %2 (%3) | %4 (%5)%6").arg(i).arg((quint8)oldData.at(i),2,16,QChar('0')).arg((quint8)oldData.at(i),8,2,QChar('0')).arg((quint8)newData.at(i),2,16,QChar('0')).arg((quint8)newData.at(i),8,2,QChar('0')).arg(i >= 3808 && i<5088 ? QString(" var%1").arg(i-3808) : "").toLatin1().constData();
		}
	}
}
