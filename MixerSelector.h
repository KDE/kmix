#ifndef MixerSelector_h
#define MixerSelector_h

#include <qwidget.h>
#include <qptrlist.h>
#include <qwidget.h>
class QVBox;
class QComboBox;
class QLineEdit;
class QCheckBox;

class KDialogBase;

class Mixer;
class MixerSelectionInfo;

class MixerSelector : public QWidget
{
	Q_OBJECT
public:
	MixerSelector(QPtrList<Mixer> &mixers, QWidget * parent, const char * name=0, WFlags f = 0);
	~MixerSelector();
	
	MixerSelectionInfo* exec();
	
	QVBox *vbox;
	QComboBox *hwNames;
	QLineEdit *shownName;
	QCheckBox *distributeCheck;
	KDialogBase *dialog;

private slots:
	void newMixerSelected(const QString &newMixer);
};

#endif // MixerSelector_h

