#include "MetadataDialog.h"
#include "UserDirectory.h"

MetadataDialog::MetadataDialog(QWidget *parent) :
	QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint)
{
	build();
}

MetadataDialog::MetadataDialog(const QMap<FF8Installation::Type, FF8Installation> &ff8Installations, QWidget *parent) :
	QDialog(parent, Qt::Dialog | Qt::WindowCloseButtonHint)
{
	build();
	fill(ff8Installations);
}

void MetadataDialog::build()
{
	HelpWidget *help = new HelpWidget(32, tr("Pour être placées dans le Cloud, "
											 "les sauvegardes doivent être signées. "
											 "Ces signatures sont écrites dans un fichier "
											 "nommé metadata.xml, normalement situé "
											 "dans le même dossier que vos sauvegardes.\n"
											 "Pour trouver votre numéro d'utilisateur, "
											 "regardez le nom du dossier où se trouve vos sauvegardes. "
											 "Si vous ne voyez pas, essayez en laissant le champ vide.\n"
											 "Par défaut Hyne tente de signer les sauvegardes "
											 "automatiquement, mais en cas d'erreur, vous pouvez "
											 "essayer de le faire manuellement ici."));

	_path = new QLineEdit(this);
	_pathButton = new QPushButton(tr("Parcourir..."), this);

	QHBoxLayout *pathLayout = new QHBoxLayout;
	pathLayout->addWidget(_path);
	pathLayout->addWidget(_pathButton);
	pathLayout->setContentsMargins(QMargins());

	_userID = new QLineEdit(this);

	QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
													   Qt::Horizontal, this);

	connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));

	_formWidget = new QWidget(this);

	QFormLayout *formLayout = new QFormLayout(_formWidget);
	formLayout->addRow(tr("metadata.xml"), pathLayout);
	formLayout->addRow(tr("Numéro d'utilisateur"), _userID);
	formLayout->setContentsMargins(QMargins());

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->addWidget(help);
	layout->addWidget(_formWidget);
	layout->addStretch();
	layout->addWidget(buttonBox);

	connect(_pathButton, SIGNAL(clicked()), SLOT(setMetadataPath()));
}

void MetadataDialog::fill(const QMap<FF8Installation::Type, FF8Installation> &ff8Installations)
{
	if(ff8Installations.contains(FF8Installation::Steam)) {
		FF8Installation installation = ff8Installations.value(FF8Installation::Steam);
		if(installation.hasMetadata()) {
			QString savePath = installation.savePath(1);
			_path->setText(savePath + "/metadata.xml");
			UserDirectory userDir(savePath);
			_userID->setText(userDir.userID());
		}
	}
}

void MetadataDialog::setMetadataPath(const QString &path)
{
	QString metadataPath;

	if(path.isNull()) {
		QStringList filter = QStringList()
				<< tr("Fichiers XML (*.xml)")
				<< tr("Tous les fichiers (*)");
		metadataPath = QFileDialog::getOpenFileName(this, tr("Rechercher metadata.xml"), _path->text(), filter.join(";;"));
		if(metadataPath.isNull()) {
			return;
		}
	} else {
		metadataPath = path;
	}

	_path->setText(metadataPath);
}
